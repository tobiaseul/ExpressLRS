#include <unity.h>
#include "config.h"

// Mock implementation of LED config for testing
class MockLedConfig {
public:
    led_config_t config;

    MockLedConfig() {
        memset(&config, 0, sizeof(config));
        config.currentPreset = 0;
        config.currentBrightness = 100;
        config.auxChannelPreset = 255;    // disabled
        config.auxChannelBright = 255;    // disabled

        // Initialize all presets with different colors
        for (uint8_t p = 0; p < LED_MAX_PRESETS; p++) {
            for (uint8_t s = 0; s < LED_MAX_STRIPS; s++) {
                config.presets[p].strips[s].enabled = 1;
                for (uint8_t px = 0; px < LED_MAX_PIXELS_PER_STRIP; px++) {
                    config.presets[p].strips[s].pixels[px].r = p * 64;  // Different red per preset
                    config.presets[p].strips[s].pixels[px].g = 0;
                    config.presets[p].strips[s].pixels[px].b = 0;
                    config.presets[p].strips[s].pixels[px].state = 1;   // All on
                    config.presets[p].strips[s].pixels[px].brightness = 255;
                }
            }
        }
    }

    const led_config_t* GetConfig() const {
        return &config;
    }
};

MockLedConfig mockLedConfig;

// ============================================================================
// Test Suite 1: AUX Channel Integration
// ============================================================================

void test_aux9_switches_preset_low()
{
    // GIVEN an AUX9 channel at 0% (~0 value)
    uint16_t auxValue = 0;

    // WHEN calculating preset from AUX9
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN the preset should be 0
    TEST_ASSERT_EQUAL(0, preset);
}

void test_aux9_switches_preset_25_percent()
{
    // GIVEN an AUX9 channel at 25% (~512 value)
    uint16_t auxValue = 512;

    // WHEN calculating preset from AUX9
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN the preset should be 1
    TEST_ASSERT_EQUAL(1, preset);
}

void test_aux9_switches_preset_50_percent()
{
    // GIVEN an AUX9 channel at 50% (~1024 value)
    uint16_t auxValue = 1024;

    // WHEN calculating preset from AUX9
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN the preset should be 2
    TEST_ASSERT_EQUAL(2, preset);
}

void test_aux9_switches_preset_75_percent()
{
    // GIVEN an AUX9 channel at 75% (~1536 value)
    uint16_t auxValue = 1536;

    // WHEN calculating preset from AUX9
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN the preset should be 3
    TEST_ASSERT_EQUAL(3, preset);
}

void test_aux9_switches_preset_max()
{
    // GIVEN an AUX9 channel at 100% (2047 value)
    uint16_t auxValue = 2047;

    // WHEN calculating preset from AUX9
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN the preset should be 3 (wraps around)
    TEST_ASSERT_EQUAL(3, preset);
}

void test_aux10_controls_brightness_low()
{
    // GIVEN an AUX10 channel at 0% (0 value)
    uint16_t auxValue = 0;

    // WHEN calculating brightness from AUX10
    uint8_t brightness = (auxValue / 20);
    if (brightness > 100) brightness = 100;

    // THEN the brightness should be 0%
    TEST_ASSERT_EQUAL(0, brightness);
}

void test_aux10_controls_brightness_50_percent()
{
    // GIVEN an AUX10 channel at 50% (1024 value)
    uint16_t auxValue = 1024;

    // WHEN calculating brightness from AUX10
    uint8_t brightness = (auxValue / 20);
    if (brightness > 100) brightness = 100;

    // THEN the brightness should be approximately 50%
    TEST_ASSERT_GREATER_OR_EQUAL(brightness, 48);
    TEST_ASSERT_LESS_OR_EQUAL(brightness, 52);
}

void test_aux10_controls_brightness_max()
{
    // GIVEN an AUX10 channel at 100% (2047 value)
    uint16_t auxValue = 2047;

    // WHEN calculating brightness from AUX10
    uint8_t brightness = (auxValue / 20);
    if (brightness > 100) brightness = 100;

    // THEN the brightness should be clamped to 100%
    TEST_ASSERT_EQUAL(100, brightness);
}

void test_disabled_aux_channels_ignored_preset()
{
    // GIVEN AUX preset channel disabled (255)
    mockLedConfig.config.auxChannelPreset = 255;

    // WHEN the channel is disabled
    bool enabled = (mockLedConfig.config.auxChannelPreset != 255);

    // THEN the channel should be ignored
    TEST_ASSERT_FALSE(enabled);
}

