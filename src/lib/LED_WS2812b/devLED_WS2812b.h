#pragma once
#if defined(TARGET_RX) && defined(PLATFORM_ESP32)

#include <stdint.h>
#include "device.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <vector>

extern device_t RGB_LED_device;
extern class DevLED_WS2812b ledDevice;

typedef struct {
    uint8_t pwmPin;
    uint8_t stripIndex;
    Adafruit_NeoPixel* strip;
} led_strip_mapping_t;

class DevLED_WS2812b
{
public:
    DevLED_WS2812b();
    ~DevLED_WS2812b();

    /// Initialize LED driver - scans PWM channels for LED mode and creates NeoPixel strips
    void Begin();

    /// Handle CRSF channel data for preset/brightness control
    /// @param channel CRSF channel number (0-15)
    /// @param value Channel value (0-2047)
    void HandleChannelData(uint8_t channel, uint32_t value);

    /// Render all LED strips at configured refresh rate (~30 Hz)
    void Render();

    // Configuration interface
    /// Configure PWM pin for a specific LED strip
    /// @param stripIndex Strip index (0-3)
    /// @param pin GPIO pin number
    void SetPwmPin(uint8_t stripIndex, uint8_t pin);

    /// Switch to a different preset (0-3)
    /// @param preset Preset index (0-3)
    void SetPreset(uint8_t preset);

    /// Set global brightness multiplier
    /// @param brightness Brightness 0-100%
    void SetBrightness(uint8_t brightness);

private:
    static constexpr unsigned long RENDER_INTERVAL_MS = 33;  // ~30 Hz

    std::vector<led_strip_mapping_t> m_strips;
    uint8_t m_currentPreset;
    uint8_t m_currentBrightness;
    unsigned long m_lastRenderTime;

    // Blink tracking per pixel
    struct {
        uint32_t elapsedMs[LED_MAX_STRIPS][LED_MAX_PIXELS_PER_STRIP];
    } m_blinkState;

    void RenderPixel(uint8_t stripIndex, uint8_t pixelIndex, const led_pixel_t& pixel);
};

#endif
