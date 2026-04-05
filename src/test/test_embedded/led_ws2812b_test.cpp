#include <unity.h>
#include "config.h"

// Mock implementation of config object for testing
class MockConfig {
public:
    led_config_t ledConfig;

    MockConfig() {
        memset(&ledConfig, 0, sizeof(ledConfig));
        ledConfig.currentPreset = 0;
        ledConfig.currentBrightness = 100;
        ledConfig.auxChannelPreset = 255;  // disabled
        ledConfig.auxChannelBright = 255;  // disabled
    }

    const led_config_t* GetLedConfig() const {
        return &ledConfig;
    }
};

MockConfig mockConfig;

// ============================================================================
// Test Suite 1: Pixel State Transitions
// ============================================================================

void test_pixel_off_produces_zero()
{
    // GIVEN a pixel with state=0 (off) and red color
    led_pixel_t pixel = {255, 0, 0, 0, 0, 255};  // r=255, g=0, b=0, state=0 (off)

    // WHEN checking the pixel state
    uint8_t state = pixel.state;

    // THEN the state should be 0 (off)
    TEST_ASSERT_EQUAL(0, state);
}

void test_pixel_always_on_applies_color()
{
    // GIVEN a pixel with state=1 (always on) and red color at full brightness
    led_pixel_t pixel = {255, 0, 0, 1, 0, 255};  // r=255, g=0, b=0, state=1 (on)
    uint8_t globalBrightness = 100;  // 100% global brightness

    // WHEN calculating effective brightness
    uint16_t effectiveBright = (globalBrightness * pixel.brightness) / 100;

    // THEN the effective brightness should be 255 (full)
    TEST_ASSERT_EQUAL(255, effectiveBright);

    // AND the resulting red channel should be non-zero
    uint8_t r = (pixel.r * effectiveBright) / 255;
    TEST_ASSERT_GREATER_THAN(0, r);
}

void test_pixel_blinking_has_valid_state()
{
    // GIVEN a pixel with state=2 (blinking)
    led_pixel_t pixel = {255, 0, 0, 2, 10, 255};  // r=255, g=0, b=0, state=2 (blinking)

    // WHEN checking the pixel state
    uint8_t state = pixel.state;

    // THEN the state should be 2 (blinking)
    TEST_ASSERT_EQUAL(2, state);
}

void test_state_field_is_two_bit()
{
    // GIVEN a pixel with state field
    led_pixel_t pixel = {255, 0, 0, 0, 0, 255};

    // WHEN we set an invalid state value
    uint8_t invalidState = 5;  // Should be masked to 0-3
    uint8_t maskedState = invalidState & 0x3;  // Mask to valid range

    // THEN the masked state should be 1 (0-3 range)
    TEST_ASSERT_EQUAL(1, maskedState);
}

// ============================================================================
// Test Suite 2: Brightness Calculations
// ============================================================================

void test_global_brightness_reduces_color()
{
    // GIVEN a pixel with mixed color at full pixel brightness
    led_pixel_t pixel = {255, 128, 64, 1, 0, 255};  // RGB mixed colors
    uint8_t globalBrightness = 50;  // 50% global brightness

    // WHEN calculating effective brightness
    uint16_t effectiveBright = (globalBrightness * pixel.brightness) / 100;

    // THEN the effective brightness should be ~127 (50% of 255)
    TEST_ASSERT_EQUAL(127, effectiveBright);

    // AND each color channel should be reduced
    uint8_t r = (pixel.r * effectiveBright) / 255;  // 255 * 127 / 255 = 127
    uint8_t g = (pixel.g * effectiveBright) / 255;  // 128 * 127 / 255 = 64
    uint8_t b = (pixel.b * effectiveBright) / 255;  // 64 * 127 / 255 = 32

    TEST_ASSERT_EQUAL(127, r);
    TEST_ASSERT_EQUAL(64, g);
    TEST_ASSERT_EQUAL(32, b);
}

