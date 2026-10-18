#include "targets.h"
#include "devPowerSwitch.h"

#if defined(PLATFORM_ESP32) && defined(TARGET_RX)
#include "common.h"
#include "crsf_protocol.h"
#include "logging.h"
#include "PowerSwitchLogic.h"

#include <driver/gpio.h>
#include <esp_sleep.h>

// 0-based CRSF channel, default CH12 (AUX8)
#if !defined(POWER_SWITCH_CHANNEL)
#define POWER_SWITCH_CHANNEL 11
#endif
#if !defined(POWER_SWITCH_SLEEP_MS)
#define POWER_SWITCH_SLEEP_MS 6000
#endif

#define POWER_SWITCH_INTERVAL_MS 20

static const PowerSwitchTiming timing = {
    .offConfirmMs = 1000,
    // the TX visits the sync channel once per 80 hops, 3.2 s at 50 Hz
    .listenWindowMs = 4000,
    // longer than the 60 s auto WiFi start of a disconnected RX
    .powerOnListenMs = 90000,
};

static PowerSwitchLogic *logic;

static PowerSwitchBoot bootReason()
{
    switch (esp_reset_reason())
    {
    case ESP_RST_POWERON:
        return PowerSwitchBoot::PowerOn;
    case ESP_RST_DEEPSLEEP:
        return PowerSwitchBoot::WakeFromSleep;
    default:
        return PowerSwitchBoot::OtherReset;
    }
}

static void writeOutput()
{
    digitalWrite(GPIO_PIN_POWER_SWITCH, logic->outputOn() ? HIGH : LOW);
}

static void enterDeepSleep()
{
    DBGLN("power switch: sleep %u ms", POWER_SWITCH_SLEEP_MS);
    writeOutput();
    gpio_hold_en((gpio_num_t)GPIO_PIN_POWER_SWITCH);
    gpio_deep_sleep_hold_en();
    Radio.End(); // SX1280 sleep, PA/LNA off
    esp_sleep_enable_timer_wakeup((uint64_t)POWER_SWITCH_SLEEP_MS * 1000);
    esp_deep_sleep_start();
}

static bool initialize()
{
    if (GPIO_PIN_POWER_SWITCH == UNDEF_PIN)
    {
        return false;
    }
    logic = new PowerSwitchLogic(timing, bootReason(), millis());
    pinMode(GPIO_PIN_POWER_SWITCH, OUTPUT);
    writeOutput();
    // the level was held through deep sleep, release only after it is driven again
    gpio_hold_dis((gpio_num_t)GPIO_PIN_POWER_SWITCH);
    return true;
}

static int start()
{
    return POWER_SWITCH_INTERVAL_MS;
}

bool PowerSwitch_isSleepWake()
{
    return GPIO_PIN_POWER_SWITCH != UNDEF_PIN && esp_reset_reason() == ESP_RST_DEEPSLEEP;
}

static int timeout()
{
    // never sleep while binding or in WiFi/update mode
    if (InBindingMode || connectionState > MODE_STATES)
    {
        return POWER_SWITCH_INTERVAL_MS;
    }
    const bool linkUp = connectionState == connected;
    const bool switchOn = ChannelData[POWER_SWITCH_CHANNEL] > CRSF_CHANNEL_VALUE_MID;
    const bool sleep = logic->update(millis(), linkUp, switchOn);
    writeOutput();
    if (sleep)
    {
        enterDeepSleep();
    }
    return POWER_SWITCH_INTERVAL_MS;
}

device_t PowerSwitch_device = {
    .initialize = initialize,
    .start = start,
    .event = nullptr,
    .timeout = timeout,
};
#endif
