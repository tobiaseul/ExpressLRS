#include <cstdint>
#include <unity.h>
#include "SleepPins.h"

void test_sleep_pins_include_servos_and_uart(void)
{
    const int16_t pwm[] = {4, 18, 19, 21, 22, 23};
    int16_t out[SLEEP_PINS_MAX];
    uint8_t count = 0;
    SleepPins_collect(pwm, 6, 1, 3, out, count);
    TEST_ASSERT_EQUAL_UINT8(8, count);
    TEST_ASSERT_EQUAL_INT16(4, out[0]);
    TEST_ASSERT_EQUAL_INT16(23, out[5]);
    TEST_ASSERT_EQUAL_INT16(1, out[6]);
    TEST_ASSERT_EQUAL_INT16(3, out[7]);
}

void test_sleep_pins_skip_undef(void)
{
    const int16_t pwm[] = {4, 18, 19, 21, 22, 23};
    int16_t out[SLEEP_PINS_MAX];
    uint8_t count = 0;
    SleepPins_collect(pwm, 6, -1, 3, out, count);
    TEST_ASSERT_EQUAL_UINT8(7, count);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_sleep_pins_include_servos_and_uart);
    RUN_TEST(test_sleep_pins_skip_undef);
    UNITY_END();
    return 0;
}
