#include "devLED_WS2812b.h"
#include "common.h"
#include "logging.h"

DevLED_WS2812b::DevLED_WS2812b()
    : m_currentPreset(0), m_currentBrightness(100), m_lastRenderTime(0)
{
    memset(&m_blinkState, 0, sizeof(m_blinkState));
}

DevLED_WS2812b::~DevLED_WS2812b()
{
    for (auto& mapping : m_strips) {
        if (mapping.strip) {
            mapping.strip->end();
            delete mapping.strip;
        }
    }
    m_strips.clear();
}

void DevLED_WS2812b::Begin()
{
    const led_config_t* ledConfig = config.GetLedConfig();
    if (!ledConfig) {
        DBGLN("LED config not available");
        return;
    }

    // Scan PWM channels for LED mode and initialize NeoPixel strips
    uint8_t stripIdx = 0;
    for (uint8_t ch = 0; ch < PWM_MAX_CHANNELS && stripIdx < LED_MAX_STRIPS; ch++) {
        const rx_config_pwm_t* pwmCh = config.GetPwmChannel(ch);
        if (!pwmCh) continue;

        // Check if PWM channel mode is LED_OUTPUT (somLED = 16 from common.h)
        if (pwmCh->val.mode == 16) {  // somLED mode for WS2812b NeoPixel output
            // Create NeoPixel strip for this channel
            Adafruit_NeoPixel* strip = new Adafruit_NeoPixel(
                LED_MAX_PIXELS_PER_STRIP,
                ch,  // GPIO pin number = channel number (simplified)
                NEO_GRB + NEO_KHZ800
            );

            if (strip) {
                strip->begin();
                strip->clear();
                strip->show();

                m_strips.push_back({(uint8_t)ch, stripIdx, strip});
                DBGLN("LED Strip %d initialized on GPIO %d", stripIdx, ch);
                stripIdx++;
            } else {
                DBGLN("Failed to allocate NeoPixel strip for GPIO %d", ch);
            }
        }
    }

    m_currentPreset = ledConfig->currentPreset;
    m_currentBrightness = ledConfig->currentBrightness;
    m_lastRenderTime = 0;  // Force render on first call

    DBGLN("WS2812b LED Driver initialized with %d strips", stripIdx);
}

void DevLED_WS2812b::HandleChannelData(uint8_t channel, uint32_t value)
{
    const led_config_t* ledConfig = config.GetLedConfig();
    if (!ledConfig) return;

    // Check preset channel (value 0-2047)
    if (ledConfig->auxChannelPreset < 16 && channel == ledConfig->auxChannelPreset) {
        // Map [0-2047] to [0-3] (4 presets)
        uint8_t preset = (value / 512) & 0x3;
        SetPreset(preset);
    }

    // Check brightness channel (value 0-2047)
    if (ledConfig->auxChannelBright < 16 && channel == ledConfig->auxChannelBright) {
        // Map [0-2047] to [0-100]
        uint8_t brightness = (value / 20);  // 2047 / 20 ≈ 100
        if (brightness > 100) brightness = 100;
        SetBrightness(brightness);
    }
}

void DevLED_WS2812b::SetPreset(uint8_t preset)
{
    if (preset < LED_MAX_PRESETS && preset != m_currentPreset) {
        m_currentPreset = preset;
        config.SetLedPreset(preset);
        DBGLN("LED Preset switched to %d", preset);
    }
}

void DevLED_WS2812b::SetBrightness(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;
    if (brightness != m_currentBrightness) {
        m_currentBrightness = brightness;
        config.SetLedBrightness(brightness);
        DBGLN("LED Brightness set to %d%%", brightness);
    }
}

void DevLED_WS2812b::SetPwmPin(uint8_t stripIndex, uint8_t pin)
{
    // Future implementation for runtime pin reconfiguration
    // For now, pins are set at Begin() time from config
}

