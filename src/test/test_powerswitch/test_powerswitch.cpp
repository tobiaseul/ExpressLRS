#include <cstdint>
#include <unity.h>
#include "PowerSwitchLogic.h"

static const PowerSwitchTiming timing = {
    .offConfirmMs = 1000,
    .listenWindowMs = 3000,
    .powerOnListenMs = 90000,
    .maxAwakeMs = 900000,
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


// --- review fixes (FMEA P6, P11) ---------------------------------------------------------

void test_armed_never_turns_off(void)
{
    // switch channel off by mistake while armed (in flight): output stays on
    PowerSwitchLogic ps(timing, PowerSwitchBoot::OtherReset, 0);
    TEST_ASSERT_FALSE(ps.update(0, true, false, true));
    TEST_ASSERT_FALSE(ps.update(5000, true, false, true));
    TEST_ASSERT_TRUE(ps.outputOn());
    // disarmed: the normal off confirmation starts only now
    TEST_ASSERT_FALSE(ps.update(6000, true, false, false));
    TEST_ASSERT_TRUE(ps.update(7000, true, false, false));
    TEST_ASSERT_FALSE(ps.outputOn());
}

void test_busy_delays_sleep_up_to_max_awake(void)
{
    // WiFi / bind mode left on with the output off must not drain the battery
    PowerSwitchLogic ps(timing, PowerSwitchBoot::WakeFromSleep, 0);
    TEST_ASSERT_FALSE(ps.update(3000, false, false, false, true));
    TEST_ASSERT_FALSE(ps.update(899999, false, false, false, true));
    TEST_ASSERT_TRUE(ps.update(900000, false, false, false, true));
}

void test_busy_never_sleeps_while_on(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::OtherReset, 0);
    TEST_ASSERT_FALSE(ps.update(10000000, false, false, false, true));
    TEST_ASSERT_TRUE(ps.outputOn());
}

void test_low_battery_turns_sleep_into_cutoff(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::WakeFromSleep, 0);
    TEST_ASSERT_FALSE(ps.update(2999, false, false, false, false, true));
    TEST_ASSERT_TRUE(ps.update(3000, false, false, false, false, true));
    TEST_ASSERT_TRUE(ps.cutoff());
}

void test_normal_sleep_is_no_cutoff(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::WakeFromSleep, 0);
    TEST_ASSERT_TRUE(ps.update(3000, false, false));
    TEST_ASSERT_FALSE(ps.cutoff());
}

void test_low_battery_keeps_output_on_in_flight(void)
{
    PowerSwitchLogic ps(timing, PowerSwitchBoot::OtherReset, 0);
    TEST_ASSERT_FALSE(ps.update(100000, true, true, false, false, true));
    TEST_ASSERT_FALSE(ps.update(200000, false, false, false, false, true));
    TEST_ASSERT_TRUE(ps.outputOn());
}

void test_low_battery_keeps_power_on_setup_window(void)
{
    // a wrong VBAT calibration must not lock out WiFi after plugging in
    PowerSwitchLogic ps(timing, PowerSwitchBoot::PowerOn, 0);
    TEST_ASSERT_FALSE(ps.update(89999, false, false, false, false, true));
    TEST_ASSERT_TRUE(ps.update(90000, false, false, false, false, true));
}

void test_low_battery_threshold_per_cell(void)
{
    TEST_ASSERT_FALSE(PowerSwitch_lowBattery(-1)); // no reading
    TEST_ASSERT_FALSE(PowerSwitch_lowBattery(0));
    TEST_ASSERT_TRUE(PowerSwitch_lowBattery(3150));  // 1S
    TEST_ASSERT_FALSE(PowerSwitch_lowBattery(3300));
    TEST_ASSERT_FALSE(PowerSwitch_lowBattery(4200)); // 1S full
    TEST_ASSERT_TRUE(PowerSwitch_lowBattery(4500));  // 2S deeply discharged
    TEST_ASSERT_TRUE(PowerSwitch_lowBattery(6300));
    TEST_ASSERT_FALSE(PowerSwitch_lowBattery(6600));
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
    RUN_TEST(test_armed_never_turns_off);
    RUN_TEST(test_busy_delays_sleep_up_to_max_awake);
    RUN_TEST(test_busy_never_sleeps_while_on);
    RUN_TEST(test_low_battery_turns_sleep_into_cutoff);
    RUN_TEST(test_normal_sleep_is_no_cutoff);
    RUN_TEST(test_low_battery_keeps_output_on_in_flight);
    RUN_TEST(test_low_battery_keeps_power_on_setup_window);
    RUN_TEST(test_low_battery_threshold_per_cell);
    UNITY_END();

    return 0;
}
