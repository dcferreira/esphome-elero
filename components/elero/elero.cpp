#include "elero.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/components/elero/cover/EleroCover.h"
#include "esphome/components/elero/sensor/EleroSensor.h"
#include <cinttypes>

namespace esphome {
namespace elero {

static const char *TAG = "elero";
static const uint8_t flash_table_encode[] = {0x08, 0x02, 0x0d, 0x01, 0x0f, 0x0e, 0x07, 0x05, 0x09, 0x0c, 0x00, 0x0a, 0x03, 0x04, 0x0b, 0x06};
static const uint8_t flash_table_decode[] = {0x0a, 0x03, 0x01, 0x0c, 0x0d, 0x07, 0x0f, 0x06, 0x00, 0x08, 0x0b, 0x0e, 0x09, 0x02, 0x05, 0x04};

void Elero::loop() {
  if(this->received_) {
    ESP_LOGVV(TAG, "loop says \"received\"");
    this->received_ = false;
    uint8_t len = this->read_status(CC1101_RXBYTES);
    if(len & 0x7F) { // bytes available
      if((len & 0x7F) > CC1101_FIFO_LENGTH) {
        ESP_LOGV(TAG, "Received more bytes than FIFO length - wtf?");
        this->read_buf(CC1101_RXFIFO, this->msg_rx_, CC1101_FIFO_LENGTH);
      } else {
        this->read_buf(CC1101_RXFIFO, this->msg_rx_, (len & 0x7f));
      }
      // Sanity check
      if(this->msg_rx_[0] + 3 <= (len & 0x7f)) {
        this->interpret_msg();
      }
    }
    if(len & 0x80) { // overflow
      ESP_LOGV(TAG, "Rx overflow, flushing FIFOs");
      this->flush_and_rx();
    }
  }
}

void IRAM_ATTR Elero::interrupt(Elero *arg) {
  arg->set_received();
}

void IRAM_ATTR Elero::set_received() {
  this->received_ = true;
}

void Elero::dump_config() {
  ESP_LOGCONFIG(TAG, "Elero Config: ");
}

void Elero::setup() {
  ESP_LOGI(TAG, "Setting up Elero Component...");
  this->spi_setup();
  this->gdo0_pin_->setup();
  this->gdo0_irq_pin_ = this->gdo0_pin_->to_isr();
  this->gdo0_pin_->attach_interrupt(Elero::interrupt, this, gpio::INTERRUPT_FALLING_EDGE);
  this->reset();
  this->init();
}

void Elero::flush_and_rx() {
  ESP_LOGVV(TAG, "flush_and_rx");
  this->write_cmd(CC1101_SIDLE);
  this->wait_idle();
  this->write_cmd(CC1101_SFRX);
  this->write_cmd(CC1101_SFTX);
  this->write_cmd(CC1101_SRX);
  this->received_ = false;
}

void Elero::reset() {
  // We don't do a hardware reset as we can't read
  // the MISO pin directly. Rely on software-reset only.
  
  this->enable();
  this->write_byte(CC1101_SRES);
  delay_microseconds_safe(50);
  this->write_byte(CC1101_SIDLE);
  delay_microseconds_safe(50);
  this->disable();
}

void Elero::init() {
  uint8_t patable_data[] = {0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};

  this->write_reg(CC1101_FSCTRL1, 0x08);
  this->write_reg(CC1101_FSCTRL0, 0x00);
  this->write_reg(CC1101_FREQ2, this->freq2_);
  this->write_reg(CC1101_FREQ1, this->freq1_);
  this->write_reg(CC1101_FREQ0, this->freq0_);
  this->write_reg(CC1101_MDMCFG4, 0x7B);
  this->write_reg(CC1101_MDMCFG3, 0x83);
  this->write_reg(CC1101_MDMCFG2, 0x13); 
  this->write_reg(CC1101_MDMCFG1, 0x52);
  this->write_reg(CC1101_MDMCFG0, 0xF8);
  this->write_reg(CC1101_CHANNR, 0x00);
  this->write_reg(CC1101_DEVIATN, 0x43);
  this->write_reg(CC1101_FREND1, 0xB6);
  this->write_reg(CC1101_FREND0, 0x10);
  this->write_reg(CC1101_MCSM0, 0x18);
  this->write_reg(CC1101_MCSM1, 0x3F);
  this->write_reg(CC1101_FOCCFG, 0x1D);
  this->write_reg(CC1101_BSCFG, 0x1F);
  this->write_reg(CC1101_AGCCTRL2, 0xC7);
  this->write_reg(CC1101_AGCCTRL1, 0x00);
  this->write_reg(CC1101_AGCCTRL0, 0xB2);
  this->write_reg(CC1101_FSCAL3, 0xEA);
  this->write_reg(CC1101_FSCAL2, 0x2A);
  this->write_reg(CC1101_FSCAL1, 0x00);
  this->write_reg(CC1101_FSCAL0, 0x1F);
  this->write_reg(CC1101_FSTEST, 0x59);
  this->write_reg(CC1101_TEST2, 0x81);
  this->write_reg(CC1101_TEST1, 0x35);
  this->write_reg(CC1101_TEST0, 0x09);
  this->write_reg(CC1101_IOCFG0, 0x06);
  this->write_reg(CC1101_PKTCTRL1, 0x8C);  
  this->write_reg(CC1101_PKTCTRL0, 0x45);
  this->write_reg(CC1101_ADDR, 0x00);
  this->write_reg(CC1101_PKTLEN, 0x3C);
  this->write_reg(CC1101_SYNC1, 0xD3);
  this->write_reg(CC1101_SYNC0, 0x91);
  this->write_burst(CC1101_PATABLE, patable_data, 8);

  this->write_cmd(CC1101_SRX);
  this->wait_rx();
}

void Elero::write_reg(uint8_t addr, uint8_t data) {
  this->enable();
  this->write_byte(addr);
  this->write_byte(data);
  this->disable();
  delay_microseconds_safe(15);
}

void Elero::write_burst(uint8_t addr, uint8_t *data, uint8_t len) {
  this->enable();
  this->write_byte(addr | CC1101_WRITE_BURST);
  for(int i=0; i<len; i++)
    this->write_byte(data[i]);
  this->disable();
  delay_microseconds_safe(15);
}

void Elero::write_cmd(uint8_t cmd) {
  this->enable();
  this->write_byte(cmd);
  this->disable();
  delay_microseconds_safe(15);
}

bool Elero::wait_rx() {
  ESP_LOGVV(TAG, "wait_rx");
  uint8_t timeout = 200;
  while ((this->read_status(CC1101_MARCSTATE) != CC1101_MARCSTATE_RX) && (--timeout != 0)) {
    delay_microseconds_safe(200);
  }
  
  if(timeout > 0)
    return true;
  ESP_LOGE(TAG, "Timed out waiting for RX: 0x%02x", this->read_status(CC1101_MARCSTATE));
  return false;
}

bool Elero::wait_idle() {
  ESP_LOGVV(TAG, "wait_idle");
  uint8_t timeout = 200;
  while ((this->read_status(CC1101_MARCSTATE) != CC1101_MARCSTATE_IDLE) && (--timeout != 0)) {
    delay_microseconds_safe(200);
  }
  
  if(timeout > 0)
    return true;
  ESP_LOGE(TAG, "Timed out waiting for Idle: 0x%02x", this->read_status(CC1101_MARCSTATE));
  return false;
}

bool Elero::wait_tx() {
  ESP_LOGVV(TAG, "wait_tx");
  uint8_t timeout = 200;

  while ((this->read_status(CC1101_MARCSTATE) != CC1101_MARCSTATE_TX) && (--timeout != 0)) {
    delay_microseconds_safe(200);
  }

  if(timeout > 0)
    return true;
  ESP_LOGE(TAG, "Timed out waiting for TX: 0x%02x", this->read_status(CC1101_MARCSTATE));
  return false;
}

bool Elero::wait_tx_done() {
  ESP_LOGVV(TAG, "wait_tx_done");
  uint8_t timeout = 200;
  
  //while (((this->read_status(CC1101_TXBYTES) & 0x7f) != 0) && (--timeout != 0)) {
  while((!this->received_) && (--timeout != 0)) {
    delay_microseconds_safe(200);
  }

  if(timeout > 0)
    return true;
  ESP_LOGE(TAG, "Timed out waiting for TX Done: 0x%02x", this->read_status(CC1101_MARCSTATE));
  return false;
}

bool Elero::transmit() {
  uint32_t start_time = millis();
  uint8_t initial_state = this->read_status(CC1101_MARCSTATE);
  
  ESP_LOGD(TAG, "TX START: %d bytes, initial_state=0x%02x", this->msg_tx_[0], initial_state);
  
  //this->flush_and_rx();
  this->write_cmd(CC1101_SRX);
  uint32_t rx_setup_time = millis();
  
  if(!this->wait_rx()) {
    uint8_t failed_state = this->read_status(CC1101_MARCSTATE);
    ESP_LOGE(TAG, "TX FAILED: RX setup failed, state=0x%02x, duration=%" PRIu32 "ms", failed_state, millis() - start_time);
    return false;
  }

  this->write_burst(CC1101_TXFIFO, this->msg_tx_, this->msg_tx_[0] + 1);
  this->write_cmd(CC1101_STX);
  uint32_t tx_start_time = millis();

  if(!this->wait_tx()) {
    uint8_t failed_state = this->read_status(CC1101_MARCSTATE);
    ESP_LOGE(TAG, "TX FAILED: TX start failed, state=0x%02x, setup_time=%" PRIu32 "ms, total_time=%" PRIu32 "ms", 
             failed_state, tx_start_time - rx_setup_time, millis() - start_time);
    this->flush_and_rx();
    return false;
  }
  
  uint32_t tx_active_time = millis();
  if(!this->wait_tx_done()) {
    uint8_t failed_state = this->read_status(CC1101_MARCSTATE);
    ESP_LOGE(TAG, "TX FAILED: TX completion failed, state=0x%02x, tx_time=%" PRIu32 "ms, total_time=%" PRIu32 "ms", 
             failed_state, millis() - tx_active_time, millis() - start_time);
    this->flush_and_rx();
    return false;
  }

  uint8_t bytes = this->read_status(CC1101_TXBYTES) & 0x7f;
  uint8_t final_state = this->read_status(CC1101_MARCSTATE);
  uint32_t total_time = millis() - start_time;
  
  if(bytes != 0) {
    ESP_LOGE(TAG, "TX FAILED: %d bytes left in buffer, final_state=0x%02x, total_time=%" PRIu32 "ms", 
             bytes, final_state, total_time);
    this->flush_and_rx();
    return false;
  } else {
    ESP_LOGD(TAG, "TX SUCCESS: final_state=0x%02x, setup=%" PRIu32 "ms, tx=%" PRIu32 "ms, completion=%" PRIu32 "ms, total=%" PRIu32 "ms",
             final_state, 
             rx_setup_time - start_time,
             tx_active_time - tx_start_time, 
             millis() - tx_active_time,
             total_time);
    return true;
  }
}

uint8_t Elero::read_reg(uint8_t addr) {
  uint8_t data;

  this->enable();
  this->write_byte(addr);
  data = this->read_byte();
  return data;
}

uint8_t Elero::read_status(uint8_t addr) {
  uint8_t data;

  this->enable();
  this->write_byte(addr | CC1101_READ_BURST);
  data = this->read_byte();
  this->disable();
  delay_microseconds_safe(15);
  return data;
}

void Elero::read_buf(uint8_t addr, uint8_t *buf, uint8_t len) {
  this->enable();
  this->write_byte(addr | CC1101_READ_BURST);
  for(uint8_t i=0; i<len; i++)
    buf[i] = this->read_byte();
  this->disable();
  delay_microseconds_safe(15);
}

uint8_t Elero::count_bits(uint8_t byte)
{
  uint8_t i;
  uint8_t ones = 0;
  uint8_t mask = 1;

  for( i = 0; i < 8; i++ )
  {
    if( mask & byte )
    {
      ones += 1;
    }

    mask <<= 1;
  }

  return ones & 0x01;
}


void Elero::calc_parity(uint8_t* msg)
{
  uint8_t i;
  uint8_t p = 0;

  for( i = 0; i < 4; i++ )
  {
    uint8_t a = count_bits( msg[0 + i*2] );
    uint8_t b = count_bits( msg[1 + i*2] );

    p |= a ^ b;
    p <<= 1;
  }

  msg[7] = (p << 3);
}

void Elero::add_r20_to_nibbles(uint8_t* msg, uint8_t r20, uint8_t start, uint8_t length)
{
  uint8_t i;

  for( i = 0; i < 8; i++ )
  {
    uint8_t d = msg[i];

    uint8_t ln = (d + r20) & 0x0F;
    uint8_t hn = ((d & 0xF0) + (r20 & 0xF0)) & 0xFF;

    msg[i] = hn | ln;

    r20 = (r20 - 0x22) & 0xFF;
  }
}

void Elero::sub_r20_from_nibbles(uint8_t* msg, uint8_t r20, uint8_t start, uint8_t length)
{
  uint8_t i;

  for(i = start; i < length; i++)
  {
    uint8_t d = msg[i];

    uint8_t ln = (d - r20) & 0x0F;
    uint8_t hn = ((d & 0xF0) - (r20 & 0xF0)) & 0xFF;

    msg[i] = hn | ln;

    r20 = (r20 - 0x22) & 0xFF;
  }
}

void Elero::xor_2byte_in_array_encode(uint8_t* msg, uint8_t xor0, uint8_t xor1)
{
  uint8_t i;

  for( i = 1; i < 4; i++ )
  {
    msg[i*2 + 0] = msg[i*2 + 0] ^ xor0;
    msg[i*2 + 1] = msg[i*2 + 1] ^ xor1;
  }
}

void Elero::xor_2byte_in_array_decode(uint8_t* msg, uint8_t xor0, uint8_t xor1)
{
  uint8_t i;

  for( i = 0; i < 4; i++ )
  {
    msg[i*2 + 0] = msg[i*2 + 0] ^ xor0;
    msg[i*2 + 1] = msg[i*2 + 1] ^ xor1;
  }
}

void Elero::encode_nibbles(uint8_t* msg)
{
  uint8_t i;

  for( i = 0; i < 8; i++ )
  {
    uint8_t nh = (msg[i] >> 4) & 0x0F;
    uint8_t nl = msg[i] & 0x0F;

    uint8_t dh = flash_table_encode[nh];
    uint8_t dl = flash_table_encode[nl];

    msg[i] = ((dh << 4) & 0xFF) | ((dl) & 0xFF);
  }
}

void Elero::decode_nibbles(uint8_t* msg, uint8_t len)
{
  uint8_t i;

  for( i = 0; i < len; i++ )
  {
    uint8_t nh = (msg[i] >> 4) & 0x0F;
    uint8_t nl = msg[i] & 0x0F;

    uint8_t dh = flash_table_decode[nh];
    uint8_t dl = flash_table_decode[nl];

    msg[i] = ((dh << 4) & 0xFF) | ((dl) & 0xFF);
  }
}

void Elero::msg_decode(uint8_t *msg) {
  decode_nibbles(msg, 8);
  sub_r20_from_nibbles(msg, 0xFE, 0, 2);
  xor_2byte_in_array_decode(msg, msg[0], msg[1]);
  sub_r20_from_nibbles(msg, 0xBA, 2, 8);
}

void Elero::msg_encode(uint8_t* msg) {
  uint8_t xor0 = msg[0];
  uint8_t xor1 = msg[1];
  calc_parity(msg);
  add_r20_to_nibbles(msg, 0xFE, 0, 8);
  xor_2byte_in_array_encode(msg, xor0, xor1);
  encode_nibbles(msg);
}

void Elero::interpret_msg() {
  uint8_t length = this->msg_rx_[0];
  // Sanity check
  if(length > ELERO_MAX_PACKET_SIZE) {
    ESP_LOGE(TAG, "Received invalid packet: too long (%d)", length);
    return;
  }

  uint8_t cnt = this->msg_rx_[1];
  uint8_t typ = this->msg_rx_[2];
  uint8_t typ2 = this->msg_rx_[3];
  uint8_t hop = this->msg_rx_[4];
  uint8_t syst = this->msg_rx_[5];
  uint8_t chl = this->msg_rx_[6];
  uint32_t src = ((uint32_t)this->msg_rx_[7] << 16) | ((uint32_t)this->msg_rx_[8] << 8) | (this->msg_rx_[9]);
  uint32_t bwd = ((uint32_t)this->msg_rx_[10] << 16) | ((uint32_t)this->msg_rx_[11] << 8) | (this->msg_rx_[12]);
  uint32_t fwd = ((uint32_t)this->msg_rx_[13] << 16) | ((uint32_t)this->msg_rx_[14] << 8) | (this->msg_rx_[15]);
  uint8_t num_dests = this->msg_rx_[16];
  uint32_t dst;
  uint8_t dests_len;
  if(typ > 0x60) {
    dests_len = this->msg_rx_[16] * 3;
    dst = ((uint32_t)this->msg_rx_[17] << 16) | ((uint32_t)this->msg_rx_[18] << 8) | (this->msg_rx_[19]);
  } else {
    dests_len = this->msg_rx_[16];
    dst = this->msg_rx_[17];
  }

  // Sanity check
  if(dests_len + 15 > CC1101_FIFO_LENGTH) {
    ESP_LOGE(TAG, "Received invalid packet: dests_len too long (%d)", dests_len);
    return;
  }

  uint8_t payload1 = this->msg_rx_[17 + dests_len];
  uint8_t payload2 = this->msg_rx_[18 + dests_len];
  uint8_t crc = this->msg_rx_[length + 2] >> 7;
  uint8_t lqi = this->msg_rx_[length + 2] & 0x7f;
  float rssi;
  if(this->msg_rx_[length+1] > 127)
    rssi = (float)((this->msg_rx_[length+1]-256)/2-74);
  else
    rssi = (float)((this->msg_rx_[length+1])/2-74);
  uint8_t *payload = &this->msg_rx_[19 + dests_len];
  msg_decode(payload);
  
  // Update signal quality statistics
  last_rssi_ = rssi;
  last_lqi_ = lqi;
  
  ESP_LOGD(TAG, "rcv'd: len=%02d, cnt=%02d, typ=0x%02x, typ2=0x%02x, hop=0x%02x, syst=0x%02x, chl=%02d, src=0x%06x, bwd=0x%06x, fwd=0x%06x, #dst=%02d, dst=0x%06x, rssi=%2.1f, lqi=%2d, crc=%2d, payload=[0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]", length, cnt, typ, typ2, hop, syst, chl, src, bwd, fwd, num_dests, dst, rssi, lqi, crc, payload1, payload2, payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7]);

