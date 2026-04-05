# WS2812b LED Support Design Specification

**Date:** 2026-04-05  
**Target Platform:** ESP32 only  
**Feature:** Configurable WS2812b LED strips on ExpressLRS Radiomaster ER6/ER8 receivers

---

## Overview

This design adds support for controlling WS2812b (NeoPixel) addressable LED strips on PWM output channels of ESP32-based ExpressLRS receivers. Users can configure up to 4 independent LED strips with 30 pixels each, organized into 4 presets. Each pixel supports:
- **Static state:** off, always on, or blinking with configurable frequency
- **Color:** full RGB (0-255 per channel)
- **Brightness:** per-pixel control (0-255)

Presets and brightness can be switched in real-time from the transmitter using dedicated AUX channels.

---

## Requirements

### Functional Requirements

1. **LED Channel Configuration**
   - User selects which PWM output channels (0-15) drive LED strips
   - PWM channel mode set to `MODE_LED_OUTPUT` identifies it as LED-enabled
   - Support 4 independent LED strips maximum

2. **Per-Pixel Configuration**
   - 30 pixels per strip
   - Each pixel configurable with:
     - RGB color (0-255 per channel)
     - State: off, on, or blinking
     - Blink frequency (if blinking): 0.1-100 Hz (numeric input)
     - Brightness: 0-255 (per-pixel, multiplied by global brightness)

3. **Preset System**
   - 4 independent presets (named Preset 0-3)
   - Each preset contains full configuration for all 4 strips
   - Per-strip enable/disable toggle (preserves config when disabled)
   - Current active preset stored in EEPROM

4. **Global Brightness Control**
   - Global brightness multiplier (0-100%) applied to all pixels
   - Independent of preset selection

5. **TX Control During Flight**
   - AUX channel for preset selection: maps [0-100%] → presets 0-3 (25% increments)
   - AUX channel for brightness: maps [0-100%] → brightness 0-100%
   - Both channels configurable (user selects which AUX, or disables)
   - Changes take effect immediately (no reboot needed)

6. **Persistent Storage**
   - All LED configuration saved to EEPROM/NVS
   - Survives power cycles
   - Configuration version tracked for future upgrades

7. **WebUI Configuration Page**
   - New dedicated page: "LED Configuration"
   - Sections:
     - PWM channel assignment to LED strips
     - Preset selector and per-pixel editor
     - TX control channel assignment

### Non-Functional Requirements

- **ESP32 only:** Feature disabled on ESP8266
- **Rendering rate:** ~30 Hz LED refresh (non-blocking)
- **Library:** Use existing Arduino WS2812b library (e.g., Adafruit NeoPixel or FastLED)
- **Extensible:** Architecture designed to support animations in future without breaking current config

---

## Data Structures

### LED Pixel Configuration

```cpp
typedef struct {
    uint8_t r, g, b;           // RGB color (0-255 each)
    uint8_t state:2;           // 0=off, 1=on, 2=blinking
    uint16_t blinkFreqHz:10;   // blink frequency * 10 (0.1-100 Hz)
    uint8_t brightness:8;      // per-pixel brightness (0-255)
} led_pixel_t;
```

**Size:** 5 bytes  
**Blink frequency encoding:** stored as `blinkFreqHz * 10` to support 0.1-100 Hz in 10-bit field

### LED Strip Configuration

```cpp
typedef struct {
    uint8_t enabled:1;
    uint8_t unused:7;
    led_pixel_t pixels[30];    // 30 pixels per strip
} led_strip_t;
```

**Size:** 1 + (30 × 5) = 151 bytes per strip

### LED Preset

```cpp
typedef struct {
    led_strip_t strips[4];     // 4 independent LED strips
} led_preset_t;
```

**Size:** 151 × 4 = 604 bytes per preset

### LED Configuration (Top-level)

```cpp
typedef struct {
    led_preset_t presets[4];   // 4 presets
    uint8_t currentPreset;     // active preset (0-3)
    uint8_t currentBrightness; // global brightness (0-100%)
    uint8_t auxChannelPreset;  // AUX channel for preset (0-15, 255=disabled)
    uint8_t auxChannelBright;  // AUX channel for brightness (0-15, 255=disabled)
} led_config_t;
```

**Total size:** (604 × 4) + 4 = 2,420 bytes

