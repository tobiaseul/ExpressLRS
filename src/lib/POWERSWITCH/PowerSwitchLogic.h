#pragma once

#include <stdint.h>

// How the receiver came out of reset. Decides the servo supply state before
// the first packet arrives.
enum class PowerSwitchBoot : uint8_t {
    PowerOn,       // battery plugged in -> off
    WakeFromSleep, // periodic wake to listen -> off
    OtherReset,    // watchdog, brown-out, panic: may be in flight -> on
};

struct PowerSwitchTiming {
    uint32_t offConfirmMs;   // switch must read OFF this long on a live link
    uint32_t listenWindowMs; // time awake without link before sleeping again
    uint32_t powerOnListenMs; // same after plugging in, leaves time for WiFi/binding
};

// Hardware independent decision logic of the remote power switch.
// It never turns the output off without a live link.
class PowerSwitchLogic
{
public:
    PowerSwitchLogic(const PowerSwitchTiming &timing, PowerSwitchBoot boot, uint32_t now);

    // Returns true when the receiver should enter deep sleep now.
    bool update(uint32_t now, bool linkUp, bool switchOn);
    bool outputOn() const { return on; }

private:
    const PowerSwitchTiming timing;
    const uint32_t awakeSince;
    const uint32_t listenMs;
    bool on;
    bool offPending = false;
    uint32_t offSince = 0;
};
