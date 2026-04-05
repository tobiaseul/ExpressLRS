#pragma once
#if defined(TARGET_RX) && defined(PLATFORM_ESP32)

#include "device.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <vector>

extern device_t RGB_LED_device;

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

    void Begin();
    void HandleChannelData(uint8_t channel, uint16_t value);
    void Render();

    // Configuration interface
    void SetPwmPin(uint8_t stripIndex, uint8_t pin);
    void SetPreset(uint8_t preset);
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
    bool GetPixelBlinkState(uint16_t elapsedMs, uint16_t freqHz);
};

#endif
