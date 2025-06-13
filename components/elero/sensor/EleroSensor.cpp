#include "EleroSensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace elero {

static const char *const TAG = "elero.sensor";

void EleroSensor::dump_config() {
  LOG_SENSOR("", "Elero Sensor", this);
  const char *metric_name = "Unknown";
  switch (metric_type_) {
    case SILENT_FAILURES:
      metric_name = "Silent Failures";
      break;
    case SUCCESS_RATE:
      metric_name = "Success Rate";
      break;
    case AVG_RESPONSE_TIME:
      metric_name = "Average Response Time";
      break;
    case LAST_RSSI:
      metric_name = "Last RSSI";
      break;
    default:
      metric_name = "Unknown";
      break;
  }
  ESP_LOGCONFIG(TAG, "  Metric Type: %s", metric_name);
}

void EleroSensor::update_value(float value) {
  this->publish_state(value);
}

}  // namespace elero
}  // namespace esphome 