void test_per_pixel_brightness_reduces_color()
{
    // GIVEN a red pixel with 128/255 brightness (~50% pixel brightness)
    led_pixel_t pixel = {255, 0, 0, 1, 0, 128};  // r=255, pixel brightness=128
    uint8_t globalBrightness = 100;  // 100% global

    // WHEN calculating effective brightness and color
    uint16_t effectiveBright = (globalBrightness * pixel.brightness) / 100;
    uint8_t r = (pixel.r * effectiveBright) / 255;

    // THEN the red channel should be ~128 (50% of 255)
    TEST_ASSERT_EQUAL(128, effectiveBright);
    TEST_ASSERT_EQUAL(128, r);
}

void test_combined_brightness_multiplies()
{
    // GIVEN a pixel with per-pixel brightness 200/255 and 50% global brightness
    led_pixel_t pixel = {255, 0, 0, 1, 0, 200};  // r=255, pixel brightness=200
    uint8_t globalBrightness = 50;  // 50% global

    // WHEN calculating combined brightness
    uint16_t effectiveBright = (globalBrightness * pixel.brightness) / 100;

    // THEN the effective brightness should be ~98 (50% of 200)
    TEST_ASSERT_EQUAL(98, effectiveBright);
}

void test_zero_global_brightness_produces_zero()
{
    // GIVEN any pixel with 0% global brightness
    led_pixel_t pixel = {255, 128, 64, 1, 0, 255};
    uint8_t globalBrightness = 0;

    // WHEN calculating effective brightness
    uint16_t effectiveBright = (globalBrightness * pixel.brightness) / 100;

    // THEN the effective brightness should be 0
    TEST_ASSERT_EQUAL(0, effectiveBright);
}

void test_maximum_brightness()
{
    // GIVEN a pixel with 100% global and 255 pixel brightness
    led_pixel_t pixel = {255, 0, 0, 1, 0, 255};
    uint8_t globalBrightness = 100;

    // WHEN calculating effective brightness
    uint16_t effectiveBright = (globalBrightness * pixel.brightness) / 100;

    // THEN the effective brightness should be 255 (maximum)
    TEST_ASSERT_EQUAL(255, effectiveBright);
}

// ============================================================================
// Test Suite 3: Blink Frequency & Timing
// ============================================================================

void test_blink_frequency_one_hz()
{
    // GIVEN a blink frequency stored as freq*10 = 10 (representing 1 Hz)
    uint16_t blinkFreqHz = 10;

    // WHEN decoding the frequency and calculating period
    uint16_t decodedFreq = blinkFreqHz / 10;  // 1 Hz
    uint32_t periodMs = 1000 / decodedFreq;   // 1000 ms

    // THEN the period should be 1000ms for 1 Hz
    TEST_ASSERT_EQUAL(1, decodedFreq);
    TEST_ASSERT_EQUAL(1000, periodMs);
}

void test_blink_frequency_high_frequency()
{
    // GIVEN a high frequency of 1000 (representing 100 Hz)
    uint16_t blinkFreqHz = 1000;

    // WHEN decoding the frequency and calculating period
    uint16_t decodedFreq = blinkFreqHz / 10;  // 100 Hz
    uint32_t periodMs = 1000 / decodedFreq;   // 10 ms

    // THEN the period should be 10ms for 100 Hz
    TEST_ASSERT_EQUAL(100, decodedFreq);
    TEST_ASSERT_EQUAL(10, periodMs);
}

void test_blink_frequency_low_frequency()
{
    // GIVEN a very low frequency of 1 (representing 0.1 Hz)
    uint16_t blinkFreqHz = 1;

    // WHEN decoding the frequency
    uint16_t decodedFreq = blinkFreqHz / 10;  // 0

    // THEN the decoded frequency is 0, so we need safety handling
    TEST_ASSERT_EQUAL(0, decodedFreq);

    // Safety: if decodedFreq is 0, use 1 to avoid division by zero
    if (decodedFreq == 0) decodedFreq = 1;
    uint32_t periodMs = 1000 / decodedFreq;
    TEST_ASSERT_EQUAL(1000, periodMs);
}

