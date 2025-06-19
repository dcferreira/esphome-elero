#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/elero/cc1101.h"
#include <string>
#include <map>

// All encryption/decryption structures copied from https://github.com/QuadCorei8085/elero_protocol/ (MIT)
// All remote handling based on code from https://github.com/stanleypa/eleropy (GPLv3)

namespace esphome {
namespace elero {

static const uint8_t ELERO_COMMAND_COVER_CONTROL = 0x6a;
static const uint8_t ELERO_COMMAND_COVER_CHECK = 0x00;
static const uint8_t ELERO_COMMAND_COVER_STOP = 0x10;
static const uint8_t ELERO_COMMAND_COVER_UP = 0x20;
static const uint8_t ELERO_COMMAND_COVER_TILT = 0x24;
static const uint8_t ELERO_COMMAND_COVER_DOWN = 0x40;
static const uint8_t ELERO_COMMAND_COVER_INT = 0x44;

static const uint8_t ELERO_STATE_UNKNOWN = 0x00;
static const uint8_t ELERO_STATE_TOP = 0x01;
static const uint8_t ELERO_STATE_BOTTOM = 0x02;
static const uint8_t ELERO_STATE_INTERMEDIATE = 0x03;
static const uint8_t ELERO_STATE_TILT =0x04;
static const uint8_t ELERO_STATE_BLOCKING = 0x05;
static const uint8_t ELERO_STATE_OVERHEATED = 0x06;
static const uint8_t ELERO_STATE_TIMEOUT = 0x07;
static const uint8_t ELERO_STATE_START_MOVING_UP = 0x08;
static const uint8_t ELERO_STATE_START_MOVING_DOWN = 0x09;
static const uint8_t ELERO_STATE_MOVING_UP = 0x0a;
static const uint8_t ELERO_STATE_MOVING_DOWN = 0x0b;
static const uint8_t ELERO_STATE_STOPPED = 0x0d;
static const uint8_t ELERO_STATE_TOP_TILT = 0x0e;
static const uint8_t ELERO_STATE_BOTTOM_TILT = 0x0f;
static const uint8_t ELERO_STATE_OFF = 0x0f;
static const uint8_t ELERO_STATE_ON = 0x10;

static const uint8_t ELERO_MAX_PACKET_SIZE = 57; // according to FCC documents

static const uint32_t ELERO_POLL_INTERVAL_MOVING = 2000;  // poll every two seconds while moving
static const uint32_t ELERO_DELAY_SEND_PACKETS = 50; // 50ms send delay between repeats
static const uint32_t ELERO_TIMEOUT_MOVEMENT = 120000; // poll for up to two minutes while moving

static const uint8_t ELERO_SEND_RETRIES = 3;
static const uint8_t ELERO_SEND_PACKETS = 2;

typedef struct {
  uint8_t counter;
  uint32_t blind_addr;
  uint32_t remote_addr;
  uint8_t channel;
  uint8_t pck_inf[2];
  uint8_t hop;
  uint8_t payload[10];
} t_elero_command;

class EleroCover;
class EleroSensor;

class Elero : public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                    spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_2MHZ>,
                                    public Component {
 public:
  void setup() override;
  void loop() override;

  static void interrupt(Elero *arg);
  void set_received();
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void reset();
  void init();
  void write_reg(uint8_t addr, uint8_t data);
  void write_burst(uint8_t addr, uint8_t *data, uint8_t len);
  void write_cmd(uint8_t cmd);
  bool wait_rx();
  bool wait_tx();
  bool wait_tx_done();
  bool wait_idle();
  bool transmit();
  uint8_t read_reg(uint8_t addr);
  uint8_t read_status(uint8_t addr);
  void read_buf(uint8_t addr, uint8_t *buf, uint8_t len);
  void flush_and_rx();
  void interpret_msg();
  void register_cover(EleroCover *cover);
  bool send_command(t_elero_command *cmd);
  
  void set_gdo0_pin(InternalGPIOPin *pin) { gdo0_pin_ = pin; }
  void set_freq0(uint8_t freq) { freq0_ = freq; }
  void set_freq1(uint8_t freq) { freq1_ = freq; }
  void set_freq2(uint8_t freq) { freq2_ = freq; }
  
  // Debug statistics getters
  uint32_t get_commands_sent_count() const { return commands_sent_count_; }
  uint32_t get_commands_failed_count() const { return commands_failed_count_; }
  float get_last_rssi() const { return last_rssi_; }
  uint8_t get_last_lqi() const { return last_lqi_; }
  uint8_t get_cc1101_state() const { return const_cast<Elero*>(this)->read_status(CC1101_MARCSTATE); }
  uint32_t get_last_successful_communication() const { return last_successful_communication_; }
  
  // Debug sensor setters
  void set_silent_failures_sensor(EleroSensor *sensor) { silent_failures_sensor_ = sensor; }
  void set_success_rate_sensor(EleroSensor *sensor) { success_rate_sensor_ = sensor; }
  void set_avg_response_time_sensor(EleroSensor *sensor) { avg_response_time_sensor_ = sensor; }
  void set_last_rssi_sensor(EleroSensor *sensor) { last_rssi_sensor_ = sensor; }
  void set_blinds_in_recovery_sensor(EleroSensor *sensor) { blinds_in_recovery_sensor_ = sensor; }
  void set_enable_per_blind_sensors(bool enable) { enable_per_blind_sensors_ = enable; }
  
  // Methods for covers to update metrics
  void increment_silent_failures() { 
    silent_failures_count_++; 
    update_debug_sensors(); 
  }
  void add_response_time(uint32_t response_time) {
    total_response_time_ += response_time;
    response_count_++;
    update_debug_sensors();
  }
  
  // Counter recovery methods
  void register_per_blind_sensors(uint32_t blind_address, const std::string& blind_name);
  void update_per_blind_counter_stats(uint32_t blind_address, uint8_t counter, bool recovery_success, const std::string& strategy);
  void update_per_blind_current_counter(uint32_t blind_address, uint8_t counter);
  void update_per_blind_time_sensors();
  void update_per_blind_last_response_time(uint32_t blind_address);
  void update_blinds_in_recovery_count();
  void register_per_blind_sensor(uint32_t blind_address, const std::string& sensor_type, EleroSensor* sensor);
  
  // Update debug sensors
  void update_debug_sensors();

 private:
  uint8_t count_bits(uint8_t byte);
  void calc_parity(uint8_t* msg);
  void add_r20_to_nibbles(uint8_t* msg, uint8_t r20, uint8_t start, uint8_t length);
  void sub_r20_from_nibbles(uint8_t* msg, uint8_t r20, uint8_t start, uint8_t length);
  void xor_2byte_in_array_encode(uint8_t* msg, uint8_t xor0, uint8_t xor1);
  void xor_2byte_in_array_decode(uint8_t* msg, uint8_t xor0, uint8_t xor1);
  void encode_nibbles(uint8_t* msg);
  void decode_nibbles(uint8_t* msg, uint8_t len);
  void msg_decode(uint8_t *msg);
  void msg_encode(uint8_t* msg);
 
 
  bool received_{false};
  uint8_t msg_rx_[CC1101_FIFO_LENGTH];
  uint8_t msg_tx_[CC1101_FIFO_LENGTH];
  uint8_t freq0_{0x7a};
  uint8_t freq1_{0x71};
  uint8_t freq2_{0x21};
  InternalGPIOPin *gdo0_pin_{nullptr};
  ISRInternalGPIOPin gdo0_irq_pin_{nullptr};
  std::map<uint32_t, EleroCover*> address_to_cover_mapping_;
  
  // Debug statistics
  uint32_t commands_sent_count_{0};
  uint32_t commands_failed_count_{0};
  uint32_t silent_failures_count_{0};
  float last_rssi_{0.0};
  uint8_t last_lqi_{0};
  uint32_t last_successful_communication_{0};
  
  // Response time tracking
  uint32_t total_response_time_{0};
  uint32_t response_count_{0};
  
  // Debug sensors
  EleroSensor *silent_failures_sensor_{nullptr};
  EleroSensor *success_rate_sensor_{nullptr};
  EleroSensor *avg_response_time_sensor_{nullptr};
  EleroSensor *last_rssi_sensor_{nullptr};
  EleroSensor *blinds_in_recovery_sensor_{nullptr};
  bool enable_per_blind_sensors_{false};
  
  // Per-blind sensor tracking
  struct PerBlindStats {
    uint32_t recovery_attempts = 0;
    uint32_t recovery_successes = 0;
    uint8_t current_counter = 1;
    uint8_t last_working_counter = 1;
    std::string last_strategy_success = "none";
    uint32_t last_response_time = 0;
    std::map<std::string, EleroSensor*> sensors;
  };
  std::map<uint32_t, PerBlindStats> per_blind_stats_;
};

}  // namespace elero
}  // namespace esphome

