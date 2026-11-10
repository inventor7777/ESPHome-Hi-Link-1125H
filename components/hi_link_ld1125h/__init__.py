"""Hi-Link LD1125H ASCII UART hub."""
import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
# The C++ header declares optional entity types even when not configured.
AUTO_LOAD = ["sensor", "binary_sensor", "number", "switch", "button", "text_sensor"]
MULTI_CONF = True
ns = cg.esphome_ns.namespace("hi_link_ld1125h")
Radar = ns.class_("LD1125H", cg.Component, uart.UARTDevice)
CONF_HUB_ID = "hi_link_ld1125h_id"
HUB_SCHEMA = cv.Schema({cv.GenerateID(CONF_HUB_ID): cv.use_id(Radar)})

def validate_platform(domain):
    def validate(config):
        groups = [entry for entry in fv.full_config.get().get(domain, [])
                  if entry.get("platform") == "hi_link_ld1125h"
                  and str(entry.get(CONF_HUB_ID)) == str(config[CONF_HUB_ID])]
        if len(groups) > 1:
            raise cv.Invalid(f"Only one {domain} platform group is allowed per hi_link_ld1125h hub")
        return config
    return validate

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Radar),
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)
FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "hi_link_ld1125h", baud_rate=115200, require_tx=True, require_rx=True,
    data_bits=8, parity="NONE", stop_bits=1,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
