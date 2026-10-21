#include <cstdint>
#include <unity.h>
#include "BuzzerLogic.h"

static const BuzzerTiming timing = {
    .onMs = 300,
    .periodMs = 1000,
    .holdAfterLinkLossMs = 10000,
};

void test_silent_by_default(void)
{
    BuzzerLogic bz(timing);
    TEST_ASSERT_FALSE(bz.update(0, true, false));
    TEST_ASSERT_FALSE(bz.update(5000, false, false));
}

void test_channel_on_beeps_in_bursts(void)
{
    BuzzerLogic bz(timing);
    TEST_ASSERT_TRUE(bz.update(1000, true, true));      // starts with a tone
    TEST_ASSERT_TRUE(bz.update(1299, true, true));
    TEST_ASSERT_FALSE(bz.update(1300, true, true));     // pause
    TEST_ASSERT_FALSE(bz.update(1999, true, true));
    TEST_ASSERT_TRUE(bz.update(2000, true, true));      // next burst
}

void test_channel_off_stops_at_once(void)
{
    BuzzerLogic bz(timing);
    bz.update(0, true, true);
    TEST_ASSERT_FALSE(bz.update(100, true, false));
}

void test_short_link_loss_keeps_beeping(void)
{
    // the model in a tree: the link may fade in and out while walking up to it
    BuzzerLogic bz(timing);
    bz.update(0, true, true);
    TEST_ASSERT_TRUE(bz.update(5000, false, true));
    TEST_ASSERT_TRUE(bz.update(9000, false, true));
}

void test_long_link_loss_stops(void)
{
    BuzzerLogic bz(timing);
    bz.update(0, true, true);
    bz.update(100, false, true);
    TEST_ASSERT_FALSE(bz.update(10200, false, true));
    TEST_ASSERT_FALSE(bz.update(20000, false, true));
}

void test_no_link_never_starts(void)
{
    // only the transmitter switches the buzzer on (no automatic search mode)
    BuzzerLogic bz(timing);
    TEST_ASSERT_FALSE(bz.update(0, false, true));
    TEST_ASSERT_FALSE(bz.update(60000, false, true));
}

void test_link_back_restarts_burst(void)
{
    BuzzerLogic bz(timing);
    bz.update(0, true, true);
    bz.update(100, false, true);
    bz.update(20000, false, true);                      // stopped
    TEST_ASSERT_TRUE(bz.update(20500, true, true));
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_silent_by_default);
    RUN_TEST(test_channel_on_beeps_in_bursts);
    RUN_TEST(test_channel_off_stops_at_once);
    RUN_TEST(test_short_link_loss_keeps_beeping);
    RUN_TEST(test_long_link_loss_stops);
    RUN_TEST(test_no_link_never_starts);
    RUN_TEST(test_link_back_restarts_burst);
    UNITY_END();

    return 0;
}
