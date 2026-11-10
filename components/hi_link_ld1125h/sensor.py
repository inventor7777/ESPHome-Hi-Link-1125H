import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from . import CONF_HUB_ID, HUB_SCHEMA, ns, validate_platform

DEPENDENCIES = ["hi_link_ld1125h"]


CONFIG_SCHEMA = HUB_SCHEMA.extend({
    **{cv.Optional(k): sensor.sensor_schema(unit_of_measurement="m" if k == "distance" else "",
            accuracy_decimals=2, state_class="measurement",
            **({"device_class": "distance"} if k == "distance" else {})) for k in ("distance", "signal_strength")},

})
FINAL_VALIDATE_SCHEMA = validate_platform("sensor")

async def to_code(config):
    var = await cg.get_variable(config[CONF_HUB_ID])
    for key in ("distance", "signal_strength"):
        if key in config:
            cg.add(getattr(var, "set_" + key)(await sensor.new_sensor(config[key])))