  if((typ == 0xca) || (typ == 0xc9)) { // Status message from a blind
    // Check if we know the blind
    // status = payload[6]
    auto search = this->address_to_cover_mapping_.find(src);
    if(search != this->address_to_cover_mapping_.end()) {
      search->second->set_rx_state(payload[6]);
    }
  }
  
  // Handle remote control commands (0x6a messages)
  if(typ == 0x6a) { // Command message from a remote
    // Check if this is directed to one of our blinds
    auto search = this->address_to_cover_mapping_.find(dst);
    if(search != this->address_to_cover_mapping_.end()) {
      EleroCover* cover = search->second;
      
      // Check if the command comes from the configured remote for this blind
      // We need to get the remote address from the cover to verify
      uint8_t command_byte = payload[4];
      
      // Decode command for logging
      const char* cmd_name = "UNKNOWN";
      switch(command_byte) {
        case ELERO_COMMAND_COVER_STOP: cmd_name = "STOP"; break;
        case ELERO_COMMAND_COVER_UP: cmd_name = "UP"; break;
        case ELERO_COMMAND_COVER_DOWN: cmd_name = "DOWN"; break;
        case ELERO_COMMAND_COVER_TILT: cmd_name = "TILT"; break;
        case ELERO_COMMAND_COVER_INT: cmd_name = "INT"; break;
        case ELERO_COMMAND_COVER_CHECK: cmd_name = "CHECK"; break;
      }
      
      ESP_LOGD(TAG, "OVERHEARD CMD: %s (0x%02x) from remote 0x%06x to blind 0x%06x (cnt=%d)",
               cmd_name, command_byte, src, dst, cnt);

      // Sync rolling counter if the command came from the same remote identity we impersonate
      if (src == cover->get_remote_address()) {
        cover->sync_counter(cnt);
      }

      // Update cover operation based on overheard command
      // This helps keep ESPHome in sync when physical remote is used
      if(command_byte == cover->get_command_up()) {
        ESP_LOGD(TAG, "SYNC: Physical remote opened blind 0x%06x, updating ESPHome state", dst);
        cover->sync_external_command(cover::COVER_OPERATION_OPENING);
      } else if(command_byte == cover->get_command_down()) {
        ESP_LOGD(TAG, "SYNC: Physical remote closed blind 0x%06x, updating ESPHome state", dst);
        cover->sync_external_command(cover::COVER_OPERATION_CLOSING);
      } else if(command_byte == cover->get_command_stop()) {
        ESP_LOGD(TAG, "SYNC: Physical remote stopped blind 0x%06x, updating ESPHome state", dst);
        cover->sync_external_command(cover::COVER_OPERATION_IDLE);
      }
    }
  }
  
