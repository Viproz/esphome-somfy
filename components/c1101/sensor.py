import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
)

CONF_FREQ_KEY = 'Frequency'

c1101_ns = cg.esphome_ns.namespace("c1101")
C1101Wrapper = c1101_ns.class_("C1101Wrapper", sensor.Sensor, cg.Component)

CONFIG_SCHEMA = sensor.SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(C1101Wrapper),
        cv.Required(CONF_FREQ_KEY): cv.float_,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)

    cg.add(var.setFrequency(config[CONF_FREQ_KEY]))
    
    cg.add_library("SPI", None)
    cg.add_library("Preferences", None)
    cg.add_library(
        name="CC1101",
        repository="https://github.com/Viproz/SmartRC-CC1101-Driver-Lib",
        version=None
    )