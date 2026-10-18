#include "targets.h"
#include "devWakeBurst.h"

#if defined(PLATFORM_ESP32) && defined(TARGET_TX) && defined(RADIO_SX128X)
#include "common.h"
#include "crsf_protocol.h"
#include "hwTimer.h"
#include "LBT.h"
#include "logging.h"
#include "POWERMGNT.h"
#include "WakeBurstLogic.h"
#include "WakeRadioHw.h"

// 0-based CRSF channel, same as on the RX, default CH12 (AUX8)
#if !defined(POWER_SWITCH_CHANNEL)
#define POWER_SWITCH_CHANNEL 11
#endif

#define WAKE_BURST_INTERVAL_MS 20
#define WAKE_BURST_LBT_RETRY_MS 100

extern volatile bool busyTransmitting;
extern void SetRFLinkRate(uint8_t index);

static const WakeBurstTiming timing = {
    // 0.57 s on air every 6 s, about 10 % channel use; the gap is longer than the RX listen window
    .periodMs = 6000,
    .sessionMs = 120000,
};

static WakeBurstLogic logic(timing);
static PowerLevels_e savedPower;
static bool bursting = false;

static bool channelClear()
{
    if (!LbtIsEnabled)
    {
        return true;
    }
    Radio.RXnb();
    LbtCcaTimerStart();
    return LbtChannelIsClear(SX12XX_Radio_1) != SX12XX_Radio_NONE;
}

static void restoreLink()
{
    POWERMGNT::setPower(savedPower);
    // SetRFLinkRate() skips an unchanged rate; point at another valid rate (other cores
    // read it) so the radio is configured for the link again
    const uint8_t index = ExpressLRS_currAirRate_Modparams->index;
    ExpressLRS_currAirRate_Modparams = get_elrs_airRateConfig((index + 1) % RATE_MAX);
    SetRFLinkRate(index);
    hwTimer::resume();
}

// Pauses the link and sends one wake burst. Returns false if LBT found the channel busy.
static bool startBurst(uint32_t now)
{
    hwTimer::stop();
    while (busyTransmitting);

    savedPower = POWERMGNT::currPower();
    POWERMGNT::setPower(POWERMGNT::getMinPower());
    Radio.CommitOutputPower();
    WakeRadio_configure();
    if (!channelClear())
    {
        logic.retryIn(now, WAKE_BURST_LBT_RETRY_MS);
        restoreLink();
        return false;
    }

    WORD_ALIGNED_ATTR uint8_t payload[WakeRadio::payloadLength] = {'W', 'A', 'K', 'E', UID[2], UID[3], UID[4], UID[5]};
    // TXdoneISR() ignores the TX done IRQ because busyTransmitting is false
    Radio.TXnb(payload, false, nullptr, SX12XX_Radio_1);
    return true;
}

static int timeout()
{
    if (bursting)
    {
        bursting = false;
        restoreLink();
        return WAKE_BURST_INTERVAL_MS;
    }
    // without telemetry the TX never sees a link and would keep bursting into it
    if (connectionState > MODE_STATES || InBindingMode || ExpressLRS_currTlmDenom == 1)
    {
        return WAKE_BURST_INTERVAL_MS;
    }

    const uint32_t now = millis();
    const bool switchOn = ChannelData[POWER_SWITCH_CHANNEL] > CRSF_CHANNEL_VALUE_MID;
    if (!logic.update(now, switchOn, connectionState == connected, isArmed) || !startBurst(now))
    {
        return WAKE_BURST_INTERVAL_MS;
    }
    DBGLN("wake burst on %u", WakeRadio_freq());
    bursting = true;
    return WakeRadio::airtimeUs / 1000 + 20;
}

static int start()
{
    return WAKE_BURST_INTERVAL_MS;
}

device_t WakeBurst_device = {
    .initialize = nullptr,
    .start = start,
    .event = nullptr,
    .timeout = timeout,
};
#endif