  // Update debug sensors when we receive messages
  this->update_debug_sensors();
}

void Elero::register_cover(EleroCover *cover) {
  uint32_t address = cover->get_blind_address();
  if(this->address_to_cover_mapping_.find(address) != this->address_to_cover_mapping_.end()) {
    ESP_LOGE(TAG, "A blind with this address is already registered - this is currently not supported");
    return;
  }
  this->address_to_cover_mapping_.insert({address, cover});
  cover->set_poll_offset((this->address_to_cover_mapping_.size() - 1) * 5000);
}

bool Elero::send_command(t_elero_command *cmd) {
  // Decode command type for logging
  const char* cmd_name = "UNKNOWN";
  uint8_t command_byte = cmd->payload[4];
  switch(command_byte) {
    case ELERO_COMMAND_COVER_CHECK: cmd_name = "CHECK"; break;
    case ELERO_COMMAND_COVER_STOP: cmd_name = "STOP"; break;
    case ELERO_COMMAND_COVER_UP: cmd_name = "UP"; break;
    case ELERO_COMMAND_COVER_DOWN: cmd_name = "DOWN"; break;
    case ELERO_COMMAND_COVER_TILT: cmd_name = "TILT"; break;
    case ELERO_COMMAND_COVER_INT: cmd_name = "INT"; break;
  }
  
  ESP_LOGD(TAG, "CMD SEND: %s (0x%02x) to blind 0x%06x, counter=%d, channel=%d", 
           cmd_name, command_byte, cmd->blind_addr, cmd->counter, cmd->channel);
  
  uint16_t code = (0x00 - (cmd->counter * 0x708f)) & 0xffff;
  this->msg_tx_[0] = 0x1d; // message length
  this->msg_tx_[1] = cmd->counter; // message counter
  this->msg_tx_[2] = cmd->pck_inf[0];
  this->msg_tx_[3] = cmd->pck_inf[1];
  this->msg_tx_[4] = cmd->hop; // hop info
  this->msg_tx_[5] = 0x01; // sys_addr = 1
  this->msg_tx_[6] = cmd->channel; // channel
  this->msg_tx_[7] = ((cmd->remote_addr >> 16) & 0xff); // source address
  this->msg_tx_[8] = ((cmd->remote_addr >> 8) & 0xff);
  this->msg_tx_[9] =((cmd->remote_addr) & 0xff);
  this->msg_tx_[10] = ((cmd->remote_addr >> 16) & 0xff); // backward address
  this->msg_tx_[11] = ((cmd->remote_addr >> 8) & 0xff);
  this->msg_tx_[12] =((cmd->remote_addr) & 0xff);
  this->msg_tx_[13] = ((cmd->remote_addr >> 16) & 0xff); // forward address
  this->msg_tx_[14] = ((cmd->remote_addr >> 8) & 0xff);
  this->msg_tx_[15] =((cmd->remote_addr) & 0xff);
  this->msg_tx_[16] = 0x01; // destination count
  this->msg_tx_[17] = ((cmd->blind_addr >> 16) & 0xff); // blind address
  this->msg_tx_[18] = ((cmd->blind_addr >> 8) & 0xff);
  this->msg_tx_[19] = ((cmd->blind_addr) & 0xff);
  for(int i=0; i<10; i++)
    this->msg_tx_[20 + i] = cmd->payload[i];
  this->msg_tx_[22] = ((code >> 8) & 0xff);
  this->msg_tx_[23] = (code & 0xff);

  uint8_t *payload = &this->msg_tx_[22];
  msg_encode(payload);

  ESP_LOGV(TAG, "send: len=%02d, cnt=%02d, typ=0x%02x, typ2=0x%02x, hop=0x%02x, syst=0x%02x, chl=%02d, src=0x%02x%02x%02x, bwd=0x%02x%02x%02x, fwd=0x%02x%02x%02x, #dst=%02d, dst=0x%02x%02x%02x, payload=[0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]", this->msg_tx_[0], this->msg_tx_[1], this->msg_tx_[2], this->msg_tx_[3], this->msg_tx_[4], this->msg_tx_[5], this->msg_tx_[6], this->msg_tx_[7], this->msg_tx_[8], this->msg_tx_[9], this->msg_tx_[10], this->msg_tx_[11], this->msg_tx_[12], this->msg_tx_[13], this->msg_tx_[14], this->msg_tx_[15], this->msg_tx_[16], this->msg_tx_[17], this->msg_tx_[18], this->msg_tx_[19], this->msg_tx_[20], this->msg_tx_[21], this->msg_tx_[22], this->msg_tx_[23], this->msg_tx_[24], this->msg_tx_[25], this->msg_tx_[26], this->msg_tx_[27], this->msg_tx_[28], this->msg_tx_[29]);
  
  bool result = transmit();
  
  // Update statistics
  commands_sent_count_++;
  if (result) {
    last_successful_communication_ = millis();
  } else {
    commands_failed_count_++;
  }
  
  // Update debug sensors after every command
  this->update_debug_sensors();
  
  ESP_LOGD(TAG, "CMD RESULT: %s command %s (total_sent=%d, total_failed=%d)", 
           cmd_name, result ? "SUCCESS" : "FAILED", commands_sent_count_, commands_failed_count_);
  return result;
}

void Elero::update_debug_sensors() {
  // Calculate success rate
  float success_rate = 0.0;
  if (commands_sent_count_ > 0) {
    success_rate = ((float)(commands_sent_count_ - commands_failed_count_) / commands_sent_count_) * 100.0;
  }
  
  // Calculate average response time
  float avg_response_time = 0.0;
  if (response_count_ > 0) {
    avg_response_time = (float)total_response_time_ / response_count_;
  }
  
  // Update sensors if they exist
  if (silent_failures_sensor_ != nullptr) {
    silent_failures_sensor_->update_value(silent_failures_count_);
  }
  
  if (success_rate_sensor_ != nullptr) {
    success_rate_sensor_->update_value(success_rate);
  }
  
  if (avg_response_time_sensor_ != nullptr) {
    avg_response_time_sensor_->update_value(avg_response_time);
  }
  
  if (last_rssi_sensor_ != nullptr) {
    last_rssi_sensor_->update_value(last_rssi_);
  }
  
  // Update blinds in recovery count
  update_blinds_in_recovery_count();
  
  // Update time-based per-blind sensors
  update_per_blind_time_sensors();
}

void Elero::update_blinds_in_recovery_count() {
  if (blinds_in_recovery_sensor_ == nullptr) {
    return;
  }
  
  // Count how many covers are currently in recovery mode
  uint32_t recovery_count = 0;
  for (auto& cover_pair : address_to_cover_mapping_) {
    if (cover_pair.second->is_in_recovery()) {
      recovery_count++;
    }
  }
  
  blinds_in_recovery_sensor_->update_value(recovery_count);
}

void Elero::register_per_blind_sensor(uint32_t blind_address, const std::string& sensor_type, EleroSensor* sensor) {
  // Initialize per-blind stats if not exists
  if (per_blind_stats_.find(blind_address) == per_blind_stats_.end()) {
    per_blind_stats_[blind_address] = PerBlindStats();
    ESP_LOGD("elero", "Created new per-blind stats for blind 0x%06x", blind_address);
  }
  
  // Store the sensor reference
  per_blind_stats_[blind_address].sensors[sensor_type] = sensor;
  
  ESP_LOGD("elero", "Registered per-blind sensor: %s for blind 0x%06x", sensor_type.c_str(), blind_address);
  
  // Initialize sensor with current value
  PerBlindStats& stats = per_blind_stats_[blind_address];
  if (sensor_type == "recovery_attempts") {
    sensor->update_value(stats.recovery_attempts);
    ESP_LOGD("elero", "Initialized %s sensor for blind 0x%06x with value: %d", sensor_type.c_str(), blind_address, stats.recovery_attempts);
  } else if (sensor_type == "recovery_successes") {
    sensor->update_value(stats.recovery_successes);
    ESP_LOGD("elero", "Initialized %s sensor for blind 0x%06x with value: %d", sensor_type.c_str(), blind_address, stats.recovery_successes);
  } else if (sensor_type == "current_counter") {
    sensor->update_value(stats.current_counter);
    ESP_LOGD("elero", "Initialized %s sensor for blind 0x%06x with value: %d", sensor_type.c_str(), blind_address, stats.current_counter);
  } else if (sensor_type == "last_working_counter") {
    sensor->update_value(stats.last_working_counter);
    ESP_LOGD("elero", "Initialized %s sensor for blind 0x%06x with value: %d", sensor_type.c_str(), blind_address, stats.last_working_counter);
  } else if (sensor_type == "seconds_since_last_response") {
    uint32_t seconds_since = stats.last_response_time > 0 ? (millis() - stats.last_response_time) / 1000 : 0;
    sensor->update_value(seconds_since);
    ESP_LOGD("elero", "Initialized %s sensor for blind 0x%06x with value: %d", sensor_type.c_str(), blind_address, (int)seconds_since);
  }
}

void Elero::update_per_blind_current_counter(uint32_t blind_address, uint8_t counter) {
  auto it = per_blind_stats_.find(blind_address);
  if (it != per_blind_stats_.end()) {
    PerBlindStats& stats = it->second;
    stats.current_counter = counter;
    
    // Update current counter sensor if it exists
    if (stats.sensors["current_counter"] != nullptr) {
      stats.sensors["current_counter"]->update_value(stats.current_counter);
      ESP_LOGD("elero", "Updated current_counter sensor for blind 0x%06x: %d", blind_address, counter);
    }
  } else {
    ESP_LOGD("elero", "No per-blind stats found for blind 0x%06x when updating current counter", blind_address);
  }
}

void Elero::update_per_blind_time_sensors() {
  for (auto& pair : per_blind_stats_) {
    PerBlindStats& stats = pair.second;
    
    // Update time-based sensors if they exist
    if (stats.sensors["seconds_since_last_response"] != nullptr) {
      uint32_t seconds_since = (millis() - stats.last_response_time) / 1000;
      stats.sensors["seconds_since_last_response"]->update_value(seconds_since);
    }
  }
}

void Elero::update_per_blind_last_response_time(uint32_t blind_address) {
  auto it = per_blind_stats_.find(blind_address);
  if (it != per_blind_stats_.end()) {
    it->second.last_response_time = millis();
  }
}

void Elero::update_per_blind_counter_stats(uint32_t blind_address, uint8_t counter, bool recovery_success, const std::string& strategy) {
  auto it = per_blind_stats_.find(blind_address);
  if (it != per_blind_stats_.end()) {
    PerBlindStats& stats = it->second;
    
    stats.current_counter = counter;
    
    if (recovery_success) {
      stats.recovery_successes++;
      stats.last_working_counter = counter;
      stats.last_strategy_success = strategy;
    } else {
      stats.recovery_attempts++;
    }
    
    stats.last_response_time = millis();
    
    // Update individual sensors if they exist
    if (stats.sensors["recovery_attempts"] != nullptr) {
      stats.sensors["recovery_attempts"]->update_value(stats.recovery_attempts);
    }
    if (stats.sensors["recovery_successes"] != nullptr) {
      stats.sensors["recovery_successes"]->update_value(stats.recovery_successes);
    }
    if (stats.sensors["current_counter"] != nullptr) {
      stats.sensors["current_counter"]->update_value(stats.current_counter);
    }
    if (stats.sensors["last_working_counter"] != nullptr) {
      stats.sensors["last_working_counter"]->update_value(stats.last_working_counter);
    }
    if (stats.sensors["seconds_since_last_response"] != nullptr) {
      uint32_t seconds_since = (millis() - stats.last_response_time) / 1000;
      stats.sensors["seconds_since_last_response"]->update_value(seconds_since);
    }
  }
}

}  // namespace elero
}  // namespace esphome
