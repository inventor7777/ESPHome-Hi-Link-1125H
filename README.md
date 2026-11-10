# Hi-Link LD1125H

An ESPHome custom component for the Hi-Link LD1125H radar, found in devices like the Govee H5127.

## Installation

Hub IDs and platform hub references are optional when only one matching hub
exists. These examples omit unnecessary references. Keep a hub `id` when
lambdas or actions use it; specify the platform hub reference when selecting
between multiple hubs. Entity IDs, UART IDs and other references are separate.

```yaml
external_components:
  - source: github://inventor7777/ESPHome-Hi-Link-1125H
    components: [hi_link_ld1125h]

uart:
  id: radar_uart
  tx_pin: PA14  # H5127 Wi-Fi MCU TX -> radar RX
  rx_pin: PA13  # H5127 Wi-Fi MCU RX <- radar TX
  baud_rate: 115200

hi_link_ld1125h:
  uart_id: radar_uart

binary_sensor:
  - platform: hi_link_ld1125h
    target:
      id: radar_presence
      name: Radar Presence
      filters:
        - delayed_off: 20s
    movement:
      name: Radar Movement
      filters:
        - delayed_off: 2s
    stationary:
      name: Radar Stationary Detection
      filters:
        - delayed_off: 2s

sensor:
  - platform: hi_link_ld1125h
    distance:
      name: Radar Distance
      filters:
        - throttle_with_priority: 1s
    signal_strength:
      name: Radar Signal Strength
      filters:
        - throttle_with_priority: 1s

number:
  - platform: hi_link_ld1125h
    max_distance:
      name: Radar Maximum Distance
      max_value: 8
    movement_threshold_1:
      name: Radar Movement Threshold Near
    stationary_threshold_1:
      name: Radar Stationary Threshold Near

switch:
  - platform: hi_link_ld1125h
    test_mode:
      name: Radar Test Mode

button:
  - platform: hi_link_ld1125h
    query_settings:
      name: Query Radar Settings
    save_settings:
      name: Save Radar Settings
    query_version:
      name: Query Radar Version

text_sensor:
  - platform: hi_link_ld1125h
    firmware_version:
      name: Radar Firmware
```

On the H5127's LibreTiny hardware UART, its Arduino UART0 pin definitions may
also need the same overrides used by the existing Govee drivers. Put these in
the device YAML; this generic driver never changes global board pin definitions:

```yaml
esphome:
  platformio_options:
    build_flags:
      - -DPIN_SERIAL0_TX=14
      - -DPIN_SERIAL0_RX=13
```

## Configuration options

The `hi_link_ld1125h:` hub contains only device settings (`id` and `uart_id`).
Entities belong under their native ESPHome domains, with
`platform: hi_link_ld1125h` automatically linking each group to the sole hub. All entities are optional; use at most one group per domain per radar.

**Migration:** move each entity block from the old hub configuration into its
matching platform group below. Keep its existing ID, name, filters and other
options unchanged. The old nested syntax is no longer accepted.

