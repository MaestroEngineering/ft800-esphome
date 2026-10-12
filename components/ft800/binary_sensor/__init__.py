"""FT800 touch binary sensor — fires on press/release of a TAG."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

from .. import ft800_ns, FT800Component

CONF_FT800_ID = "ft800_id"
CONF_TAG      = "tag"

FT800BinarySensor = ft800_ns.class_(
    "FT800BinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FT800BinarySensor),
        cv.GenerateID(CONF_FT800_ID): cv.use_id(FT800Component),
        cv.Required(CONF_TAG): cv.int_range(min=1, max=255),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_FT800_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_TAG])
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)
