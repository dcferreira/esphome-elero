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
  LAST_RSSI = 3
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
  
  void update_value(float value);

 protected:
  Elero *parent_{nullptr};
  EleroMetricType metric_type_{SILENT_FAILURES};
};

}  // namespace elero
}  // namespace esphome 