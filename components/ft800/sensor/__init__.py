"""FT800 tracker sensor — reports normalised 0.0–1.0 value from a slider/dial TAG."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, UNIT_PERCENT, ICON_PERCENT

from .. import ft800_ns, FT800Component

CONF_FT800_ID = "ft800_id"
CONF_TAG      = "tag"

FT800Sensor = ft800_ns.class_(
    "FT800Sensor", sensor.Sensor, cg.Component
)

CONFIG_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_PERCENT,
    icon=ICON_PERCENT,
    accuracy_decimals=0,
).extend(
    {
        cv.GenerateID(): cv.declare_id(FT800Sensor),
        cv.GenerateID(CONF_FT800_ID): cv.use_id(FT800Component),
        cv.Required(CONF_TAG): cv.int_range(min=1, max=255),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_FT800_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_TAG])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)
