#include "EleroCover.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace elero {

using namespace esphome::cover;

static const char *const TAG = "elero.cover";

void EleroCover::dump_config() {
  LOG_COVER("", "Elero Cover", this);
}

void EleroCover::setup() {
  this->parent_->register_cover(this);
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    if((this->open_duration_ > 0) && (this->close_duration_ > 0))
      this->position = 0.5f;
  }

  // Persist counter to NVS flash using a hash derived from the blind address
  uint32_t hash = this->command_.blind_addr * 2654435761UL;  // Knuth multiplicative hash
  this->counter_pref_ = global_preferences->make_preference<uint8_t>(hash);
  uint8_t saved_counter = 0;
  if (this->counter_pref_.load(&saved_counter) && saved_counter >= 1) {
    this->command_.counter = saved_counter;
    ESP_LOGI(TAG, "COUNTER RESTORE: Loaded counter=%d from NVS for blind 0x%06x",
             saved_counter, this->command_.blind_addr);
  } else {
    ESP_LOGI(TAG, "COUNTER RESTORE: No saved counter for blind 0x%06x, starting at %d",
             this->command_.blind_addr, this->command_.counter);
    this->counter_pref_.save(&this->command_.counter);
  }
}

void EleroCover::loop() {
  uint32_t intvl = this->poll_intvl_;
  uint32_t now = millis();
  if(this->current_operation != COVER_OPERATION_IDLE) {
    if((now - ELERO_TIMEOUT_MOVEMENT) < this->movement_start_) // do not poll frequently for an extended period of time
      intvl = ELERO_POLL_INTERVAL_MOVING;
  }

  if((now > this->poll_offset_) && (now - this->poll_offset_ - this->last_poll_) > intvl) {
    this->commands_to_send_.push(this->command_check_);
    this->last_poll_ = now - this->poll_offset_;
  }

  this->handle_commands(now);

  if((this->current_operation != COVER_OPERATION_IDLE) && (this->open_duration_ > 0) && (this->close_duration_ > 0)) {
    this->recompute_position();
    if(this->is_at_target()) {
      this->commands_to_send_.push(this->command_stop_);
      this->current_operation = COVER_OPERATION_IDLE;
      this->target_position_ = COVER_OPEN;
    }

    // Publish position every second
    if(now - this->last_publish_ > 1000) {
      this->publish_state(false);
      this->last_publish_ = now;
    }
  }

  // Check for silent failures
  this->check_silent_failure();
}

bool EleroCover::is_at_target() {
  // We return false as we don't want to send a stop command for completely open or
  // close - this is handled by the cover
  if((this->target_position_ == COVER_OPEN) || (this->target_position_ == COVER_CLOSED))
    return false;

  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      return this->position >= this->target_position_;
    case COVER_OPERATION_CLOSING:
      return this->position <= this->target_position_;
    case COVER_OPERATION_IDLE:
    default:
      return true;
  }
}