| Option | YAML domain | What it does |
| --- | --- | --- |
| `id` | `hi_link_ld1125h` | Component ID for use in lambdas and automations. |
| `uart_id` | `hi_link_ld1125h` | Selects the UART bus connected to the radar. Requires TX and RX at 115200 baud, 8 data bits, no parity, and 1 stop bit. |
| `target` | `binary_sensor` | Binary sensor that pulses for either movement or stationary detection. Add a `delayed_off` filter to hold presence between reports. |
| `movement` | `binary_sensor` | Binary sensor that pulses for each movement report (`mov`). Add `delayed_off` to hold its state. |
| `stationary` | `binary_sensor` | Binary sensor that pulses for each stationary or small-movement report (`occ`). Add `delayed_off` to hold its state. |
| `distance` | `sensor` | Sensor reporting the detected target's approximate distance in meters. Retains the last distance when detection stops. |
| `signal_strength` | `sensor` | Sensor reporting the radar's `str` value, usually available in test mode. Becomes unavailable when a subsequent detection report omits strength. |
| `max_distance` | `number` | Number that sets the maximum detection distance (`rmax`). Narrow the UI range to your installation and firmware, e.g. `max_value: 8` for the H5127. |
| `threshold_1` | `number` | Number for the legacy shared detection threshold below 2.8 m (`mth1`). |
| `threshold_2` | `number` | Number for the legacy shared detection threshold from 2.8–8 m (`mth2`). |
| `threshold_3` | `number` | Number for the legacy shared detection threshold above 8 m (`mth3`). |
| `movement_threshold_1` | `number` | Number for the movement threshold below 2.8 m (`mth1_mov`). |
| `movement_threshold_2` | `number` | Number for the movement threshold from 2.8–8 m (`mth2_mov`). |
| `movement_threshold_3` | `number` | Number for the movement threshold above 8 m (`mth3_mov`). |
| `stationary_threshold_1` | `number` | Number for the stationary-presence threshold below 2.8 m (`mth1_occ`). |
| `stationary_threshold_2` | `number` | Number for the stationary-presence threshold from 2.8–8 m (`mth2_occ`). |
| `stationary_threshold_3` | `number` | Number for the stationary-presence threshold above 8 m (`mth3_occ`). |
| `small_movement_threshold_1` | `number` | Number for the firmware-dependent small-movement threshold below 2.8 m (`mth1_movs`). |
| `small_movement_threshold_2` | `number` | Number for the firmware-dependent small-movement threshold from 2.8–8 m (`mth2_movs`). |
| `small_movement_threshold_3` | `number` | Number for the firmware-dependent small-movement threshold above 8 m (`mth3_movs`). |
| `test_mode` | `switch` | Switch that enables or disables test output (`test_mode=1` or `0`), including signal strength on supported firmware. Applies its restore state after one second on startup. |
| `firmware_version` | `text_sensor` | Text sensor showing a recognized firmware-version reply. Configuring it also enables automatic `VER` queries on startup. |
| `query_settings` | `button` | Button that requests current settings (`get_all`). Settings are also queried automatically after one second and again after ten seconds on startup. |
| `query_version` | `button` | Button that requests the firmware version (`VER`). Configure `firmware_version` to expose the reply as an entity. |
| `save_settings` | `button` | Optional button to explicitly save the radar's current settings to flash (`save`). Entity changes already save automatically. Logs success at DEBUG on acknowledgement, or a warning if no reply arrives within five seconds of transmission. |
| `factory_reset` | `button` | Button that sends `initial` to reset the radar's settings, including calibration, on supported firmware. |

Distance limits must stay within 0–60 m, with a fixed step of 0.1 m.
Threshold limits must stay within 0–65535, with a fixed step of 1.
Changes made through the number entities, test-mode switch, and factory-reset
button automatically send `save` after the setting command, so they survive
power loss. Startup test-mode restore and settings queries do not trigger saves.
Commands sent directly through `uart.write` still require an explicit `save`.

## Notes

- [Hi-Link application manual V1.2](https://www.tinytronics.nl/product_files/004989_HLK-LD1125H-24GApplication%20Manual.pdf):
  `mov`/`occ`, `dis`, optional `str`, `rmax`, `mth1/2/3`, `test_mode`, `get_all`, `save`.
- [Later manual](https://www.docdroid.net/N7GBxom/hlk-ld1125h-24gzh-cnen-pdf):
  separate `mth1/2/3_mov` and `mth1/2/3_occ` thresholds.
- [Independent firmware analysis](https://github.com/h0lad/LD1125H_ReverseEngineering)
  and the H5127 stock firmware: additional `mth1/2/3_movs`, `initial`, `VER`,
  and version strings. Availability depends on the module firmware revision.
- Code written by GPT-6.1 Sol, tested extensively by me on my Govee H5127.