void DevLED_WS2812b::Render()
{
    unsigned long now = millis();
    if (now - m_lastRenderTime < RENDER_INTERVAL_MS) {
        return;  // Not time to render yet (~30 Hz)
    }
    m_lastRenderTime = now;

    const led_config_t* ledConfig = config.GetLedConfig();
    if (!ledConfig || m_strips.empty()) {
        return;
    }

    uint8_t preset = ledConfig->currentPreset & 0x3;
    const led_preset_t* presetData = &ledConfig->presets[preset];

    // Render each strip
    for (uint8_t stripIdx = 0; stripIdx < m_strips.size(); stripIdx++) {
        led_strip_mapping_t& mapping = m_strips[stripIdx];
        if (!mapping.strip || mapping.stripIndex >= LED_MAX_STRIPS) continue;

        const led_strip_t& stripData = presetData->strips[mapping.stripIndex];

        if (!stripData.enabled) {
            // Strip disabled - clear it
            mapping.strip->clear();
            mapping.strip->show();
            continue;
        }

        // Render each pixel in this strip
        for (uint8_t pixel = 0; pixel < LED_MAX_PIXELS_PER_STRIP; pixel++) {
            const led_pixel_t& px = stripData.pixels[pixel];
            RenderPixel(stripIdx, pixel, px);
        }

        mapping.strip->show();
    }
}

void DevLED_WS2812b::RenderPixel(uint8_t stripIdx, uint8_t pixelIndex, const led_pixel_t& px)
{
    if (stripIdx >= m_strips.size()) return;

    Adafruit_NeoPixel* strip = m_strips[stripIdx].strip;
    if (!strip) return;

    uint32_t color = 0;

    if (px.state == 0) {
        // Off - no color
        color = 0;
    } else if (px.state == 1) {
        // Always on - apply pixel color with brightness multipliers
        uint16_t globalBright = m_currentBrightness;  // 0-100
        uint16_t pixelBright = px.brightness;        // 0-255
        uint16_t effectiveBright = (globalBright * pixelBright) / 100;  // 0-255
        if (effectiveBright > 255) effectiveBright = 255;

        uint8_t r = (px.r * effectiveBright) / 255;
        uint8_t g = (px.g * effectiveBright) / 255;
        uint8_t b = (px.b * effectiveBright) / 255;
        color = strip->Color(r, g, b);
    } else if (px.state == 2) {
        // Blinking - toggle on/off based on elapsed time
        uint16_t freqHz = px.blinkFreqHz / 10;  // Decode from packed format
        if (freqHz == 0) freqHz = 1;  // Default to 1 Hz if zero

        uint32_t periodMs = 1000 / freqHz;
        uint32_t phaseMs = m_blinkState.elapsedMs[stripIdx][pixelIndex];

        // Update elapsed time for next render
        m_blinkState.elapsedMs[stripIdx][pixelIndex] += RENDER_INTERVAL_MS;
        if (m_blinkState.elapsedMs[stripIdx][pixelIndex] >= periodMs) {
            m_blinkState.elapsedMs[stripIdx][pixelIndex] = 0;
        }

        // 50% duty cycle: on for first half of period
        if (phaseMs < (periodMs / 2)) {
            uint16_t globalBright = m_currentBrightness;  // 0-100
            uint16_t pixelBright = px.brightness;        // 0-255
            uint16_t effectiveBright = (globalBright * pixelBright) / 100;
            if (effectiveBright > 255) effectiveBright = 255;

            uint8_t r = (px.r * effectiveBright) / 255;
            uint8_t g = (px.g * effectiveBright) / 255;
            uint8_t b = (px.b * effectiveBright) / 255;
            color = strip->Color(r, g, b);
        } else {
            color = 0;  // Off phase
        }
    }

    strip->setPixelColor(pixelIndex, color);
}

// Global device instance
static DevLED_WS2812b ledDevice;

// Device interface callbacks
static void device_begin(void)
{
    ledDevice.Begin();
}

static int device_update(void)
{
    ledDevice.Render();
    return DURATION_IMMEDIATELY;
}

// Device declaration for integration into rx_main.cpp
device_t RGB_LED_device = {
    .initialize = device_begin,
    .start = nullptr,
    .update = device_update,
    .event = nullptr,
    .timeout = nullptr,
    .subscribe = 0,
};