void EleroCover::handle_commands(uint32_t now) {
  // Post-command confirmation CHECK: send a CHECK ~3s after a movement command completes
  if (this->pending_confirmation_check_ && this->commands_to_send_.empty() &&
      !this->waiting_for_response_ && now >= this->confirmation_check_time_) {
    ESP_LOGD(TAG, "CONFIRM CHECK: Sending confirmation CHECK for blind 0x%06x after command 0x%02x",
             this->command_.blind_addr, this->confirmation_command_sent_);
    this->commands_to_send_.push(ELERO_COMMAND_COVER_CHECK);
    this->pending_confirmation_check_ = false;
  }

  // Periodic CHECK command to keep counter in sync and update status
  if (check_interval_ms_ > 0 && (now - last_check_time_) > check_interval_ms_) {
    // Only send CHECK if we're not currently processing commands
    if (this->commands_to_send_.empty() && !this->waiting_for_response_) {
      ESP_LOGD(TAG, "PERIODIC CHECK: Sending CHECK command to blind 0x%06x (interval=%dms)", 
               this->command_.blind_addr, check_interval_ms_);
      this->commands_to_send_.push(ELERO_COMMAND_COVER_CHECK);
      last_check_time_ = now;
    } else {
      // Defer CHECK if busy, but don't let it drift too far
      if ((now - last_check_time_) > (check_interval_ms_ * 2)) {
        ESP_LOGD(TAG, "PERIODIC CHECK: Forcing CHECK despite busy state for blind 0x%06x", 
                 this->command_.blind_addr);
        this->commands_to_send_.push(ELERO_COMMAND_COVER_CHECK);
        last_check_time_ = now;
      }
    }
  }

  if((now - this->last_command_) > ELERO_DELAY_SEND_PACKETS) {
    if(this->commands_to_send_.size() > 0) {
      uint8_t command_byte = this->commands_to_send_.front();
      this->command_.payload[4] = command_byte;
      
      // Log command attempt with readable name
      const char* cmd_name = "UNKNOWN";
      switch(command_byte) {
        case ELERO_COMMAND_COVER_CHECK: cmd_name = "CHECK"; break;
        case ELERO_COMMAND_COVER_STOP: cmd_name = "STOP"; break;
        case ELERO_COMMAND_COVER_UP: cmd_name = "UP"; break;
        case ELERO_COMMAND_COVER_DOWN: cmd_name = "DOWN"; break;
        case ELERO_COMMAND_COVER_TILT: cmd_name = "TILT"; break;
        case ELERO_COMMAND_COVER_INT: cmd_name = "INT"; break;
      }
      
      // Track command for silent failure detection
      uint32_t command_start_time = millis();
      
      if(this->parent_->send_command(&this->command_)) {
        this->send_packets_++;
        this->send_retries_ = 0;
        ESP_LOGD(TAG, "CMD SENT: %s (packet %d/%d) to blind 0x%06x", 
                 cmd_name, this->send_packets_, ELERO_SEND_PACKETS, this->command_.blind_addr);
        
        // Track command for silent failure detection (all commands expect responses)
        this->last_command_sent_time_ = command_start_time;
        this->last_command_sent_ = command_byte;
        this->waiting_for_response_ = true;
        
        if(this->send_packets_ >= ELERO_SEND_PACKETS) {
          this->commands_to_send_.pop();
          this->send_packets_ = 0;
          this->increase_counter();
          ESP_LOGD(TAG, "CMD COMPLETE: %s command finished, counter incremented to %d", 
                   cmd_name, this->command_.counter);
          
          // Update current counter for per-blind sensors
          if (this->parent_) {
            this->parent_->update_per_blind_current_counter(this->command_.blind_addr, this->command_.counter);
          }

          // Schedule a confirmation CHECK after non-CHECK commands
          if (command_byte != ELERO_COMMAND_COVER_CHECK) {
            this->pending_confirmation_check_ = true;
            this->confirmation_check_time_ = millis() + CONFIRMATION_DELAY_MS;
            this->confirmation_command_sent_ = command_byte;
          }
        }
      } else {
        ESP_LOGW(TAG, "CMD RETRY: %s command failed (retry #%d for blind 0x%06x)", 
                 cmd_name, this->send_retries_, this->command_.blind_addr);
        this->send_retries_++;
        if(this->send_retries_ > ELERO_SEND_RETRIES) {
          ESP_LOGE(TAG, "CMD FAILED: %s command failed after %d retries, giving up.", 
                   cmd_name, ELERO_SEND_RETRIES);
          this->send_retries_ = 0;
          this->commands_to_send_.pop();
          this->waiting_for_response_ = false; // Clear waiting state on failure
        }
      }
      this->last_command_ = now;
    }
  }
}

float EleroCover::get_setup_priority() const { return setup_priority::DATA; }

cover::CoverTraits EleroCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_supports_stop(true);
  if((this->open_duration_ > 0) && (this->close_duration_ > 0))
    traits.set_supports_position(true);
  else
    traits.set_supports_position(false);
  traits.set_supports_toggle(true);
  traits.set_is_assumed_state(true);
  traits.set_supports_tilt(this->supports_tilt_);
  return traits;
}

