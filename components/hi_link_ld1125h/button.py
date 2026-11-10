import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from . import CONF_HUB_ID, HUB_SCHEMA, ns, validate_platform

DEPENDENCIES = ["hi_link_ld1125h"]

CommandButton = ns.class_("CommandButton", button.Button)

BUTTONS = {"query_settings": "get_all", "save_settings": "save", "factory_reset": "initial", "query_version": "VER"}

CONFIG_SCHEMA = HUB_SCHEMA.extend({
    **{cv.Optional(k): button.button_schema(CommandButton, entity_category="config") for k in BUTTONS},

})
FINAL_VALIDATE_SCHEMA = validate_platform("button")

async def to_code(config):
    var = await cg.get_variable(config[CONF_HUB_ID])
    for key, command in BUTTONS.items():
        if key in config:
            await button.new_button(config[key], var, command)
