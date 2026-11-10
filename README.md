# Hi-Link LD1125H

The `hi_link_ld1125h` component connects a Hi-Link LD1125H-compatible radar to ESPHome over UART. It supports movement, stationary presence and firmware-dependent configuration commands. Tested on the radar in the Govee H5127.

# Component/Hub

Configure a transmit and receive UART at **115200 baud, 8 data bits, no parity, 1 stop bit**. The pins below are the H5127 assignments; change them for your board.

```yaml
external_components:
  - source: github://inventor7777/ESPHome-Hi-Link-1125H
    components: [hi_link_ld1125h]

uart:
  tx_pin: PA14
  rx_pin: PA13
  baud_rate: 115200

hi_link_ld1125h:
```

### Configuration variables

- **uart_id** (*Optional*, ID): Select the [UART bus](https://esphome.io/components/uart/) when multiple buses are configured.
- **id** (*Optional*, ID): Set a hub ID for use in lambdas or when configuring multiple radars.

All entity platforms below are optional. Use one platform entry per domain per radar. Each platform accepts **hi_link_ld1125h_id** (*Optional*, ID) to select its hub; omit it when only one hub exists.

On LibreTiny H5127 boards, UART0 may also require these pin overrides:

```yaml
esphome:
  platformio_options:
    build_flags:
      - -DPIN_SERIAL0_TX=14
      - -DPIN_SERIAL0_RX=13
```

# Binary Sensor

Detection reports produce pulses rather than a persistent occupied state. Use `delayed_off` to hold detection between reports.

```yaml
binary_sensor:
  - platform: hi_link_ld1125h
    target:
      name: Radar Presence
      filters:
        - delayed_off: 20s
    movement:
      name: Radar Movement
      filters:
        - delayed_off: 2s
    stationary:
      name: Radar Stationary Presence
      filters:
        - delayed_off: 2s
```

### Configuration variables

- **target** (*Optional*): Detection of either movement or stationary presence.
- **movement** (*Optional*): Detection from a movement report (`mov`).
- **stationary** (*Optional*): Detection from a stationary/small-movement report (`occ`).

Each accepts all options from [Binary Sensor](https://esphome.io/components/binary_sensor/). No hold filter is added automatically.

# Sensor

```yaml
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
```

### Configuration variables

- **distance** (*Optional*): Detected target distance in meters, with the `distance` device class. Keeps its last value when detection stops.
- **signal_strength** (*Optional*): Raw `str` value, available only when reported by the radar, usually in Test Mode. Becomes unavailable if subsequent detection reports omit it; no physical unit is assigned.

Each accepts all options from [Sensor](https://esphome.io/components/sensor/). Reports are not throttled automatically; the example limits publishing to once per second.

# Number

Number entities read and adjust the radar's settings. Firmware revisions expose different threshold sets; unsupported settings remain unknown. Configure the keys returned by your radar's `get_all` response.

```yaml
number:
  - platform: hi_link_ld1125h
    max_distance:
      name: Radar Maximum Distance
      max_value: 8
    threshold_1:
      name: Radar Threshold Near
    threshold_2:
      name: Radar Threshold Middle
    threshold_3:
      name: Radar Threshold Far
```

### Configuration variables

- **max_distance** (*Optional*): Maximum detection distance (`rmax`), in meters.
- **threshold_1** (*Optional*): Legacy shared near threshold (`mth1`).
- **threshold_2** (*Optional*): Legacy shared middle threshold (`mth2`).
- **threshold_3** (*Optional*): Legacy shared far threshold (`mth3`).
- **movement_threshold_1** (*Optional*): Near movement threshold (`mth1_mov`).
- **movement_threshold_2** (*Optional*): Middle movement threshold (`mth2_mov`).
- **movement_threshold_3** (*Optional*): Far movement threshold (`mth3_mov`).
- **stationary_threshold_1** (*Optional*): Near stationary threshold (`mth1_occ`).
- **stationary_threshold_2** (*Optional*): Middle stationary threshold (`mth2_occ`).
- **stationary_threshold_3** (*Optional*): Far stationary threshold (`mth3_occ`).
- **small_movement_threshold_1** (*Optional*): Near small-movement threshold (`mth1_movs`).
- **small_movement_threshold_2** (*Optional*): Middle small-movement threshold (`mth2_movs`).
- **small_movement_threshold_3** (*Optional*): Far small-movement threshold (`mth3_movs`).

Each accepts all options from [Number](https://esphome.io/components/number/), plus the limits described above.

Bands 1, 2 and 3 cover below 2.8 m, 2.8–8 m and above 8 m respectively. Larger thresholds reduce sensitivity.

Each number accepts:

- **min_value** (*Optional*, float): UI lower limit. Defaults to `0`.
- **max_value** (*Optional*, float): UI upper limit. Defaults to `60` for `max_distance`, or `65535` for thresholds. Must exceed `min_value`.

Distance limits must be within `0`–`60`, in `0.1` m increments. Threshold limits must be integers within `0`–`65535`, with step `1`. These are configurable UI bounds, not the radar's default settings or guaranteed physical range. Values come from radar readback; no number setting is written on startup.

Changes through number entities automatically send `save` to persist the setting.

# Switch

```yaml
switch:
  - platform: hi_link_ld1125h
    test_mode:
      name: Radar Test Mode
```

### Configuration variables

- **test_mode** (*Optional*): Enable or disable diagnostic output with `test_mode=1` or `test_mode=0`. May expose signal strength on supported firmware. Accepts [Switch](https://esphome.io/components/switch/) options except `inverted`.
  - **restore_mode** (*Optional*): Defaults to `ALWAYS_OFF`. Applies the selected startup state after one second; `DISABLED` leaves the radar's startup state unchanged.

User changes automatically send `save`. Applying the startup restore state does not save it to the radar's flash.

# Button

```yaml
button:
  - platform: hi_link_ld1125h
    query_settings:
      name: Query Radar Settings
    query_version:
      name: Query Radar Version
    save_settings:
      name: Save Radar Settings
    factory_reset:
      name: Reset Radar Settings
```

### Configuration variables

- **query_settings** (*Optional*): Request current settings (`get_all`). The hub also queries automatically one and ten seconds after startup.
- **query_version** (*Optional*): Request firmware version (`VER`). Configure the `firmware_version` text sensor to expose the reply.
- **save_settings** (*Optional*): Save current radar settings to flash (`save`). Entity changes already save automatically. A missing save acknowledgement produces a warning after five seconds.
- **factory_reset** (*Optional*): Reset radar settings, including calibration, using `initial`, followed by `save`. Requires firmware support.

Each accepts all options from [Button](https://esphome.io/components/button/).

# Text Sensor

```yaml
text_sensor:
  - platform: hi_link_ld1125h
    firmware_version:
      name: Radar Firmware
```

### Configuration variables

- **firmware_version** (*Optional*): Report recognized firmware version replies (`BW200M`, `RKB1125` or `LD1125H` prefixes). Also enables automatic `VER` queries one and ten seconds after startup. Accepts all options from [Text Sensor](https://esphome.io/components/text_sensor/).

## References

- [Hi-Link application manual V1.2](https://www.tinytronics.nl/product_files/004989_HLK-LD1125H-24GApplication%20Manual.pdf)
- [Later manual with separate movement/stationary thresholds](https://www.docdroid.net/N7GBxom/hlk-ld1125h-24gzh-cnen-pdf)
- [Independent firmware analysis](https://github.com/h0lad/LD1125H_ReverseEngineering)

Code written by GPT-6.1 Sol, tested extensively by me on my Govee H5127.
