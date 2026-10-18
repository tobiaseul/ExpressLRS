#pragma once

#include <stdint.h>

struct WakeBurstTiming {
    uint32_t periodMs;  // time between wake bursts
    uint32_t sessionMs; // give up waking after this long
};

// TX side: decides when to send a wake burst to a sleeping receiver.
// A session starts when the power switch channel goes on. Bursts stop for the
// rest of the session as soon as a link was seen, so a link lost in flight
// is never disturbed by bursts.
class WakeBurstLogic
{
public:
    explicit WakeBurstLogic(const WakeBurstTiming &timing) : timing(timing) {}

    // Returns true when a wake burst should be sent now.
    bool update(uint32_t now, bool switchOn, bool linkUp, bool armed);
    // The channel was busy (LBT): try again after retryMs.
    void retryIn(uint32_t now, uint32_t retryMs) { nextBurst = now + retryMs; }

private:
    const WakeBurstTiming timing;
    bool sessionActive = false;
    bool everLinked = false;
    uint32_t sessionStart = 0;
    uint32_t nextBurst = 0;
};
