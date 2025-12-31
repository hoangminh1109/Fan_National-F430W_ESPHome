# Copyright 2025 Minh Hoang
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from esphome import pins
from esphome.core import CORE
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan, i2c, text_sensor, button, remote_base
from esphome.const import (
    CONF_OUTPUT_ID,
    CONF_ID,
    CONF_NAME,
    CONF_DISABLED_BY_DEFAULT,
    CONF_INPUT,
    CONF_PULLDOWN
)

DEPENDENCIES = ["remote_transmitter", "i2c"]
AUTO_LOAD = ["fan", "remote_base", "i2c", "text_sensor", "button"]

f430wfan_ns = cg.esphome_ns.namespace('f430wfan')
F430WFan = f430wfan_ns.class_(
    "F430WFan",
    cg.Component,
    fan.Fan,
    i2c.I2CDevice,
    remote_base.RemoteReceiverListener,
    remote_base.RemoteTransmittable,
    )
F430WFanTimer = f430wfan_ns.class_(
    'F430WFanTimer',
    text_sensor.TextSensor,
    cg.Component
    )
F430WFanSetTimer = f430wfan_ns.class_(
    'F430WFanSetTimer',
    button.Button,
    cg.Component
    )

CONF_FANTIMER_ID = "fantimer_id"
CONF_FANSETTIMER_ID = "fansettimer_id"
CONF_INTERVAL_MS = "interval"
CONF_PIN_SWING = "pin_sw"
CONF_PIN_LOW = "pin_lo"
CONF_PIN_MED = "pin_med"
CONF_PIN_HI = "pin_hi"
CONF_PIN_ROW1 = "pin_row1"
CONF_PIN_ROW2 = "pin_row2"
CONF_PIN_ROW3 = "pin_row3"
CONF_PIN_ROW4 = "pin_row4"
CONF_PIN_COL1 = "pin_col1"
CONF_PIN_COL2 = "pin_col2"

CONF_F430W_PINIDX_LOW = 0
CONF_F430W_PINIDX_MED = 1
CONF_F430W_PINIDX_HI = 2
CONF_F430W_PINIDX_SW = 3
CONF_F430W_PINIDX_ROW1 = 4
CONF_F430W_PINIDX_ROW2 = 5
CONF_F430W_PINIDX_ROW3 = 6
CONF_F430W_PINIDX_ROW4 = 7
CONF_F430W_PINIDX_COL1 = 8
CONF_F430W_PINIDX_COL2 = 9

CONF_F430W_DEFAULT_PIN_SW = 34
CONF_F430W_DEFAULT_PIN_LOW = 33
CONF_F430W_DEFAULT_PIN_MED = 32
CONF_F430W_DEFAULT_PIN_HI = 35
CONF_F430W_DEFAULT_PIN_ROW1 = 13
CONF_F430W_DEFAULT_PIN_ROW2 = 27
CONF_F430W_DEFAULT_PIN_ROW3 = 16
CONF_F430W_DEFAULT_PIN_ROW4 = 17
CONF_F430W_DEFAULT_PIN_COL1 = 25
CONF_F430W_DEFAULT_PIN_COL2 = 26

def validate_platform(config):
    if not CORE.is_esp32:
        raise cv.Invalid("F430W fan component only supports ESP32.")
    return config

