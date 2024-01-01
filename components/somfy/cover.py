import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import (
    CONF_ID,
)

CONF_REMOTEID_KEY = 'RemoteID'

somfy_ns = cg.esphome_ns.namespace("somfy")
SomfyCover = somfy_ns.class_("RFsomfy", cover.Cover, cg.Component)

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SomfyCover),
        cv.Required(CONF_REMOTEID_KEY): cv.int_,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    cg.add(var.setCoverID(config[CONF_REMOTEID_KEY]))
    
    cg.add_library("FS", None)
    cg.add_library("SPIFFS", None)
    cg.add_library("SPI", None)
    cg.add_library(
        name="CC1101",
        repository="https://github.com/Viproz/SmartRC-CC1101-Driver-Lib",
        version=None
    )