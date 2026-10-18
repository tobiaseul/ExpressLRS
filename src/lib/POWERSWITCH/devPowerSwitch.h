#pragma once

#include "device.h"

// Remote power switch: drives the servo supply FET from a channel and puts the
// receiver into deep sleep while switched off, waking periodically to listen.
extern device_t PowerSwitch_device;

// True after a periodic wake from deep sleep (not a real power cycle).
bool PowerSwitch_isSleepWake();
