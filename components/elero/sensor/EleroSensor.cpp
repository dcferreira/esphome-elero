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
    case BLINDS_IN_RECOVERY:
      metric_name = "Blinds In Recovery";
      break;
    case COUNTER_RECOVERY_ATTEMPTS:
      metric_name = "Counter Recovery Attempts";
      break;
    case COUNTER_RECOVERY_SUCCESSES:
      metric_name = "Counter Recovery Successes";
      break;
    case CURRENT_COUNTER:
      metric_name = "Current Counter";
      break;
    case LAST_WORKING_COUNTER:
      metric_name = "Last Working Counter";
      break;
    case RECOVERY_STRATEGY_SUCCESS:
      metric_name = "Recovery Strategy Success";
      break;
    case SECONDS_SINCE_LAST_RESPONSE:
      metric_name = "Seconds Since Last Response";
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