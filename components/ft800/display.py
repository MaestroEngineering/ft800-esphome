"""FT800 display platform for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display
from esphome.const import CONF_ID, CONF_LAMBDA, CONF_UPDATE_INTERVAL

from . import ft800_ns, FT800Component

CONF_FT800_ID = "ft800_id"

FT800Display = ft800_ns.class_("FT800Display", cg.PollingComponent)

CONFIG_SCHEMA = display.FULL_DISPLAY_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FT800Display),
        cv.GenerateID(CONF_FT800_ID): cv.use_id(FT800Component),
        cv.Optional(CONF_LAMBDA): cv.lambda_,
        cv.Optional(CONF_UPDATE_INTERVAL, default="50ms"): cv.update_interval,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_FT800_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent)
    await cg.register_component(var, config)
    await cg.register_polling_component(var, config)

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA],
            [(FT800Display.operator("ref"), "it")],
            return_type=cg.void,
        )
        cg.add(var.set_lambda(lambda_))
