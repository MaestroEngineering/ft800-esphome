"""FT800 EVE display component for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi
from esphome.const import (
    CONF_ID,
    CONF_BRIGHTNESS,
)

CODEOWNERS = ["@you"]
DEPENDENCIES = ["spi"]
AUTO_LOAD = ["sensor", "binary_sensor"]

# Namespace / class references
ft800_ns = cg.esphome_ns.namespace("ft800")
FT800Component = ft800_ns.class_("FT800Component", cg.Component, spi.SPIDevice)
DisplayProfile = ft800_ns.struct("DisplayProfile")

# ---- Config keys -----------------------------------------------------------
CONF_PD_PIN       = "pd_pin"
CONF_INT_PIN      = "int_pin"
CONF_DISPLAY_SIZE = "display_size"

# Display profiles — maps config string to C++ constant
DISPLAY_PROFILES = {
    "5in":   ft800_ns.PROFILE_WQVGA,
    "4.3in": ft800_ns.PROFILE_WQVGA,
    "3.5in": ft800_ns.PROFILE_QVGA,
}

DISPLAY_SIZE_HELP = (
    "Panel size. '5in' and '4.3in' both use WQVGA 480×272 timing. "
    "'3.5in' uses QVGA 320×240."
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(FT800Component),
            cv.Optional(CONF_PD_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_INT_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_DISPLAY_SIZE, default="5in"): cv.one_of(
                *DISPLAY_PROFILES.keys(), lower=True, msg=DISPLAY_SIZE_HELP
            ),
            cv.Optional(CONF_BRIGHTNESS, default=80): cv.int_range(min=0, max=100),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    if CONF_PD_PIN in config:
        pd = await cg.gpio_pin_expression(config[CONF_PD_PIN])
        cg.add(var.set_pd_pin(pd))

    if CONF_INT_PIN in config:
        intr = await cg.gpio_pin_expression(config[CONF_INT_PIN])
        cg.add(var.set_int_pin(intr))

    profile = DISPLAY_PROFILES[config[CONF_DISPLAY_SIZE]]
    cg.add(var.set_profile(profile))
    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
