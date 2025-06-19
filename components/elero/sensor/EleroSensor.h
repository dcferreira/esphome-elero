#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace elero {

class Elero;  // Forward declaration

enum EleroMetricType {
  SILENT_FAILURES = 0,
  SUCCESS_RATE = 1,
  AVG_RESPONSE_TIME = 2,
  LAST_RSSI = 3,
  BLINDS_IN_RECOVERY = 4,
  COUNTER_RECOVERY_ATTEMPTS = 5,
  COUNTER_RECOVERY_SUCCESSES = 6,
  CURRENT_COUNTER = 7,
  LAST_WORKING_COUNTER = 8,
  RECOVERY_STRATEGY_SUCCESS = 9,
  SECONDS_SINCE_LAST_RESPONSE = 10
};

class EleroSensor : public sensor::Sensor, public Component {
 public:
  EleroSensor() = default;
  
  void setup() override {}
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  void set_elero_parent(Elero *parent) { parent_ = parent; }
  void set_metric_type(int type) { 
    metric_type_ = static_cast<EleroMetricType>(type); 
  }
  void set_blind_address(uint32_t address) { blind_address_ = address; }
  
  void update_value(float value);
  uint32_t get_blind_address() const { return blind_address_; }

 protected:
  Elero *parent_{nullptr};
  EleroMetricType metric_type_{SILENT_FAILURES};
  uint32_t blind_address_{0};
};

}  // namespace elero
}  // namespace esphome 