void EleroCover::set_rx_state(uint8_t state) {
  // Cancel pending confirmation CHECK — we got a response
  this->pending_confirmation_check_ = false;

  // Track response time for debug sensors
  uint32_t response_time = 0;
  if (this->waiting_for_response_) {
    response_time = millis() - this->last_command_sent_time_;
    this->waiting_for_response_ = false;
    
    // Reset counter recovery on successful response
    if (this->counter_recovery_attempts_ > 0) {
      int offset = (int)this->command_.counter - (int)this->original_counter_;
      if (offset > 127) offset -= 255;
      if (offset < -127) offset += 255;
      char strategy_buf[16];
      snprintf(strategy_buf, sizeof(strategy_buf), "%+d", offset);
      const char* strategy_name = strategy_buf;

      ESP_LOGI(TAG, "[COUNTER_RECOVERY] SUCCESS: CHECK command succeeded with counter %d (offset %s) for blind 0x%06x",
               this->command_.counter, strategy_name, this->command_.blind_addr);
      
      // Update parent stats for per-blind sensors
      if (this->parent_) {
        this->parent_->update_per_blind_counter_stats(this->command_.blind_addr, this->command_.counter, true, std::string(strategy_name));
      }

      // Persist recovered counter to NVS
      this->counter_pref_.save(&this->command_.counter);

      // Now retry the original command with the working counter
      ESP_LOGI(TAG, "[COUNTER_RECOVERY] Retrying original command with working counter %d for blind 0x%06x",
               this->command_.counter, this->command_.blind_addr);
      this->commands_to_send_.push(this->last_command_sent_);

      this->counter_recovery_attempts_ = 0;
    }
    
    // Notify parent about response time
    if (this->parent_) {
      this->parent_->add_response_time(response_time);
    }
  } else {
    // Even if not waiting for response, update last response time for time-based sensors
    if (this->parent_) {
      this->parent_->update_per_blind_last_response_time(this->command_.blind_addr);
    }
  }
  
  // Decode state for readable logging
  const char* state_name = "UNKNOWN";
  switch(state) {
    case ELERO_STATE_TOP: state_name = "TOP"; break;
    case ELERO_STATE_BOTTOM: state_name = "BOTTOM"; break;
    case ELERO_STATE_INTERMEDIATE: state_name = "INTERMEDIATE"; break;
    case ELERO_STATE_TILT: state_name = "TILT"; break;
    case ELERO_STATE_BLOCKING: state_name = "BLOCKING"; break;
    case ELERO_STATE_OVERHEATED: state_name = "OVERHEATED"; break;
    case ELERO_STATE_TIMEOUT: state_name = "TIMEOUT"; break;
    case ELERO_STATE_START_MOVING_UP: state_name = "START_MOVING_UP"; break;
    case ELERO_STATE_START_MOVING_DOWN: state_name = "START_MOVING_DOWN"; break;
    case ELERO_STATE_MOVING_UP: state_name = "MOVING_UP"; break;
    case ELERO_STATE_MOVING_DOWN: state_name = "MOVING_DOWN"; break;
    case ELERO_STATE_STOPPED: state_name = "STOPPED"; break;
    case ELERO_STATE_TOP_TILT: state_name = "TOP_TILT"; break;
    case ELERO_STATE_BOTTOM_TILT: state_name = "BOTTOM_TILT"; break;
  }
  
  if (response_time > 0) {
    if (this->last_command_sent_ == ELERO_COMMAND_COVER_CHECK) {
      if (this->counter_recovery_attempts_ > 0) {
        ESP_LOGD(TAG, "[RECOVERY_CHECK] Response: %s (0x%02x) from blind 0x%06x (response_time=%" PRIu32 "ms)", 
                 state_name, state, this->command_.blind_addr, response_time);
      } else {
        ESP_LOGD(TAG, "[PERIODIC_CHECK] Response: %s (0x%02x) from blind 0x%06x (response_time=%" PRIu32 "ms)", 
                 state_name, state, this->command_.blind_addr, response_time);
      }
    } else {
      ESP_LOGD(TAG, "STATE RX: %s (0x%02x) from blind 0x%06x (response_time=%" PRIu32 "ms)", 
               state_name, state, this->command_.blind_addr, response_time);
    }
  } else {
    ESP_LOGD(TAG, "STATE RX: %s (0x%02x) from blind 0x%06x", state_name, state, this->command_.blind_addr);
  }
  
  float pos = this->position;
  float current_tilt = this->tilt;
  CoverOperation op = this->current_operation;
  CoverOperation prev_op = this->current_operation;

  switch(state) {
  case ELERO_STATE_TOP:
    pos = COVER_OPEN;
    op = COVER_OPERATION_IDLE;
    current_tilt = 0.0;
    break;
  case ELERO_STATE_BOTTOM:
    pos = COVER_CLOSED;
    op = COVER_OPERATION_IDLE;
    current_tilt = 0.0;
    break;
  case ELERO_STATE_START_MOVING_UP:
  case ELERO_STATE_MOVING_UP:
    op = COVER_OPERATION_OPENING;
    current_tilt = 0.0;
    break;
  case ELERO_STATE_START_MOVING_DOWN:
  case ELERO_STATE_MOVING_DOWN:
    op = COVER_OPERATION_CLOSING;
    current_tilt = 0.0;
    break;
  case ELERO_STATE_TILT:
    op = COVER_OPERATION_IDLE;
    current_tilt = 1.0;
    break;
  case ELERO_STATE_STOPPED:
    op = COVER_OPERATION_IDLE;
    current_tilt = 0.0;
    break;
  default:
    op = COVER_OPERATION_IDLE;
    current_tilt = 0.0;
  }

  bool state_changed = (pos != this->position) || (op != this->current_operation) || (current_tilt != this->tilt);
  if(state_changed) {
    ESP_LOGD(TAG, "STATE CHANGE: blind 0x%06x %s -> %s, pos=%.2f->%.2f, op=%d->%d", 
             this->command_.blind_addr, 
             (prev_op == COVER_OPERATION_IDLE) ? "IDLE" : 
             (prev_op == COVER_OPERATION_OPENING) ? "OPENING" : "CLOSING",
             (op == COVER_OPERATION_IDLE) ? "IDLE" : 
             (op == COVER_OPERATION_OPENING) ? "OPENING" : "CLOSING",
             this->position, pos, prev_op, op);
    
    this->position = pos;
    this->tilt = current_tilt;
    this->current_operation = op;
    this->publish_state();
  } else {
    ESP_LOGV(TAG, "STATE UNCHANGED: %s state received but no change needed", state_name);
  }
  
  this->last_rx_ = millis();
}

