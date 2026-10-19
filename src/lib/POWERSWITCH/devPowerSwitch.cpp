#include "targets.h"
#include "devPowerSwitch.h"

#if defined(PLATFORM_ESP32) && defined(TARGET_RX)
#include "common.h"
#include "crsf_protocol.h"
#include "logging.h"
#include "PowerSwitchLogic.h"
#include "SleepPins.h"

#include "hwTimer.h"
#include "RFAMP_hal.h"
#include "WakeRadioHw.h"

#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

extern RFAMP_hal RFAMP;
// devAnalogVbat.h, declared here: including it would pull the AnalogVbat library into the native test builds
extern int32_t Vbat_getMilliVolts();

// 0-based CRSF channel, default CH12 (AUX8)
#if !defined(POWER_SWITCH_CHANNEL)
#define POWER_SWITCH_CHANNEL 11
#endif
// timer wake when DIO1 cannot wake the ESP32, emergency wake otherwise
#if !defined(POWER_SWITCH_SLEEP_MS)
#define POWER_SWITCH_SLEEP_MS 6000
#endif
#define POWER_SWITCH_EMERGENCY_WAKE_MS (6ull * 3600 * 1000)

#define POWER_SWITCH_INTERVAL_MS 20

static const PowerSwitchTiming timing = {
    .offConfirmMs = 1000,
    // the TX visits the sync channel once per 80 hops, 3.2 s at 50 Hz,
    // and keeps 5.4 s between wake bursts
    .listenWindowMs = 5000,
    // longer than the 60 s auto WiFi start of a disconnected RX
    .powerOnListenMs = 90000,
    // WiFi/bind mode left on with the output off: sleep after 15 min instead of draining the battery
    .maxAwakeMs = 15 * 60 * 1000,
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

static void holdPin(int pin)
{
    if (pin != UNDEF_PIN)
    {
        gpio_hold_en((gpio_num_t)pin);
    }
}

static void releasePin(int pin)
{
    if (pin != UNDEF_PIN)
    {
        gpio_hold_dis((gpio_num_t)pin);
    }
}

static bool canWakeOnRadio()
{
    return GPIO_PIN_DIO1 != UNDEF_PIN && rtc_gpio_is_valid_gpio((gpio_num_t)GPIO_PIN_DIO1);
}

// The SX1280 keeps listening in RX duty cycle on the wake channel and raises DIO1
// on a wake burst, while the ESP32 stays in deep sleep.
static void startWakeOnRadio()
{
    hwTimer::stop();
    WakeRadio_configure();
    // front end off (< 1 uA): the wake burst comes from a transmitter nearby and gets through
    // the unpowered LNA, keeping the LNA on would cost mA
    RFAMP.TXRXdisable();
    Radio.RXdutyCycle(SX1280_IRQ_RX_DONE, WakeRadio::dutyPeriodBase, WakeRadio::dutyRxTicks, WakeRadio::dutySleepTicks);
    // keep the radio selected-off, out of reset and the front end in RX while the ESP32 sleeps
    holdPin(GPIO_PIN_NSS);
    holdPin(GPIO_PIN_RST);
    holdPin(GPIO_PIN_TX_ENABLE);
    holdPin(GPIO_PIN_RX_ENABLE);
    holdPin(GPIO_PIN_ANT_CTRL);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)GPIO_PIN_DIO1, 1);
    esp_sleep_enable_timer_wakeup(POWER_SWITCH_EMERGENCY_WAKE_MS * 1000);
}

static void floatSignalPins()
{
    int16_t pins[SLEEP_PINS_MAX];
    uint8_t count = 0;
    SleepPins_collect(GPIO_PIN_PWM_OUTPUTS, GPIO_PIN_PWM_OUTPUTS_COUNT, GPIO_PIN_RCSIGNAL_TX, GPIO_PIN_RCSIGNAL_RX,
                      pins, count);
    for (uint8_t i = 0; i < count; i++)
    {
        pinMode(pins[i], INPUT);
    }
}

static void enterDeepSleep(bool cutoff)
{
    writeOutput();
    floatSignalPins();
    holdPin(GPIO_PIN_POWER_SWITCH);
    if (cutoff)
    {
        // battery below 3.2 V per cell: no wake source, the radio sleeps too; only replugging the battery
        // (power-on reset) wakes the receiver
        DBGLN("power switch: low battery, sleep until the battery is replugged");
        Radio.End();
    }
    else if (canWakeOnRadio())
    {
        DBGLN("power switch: sleep until wake burst");
        startWakeOnRadio();
    }
    else
    {
        DBGLN("power switch: sleep %u ms", POWER_SWITCH_SLEEP_MS);
        Radio.End(); // SX1280 sleep, PA/LNA off
        esp_sleep_enable_timer_wakeup((uint64_t)POWER_SWITCH_SLEEP_MS * 1000);
    }
    gpio_deep_sleep_hold_en();
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
    releasePin(GPIO_PIN_POWER_SWITCH);
    // radio pins held for wake-on-radio, released before setupRadio() resets the SX1280
    releasePin(GPIO_PIN_NSS);
    releasePin(GPIO_PIN_RST);
    releasePin(GPIO_PIN_TX_ENABLE);
    releasePin(GPIO_PIN_RX_ENABLE);
    releasePin(GPIO_PIN_ANT_CTRL);
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
    // binding, WiFi/update mode or a link being established: no sleep (with the output off at most maxAwakeMs)
    const bool busy = InBindingMode || connectionState > MODE_STATES || connectionState == tentative;
    const bool linkUp = connectionState == connected;
    const bool switchOn = ChannelData[POWER_SWITCH_CHANNEL] > CRSF_CHANNEL_VALUE_MID;
    const bool armed = CRSF_to_BIT(ChannelData[AUX1]);
    const bool lowBattery = PowerSwitch_lowBattery(Vbat_getMilliVolts());
    const bool sleep = logic->update(millis(), linkUp, switchOn, armed, busy, lowBattery);
    writeOutput();
    if (sleep)
    {
        enterDeepSleep(logic->cutoff());
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
