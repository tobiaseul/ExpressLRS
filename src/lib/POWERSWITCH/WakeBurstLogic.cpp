#include "WakeBurstLogic.h"

bool WakeBurstLogic::update(uint32_t now, bool switchOn, bool linkUp, bool armed)
{
    if (!switchOn)
    {
        sessionActive = false;
        return false;
    }
    if (!sessionActive)
    {
        sessionActive = true;
        everLinked = false;
        sessionStart = now;
        nextBurst = now;
    }
    if (linkUp)
    {
        everLinked = true;
    }
    if (everLinked || armed || now - sessionStart >= timing.sessionMs)
    {
        return false;
    }
    if ((int32_t)(now - nextBurst) < 0)
    {
        return false;
    }
    nextBurst = now + timing.periodMs;
    return true;
}