### Integration into RxConfig

Add to existing `rx_config_t` struct:
```cpp
typedef struct __attribute__((packed)) {
    // ... existing fields ...
    led_config_t ledConfig;    // +2,420 bytes (new)
    // ... rest of existing fields ...
} rx_config_t;
```

**Impact:** Increases `rx_config_t` by ~2.4 KB. Fits within available EEPROM/NVS space.

---

## Architecture

### Device Driver: `devLED_WS2812b`

**New files:**
- `src/lib/LED_WS2812b/devLED_WS2812b.h`
- `src/lib/LED_WS2812b/devLED_WS2812b.cpp`

**Responsibilities:**
1. Initialize WS2812b on configured PWM pins
2. Monitor CRSF channel data for AUX channel updates
3. Detect preset/brightness changes and apply immediately
4. Render LED strip data at ~30 Hz
5. Handle pixel state transitions (on/off/blinking)

**Key Methods:**
```cpp
void begin();                                    // Initialize device
void handleChannelData(uint8_t ch, uint16_t us); // Update AUX channel
void render();                                   // Render LED strips
void setPreset(uint8_t preset);                 // Switch preset
void setBrightness(uint8_t brightness);         // Update brightness
```

**Blink State Machine:**
- Per-pixel blink state tracked with elapsed time
- Blink frequency determines on/off duration
- 50/50 duty cycle (configurable via brightness)

### Integration into RxConfig

**New methods in `RxConfig` class:**
```cpp
// Getters
const led_config_t* GetLedConfig() const;
uint8_t GetLedCurrentPreset() const;
uint8_t GetLedCurrentBrightness() const;
const led_pixel_t* GetLedPixel(uint8_t strip, uint8_t pixel) const;

// Setters
void SetLedPreset(uint8_t preset);
void SetLedBrightness(uint8_t brightness);
void SetLedPixel(uint8_t strip, uint8_t pixel, const led_pixel_t& px);
void SetLedStripEnabled(uint8_t strip, bool enabled);
void SetLedAuxChannels(uint8_t presetCh, uint8_t brightCh);
```

### Integration into rx_main.cpp

1. Add `RGB_LED_device` to `ui_devices[]` array
2. Hook LED device into channel update callback:
   ```cpp
   if (newChannelsAvailable) {
       RGB_LED_device.handleChannelData(...);
   }
   ```
3. Add to render loop:
   ```cpp
   RGB_LED_device.render();
   ```

### PWM Channel Mode Extension

Add new mode to existing `eServoOutputMode` enum in `config.h`:
```cpp
typedef enum {
    SERVO_MODE = 0,
    PWM_MODE = 1,
    LED_OUTPUT = 5,  // New mode for WS2812b
    // ... other existing modes
} eServoOutputMode;
```

When a PWM channel's mode is set to `LED_OUTPUT`:
- `devServoOutput` skips rendering for that channel
- `devLED_WS2812b` claims the pin and controls it

---

## WebUI Integration

### New Page: `led-config-panel.js`

**File:** `src/html/src/pages/led-config-panel.js`

**Sections:**

#### 1. PWM Channel Assignment
- Dropdown for each of 4 LED strips: "Select PWM channel (0-15)"
- Visual indicator: ✓ if valid, ✗ if conflict (channel in use by servo)
- Strip enable/disable toggle

#### 2. Preset Editor
- Dropdown: "Select Preset (0-3)"
- For each strip in preset:
  - Enabled/disabled toggle
  - Pixel editor (30 rows):
    - Color picker (RGB)
    - State dropdown: "Off", "On", "Blinking"
    - Blink frequency input (0.1-100 Hz, only visible if "Blinking" selected)
    - Brightness slider (0-100%)

#### 3. TX Control Setup
- Dropdown: "Preset Control AUX Channel" (0-15, 255=disabled)
- Dropdown: "Brightness Control AUX Channel" (0-15, 255=disabled)
- Info text: "Preset: 0-25%=P0, 25-50%=P1, 50-75%=P2, 75-100%=P3"

#### 4. Status Display
- Current active preset
- Current global brightness
- Render stats (FPS, CPU usage)

### API Endpoints

**New REST endpoints in WebUI backend:**

