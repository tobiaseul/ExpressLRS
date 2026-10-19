#include "PowerSwitchLogic.h"

bool PowerSwitch_lowBattery(int32_t milliVolts)
{
    if (milliVolts <= 0)
    {
        return false;
    }
    const int32_t cells = milliVolts > 4400 ? 2 : 1;   // 1S tops out at 4.2-4.35 V
    return milliVolts < 3200 * cells;
}

PowerSwitchLogic::PowerSwitchLogic(const PowerSwitchTiming &timing, PowerSwitchBoot boot, uint32_t now)
    : timing(timing), awakeSince(now),
      listenMs(boot == PowerSwitchBoot::PowerOn ? timing.powerOnListenMs : timing.listenWindowMs),
      on(boot == PowerSwitchBoot::OtherReset)
{
}

bool PowerSwitchLogic::update(uint32_t now, bool linkUp, bool switchOn, bool armed, bool busy, bool lowBattery)
{
    const bool sleep = decide(now, linkUp, switchOn, armed, busy);
    cut = sleep && lowBattery;
    return sleep;
}

bool PowerSwitchLogic::decide(uint32_t now, bool linkUp, bool switchOn, bool armed, bool busy)
{
    if (!linkUp)
    {
        offPending = false;
        if (on)
        {
            return false;
        }
        return now - awakeSince >= (busy ? timing.maxAwakeMs : listenMs);
    }

    if (switchOn || armed)
    {
        on = on || switchOn;
        offPending = false;
        return false;
    }

    if (!offPending)
    {
        offPending = true;
        offSince = now;
    }
    if (now - offSince < timing.offConfirmMs)
    {
        return false;
    }
    on = false;
    return true;
}