void EleroCover::check_silent_failure() {
  if (this->waiting_for_response_) {
    uint32_t elapsed = millis() - this->last_command_sent_time_;
    if (elapsed > RESPONSE_TIMEOUT_MS) {
      // Silent failure detected — cancel any pending confirmation CHECK
      this->pending_confirmation_check_ = false;

      const char* cmd_name = "UNKNOWN";
      switch(this->last_command_sent_) {
        case ELERO_COMMAND_COVER_STOP: cmd_name = "STOP"; break;
        case ELERO_COMMAND_COVER_UP: cmd_name = "UP"; break;
        case ELERO_COMMAND_COVER_DOWN: cmd_name = "DOWN"; break;
        case ELERO_COMMAND_COVER_TILT: cmd_name = "TILT"; break;
        case ELERO_COMMAND_COVER_INT: cmd_name = "INT"; break;
        case ELERO_COMMAND_COVER_CHECK: cmd_name = "CHECK"; break;
      }
      
      // Handle CHECK command failures differently
      if (this->last_command_sent_ == ELERO_COMMAND_COVER_CHECK) {
        if (this->counter_recovery_attempts_ > 0) {
          // This was a recovery CHECK that failed
          ESP_LOGW(TAG, "[RECOVERY_CHECK] FAILED: CHECK command with counter=%d failed for blind 0x%06x (attempt %d/%d)",
                   this->command_.counter, this->command_.blind_addr, this->counter_recovery_attempts_, RECOVERY_MAX_ATTEMPTS);

          // Continue with next recovery attempt or give up
          if (this->counter_recovery_attempts_ < RECOVERY_MAX_ATTEMPTS) {
            uint8_t test_counter = this->get_sweep_counter(this->original_counter_, this->counter_recovery_attempts_);
            this->command_.counter = test_counter;
            this->counter_recovery_attempts_++;

            int offset = (int)test_counter - (int)this->original_counter_;
            if (offset > 127) offset -= 255;
            if (offset < -127) offset += 255;

            ESP_LOGI(TAG, "[COUNTER_RECOVERY] Attempt %d/%d: trying CHECK with counter=%d (offset %+d) for blind 0x%06x",
                     this->counter_recovery_attempts_, RECOVERY_MAX_ATTEMPTS, test_counter, offset, this->command_.blind_addr);

            if (this->parent_) {
              char buf[16];
              snprintf(buf, sizeof(buf), "%+d", offset);
              this->parent_->update_per_blind_counter_stats(this->command_.blind_addr, test_counter, false, std::string(buf));
            }

            this->commands_to_send_.push(ELERO_COMMAND_COVER_CHECK);
            this->waiting_for_response_ = false;
          } else {
            ESP_LOGE(TAG, "[COUNTER_RECOVERY] FAILED: giving up after %d attempts for blind 0x%06x (tried offsets -1..+%d from counter %d)",
                     RECOVERY_MAX_ATTEMPTS, this->command_.blind_addr, RECOVERY_SWEEP_RANGE, this->original_counter_);

            if (this->parent_) {
              this->parent_->increment_silent_failures();
            }

            this->waiting_for_response_ = false;
            this->counter_recovery_attempts_ = 0;
          }
        } else {
          // This was a periodic CHECK that failed
          ESP_LOGW(TAG, "[PERIODIC_CHECK] FAILED: CHECK command failed for blind 0x%06x (no response after %" PRIu32 "ms)",
                   this->command_.blind_addr, elapsed);
          this->waiting_for_response_ = false;
        }
        return; // Don't continue with normal recovery logic for CHECK commands
      }

      // Start CHECK-based counter recovery for action commands
      if (this->counter_recovery_attempts_ == 0) {
        this->original_counter_ = this->command_.counter;
        ESP_LOGI(TAG, "[COUNTER_RECOVERY] Starting CHECK-based recovery for blind 0x%06x - %s command failed, original_counter=%d",
                 this->command_.blind_addr, cmd_name, this->original_counter_);
      }

      // Try CHECK commands with different counter values using sweep pattern
      if (this->counter_recovery_attempts_ < RECOVERY_MAX_ATTEMPTS) {
        uint8_t test_counter = this->get_sweep_counter(this->original_counter_, this->counter_recovery_attempts_);
        this->command_.counter = test_counter;
        this->counter_recovery_attempts_++;

        int offset = (int)test_counter - (int)this->original_counter_;
        if (offset > 127) offset -= 255;
        if (offset < -127) offset += 255;

        ESP_LOGI(TAG, "[COUNTER_RECOVERY] Attempt %d/%d: trying CHECK with counter=%d (offset %+d) for blind 0x%06x",
                 this->counter_recovery_attempts_, RECOVERY_MAX_ATTEMPTS, test_counter, offset, this->command_.blind_addr);

        if (this->parent_) {
          char buf[16];
          snprintf(buf, sizeof(buf), "%+d", offset);
          this->parent_->update_per_blind_counter_stats(this->command_.blind_addr, test_counter, false, std::string(buf));
        }

        this->commands_to_send_.push(ELERO_COMMAND_COVER_CHECK);
        this->waiting_for_response_ = false;

      } else {
        ESP_LOGE(TAG, "[COUNTER_RECOVERY] FAILED: giving up after %d attempts for blind 0x%06x (tried offsets -1..+%d from counter %d)",
                 RECOVERY_MAX_ATTEMPTS, this->command_.blind_addr, RECOVERY_SWEEP_RANGE, this->original_counter_);

        if (this->parent_) {
          this->parent_->increment_silent_failures();
        }

        this->waiting_for_response_ = false;
        this->counter_recovery_attempts_ = 0;
      }
    }
  }
}

