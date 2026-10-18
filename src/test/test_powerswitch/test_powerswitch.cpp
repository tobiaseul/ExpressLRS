#include <cstdint>
#include <unity.h>
#include "PowerSwitchLogic.h"

static const PowerSwitchTiming timing = {
    .offConfirmMs = 1000,
    .listenWindowMs = 3000,
    .powerOnListenMs = 90000,
};

void test_power_on_boot_starts_off(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::PowerOn, 0);
    TEST_ASSERT_FALSE(ps.outputOn());
}

void test_wake_from_sleep_starts_off(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::WakeFromSleep, 0);
    TEST_ASSERT_FALSE(ps.outputOn());
}

void test_other_reset_starts_on(void)
{
    // watchdog / brown-out in flight must not cut the servo supply
    PowerSwitchLogic ps(timing, PowerSwitchBoot::OtherReset, 0);
    TEST_ASSERT_TRUE(ps.outputOn());
}

void test_switch_on_turns_on_immediately(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::WakeFromSleep, 0);
    TEST_ASSERT_FALSE(ps.update(100, true, true));
    TEST_ASSERT_TRUE(ps.outputOn());
}

void test_short_off_glitch_keeps_on(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::OtherReset, 0);
    TEST_ASSERT_FALSE(ps.update(0, true, false));
    TEST_ASSERT_FALSE(ps.update(999, true, false));
    TEST_ASSERT_FALSE(ps.update(1000, true, true));
    TEST_ASSERT_FALSE(ps.update(2500, true, false));
    TEST_ASSERT_TRUE(ps.outputOn());
}

void test_confirmed_off_turns_off_and_sleeps(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::OtherReset, 0);
    TEST_ASSERT_FALSE(ps.update(0, true, false));
    TEST_ASSERT_TRUE(ps.update(1000, true, false));
    TEST_ASSERT_FALSE(ps.outputOn());
}

void test_link_loss_keeps_on_forever(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::OtherReset, 0);
    TEST_ASSERT_FALSE(ps.update(0, false, false));
    TEST_ASSERT_FALSE(ps.update(100000, false, false));
    TEST_ASSERT_TRUE(ps.outputOn());
}

void test_link_loss_resets_off_confirmation(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::OtherReset, 0);
    TEST_ASSERT_FALSE(ps.update(0, true, false));
    TEST_ASSERT_FALSE(ps.update(900, false, false));
    TEST_ASSERT_FALSE(ps.update(1000, true, false));
    TEST_ASSERT_FALSE(ps.update(1999, true, false));
    TEST_ASSERT_TRUE(ps.outputOn());
    TEST_ASSERT_TRUE(ps.update(2000, true, false));
    TEST_ASSERT_FALSE(ps.outputOn());
}

void test_off_without_link_sleeps_after_listen_window(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::WakeFromSleep, 500);
    TEST_ASSERT_FALSE(ps.update(3499, false, false));
    TEST_ASSERT_TRUE(ps.update(3500, false, false));
}

void test_power_on_boot_stays_awake_for_setup(void)
{
    // after plugging in the battery there must be time for WiFi and binding
    PowerSwitchLogic ps(timing, PowerSwitchBoot::PowerOn, 0);
    TEST_ASSERT_FALSE(ps.update(89999, false, false));
    TEST_ASSERT_TRUE(ps.update(90000, false, false));
}

void test_off_with_link_and_switch_off_sleeps_after_confirm(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::WakeFromSleep, 0);
    TEST_ASSERT_FALSE(ps.update(200, true, false));
    TEST_ASSERT_FALSE(ps.update(1199, true, false));
    TEST_ASSERT_TRUE(ps.update(1200, true, false));
    TEST_ASSERT_FALSE(ps.outputOn());
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_on_boot_starts_off);
    RUN_TEST(test_wake_from_sleep_starts_off);
    RUN_TEST(test_other_reset_starts_on);
    RUN_TEST(test_switch_on_turns_on_immediately);
    RUN_TEST(test_short_off_glitch_keeps_on);
    RUN_TEST(test_confirmed_off_turns_off_and_sleeps);
    RUN_TEST(test_link_loss_keeps_on_forever);
    RUN_TEST(test_link_loss_resets_off_confirmation);
    RUN_TEST(test_off_without_link_sleeps_after_listen_window);
    RUN_TEST(test_power_on_boot_stays_awake_for_setup);
    RUN_TEST(test_off_with_link_and_switch_off_sleeps_after_confirm);
    UNITY_END();

    return 0;
}
