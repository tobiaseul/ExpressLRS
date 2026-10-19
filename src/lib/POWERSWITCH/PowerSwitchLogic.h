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
    uint32_t maxAwakeMs;     // output off: longest stay awake in WiFi/bind mode
};

// Battery below 3.2 V per cell (1S or 2S, told apart by the voltage); mV <= 0: no reading -> false.
bool PowerSwitch_lowBattery(int32_t milliVolts);

// Hardware independent decision logic of the remote power switch.
// It never turns the output off without a live link or while armed.
class PowerSwitchLogic
{
public:
    PowerSwitchLogic(const PowerSwitchTiming &timing, PowerSwitchBoot boot, uint32_t now);

    // Returns true when the receiver should enter deep sleep now.
    // armed: arm channel set, the output is not turned off.
    // busy: WiFi, binding or a link being established; with the output off this delays sleep up to maxAwakeMs.
    // lowBattery: a sleep becomes a cutoff (cutoff() true: sleep without wake-up until the battery is replugged).
    bool update(uint32_t now, bool linkUp, bool switchOn, bool armed = false, bool busy = false,
                bool lowBattery = false);
    bool outputOn() const { return on; }
    bool cutoff() const { return cut; }

private:
    bool decide(uint32_t now, bool linkUp, bool switchOn, bool armed, bool busy);

    const PowerSwitchTiming timing;
    const uint32_t awakeSince;
    const uint32_t listenMs;
    bool on;
    bool offPending = false;
    bool cut = false;
    uint32_t offSince = 0;
};