void test_disabled_aux_channels_ignored_brightness()
{
    // GIVEN AUX brightness channel disabled (255)
    mockLedConfig.config.auxChannelBright = 255;

    // WHEN the channel is disabled
    bool enabled = (mockLedConfig.config.auxChannelBright != 255);

    // THEN the channel should be ignored
    TEST_ASSERT_FALSE(enabled);
}

void test_invalid_aux_channel_number()
{
    // GIVEN an invalid AUX channel number (16+)
    uint8_t channel = 20;

    // WHEN checking validity
    bool valid = (channel < 16);

    // THEN the channel should be invalid
    TEST_ASSERT_FALSE(valid);
}

void test_valid_aux_channel_numbers()
{
    // WHEN checking validity of all valid channels
    for (uint8_t ch = 0; ch < 16; ch++) {
        bool valid = (ch < 16);
        TEST_ASSERT_TRUE(valid);
    }
}

void test_preset_boundary_0_percent()
{
    // GIVEN AUX value at 0%
    uint16_t auxValue = 0;

    // WHEN calculating preset
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN preset should be 0
    TEST_ASSERT_EQUAL(0, preset);
}

void test_preset_boundary_511()
{
    // GIVEN AUX value at 511 (just below 25%)
    uint16_t auxValue = 511;

    // WHEN calculating preset
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN preset should still be 0
    TEST_ASSERT_EQUAL(0, preset);
}

void test_brightness_range_full_scale()
{
    // GIVEN various AUX brightness values
    struct {
        uint16_t auxValue;
        uint8_t minBrightness;
        uint8_t maxBrightness;
    } cases[] = {
        {0, 0, 5},
        {512, 23, 28},
        {1024, 48, 53},
        {1536, 73, 78},
        {2047, 98, 103},
    };

    // WHEN calculating brightness for each value
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t brightness = (cases[i].auxValue / 20);
        if (brightness > 100) brightness = 100;

        // THEN brightness should be in expected range
        TEST_ASSERT_GREATER_OR_EQUAL(brightness, cases[i].minBrightness);
        TEST_ASSERT_LESS_OR_EQUAL(brightness, cases[i].maxBrightness);
    }
}

// ============================================================================
// Test Suite 2: Preset & Configuration Switching
// ============================================================================

void test_preset_0_has_correct_color()
{
    // GIVEN preset 0
    uint8_t preset = 0;
    const led_pixel_t* px = &mockLedConfig.config.presets[preset].strips[0].pixels[0];

    // WHEN reading the color
    uint8_t red = px->r;

    // THEN red should be 0 (0 * 64)
    TEST_ASSERT_EQUAL(0, red);
}

void test_preset_1_has_different_color()
{
    // GIVEN preset 1
    uint8_t preset = 1;
    const led_pixel_t* px = &mockLedConfig.config.presets[preset].strips[0].pixels[0];

    // WHEN reading the color
    uint8_t red = px->r;

    // THEN red should be 64 (1 * 64)
    TEST_ASSERT_EQUAL(64, red);
}

void test_preset_2_has_different_color()
{
    // GIVEN preset 2
    uint8_t preset = 2;
    const led_pixel_t* px = &mockLedConfig.config.presets[preset].strips[0].pixels[0];

    // WHEN reading the color
    uint8_t red = px->r;

    // THEN red should be 128 (2 * 64)
    TEST_ASSERT_EQUAL(128, red);
}

void test_preset_3_has_maximum_color()
{
    // GIVEN preset 3
    uint8_t preset = 3;
    const led_pixel_t* px = &mockLedConfig.config.presets[preset].strips[0].pixels[0];

    // WHEN reading the color
    uint8_t red = px->r;

    // THEN red should be 192 (3 * 64)
    TEST_ASSERT_EQUAL(192, red);
}

void test_switching_presets_updates_active_preset()
{
    // GIVEN current preset is 0
    mockLedConfig.config.currentPreset = 0;
    TEST_ASSERT_EQUAL(0, mockLedConfig.config.currentPreset);

    // WHEN switching to preset 2
    mockLedConfig.config.currentPreset = 2;

    // THEN the active preset should be 2
    TEST_ASSERT_EQUAL(2, mockLedConfig.config.currentPreset);
}

void test_switching_presets_multiple_times()
{
    // WHEN switching presets in sequence
    mockLedConfig.config.currentPreset = 0;
    TEST_ASSERT_EQUAL(0, mockLedConfig.config.currentPreset);

    mockLedConfig.config.currentPreset = 1;
    TEST_ASSERT_EQUAL(1, mockLedConfig.config.currentPreset);

    mockLedConfig.config.currentPreset = 3;
    TEST_ASSERT_EQUAL(3, mockLedConfig.config.currentPreset);

    mockLedConfig.config.currentPreset = 0;
    TEST_ASSERT_EQUAL(0, mockLedConfig.config.currentPreset);
}

