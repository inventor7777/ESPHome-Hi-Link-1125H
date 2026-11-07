"""Hi-Link LD1125H ASCII UART interface (legacy and separate thresholds)."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, binary_sensor, number, switch, button, text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "number", "switch", "button", "text_sensor"]
MULTI_CONF = True
ns = cg.esphome_ns.namespace("hi_link_ld1125h")
Radar = ns.class_("LD1125H", cg.Component, uart.UARTDevice)
RadarNumber = ns.class_("RadarNumber", number.Number)
TestMode = ns.class_("TestMode", switch.Switch)
CommandButton = ns.class_("CommandButton", button.Button)

# Commands remain opt-in: module firmware revisions expose different subsets.
PARAMETERS = {
    "max_distance": ("rmax", 0, 60, 0.1),
    **{f"threshold_{i}": (f"mth{i}", 0, 65535, 1) for i in range(1, 4)},
    **{f"{kind}_threshold_{i}": (f"mth{i}_{suffix}", 0, 65535, 1)
       for kind, suffix in (("movement", "mov"), ("stationary", "occ"), ("small_movement", "movs"))
       for i in range(1, 4)},
}
BUTTONS = {"query_settings": "get_all", "save_settings": "save", "factory_reset": "initial", "query_version": "VER"}
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


CONFIG_SCHEMA = cv.All(cv.Schema({
    cv.GenerateID(): cv.declare_id(Radar),
    **{cv.Optional(k): sensor.sensor_schema(unit_of_measurement="m" if k == "distance" else "",
            accuracy_decimals=2, state_class="measurement",
            **({"device_class": "distance"} if k == "distance" else {})) for k in ("distance", "signal_strength")},
    **{cv.Optional(k): binary_sensor.binary_sensor_schema(device_class="motion" if k == "movement" else "occupancy")
       for k in ("target", "movement", "stationary")},
    cv.Optional("firmware_version"): text_sensor.text_sensor_schema(entity_category="diagnostic"),
    cv.Optional("test_mode"): switch.switch_schema(TestMode, default_restore_mode="ALWAYS_OFF", block_inverted=True,
                                                   entity_category="config"),
    **{cv.Optional(k): number.number_schema(RadarNumber, entity_category="config",
            unit_of_measurement="m" if k == "max_distance" else "",
            **({"device_class": "distance"} if k == "max_distance" else {})).extend({
                cv.Optional("min_value", default=minimum): cv.float_range(min=minimum, max=maximum),
                cv.Optional("max_value", default=maximum): cv.float_range(min=minimum, max=maximum),
            }) for k, (_, minimum, maximum, _) in PARAMETERS.items()},
    **{cv.Optional(k): button.button_schema(CommandButton, entity_category="config") for k in BUTTONS},
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA), validate_limits)
FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "hi_link_ld1125h", baud_rate=115200, require_tx=True, require_rx=True,
    data_bits=8, parity="NONE", stop_bits=1,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    for key in ("distance", "signal_strength"):
        if key in config:
            cg.add(getattr(var, "set_" + key)(await sensor.new_sensor(config[key])))
    for key in ("target", "movement", "stationary"):
        if key in config:
            cg.add(getattr(var, "set_" + key)(await binary_sensor.new_binary_sensor(config[key])))
    if "firmware_version" in config:
        cg.add(var.set_firmware_version(await text_sensor.new_text_sensor(config["firmware_version"])))
    if "test_mode" in config:
        entity = await switch.new_switch(config["test_mode"], var)
        cg.add(var.set_test_mode(entity))
    for key, (command, _, _, step) in PARAMETERS.items():
        if key in config:
            conf = config[key]
            entity = await number.new_number(conf, var, command, 1 if step == 0.1 else 0,
                                             min_value=conf["min_value"], max_value=conf["max_value"], step=step)
            cg.add(var.add_number(entity))
    for key, command in BUTTONS.items():
        if key in config:
            await button.new_button(config[key], var, command)
