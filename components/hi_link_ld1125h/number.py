import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from . import CONF_HUB_ID, HUB_SCHEMA, ns, validate_platform

DEPENDENCIES = ["hi_link_ld1125h"]

RadarNumber = ns.class_("RadarNumber", number.Number)
# Commands remain opt-in: module firmware revisions expose different subsets.
PARAMETERS = {
    "max_distance": ("rmax", 0, 60, 0.1),
    **{f"threshold_{i}": (f"mth{i}", 0, 65535, 1) for i in range(1, 4)},
    **{f"{kind}_threshold_{i}": (f"mth{i}_{suffix}", 0, 65535, 1)
       for kind, suffix in (("movement", "mov"), ("stationary", "occ"), ("small_movement", "movs"))
       for i in range(1, 4)},
}
def validate_limits(config):
    for key, (_, _, _, step) in PARAMETERS.items():
        if key not in config:
            continue
        for limit in ("min_value", "max_value"):
            value = config[key][limit]
            if round(value, 1 if step == 0.1 else 0) != value:
                raise cv.Invalid(f"{key}: {limit} must be a multiple of {step}")
        if config[key]["max_value"] <= config[key]["min_value"]:
            raise cv.Invalid(f"{key}: max_value must be greater than min_value")
    return config



CONFIG_SCHEMA = cv.All(HUB_SCHEMA.extend({
    **{cv.Optional(k): number.number_schema(RadarNumber, entity_category="config",
            unit_of_measurement="m" if k == "max_distance" else "",
            **({"device_class": "distance"} if k == "max_distance" else {})).extend({
                cv.Optional("min_value", default=minimum): cv.float_range(min=minimum, max=maximum),
                cv.Optional("max_value", default=maximum): cv.float_range(min=minimum, max=maximum),
            }) for k, (_, minimum, maximum, _) in PARAMETERS.items()},

}), validate_limits)
FINAL_VALIDATE_SCHEMA = validate_platform("number")

async def to_code(config):
    var = await cg.get_variable(config[CONF_HUB_ID])
    for key, (command, _, _, step) in PARAMETERS.items():
        if key in config:
            conf = config[key]
            entity = await number.new_number(conf, var, command, 1 if step == 0.1 else 0,
                                             min_value=conf["min_value"], max_value=conf["max_value"], step=step)
            cg.add(var.add_number(entity))
