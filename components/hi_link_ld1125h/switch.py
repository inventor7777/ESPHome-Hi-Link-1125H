import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from . import CONF_HUB_ID, HUB_SCHEMA, ns, validate_platform

DEPENDENCIES = ["hi_link_ld1125h"]

TestMode = ns.class_("TestMode", switch.Switch)

CONFIG_SCHEMA = HUB_SCHEMA.extend({
    cv.Optional("test_mode"): switch.switch_schema(TestMode, default_restore_mode="ALWAYS_OFF", block_inverted=True,
                                                   entity_category="config"),

})
FINAL_VALIDATE_SCHEMA = validate_platform("switch")

async def to_code(config):
    var = await cg.get_variable(config[CONF_HUB_ID])
    if "test_mode" in config:
        entity = await switch.new_switch(config["test_mode"], var)
        cg.add(var.set_test_mode(entity))
