#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/preferences.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/elero/elero.h"
#include <queue>

namespace esphome {
namespace elero {

class EleroCover : public cover::Cover, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  
  cover::CoverTraits get_traits() override;
  
  void set_elero_parent(Elero *parent) { this->parent_ = parent; }
  void set_blind_address(uint32_t address) { this->command_.blind_addr = address; }
  void set_channel(uint8_t channel) { this->command_.channel = channel; }
  void set_remote_address(uint32_t remote) { this->command_.remote_addr = remote; }
  void set_payload_1(uint8_t payload) { this->command_.payload[0] = payload; }
  void set_payload_2(uint8_t payload) { this->command_.payload[1] = payload; }
  void set_hop(uint8_t hop) { this->command_.hop = hop; }
  void set_pckinf_1(uint8_t pckinf) { this->command_.pck_inf[0] = pckinf; }
  void set_pckinf_2(uint8_t pckinf) { this->command_.pck_inf[1] = pckinf; }
  void set_command_up(uint8_t cmd) { this->command_up_ = cmd; }
  void set_command_down(uint8_t cmd) { this->command_down_ = cmd; }
  void set_command_stop(uint8_t cmd) { this->command_stop_ = cmd; }
  void set_command_check(uint8_t cmd) { this->command_check_ = cmd; }
  void set_command_tilt(uint8_t cmd) { this->command_tilt_ = cmd; }
  void set_poll_offset(uint32_t offset) { this->poll_offset_ = offset; }
  void set_close_duration(uint32_t dur) { this->close_duration_ = dur; }
  void set_open_duration(uint32_t dur) { this->open_duration_ = dur; }
  void set_poll_interval(uint32_t intvl) { this->poll_intvl_ = intvl; }
  void set_supports_tilt(bool tilt) { this->supports_tilt_ = tilt; }
  void set_check_interval(uint32_t check_interval_ms) { this->check_interval_ms_ = check_interval_ms; }
  uint32_t get_blind_address() { return this->command_.blind_addr; }
  uint32_t get_remote_address() const { return this->command_.remote_addr; }
  void set_rx_state(uint8_t state);
  void handle_commands(uint32_t now);
  void recompute_position();
  void start_movement(cover::CoverOperation op);
  bool is_at_target();
  bool is_in_recovery() const { return counter_recovery_attempts_ > 0; }
  
  // Methods for handling external commands from physical remotes
  uint8_t get_command_up() const { return command_up_; }
  uint8_t get_command_down() const { return command_down_; }
  uint8_t get_command_stop() const { return command_stop_; }
  void sync_external_command(cover::CoverOperation op);
  void sync_counter(uint8_t remote_cnt);

 protected:
  void control(const cover::CoverCall &call) override;
  void increase_counter();
  void check_silent_failure();
  uint8_t get_sweep_counter(uint8_t original, uint8_t attempt_index);

  static constexpr uint8_t RECOVERY_SWEEP_RANGE = 50;
  static constexpr uint8_t RECOVERY_MAX_ATTEMPTS = 10;

  t_elero_command command_ = {
    .counter = 1,
  };
  Elero *parent_;
  uint32_t last_poll_{0};
  uint32_t last_command_{0};
  uint32_t poll_offset_{0};
  uint32_t movement_start_{0};
  uint32_t open_duration_{0};
  uint32_t close_duration_{0};
  uint32_t last_publish_{0};
  uint32_t last_recompute_time_{0};
  uint32_t poll_intvl_{0};
  float target_position_{0};
  bool supports_tilt_{false};
  uint8_t command_up_{0x20};
  uint8_t command_down_{0x40};
  uint8_t command_check_{0x00};
  uint8_t command_stop_{0x10};
  uint8_t command_tilt_{0x24};
  std::queue<uint8_t> commands_to_send_;
  uint8_t send_retries_{0};
  uint8_t send_packets_{0};
  cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};
  
  // Silent failure detection
  uint32_t last_command_sent_time_{0};
  uint8_t last_command_sent_{0};
  bool waiting_for_response_{false};
  uint32_t last_rx_{0};  // Timestamp of last received message
  static constexpr uint32_t RESPONSE_TIMEOUT_MS = 10000;  // 10 seconds
  
  // Counter recovery
  uint8_t counter_recovery_attempts_{0};
  uint8_t original_counter_{0};

  // NVS counter persistence
  ESPPreferenceObject counter_pref_;

  // Post-command confirmation CHECK
  bool pending_confirmation_check_{false};
  uint32_t confirmation_check_time_{0};
  uint8_t confirmation_command_sent_{0};
  static constexpr uint32_t CONFIRMATION_DELAY_MS = 3000;

  uint32_t check_interval_ms_{0}; // Disabled by default; counter sync from overheard remotes is preferred
  uint32_t last_check_time_{0};

  // Counter conservation: suppress polls/CHECKs when desynced
  uint32_t last_successful_rx_{0};  // Last time we got ANY response from this blind
  static constexpr uint32_t DESYNC_THRESHOLD_MS = 30000;  // Consider desynced after 30s no response

  // Remote activity cooldown: don't send commands while the physical remote is active
  uint32_t last_remote_activity_{0};  // Last time we overheard the physical remote
  static constexpr uint32_t REMOTE_COOLDOWN_MS = 5000;  // Wait 5s after last remote packet
};

} // namespace elero
} // namespace esphome

