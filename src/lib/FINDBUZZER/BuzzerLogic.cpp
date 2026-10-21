#include "BuzzerLogic.h"

bool BuzzerLogic::update(uint32_t now, bool linkUp, bool channelOn)
{
    if (linkUp)
    {
        lastLinkAt = now;
        if (channelOn && !active)
        {
            startedAt = now;
        }
        active = channelOn;
    }
    else if (active && now - lastLinkAt >= timing.holdAfterLinkLossMs)
    {
        active = false;
    }
    return active && (now - startedAt) % timing.periodMs < timing.onMs;
}
