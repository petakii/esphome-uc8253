# UC8253 ESPHome External Component

Custom ESPHome display driver for UC8253-based e-paper modules, including the Elecrow CrowPanel 3.7" 2-color panel (416x240).

## Features

- UC8253 command support: `0x04`, `0x02`, `0x07`, `0x00`, `0x50`, `0x10`, `0x13`, `0x12`, `0x71`
- Soft-SPI pins (`clk`, `mosi`, `cs`, `dc`) plus `reset` and `busy`
- Black/white and black/white/red frame buffer handling
- Full/partial refresh profile selection
- Busy-pin polling and hardware reset timing
- Optional LUT upload (`lut:`)

## External component usage

```yaml
external_components:
  - source:
      type: local
      path: components

font:
  - file: "gfonts://Roboto"
    id: font_small
    size: 20

display:
  - platform: uc8253
    id: crowpanel
    model: "3.7"
    clk_pin: GPIO12
    mosi_pin: GPIO11
    cs_pin: GPIO45
    dc_pin: GPIO46
    reset_pin: GPIO47
    busy_pin: GPIO48
    refresh_mode: partial
    full_update_every: 12
    color_mode: 2color
    black_invert: false
    color_invert: false
    update_interval: 5min
    lambda: |-
      it.fill(COLOR_OFF);
      it.printf(10, 10, id(font_small), Color(0x00, 0x00, 0x00), "UC8253 online");
      it.printf(10, 40, id(font_small), Color(0xFF, 0x00, 0x00), "Red channel");
```

## Simple rendering example

```yaml
display:
  - platform: uc8253
    model: "3.7"
    clk_pin: GPIO12
    mosi_pin: GPIO11
    cs_pin: GPIO45
    dc_pin: GPIO46
    reset_pin: GPIO47
    busy_pin: GPIO48
    update_interval: 60s
    lambda: |-
      it.fill(COLOR_ON);
      it.print(10, 10, id(font_small), COLOR_OFF, "Hello CrowPanel");
```

## UC8253 command protocol summary

- **Power on**: `0x04` then wait on BUSY
- **Power off**: `0x02` then wait on BUSY
- **Deep sleep**: `0x07` with `0xA5`
- **Panel setup**: `0x00`
- **Voltage/VCOM profile**: `0x50`
- **Black RAM write**: `0x10`
- **Color RAM write**: `0x13`
- **Refresh trigger**: `0x12`
- **Status probe**: `0x71` while waiting on BUSY

## Pin connection guide (CrowPanel 3.7")

| Signal | ESP32-S3 GPIO |
| --- | --- |
| CLK | GPIO12 |
| MOSI | GPIO11 |
| CS | GPIO45 |
| DC | GPIO46 |
| RST | GPIO47 |
| BUSY | GPIO48 |

> Verify your specific board revision before flashing.

## Troubleshooting

1. **No update on panel**
   - Check BUSY pin polarity (`busy_active_high`)
   - Verify soft-SPI pin assignments and wiring
2. **Ghosting/artifacts**
   - Increase `full_update_every`
   - Use `refresh_mode: full` for periodic cleanup refreshes
3. **Inverted colors**
   - Toggle `black_invert` and/or `color_invert`
   - Use `color_mode: binary` for black/white-only rendering
4. **Controller hangs after update**
   - Confirm reset pin is connected and stable
   - Reduce update frequency and check power rail stability

## Limitations / known issues

- Partial refresh behavior can vary by panel batch and LUT tuning.
- LUT waveform tuning is exposed but panel-specific values are user supplied.
- Deep sleep is sent on shutdown; there is no dedicated runtime action to trigger sleep manually.
