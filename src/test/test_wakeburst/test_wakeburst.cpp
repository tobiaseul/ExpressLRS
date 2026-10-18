#include <cstdint>
#include <unity.h>
#include "WakeBurstLogic.h"
#include "WakeRadio.h"

static const WakeBurstTiming timing = {
    .periodMs = 6000,
    .sessionMs = 120000,
};

void test_preamble_covers_rx_duty_cycle(void)
{
    // a sleeping RX must see the preamble in at least one RX window
    TEST_ASSERT_GREATER_THAN_UINT32(WakeRadio::dutyRxUs + WakeRadio::dutySleepUs, WakeRadio::preambleUs);
    TEST_ASSERT_EQUAL_UINT8(0x97, WakeRadio::preambleReg);
    TEST_ASSERT_EQUAL_UINT32(3584, WakeRadio::preambleSymbols);
}

void test_airtime_fits_tx_timeout(void)
{
    TEST_ASSERT_UINT32_WITHIN(5000, 568000, WakeRadio::airtimeUs);
    TEST_ASSERT_LESS_THAN_UINT32(WakeRadio::txTimeoutUs, WakeRadio::airtimeUs);
}

void test_duty_cycle_times(void)
{
    TEST_ASSERT_EQUAL_UINT32(1500, WakeRadio::dutyRxUs);
    TEST_ASSERT_EQUAL_UINT32(500000, WakeRadio::dutySleepUs);
}

void test_wake_channel_avoids_sync_channel(void)
{
    for (uint32_t seed = 0; seed < 1000; seed++)
    {
        const uint8_t ch = WakeRadio::channel(seed, 80);
        TEST_ASSERT_LESS_THAN_UINT8(80, ch);
        TEST_ASSERT_NOT_EQUAL(40, ch);
    }
    TEST_ASSERT_EQUAL_UINT8(39, WakeRadio::channel(39, 80));
    TEST_ASSERT_EQUAL_UINT8(41, WakeRadio::channel(40, 80));
    TEST_ASSERT_EQUAL_UINT8(79, WakeRadio::channel(78, 80));
}

void test_burst_on_switch_on(void)
{
    WakeBurstLogic wb(timing);
    TEST_ASSERT_FALSE(wb.update(0, false, false, false));
    TEST_ASSERT_TRUE(wb.update(100, true, false, false));
}

void test_burst_on_boot_with_switch_on(void)
{
    WakeBurstLogic wb(timing);
    TEST_ASSERT_TRUE(wb.update(0, true, false, false));
}

void test_burst_period(void)
{
    WakeBurstLogic wb(timing);
    TEST_ASSERT_TRUE(wb.update(0, true, false, false));
    TEST_ASSERT_FALSE(wb.update(5999, true, false, false));
    TEST_ASSERT_TRUE(wb.update(6000, true, false, false));
}

void test_no_burst_while_linked(void)
{
    WakeBurstLogic wb(timing);
    TEST_ASSERT_FALSE(wb.update(0, true, true, false));
}

void test_no_burst_after_link_loss(void)
{
    // a link lost in flight must not be disturbed by bursts
    WakeBurstLogic wb(timing);
    TEST_ASSERT_TRUE(wb.update(0, true, false, false));
    TEST_ASSERT_FALSE(wb.update(3000, true, true, false));
    TEST_ASSERT_FALSE(wb.update(9000, true, false, false));
    TEST_ASSERT_FALSE(wb.update(60000, true, false, false));
}

void test_no_burst_when_armed(void)
{
    WakeBurstLogic wb(timing);
    TEST_ASSERT_FALSE(wb.update(0, true, false, true));
}

void test_session_times_out(void)
{
    WakeBurstLogic wb(timing);
    TEST_ASSERT_TRUE(wb.update(0, true, false, false));
    TEST_ASSERT_TRUE(wb.update(114000, true, false, false));
    TEST_ASSERT_FALSE(wb.update(120000, true, false, false));
}

void test_switch_cycle_starts_new_session(void)
{
    WakeBurstLogic wb(timing);
    TEST_ASSERT_TRUE(wb.update(0, true, false, false));
    TEST_ASSERT_FALSE(wb.update(1000, true, true, false));
    TEST_ASSERT_FALSE(wb.update(2000, false, false, false));
    TEST_ASSERT_TRUE(wb.update(3000, true, false, false));
}

void test_retry_after_busy_channel(void)
{
    WakeBurstLogic wb(timing);
    TEST_ASSERT_TRUE(wb.update(0, true, false, false));
    wb.retryIn(0, 100);
    TEST_ASSERT_FALSE(wb.update(99, true, false, false));
    TEST_ASSERT_TRUE(wb.update(100, true, false, false));
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_preamble_covers_rx_duty_cycle);
    RUN_TEST(test_airtime_fits_tx_timeout);
    RUN_TEST(test_duty_cycle_times);
    RUN_TEST(test_wake_channel_avoids_sync_channel);
    RUN_TEST(test_burst_on_switch_on);
    RUN_TEST(test_burst_on_boot_with_switch_on);
    RUN_TEST(test_burst_period);
    RUN_TEST(test_no_burst_while_linked);
    RUN_TEST(test_no_burst_after_link_loss);
    RUN_TEST(test_no_burst_when_armed);
    RUN_TEST(test_session_times_out);
    RUN_TEST(test_switch_cycle_starts_new_session);
    RUN_TEST(test_retry_after_busy_channel);
    UNITY_END();

    return 0;
}