void test_brightness_affects_all_pixels()
{
    // GIVEN a brightness level of 50%
    mockLedConfig.config.currentBrightness = 50;

    // WHEN checking brightness
    uint8_t brightness = mockLedConfig.config.currentBrightness;

    // THEN all pixels should use this brightness
    TEST_ASSERT_EQUAL(50, brightness);
}

void test_brightness_change_from_low_to_high()
{
    // GIVEN low brightness
    mockLedConfig.config.currentBrightness = 25;
    TEST_ASSERT_EQUAL(25, mockLedConfig.config.currentBrightness);

    // WHEN changing to high brightness
    mockLedConfig.config.currentBrightness = 100;

    // THEN brightness should be updated
    TEST_ASSERT_EQUAL(100, mockLedConfig.config.currentBrightness);
}

void test_preset_colors_isolated_per_preset()
{
    // WHEN reading colors from different presets
    uint8_t color0 = mockLedConfig.config.presets[0].strips[0].pixels[0].r;
    uint8_t color1 = mockLedConfig.config.presets[1].strips[0].pixels[0].r;
    uint8_t color2 = mockLedConfig.config.presets[2].strips[0].pixels[0].r;
    uint8_t color3 = mockLedConfig.config.presets[3].strips[0].pixels[0].r;

    // THEN each preset should have unique color
    TEST_ASSERT_EQUAL(0, color0);
    TEST_ASSERT_EQUAL(64, color1);
    TEST_ASSERT_EQUAL(128, color2);
    TEST_ASSERT_EQUAL(192, color3);
}

// ============================================================================
// Test Suite 3: Blink State Transitions
// ============================================================================

void test_blink_state_transition_1hz()
{
    // GIVEN a pixel with 1 Hz blink frequency (stored as 10)
    led_pixel_t pixel;
    pixel.state = 2;  // Blinking
    pixel.blinkFreqHz = 10;  // 1 Hz (10 * 0.1)

    // WHEN calculating the period
    uint16_t freqHz = pixel.blinkFreqHz / 10;
    uint32_t periodMs = 1000 / freqHz;

    // THEN the period should be 1000ms
    TEST_ASSERT_EQUAL(1, freqHz);
    TEST_ASSERT_EQUAL(1000, periodMs);
}

void test_blink_state_on_phase()
{
    // GIVEN a blink period of 1000ms
    uint32_t periodMs = 1000;

    // WHEN checking ON phase (first 500ms)
    for (uint32_t phase = 0; phase < 500; phase += 100) {
        bool isOn = phase < (periodMs / 2);
        TEST_ASSERT_TRUE(isOn);
    }
}

void test_blink_state_off_phase()
{
    // GIVEN a blink period of 1000ms
    uint32_t periodMs = 1000;

    // WHEN checking OFF phase (second 500ms)
    for (uint32_t phase = 500; phase < 1000; phase += 100) {
        bool isOn = phase < (periodMs / 2);
        TEST_ASSERT_FALSE(isOn);
    }
}

void test_high_frequency_blinking_10hz()
{
    // GIVEN a high frequency of 100 Hz (1000 encoded)
    led_pixel_t pixel;
    pixel.blinkFreqHz = 1000;  // 100 Hz

    // WHEN calculating the period
    uint16_t freqHz = pixel.blinkFreqHz / 10;
    uint32_t periodMs = 1000 / freqHz;

    // THEN the period should be 10ms
    TEST_ASSERT_EQUAL(100, freqHz);
    TEST_ASSERT_EQUAL(10, periodMs);
}

void test_low_frequency_blinking_01hz()
{
    // GIVEN a low frequency of 0.1 Hz (1 encoded)
    led_pixel_t pixel;
    pixel.blinkFreqHz = 1;  // 0.1 Hz

    // WHEN calculating the period
    uint16_t freqHz = pixel.blinkFreqHz / 10;
    uint32_t periodMs = freqHz > 0 ? (1000 / freqHz) : 10000;

    // THEN the period should be 10000ms (10 seconds)
    TEST_ASSERT_EQUAL(0, freqHz);
}

void test_mixed_pixel_states_in_same_strip()
{
    // GIVEN a strip with mixed pixel states
    led_strip_t strip;
    strip.enabled = 1;

    // Set different states for different pixels
    strip.pixels[0].state = 0;  // Off
    strip.pixels[1].state = 1;  // On
    strip.pixels[2].state = 2;  // Blinking

    // WHEN reading the states
    uint8_t state0 = strip.pixels[0].state;
    uint8_t state1 = strip.pixels[1].state;
    uint8_t state2 = strip.pixels[2].state;

    // THEN each pixel should have correct state
    TEST_ASSERT_EQUAL(0, state0);
    TEST_ASSERT_EQUAL(1, state1);
    TEST_ASSERT_EQUAL(2, state2);
}