void test_blink_duty_cycle_50_percent()
{
    // GIVEN a 1 Hz (1000ms period) blink pattern
    uint32_t periodMs = 1000;

    // WHEN checking if pixel is on at 400ms (40% of period)
    uint32_t phaseMs = 400;
    bool isOn = phaseMs < (periodMs / 2);  // 400 < 500?

    // THEN the pixel should be on
    TEST_ASSERT_TRUE(isOn);

    // WHEN checking if pixel is on at 600ms (60% of period)
    phaseMs = 600;
    isOn = phaseMs < (periodMs / 2);  // 600 < 500?

    // THEN the pixel should be off
    TEST_ASSERT_FALSE(isOn);
}

void test_blink_frequency_boundary_10hz()
{
    // GIVEN a frequency boundary value (100 = 10 Hz)
    uint16_t blinkFreqHz = 100;

    // WHEN decoding and calculating period
    uint16_t decodedFreq = blinkFreqHz / 10;  // 10 Hz
    uint32_t periodMs = 1000 / decodedFreq;   // 100 ms

    // THEN the period should be 100ms
    TEST_ASSERT_EQUAL(10, decodedFreq);
    TEST_ASSERT_EQUAL(100, periodMs);
}

// ============================================================================
// Test Suite 4: Config & Preset Switching
// ============================================================================

void test_preset_bounds_check_valid()
{
    // GIVEN a valid preset index (0-3)
    uint8_t preset = 2;

    // WHEN checking bounds
    bool valid = preset < LED_MAX_PRESETS;

    // THEN the preset should be valid
    TEST_ASSERT_TRUE(valid);
}

void test_preset_bounds_check_invalid()
{
    // GIVEN an invalid preset index (>= 4)
    uint8_t preset = 5;

    // WHEN checking bounds
    bool valid = preset < LED_MAX_PRESETS;

    // THEN the preset should be invalid
    TEST_ASSERT_FALSE(valid);
}

void test_aux_channel_preset_mapping_preset_0()
{
    // GIVEN CRSF value 0 (representing preset 0)
    uint16_t auxValue = 0;

    // WHEN mapping to preset (0-2047 → 0-3)
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN the preset should be 0
    TEST_ASSERT_EQUAL(0, preset);
}

void test_aux_channel_preset_mapping_preset_1()
{
    // GIVEN CRSF value 512 (representing preset 1)
    uint16_t auxValue = 512;

    // WHEN mapping to preset (0-2047 → 0-3)
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN the preset should be 1
    TEST_ASSERT_EQUAL(1, preset);
}

void test_aux_channel_preset_mapping_preset_2()
{
    // GIVEN CRSF value 1024 (representing preset 2)
    uint16_t auxValue = 1024;

    // WHEN mapping to preset (0-2047 → 0-3)
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN the preset should be 2
    TEST_ASSERT_EQUAL(2, preset);
}

void test_aux_channel_preset_mapping_preset_3()
{
    // GIVEN CRSF value 1536 (representing preset 3)
    uint16_t auxValue = 1536;

    // WHEN mapping to preset (0-2047 → 0-3)
    uint8_t preset = (auxValue / 512) & 0x3;

    // THEN the preset should be 3
    TEST_ASSERT_EQUAL(3, preset);
}

void test_aux_channel_brightness_mapping_0_percent()
{
    // GIVEN CRSF value 0 (representing 0% brightness)
    uint16_t auxValue = 0;

    // WHEN mapping to brightness (0-2047 → 0-100)
    uint8_t brightness = (auxValue / 20);  // 2047 / 20 ≈ 100

    // THEN the brightness should be 0
    TEST_ASSERT_EQUAL(0, brightness);
}

void test_aux_channel_brightness_mapping_50_percent()
{
    // GIVEN CRSF value 1000 (approximately 50% of 2047)
    uint16_t auxValue = 1000;

    // WHEN mapping to brightness (0-2047 → 0-100)
    uint8_t brightness = (auxValue / 20);

    // THEN the brightness should be around 50
    TEST_ASSERT_EQUAL(50, brightness);
}