CONFIG_SCHEMA = (fan.fan_schema(F430WFan).extend({
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(F430WFan),
        cv.GenerateID(CONF_FANTIMER_ID): cv.declare_id(F430WFanTimer),
        cv.GenerateID(CONF_FANSETTIMER_ID): cv.declare_id(F430WFanSetTimer),
        cv.Optional(CONF_INTERVAL_MS, default=10): cv.int_range(min=1),
        cv.Optional(remote_base.CONF_RECEIVER_ID): cv.use_id(remote_base.RemoteReceiverBase),
        cv.Optional(CONF_PIN_SWING, default=CONF_F430W_DEFAULT_PIN_SW): cv.All(pins.internal_gpio_input_pin_schema),
        cv.Optional(CONF_PIN_LOW, default=CONF_F430W_DEFAULT_PIN_LOW): cv.All(pins.internal_gpio_input_pin_schema),
        cv.Optional(CONF_PIN_MED, default=CONF_F430W_DEFAULT_PIN_MED): cv.All(pins.internal_gpio_input_pin_schema),
        cv.Optional(CONF_PIN_HI, default=CONF_F430W_DEFAULT_PIN_HI): cv.All(pins.internal_gpio_input_pin_schema),
        cv.Optional(CONF_PIN_ROW1, default=CONF_F430W_DEFAULT_PIN_ROW1): cv.All(pins.internal_gpio_input_pin_schema),
        cv.Optional(CONF_PIN_ROW2, default=CONF_F430W_DEFAULT_PIN_ROW2): cv.All(pins.internal_gpio_input_pin_schema),
        cv.Optional(CONF_PIN_ROW3, default=CONF_F430W_DEFAULT_PIN_ROW3): cv.All(pins.internal_gpio_input_pin_schema),
        cv.Optional(CONF_PIN_ROW4, default=CONF_F430W_DEFAULT_PIN_ROW4): cv.All(pins.internal_gpio_input_pin_schema),
        cv.Optional(CONF_PIN_COL1, default=CONF_F430W_DEFAULT_PIN_COL1): cv.All(pins.internal_gpio_input_pin_schema),
        cv.Optional(CONF_PIN_COL2, default=CONF_F430W_DEFAULT_PIN_COL2): cv.All(pins.internal_gpio_input_pin_schema),
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(remote_base.REMOTE_TRANSMITTABLE_SCHEMA)
    # .extend(i2c.i2c_device_schema(0x20))
    .add_extra(validate_platform)
    )


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await fan.register_fan(var, config)
    # await i2c.register_i2c_device(var, config)
    await remote_base.register_transmittable(var, config)

    pin_sw = await cg.gpio_pin_expression(config[CONF_PIN_SWING])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_SW, pin_sw))

    pin_lo = await cg.gpio_pin_expression(config[CONF_PIN_LOW])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_LOW, pin_lo))

    pin_med = await cg.gpio_pin_expression(config[CONF_PIN_MED])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_MED, pin_med))

    pin_hi = await cg.gpio_pin_expression(config[CONF_PIN_HI])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_HI, pin_hi))

    pin_row1 = await cg.gpio_pin_expression(config[CONF_PIN_ROW1])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_ROW1, pin_row1))

    pin_row2 = await cg.gpio_pin_expression(config[CONF_PIN_ROW2])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_ROW2, pin_row2))

    pin_row3 = await cg.gpio_pin_expression(config[CONF_PIN_ROW3])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_ROW3, pin_row3))

    pin_row4 = await cg.gpio_pin_expression(config[CONF_PIN_ROW4])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_ROW4, pin_row4))

    pin_col1 = await cg.gpio_pin_expression(config[CONF_PIN_COL1])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_COL1, pin_col1))

    pin_col2 = await cg.gpio_pin_expression(config[CONF_PIN_COL2])
    cg.add(var.set_pin_idx(CONF_F430W_PINIDX_COL2, pin_col2))

    # Fan timer text_sensor
    fantimer_default_config = { CONF_ID: config[CONF_FANTIMER_ID],
                                CONF_NAME: "Timer",
                                CONF_DISABLED_BY_DEFAULT: False}
    fantimer = cg.new_Pvariable(config[CONF_FANTIMER_ID])
    await text_sensor.register_text_sensor(fantimer, fantimer_default_config)
    await cg.register_component(fantimer, fantimer_default_config)
    cg.add(fantimer.set_parent_fan(var))
    cg.add(var.set_fan_timer(fantimer))

    # Fan set timer button
    fansettimer_default_config = { CONF_ID: config[CONF_FANSETTIMER_ID],
                                CONF_NAME: "Set Timer",
                                CONF_DISABLED_BY_DEFAULT: False}
    fansettimer = cg.new_Pvariable(config[CONF_FANSETTIMER_ID])
    await button.register_button(fansettimer, fansettimer_default_config)
    await cg.register_component(fansettimer, fansettimer_default_config)
    cg.add(fansettimer.set_parent_fan(var))
    cg.add(var.set_fan_settimer(fansettimer))

    cg.add(var.set_interval_ms(config[CONF_INTERVAL_MS]))

    if remote_base.CONF_RECEIVER_ID in config:
        await remote_base.register_listener(var, config)
