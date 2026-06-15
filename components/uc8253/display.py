import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import display
from esphome.const import (
    CONF_BUSY_PIN,
    CONF_CLK_PIN,
    CONF_CS_PIN,
    CONF_DC_PIN,
    CONF_ID,
    CONF_LAMBDA,
    CONF_MODEL,
    CONF_MOSI_PIN,
    CONF_RESET_PIN,
)

from . import (
    CONF_BLACK_INVERT,
    CONF_BUSY_ACTIVE_HIGH,
    CONF_COLOR_INVERT,
    CONF_COLOR_MODE,
    CONF_FULL_UPDATE_EVERY,
    CONF_LUT,
    CONF_REFRESH_MODE,
    validate_model,
    validate_refresh_mode,
    validate_color_mode,
)

uc8253_ns = cg.esphome_ns.namespace("uc8253")
UC8253Model = uc8253_ns.enum("UC8253Model")
UC8253RefreshMode = uc8253_ns.enum("UC8253RefreshMode")
UC8253 = uc8253_ns.class_("UC8253", display.DisplayBuffer)

MODEL_ENUM = {
    "3.7": UC8253Model.UC8253_MODEL_3_7,
    "2.13": UC8253Model.UC8253_MODEL_2_13,
}

REFRESH_ENUM = {
    "full": UC8253RefreshMode.UC8253_REFRESH_FULL,
    "partial": UC8253RefreshMode.UC8253_REFRESH_PARTIAL,
}


CONFIG_SCHEMA = (
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(UC8253),
            cv.Required(CONF_MODEL): validate_model,
            cv.Required(CONF_CLK_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_MOSI_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_DC_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_REFRESH_MODE, default="full"): validate_refresh_mode,
            cv.Optional(CONF_FULL_UPDATE_EVERY, default=10): cv.int_range(min=1, max=1000),
            cv.Optional(CONF_BLACK_INVERT, default=False): cv.boolean,
            cv.Optional(CONF_COLOR_INVERT, default=False): cv.boolean,
            cv.Optional(CONF_BUSY_ACTIVE_HIGH, default=True): cv.boolean,
            cv.Optional(CONF_LUT): cv.ensure_list(cv.hex_uint8_t),
            cv.Optional(CONF_COLOR_MODE, default="2color"): validate_color_mode,
        }
    )
    .extend(cv.Schema({cv.Optional(CONF_LAMBDA): cv.returning_lambda}))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)
    await display.register_display(var, config)

    cg.add(var.set_model(MODEL_ENUM[config[CONF_MODEL]]))
    cg.add(var.set_refresh_mode(REFRESH_ENUM[config[CONF_REFRESH_MODE]]))
    cg.add(var.set_full_update_every(config[CONF_FULL_UPDATE_EVERY]))
    cg.add(var.set_black_invert(config[CONF_BLACK_INVERT]))
    cg.add(var.set_color_invert(config[CONF_COLOR_INVERT]))
    cg.add(var.set_busy_active_high(config[CONF_BUSY_ACTIVE_HIGH]))
    cg.add(var.set_has_color(config[CONF_COLOR_MODE] == "2color"))

    for key, setter in (
        (CONF_CLK_PIN, var.set_clk_pin),
        (CONF_MOSI_PIN, var.set_mosi_pin),
        (CONF_CS_PIN, var.set_cs_pin),
        (CONF_DC_PIN, var.set_dc_pin),
    ):
        pin = await cg.gpio_pin_expression(config[key])
        cg.add(setter(pin))

    if CONF_RESET_PIN in config:
        reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset))

    if CONF_BUSY_PIN in config:
        busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
        cg.add(var.set_busy_pin(busy))

    if CONF_LUT in config:
        cg.add(var.set_lut(config[CONF_LUT]))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA],
            [(display.DisplayRef, "it")],
            return_type=cg.void,
        )
        cg.add(var.set_writer(lambda_))
