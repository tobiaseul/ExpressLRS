#include "targets.h"
#include "devBuzzer.h"

#if defined(PLATFORM_ESP32) && defined(TARGET_RX)
#include "common.h"
#include "crsf_protocol.h"
#include "logging.h"
#include "BuzzerLogic.h"
#include "PWM.h"

// 0-based CRSF channel, default CH11 (AUX7); the power switch uses CH12
#if !defined(BUZZER_CHANNEL)
#define BUZZER_CHANNEL 10
#endif
// resonance of the MLT-8540H
#if !defined(BUZZER_HZ)
#define BUZZER_HZ 2700
#endif

#define BUZZER_INTERVAL_MS 20

static const BuzzerTiming timing = {
    .onMs = 300,
    .periodMs = 1000,
    .holdAfterLinkLossMs = 10000,
};

static BuzzerLogic *logic;
static pwm_channel_t channel = -1;
static bool sounding = false;

static void setTone(bool on)
{
    if (on == sounding)
    {
        return;
    }
    sounding = on;
    if (on)
    {
        channel = PWM.allocate(GPIO_PIN_BUZZER, BUZZER_HZ);
        if (channel >= 0)
        {
            PWM.setDuty(channel, 500);
        }
    }
    else
    {
        if (channel >= 0)
        {
            PWM.release(channel);
            channel = -1;
        }
        pinMode(GPIO_PIN_BUZZER, OUTPUT);
        digitalWrite(GPIO_PIN_BUZZER, LOW);
    }
}

void Buzzer_off()
{
    if (GPIO_PIN_BUZZER == UNDEF_PIN)
    {
        return;
    }
    setTone(false);
    pinMode(GPIO_PIN_BUZZER, INPUT);
}

static bool initialize()
{
    if (GPIO_PIN_BUZZER == UNDEF_PIN)
    {
        return false;
    }
    logic = new BuzzerLogic(timing);
    pinMode(GPIO_PIN_BUZZER, OUTPUT);
    digitalWrite(GPIO_PIN_BUZZER, LOW);
    return true;
}

static int start()
{
    return BUZZER_INTERVAL_MS;
}

static int timeout()
{
    const bool linkUp = connectionState == connected;
    const bool channelOn = ChannelData[BUZZER_CHANNEL] > CRSF_CHANNEL_VALUE_MID;
    setTone(logic->update(millis(), linkUp, channelOn));
    return BUZZER_INTERVAL_MS;
}

device_t Buzzer_device = {
    .initialize = initialize,
    .start = start,
    .event = nullptr,
    .timeout = timeout,
};
#endif