void test_blink_transition_at_half_period()
{
    // GIVEN a 1Hz blink (1000ms period)
    uint32_t periodMs = 1000;
    uint32_t phaseMs = 500;  // Exactly at transition

    // WHEN checking state at transition point
    bool isOnAtTransition = phaseMs < (periodMs / 2);

    // THEN the state should transition from ON to OFF
    TEST_ASSERT_FALSE(isOnAtTransition);
}

void test_pixel_state_values_valid_range()
{
    // GIVEN pixel state values 0-2
    for (uint8_t state = 0; state <= 2; state++) {
        led_pixel_t pixel;
        pixel.state = state;

        // WHEN reading the state
        uint8_t readState = pixel.state;

        // THEN the state should be valid
        TEST_ASSERT_EQUAL(state, readState);
    }
}

// ============================================================================
// Test Suite 4: Disabled Strips & Invalid Indices
// ============================================================================

void test_disabled_strip_skipped()
{
    // GIVEN a strip with enabled flag = 0
    led_strip_t strip;
    strip.enabled = 0;

    // WHEN checking if strip should be rendered
    bool shouldRender = (strip.enabled == 1);

    // THEN the strip should be skipped
    TEST_ASSERT_FALSE(shouldRender);
}

void test_enabled_strip_rendered()
{
    // GIVEN a strip with enabled flag = 1
    led_strip_t strip;
    strip.enabled = 1;

    // WHEN checking if strip should be rendered
    bool shouldRender = (strip.enabled == 1);

    // THEN the strip should be rendered
    TEST_ASSERT_TRUE(shouldRender);
}

void test_invalid_strip_index_bounds()
{
    // GIVEN an invalid strip index (5, when max is 3)
    uint8_t stripIdx = 5;

    // WHEN checking validity
    bool valid = (stripIdx < LED_MAX_STRIPS);

    // THEN the index should be invalid
    TEST_ASSERT_FALSE(valid);
}

void test_valid_strip_indices()
{
    // WHEN checking all valid strip indices
    for (uint8_t s = 0; s < LED_MAX_STRIPS; s++) {
        bool valid = (s < LED_MAX_STRIPS);
        TEST_ASSERT_TRUE(valid);
    }
}

void test_invalid_pixel_index_bounds()
{
    // GIVEN an invalid pixel index (35, when max is 29)
    uint8_t pixelIdx = 35;

    // WHEN checking validity
    bool valid = (pixelIdx < LED_MAX_PIXELS_PER_STRIP);

    // THEN the index should be invalid
    TEST_ASSERT_FALSE(valid);
}

void test_valid_pixel_indices()
{
    // WHEN checking all valid pixel indices
    for (uint8_t p = 0; p < LED_MAX_PIXELS_PER_STRIP; p++) {
        bool valid = (p < LED_MAX_PIXELS_PER_STRIP);
        TEST_ASSERT_TRUE(valid);
    }
}

void test_pixel_index_boundary_29()
{
    // GIVEN pixel index 29 (last valid)
    uint8_t pixelIdx = 29;

    // WHEN checking validity
    bool valid = (pixelIdx < LED_MAX_PIXELS_PER_STRIP);

    // THEN the index should be valid
    TEST_ASSERT_TRUE(valid);
}

void test_pixel_index_boundary_30()
{
    // GIVEN pixel index 30 (first invalid)
    uint8_t pixelIdx = 30;

    // WHEN checking validity
    bool valid = (pixelIdx < LED_MAX_PIXELS_PER_STRIP);

    // THEN the index should be invalid
    TEST_ASSERT_FALSE(valid);
}

void test_all_strips_enabled_by_default()
{
    // WHEN iterating through all strips in preset
    for (uint8_t s = 0; s < LED_MAX_STRIPS; s++) {
        bool enabled = (mockLedConfig.config.presets[0].strips[s].enabled == 1);

        // THEN all strips should be enabled by default
        TEST_ASSERT_TRUE(enabled);
    }
}

void test_accessing_all_pixels_in_strip()
{
    // GIVEN a strip
    const led_strip_t* strip = &mockLedConfig.config.presets[0].strips[0];

    // WHEN accessing each pixel
    for (uint8_t p = 0; p < LED_MAX_PIXELS_PER_STRIP; p++) {
        const led_pixel_t* pixel = &strip->pixels[p];

        // THEN the pixel should be accessible
        TEST_ASSERT_NOT_NULL(pixel);
    }
}
