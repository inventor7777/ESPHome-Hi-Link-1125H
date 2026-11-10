import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from . import CONF_HUB_ID, HUB_SCHEMA, ns, validate_platform

DEPENDENCIES = ["hi_link_ld1125h"]


CONFIG_SCHEMA = HUB_SCHEMA.extend({
    cv.Optional("firmware_version"): text_sensor.text_sensor_schema(entity_category="diagnostic"),
})
FINAL_VALIDATE_SCHEMA = validate_platform("text_sensor")

async def to_code(config):
    var = await cg.get_variable(config[CONF_HUB_ID])
    if "firmware_version" in config:
        cg.add(var.set_firmware_version(await text_sensor.new_text_sensor(config["firmware_version"])))