```
GET  /api/led/config          → returns led_config_t
POST /api/led/preset/{id}     → updates preset {id}
POST /api/led/pixel/{s}/{p}   → updates pixel p in strip s
POST /api/led/brightness      → sets global brightness
POST /api/led/auxchannels     → sets aux channel mappings
```

---

## CRSF Channel Integration

### AUX Channel Mapping

Two configurable AUX channels control LEDs in real-time:

**Preset Channel (default AUX9):**
- Maps stick position [0-100%] to preset [0-3]
- Mapping: `preset = (auxValue / 100) * 4` (clamped to 0-3)
- 0-25% = Preset 0
- 25-50% = Preset 1
- 50-75% = Preset 2
- 75-100% = Preset 3

**Brightness Channel (default AUX10):**
- Maps stick position [0-100%] to brightness [0-100%]
- Linear mapping: `brightness = (auxValue / 100) * 100`

### Runtime Updates

In `rx_main.cpp` channel update loop:
```cpp
// After CRSF channel data is available
if (ledConfig.auxChannelPreset != 255) {
    uint8_t chData = CRSFRouter.getChannelData(ledConfig.auxChannelPreset);
    uint8_t preset = (chData / 256 * 4) & 0x3;  // 0-3
    RGB_LED_device.setPreset(preset);
}

if (ledConfig.auxChannelBright != 255) {
    uint8_t chData = CRSFRouter.getChannelData(ledConfig.auxChannelBright);
    uint8_t brightness = (chData / 256 * 100);  // 0-100
    RGB_LED_device.setBrightness(brightness);
}
```

---

## Storage & Persistence

### EEPROM Layout

**Existing `rx_config_t`:** ~100 bytes  
**New `led_config_t`:** ~2,420 bytes  
**Total `rx_config_t`:** ~2,520 bytes  
**Available in EEPROM/NVS:** 1024+ bytes (ESP32 NVS is larger)

**For ESP32:** Use NVS with automatic partitioning. No changes to storage backend needed.

### Version Tracking

Increment `RX_CONFIG_VERSION` in `config.h` when LED config is added. Add migration logic in `RxConfig::Load()` to initialize LED config for old firmware versions.

---

## Implementation Plan

### Phase 1: Data Structures & Config
1. Add `led_pixel_t`, `led_preset_t`, `led_config_t` to `config.h`
2. Extend `rx_config_t` to include `ledConfig`
3. Add getters/setters to `RxConfig` class
4. Bump `RX_CONFIG_VERSION`

### Phase 2: Device Driver
1. Create `devLED_WS2812b.h/.cpp`
2. Implement WS2812b library integration (Adafruit NeoPixel)
3. Implement blink state machine
4. Hook into channel update callbacks

### Phase 3: RX Main Integration
1. Add device to `ui_devices[]`
2. Add render loop hook
3. Test basic LED rendering

### Phase 4: WebUI
1. Create `led-config-panel.js`
2. Implement preset editor UI
3. Add API endpoints
4. Test TX channel control

### Phase 5: Testing & Verification
1. Unit tests for pixel rendering logic
2. Integration tests with mock CRSF data
3. Hardware testing on ER6/ER8

---

## Future Extensibility

This design supports future additions without breaking current functionality:

1. **Animations:** Add `animation_type` field to `led_preset_t`, extend rendering logic
2. **Per-strip patterns:** Add pattern index to `led_strip_t`
3. **More strips/pixels:** Increase array sizes (requires config version bump)
4. **Custom CRSF packets:** Add alternative to AUX channel control
5. **Telemetry:** Report LED status back to TX

---

## Success Criteria

- ✓ 4 LED strips, 30 pixels each, configurable via WebUI
- ✓ 4 presets, switchable from TX in real-time
- ✓ Per-pixel color, brightness, state (off/on/blinking) control
- ✓ Global brightness multiplier
- ✓ Configuration persistent across reboots
- ✓ No impact on existing PWM servo functionality
- ✓ ESP32-only feature (clean conditional compilation)
- ✓ Extensible for future animation support

---

## References

- **WS2812b Protocol:** https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
- **Adafruit NeoPixel Library:** https://github.com/adafruit/Adafruit_NeoPixel
- **ExpressLRS Config System:** `src/lib/CONFIG/config.h`
- **ExpressLRS Device Model:** `src/lib/device.h`
- **Betaflight LED Implementation:** https://github.com/betaflight/betaflight (reference only)
