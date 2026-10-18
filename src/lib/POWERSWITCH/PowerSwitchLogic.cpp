#include "PowerSwitchLogic.h"

PowerSwitchLogic::PowerSwitchLogic(const PowerSwitchTiming &timing, PowerSwitchBoot boot, uint32_t now)
    : timing(timing), awakeSince(now),
      listenMs(boot == PowerSwitchBoot::PowerOn ? timing.powerOnListenMs : timing.listenWindowMs),
      on(boot == PowerSwitchBoot::OtherReset)
{
}

bool PowerSwitchLogic::update(uint32_t now, bool linkUp, bool switchOn)
{
    if (!linkUp)
    {
        offPending = false;
        return !on && (now - awakeSince >= listenMs);
    }

    if (switchOn)
    {
        on = true;
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
