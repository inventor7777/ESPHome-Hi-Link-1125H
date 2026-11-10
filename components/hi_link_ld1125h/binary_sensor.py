import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from . import CONF_HUB_ID, HUB_SCHEMA, ns, validate_platform

DEPENDENCIES = ["hi_link_ld1125h"]


CONFIG_SCHEMA = HUB_SCHEMA.extend({
    **{cv.Optional(k): binary_sensor.binary_sensor_schema(device_class="motion" if k == "movement" else "occupancy")
       for k in ("target", "movement", "stationary")},

})
FINAL_VALIDATE_SCHEMA = validate_platform("binary_sensor")

async def to_code(config):
    var = await cg.get_variable(config[CONF_HUB_ID])
    for key in ("target", "movement", "stationary"):
        if key in config:
            cg.add(getattr(var, "set_" + key)(await binary_sensor.new_binary_sensor(config[key])))