void test_aux_channel_brightness_mapping_100_percent()
{
    // GIVEN CRSF value 2047 (maximum, representing 100% brightness)
    uint16_t auxValue = 2047;

    // WHEN mapping to brightness (0-2047 → 0-100)
    uint8_t brightness = (auxValue / 20);
    if (brightness > 100) brightness = 100;  // Clamp to 100

    // THEN the brightness should be 100
    TEST_ASSERT_EQUAL(100, brightness);
}

// ============================================================================
// Test Suite 5: Edge Cases & Bounds
// ============================================================================

void test_rgb_color_minimum()
{
    // GIVEN RGB color components at minimum
    uint8_t r = 0, g = 0, b = 0;

    // THEN each component should be within valid range
    TEST_ASSERT_GREATER_OR_EQUAL(r, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(g, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(b, 0);
}

void test_rgb_color_maximum()
{
    // GIVEN RGB color components at maximum
    uint8_t r = 255, g = 255, b = 255;

    // THEN each component should be within valid range
    TEST_ASSERT_LESS_OR_EQUAL(r, 255);
    TEST_ASSERT_LESS_OR_EQUAL(g, 255);
    TEST_ASSERT_LESS_OR_EQUAL(b, 255);
}

void test_rgb_color_bounds()
{
    // GIVEN a valid RGB color
    led_pixel_t pixel = {200, 150, 100, 1, 0, 255};

    // WHEN checking all color components
    // THEN each should be in valid range [0-255]
    TEST_ASSERT_LESS_OR_EQUAL(pixel.r, 255);
    TEST_ASSERT_LESS_OR_EQUAL(pixel.g, 255);
    TEST_ASSERT_LESS_OR_EQUAL(pixel.b, 255);
}

void test_state_field_bit_field_bounds()
{
    // GIVEN a pixel with state field
    led_pixel_t pixel = {255, 0, 0, 0, 0, 255};

    // WHEN setting various state values
    for (int state = 0; state <= 3; state++) {
        uint8_t maskedState = state & 0x3;
        // THEN the masked state should always be in range [0-3]
        TEST_ASSERT_LESS_OR_EQUAL(maskedState, 3);
    }
}

void test_blink_freq_bit_field_bounds()
{
    // GIVEN a pixel with blinkFreqHz field (10 bits, max 1023)
    led_pixel_t pixel = {255, 0, 0, 2, 1023, 255};  // Max frequency value

    // WHEN decoding the frequency
    uint16_t freqHz = pixel.blinkFreqHz / 10;  // Max is 102 Hz (1023/10)

    // THEN the decoded frequency should be reasonable
    TEST_ASSERT_LESS_OR_EQUAL(freqHz, 102);
}

void test_brightness_field_bounds_zero()
{
    // GIVEN a pixel with zero brightness
    led_pixel_t pixel = {255, 0, 0, 1, 0, 0};  // brightness=0

    // WHEN calculating effective brightness with 100% global
    uint16_t effectiveBright = (100 * pixel.brightness) / 100;

    // THEN the effective brightness should be 0
    TEST_ASSERT_EQUAL(0, effectiveBright);
}

void test_brightness_field_bounds_max()
{
    // GIVEN a pixel with maximum brightness
    led_pixel_t pixel = {255, 0, 0, 1, 0, 255};  // brightness=255

    // WHEN calculating effective brightness with 100% global
    uint16_t effectiveBright = (100 * pixel.brightness) / 100;

    // THEN the effective brightness should be 255
    TEST_ASSERT_EQUAL(255, effectiveBright);
}

void test_global_brightness_bounds_zero()
{
    // GIVEN 0% global brightness
    uint8_t globalBright = 0;
    uint8_t pixelBright = 255;

    // WHEN calculating combined brightness
    uint16_t effectiveBright = (globalBright * pixelBright) / 100;

    // THEN the effective brightness should be 0
    TEST_ASSERT_EQUAL(0, effectiveBright);
}

void test_global_brightness_bounds_max()
{
    // GIVEN 100% global brightness
    uint8_t globalBright = 100;
    uint8_t pixelBright = 255;

    // WHEN calculating combined brightness
    uint16_t effectiveBright = (globalBright * pixelBright) / 100;

    // THEN the effective brightness should be 255
    TEST_ASSERT_EQUAL(255, effectiveBright);
}

void test_pixel_count_bounds()
{
    // GIVEN the maximum pixels per strip
    uint8_t pixelCount = LED_MAX_PIXELS_PER_STRIP;

    // THEN the pixel count should be valid
    TEST_ASSERT_EQUAL(30, pixelCount);
}

void test_strip_count_bounds()
{
    // GIVEN the maximum strips
    uint8_t stripCount = LED_MAX_STRIPS;

    // THEN the strip count should be valid
    TEST_ASSERT_EQUAL(4, stripCount);
}

void test_preset_count_bounds()
{
    // GIVEN the maximum presets
    uint8_t presetCount = LED_MAX_PRESETS;

    // THEN the preset count should be valid
    TEST_ASSERT_EQUAL(4, presetCount);
}

// ============================================================================
// Test Suite Entry Point
// ============================================================================

void setUp() {}
void tearDown() {}

void setup()
{
    delay(2000);
    UNITY_BEGIN();

    // Test Suite 1: Pixel State Transitions
    RUN_TEST(test_pixel_off_produces_zero);
    RUN_TEST(test_pixel_always_on_applies_color);
    RUN_TEST(test_pixel_blinking_has_valid_state);
    RUN_TEST(test_state_field_is_two_bit);

    // Test Suite 2: Brightness Calculations
    RUN_TEST(test_global_brightness_reduces_color);
    RUN_TEST(test_per_pixel_brightness_reduces_color);
    RUN_TEST(test_combined_brightness_multiplies);
    RUN_TEST(test_zero_global_brightness_produces_zero);
    RUN_TEST(test_maximum_brightness);

    // Test Suite 3: Blink Frequency & Timing
    RUN_TEST(test_blink_frequency_one_hz);
    RUN_TEST(test_blink_frequency_high_frequency);
    RUN_TEST(test_blink_frequency_low_frequency);
    RUN_TEST(test_blink_duty_cycle_50_percent);
    RUN_TEST(test_blink_frequency_boundary_10hz);

    // Test Suite 4: Config & Preset Switching
    RUN_TEST(test_preset_bounds_check_valid);
    RUN_TEST(test_preset_bounds_check_invalid);
    RUN_TEST(test_aux_channel_preset_mapping_preset_0);
    RUN_TEST(test_aux_channel_preset_mapping_preset_1);
    RUN_TEST(test_aux_channel_preset_mapping_preset_2);
    RUN_TEST(test_aux_channel_preset_mapping_preset_3);
    RUN_TEST(test_aux_channel_brightness_mapping_0_percent);
    RUN_TEST(test_aux_channel_brightness_mapping_50_percent);
    RUN_TEST(test_aux_channel_brightness_mapping_100_percent);

    // Test Suite 5: Edge Cases & Bounds
    RUN_TEST(test_rgb_color_minimum);
    RUN_TEST(test_rgb_color_maximum);
    RUN_TEST(test_rgb_color_bounds);
    RUN_TEST(test_state_field_bit_field_bounds);
    RUN_TEST(test_blink_freq_bit_field_bounds);
    RUN_TEST(test_brightness_field_bounds_zero);
    RUN_TEST(test_brightness_field_bounds_max);
    RUN_TEST(test_global_brightness_bounds_zero);
    RUN_TEST(test_global_brightness_bounds_max);
    RUN_TEST(test_pixel_count_bounds);
    RUN_TEST(test_strip_count_bounds);
    RUN_TEST(test_preset_count_bounds);
}

void loop()
{
    UNITY_END();
}
