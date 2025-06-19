"""ESPHome sensor platform for Elero component debug metrics."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_NAME,
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

# Counter recovery metrics
CONF_BLINDS_IN_RECOVERY = "blinds_in_recovery"
CONF_ENABLE_PER_BLIND_SENSORS = "enable_per_blind_sensors"

# Per-blind sensor configuration
CONF_BLIND_ADDRESS = "blind_address"
CONF_COUNTER_RECOVERY_ATTEMPTS = "counter_recovery_attempts"
CONF_COUNTER_RECOVERY_SUCCESSES = "counter_recovery_successes"
CONF_CURRENT_COUNTER = "current_counter"
CONF_LAST_WORKING_COUNTER = "last_working_counter"
CONF_SECONDS_SINCE_LAST_RESPONSE = "seconds_since_last_response"

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
        cv.Optional(CONF_BLINDS_IN_RECOVERY): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_ENABLE_PER_BLIND_SENSORS): cv.boolean,
        # Per-blind sensor configuration
        cv.Optional(CONF_BLIND_ADDRESS): cv.hex_uint32_t,
        cv.Optional(CONF_NAME): cv.string,
        cv.Optional(CONF_COUNTER_RECOVERY_ATTEMPTS): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_COUNTER_RECOVERY_SUCCESSES): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_CURRENT_COUNTER): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_LAST_WORKING_COUNTER): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_SECONDS_SINCE_LAST_RESPONSE): sensor.sensor_schema(
            EleroSensor,
            unit_of_measurement="s",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate C++ code for Elero sensor platform."""
    elero_parent = await cg.get_variable(config[CONF_ELERO_ID])

    # System-wide sensors
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

    if CONF_BLINDS_IN_RECOVERY in config:
        var = await sensor.new_sensor(config[CONF_BLINDS_IN_RECOVERY])
        await cg.register_component(var, config[CONF_BLINDS_IN_RECOVERY])
        cg.add(var.set_elero_parent(elero_parent))
        cg.add(var.set_metric_type(4))  # BLINDS_IN_RECOVERY
        cg.add(elero_parent.set_blinds_in_recovery_sensor(var))

    if config.get(CONF_ENABLE_PER_BLIND_SENSORS, False):
        cg.add(elero_parent.set_enable_per_blind_sensors(True))

    # Per-blind sensors - require blind_address
    blind_address = config.get(CONF_BLIND_ADDRESS)
    if blind_address is not None:
        if CONF_COUNTER_RECOVERY_ATTEMPTS in config:
            var = await sensor.new_sensor(config[CONF_COUNTER_RECOVERY_ATTEMPTS])
            await cg.register_component(var, config[CONF_COUNTER_RECOVERY_ATTEMPTS])
            cg.add(var.set_elero_parent(elero_parent))
            cg.add(var.set_metric_type(5))  # COUNTER_RECOVERY_ATTEMPTS
            cg.add(var.set_blind_address(blind_address))
            cg.add(
                elero_parent.register_per_blind_sensor(
                    blind_address, "recovery_attempts", var
                )
            )

        if CONF_COUNTER_RECOVERY_SUCCESSES in config:
            var = await sensor.new_sensor(config[CONF_COUNTER_RECOVERY_SUCCESSES])
            await cg.register_component(var, config[CONF_COUNTER_RECOVERY_SUCCESSES])
            cg.add(var.set_elero_parent(elero_parent))
            cg.add(var.set_metric_type(6))  # COUNTER_RECOVERY_SUCCESSES
            cg.add(var.set_blind_address(blind_address))
            cg.add(
                elero_parent.register_per_blind_sensor(
                    blind_address, "recovery_successes", var
                )
            )

        if CONF_CURRENT_COUNTER in config:
            var = await sensor.new_sensor(config[CONF_CURRENT_COUNTER])
            await cg.register_component(var, config[CONF_CURRENT_COUNTER])
            cg.add(var.set_elero_parent(elero_parent))
            cg.add(var.set_metric_type(7))  # CURRENT_COUNTER
            cg.add(var.set_blind_address(blind_address))
            cg.add(
                elero_parent.register_per_blind_sensor(
                    blind_address, "current_counter", var
                )
            )

        if CONF_LAST_WORKING_COUNTER in config:
            var = await sensor.new_sensor(config[CONF_LAST_WORKING_COUNTER])
            await cg.register_component(var, config[CONF_LAST_WORKING_COUNTER])
            cg.add(var.set_elero_parent(elero_parent))
            cg.add(var.set_metric_type(8))  # LAST_WORKING_COUNTER
            cg.add(var.set_blind_address(blind_address))
            cg.add(
                elero_parent.register_per_blind_sensor(
                    blind_address, "last_working_counter", var
                )
            )

        if CONF_SECONDS_SINCE_LAST_RESPONSE in config:
            var = await sensor.new_sensor(config[CONF_SECONDS_SINCE_LAST_RESPONSE])
            await cg.register_component(var, config[CONF_SECONDS_SINCE_LAST_RESPONSE])
            cg.add(var.set_elero_parent(elero_parent))
            cg.add(var.set_metric_type(10))  # SECONDS_SINCE_LAST_RESPONSE
            cg.add(var.set_blind_address(blind_address))
            cg.add(
                elero_parent.register_per_blind_sensor(
                    blind_address, "seconds_since_last_response", var
                )
            )