uint8_t EleroCover::get_sweep_counter(uint8_t original, uint8_t attempt_index) {
  // Sweep order: -1, +1, -2, +2, -3, +3, -4, +4, -5, +5
  int offset = (attempt_index / 2) + 1;
  if (attempt_index % 2 == 0)
    offset = -offset;  // even indices: negative offsets
  int result = (int)original + offset;
  // Wrap within valid range 1-255 (counter 0 is invalid)
  if (result < 1)
    result += 255;
  else if (result > 255)
    result -= 255;
  return (uint8_t)result;
}

void EleroCover::increase_counter() {
  if(this->command_.counter == 0xff)
    this->command_.counter = 1;
  else
    this->command_.counter += 1;
  this->counter_pref_.save(&this->command_.counter);
}

void EleroCover::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    this->start_movement(COVER_OPERATION_IDLE);
  }
  if (call.get_position().has_value()) {
    auto pos = *call.get_position();
    this->target_position_ = pos;
    if((pos > this->position) || (pos == COVER_OPEN)) {
      this->start_movement(COVER_OPERATION_OPENING);
    } else {
      this->start_movement(COVER_OPERATION_CLOSING);
    }
  }
  if (call.get_tilt().has_value()) {
    auto tilt = *call.get_tilt();
    if(tilt > 0) {
      this->commands_to_send_.push(this->command_tilt_);
      this->tilt = 1.0;
    } else {
      this->tilt = 0.0;
    }
  }
  if (call.get_toggle().has_value()) {
    if(this->current_operation != COVER_OPERATION_IDLE) {
      this->start_movement(COVER_OPERATION_IDLE);
    } else {
      if(this->position == COVER_CLOSED || this->last_operation_ == COVER_OPERATION_CLOSING) {
        this->target_position_ = COVER_OPEN;
        this->start_movement(COVER_OPERATION_OPENING);
      } else {
        this->target_position_ = COVER_CLOSED;
        this->start_movement(COVER_OPERATION_CLOSING);
      }
    }
  }
}

