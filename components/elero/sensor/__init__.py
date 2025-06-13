"""ESPHome sensor platform for Elero component debug metrics."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_SIGNAL_STRENGTH,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_EMPTY,
    UNIT_MILLISECOND,
    UNIT_PERCENT,
)

from .. import CONF_ELERO_ID, elero_ns

DEPENDENCIES = ["elero"]

EleroSensor = elero_ns.class_("EleroSensor", sensor.Sensor, cg.Component)

# Core debug metrics
CONF_SILENT_FAILURES = "silent_failures"
CONF_SUCCESS_RATE = "success_rate"
CONF_AVG_RESPONSE_TIME = "average_response_time"
CONF_LAST_RSSI = "last_rssi"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ELERO_ID): cv.use_id("Elero"),
        cv.Optional(CONF_SILENT_FAILURES): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_SUCCESS_RATE): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AVG_RESPONSE_TIME): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement=UNIT_MILLISECOND,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_LAST_RSSI): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement="dBm",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate C++ code for Elero sensor platform."""
    elero_parent = await cg.get_variable(config[CONF_ELERO_ID])

    if CONF_SILENT_FAILURES in config:
        var = await sensor.new_sensor(config[CONF_SILENT_FAILURES])
        await cg.register_component(var, config[CONF_SILENT_FAILURES])
        cg.add(var.set_elero_parent(elero_parent))
        cg.add(var.set_metric_type(0))  # SILENT_FAILURES
        cg.add(elero_parent.set_silent_failures_sensor(var))

    if CONF_SUCCESS_RATE in config:
        var = await sensor.new_sensor(config[CONF_SUCCESS_RATE])
        await cg.register_component(var, config[CONF_SUCCESS_RATE])
        cg.add(var.set_elero_parent(elero_parent))
        cg.add(var.set_metric_type(1))  # SUCCESS_RATE
        cg.add(elero_parent.set_success_rate_sensor(var))

    if CONF_AVG_RESPONSE_TIME in config:
        var = await sensor.new_sensor(config[CONF_AVG_RESPONSE_TIME])
        await cg.register_component(var, config[CONF_AVG_RESPONSE_TIME])
        cg.add(var.set_elero_parent(elero_parent))
        cg.add(var.set_metric_type(2))  # AVG_RESPONSE_TIME
        cg.add(elero_parent.set_avg_response_time_sensor(var))

    if CONF_LAST_RSSI in config:
        var = await sensor.new_sensor(config[CONF_LAST_RSSI])
        await cg.register_component(var, config[CONF_LAST_RSSI])
        cg.add(var.set_elero_parent(elero_parent))
        cg.add(var.set_metric_type(3))  # LAST_RSSI
        cg.add(elero_parent.set_last_rssi_sensor(var))
