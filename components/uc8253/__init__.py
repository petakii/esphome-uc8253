"""UC8253 external component configuration helpers."""

import esphome.config_validation as cv

CONF_REFRESH_MODE = "refresh_mode"
CONF_BLACK_INVERT = "black_invert"
CONF_COLOR_INVERT = "color_invert"
CONF_FULL_UPDATE_EVERY = "full_update_every"
CONF_BUSY_ACTIVE_HIGH = "busy_active_high"
CONF_LUT = "lut"
CONF_COLOR_MODE = "color_mode"

MODEL_OPTIONS = ("3.7", "2.13")

REFRESH_MODES = ("full", "partial")
COLOR_MODES = ("binary", "2color")
CODEOWNERS = ["@petakii"]


def validate_model(value):
    return cv.one_of(*MODEL_OPTIONS, lower=True)(value)


def validate_refresh_mode(value):
    return cv.one_of(*REFRESH_MODES, lower=True)(value)


def validate_color_mode(value):
    return cv.one_of(*COLOR_MODES, lower=True)(value)