// FIXME: Most of this should probably be moved to the
// handle_commands function to only publish a new state
// if at least the transmission was successful
void EleroCover::start_movement(CoverOperation dir) {
  const char* dir_name = (dir == COVER_OPERATION_OPENING) ? "OPENING" : 
                        (dir == COVER_OPERATION_CLOSING) ? "CLOSING" : "STOP";
  
  switch(dir) {
    case COVER_OPERATION_OPENING:
      ESP_LOGD(TAG, "MOVEMENT START: OPEN command queued for blind 0x%06x", this->command_.blind_addr);
      this->commands_to_send_.push(this->command_up_);
      // Reset tilt state on movement
      this->tilt = 0.0;
      this->last_operation_ = COVER_OPERATION_OPENING;
    break;
    case COVER_OPERATION_CLOSING:
      ESP_LOGD(TAG, "MOVEMENT START: CLOSE command queued for blind 0x%06x", this->command_.blind_addr);
      this->commands_to_send_.push(this->command_down_);
      // Reset tilt state on movement
      this->tilt = 0.0;
      this->last_operation_ = COVER_OPERATION_CLOSING;
    break;
    case COVER_OPERATION_IDLE:
      ESP_LOGD(TAG, "MOVEMENT START: STOP command queued for blind 0x%06x", this->command_.blind_addr);
      this->commands_to_send_.push(this->command_stop_);
    break;
  }

  if(dir == this->current_operation) {
    ESP_LOGV(TAG, "MOVEMENT SKIP: Already in %s state, ignoring duplicate command", dir_name);
    return;
  }

  ESP_LOGD(TAG, "MOVEMENT CHANGE: blind 0x%06x %s -> %s at t=%" PRIu32, 
           this->command_.blind_addr,
           (this->current_operation == COVER_OPERATION_IDLE) ? "IDLE" : 
           (this->current_operation == COVER_OPERATION_OPENING) ? "OPENING" : "CLOSING",
           dir_name, millis());

  this->current_operation = dir;
  this->movement_start_ = millis();
  this->last_recompute_time_ = millis();
  this->publish_state();
}

void EleroCover::recompute_position() {
  if(this->current_operation == COVER_OPERATION_IDLE)
    return;


  float dir;
  float action_dur;
  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      dir = 1.0f;
      action_dur = this->open_duration_;
      break;
    case COVER_OPERATION_CLOSING:
      dir = -1.0f;
      action_dur = this->close_duration_;
      break;
    default:
      return;
  }

  const uint32_t now = millis();
  this->position += dir * (now - this->last_recompute_time_) / action_dur;
  this->position = clamp(this->position, 0.0f, 1.0f);

  this->last_recompute_time_ = now;

}

void EleroCover::sync_external_command(cover::CoverOperation op) {
  // This method is called when we overhear a command from a physical remote
  // We need to sync our internal state to match what the physical remote is doing
  
  if (op == this->current_operation) {
    ESP_LOGV(TAG, "SYNC: Already in requested state %d, no change needed", (int)op);
    return;
  }
  
  const char* op_name = (op == cover::COVER_OPERATION_OPENING) ? "OPENING" : 
                       (op == cover::COVER_OPERATION_CLOSING) ? "CLOSING" : "IDLE";
  
  ESP_LOGD(TAG, "SYNC: External command changing operation from %d to %d (%s) for blind 0x%06x", 
           (int)this->current_operation, (int)op, op_name, this->command_.blind_addr);
  
  // Update our operation state
  this->current_operation = op;
  
  // If starting movement, reset timing
  if (op != cover::COVER_OPERATION_IDLE) {
    this->movement_start_ = millis();
    this->last_recompute_time_ = millis();
    
    // Set appropriate target position based on operation
    // For external commands, we assume full movement unless stopped
    if (op == cover::COVER_OPERATION_OPENING) {
      this->target_position_ = COVER_OPEN;
      this->last_operation_ = cover::COVER_OPERATION_OPENING;
    } else if (op == cover::COVER_OPERATION_CLOSING) {
      this->target_position_ = COVER_CLOSED;
      this->last_operation_ = cover::COVER_OPERATION_CLOSING;
    }
  }
  
  // Publish the updated state
  this->publish_state();
}

} // namespace elero
} // namespace esphome
