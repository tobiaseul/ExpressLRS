#pragma once

#include <stdint.h>

struct BuzzerTiming {
    uint32_t onMs;                 // tone length of a burst
    uint32_t periodMs;             // burst repeat period
    uint32_t holdAfterLinkLossMs;  // keeps beeping this long after the link drops
};

// Hardware independent logic of the model finder buzzer: it only beeps when the transmitter asks for it
// (channel on with a live link), in bursts, and bridges short link losses (a model in a tree with a weak link).
class BuzzerLogic
{
public:
    explicit BuzzerLogic(const BuzzerTiming &timing) : timing(timing) {}

    // Returns true while the tone should sound.
    bool update(uint32_t now, bool linkUp, bool channelOn);

private:
    const BuzzerTiming timing;
    bool active = false;
    uint32_t startedAt = 0;
    uint32_t lastLinkAt = 0;
};
