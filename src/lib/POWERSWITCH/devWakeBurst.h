#pragma once

#include "device.h"

// TX side of the remote power switch: sends wake bursts to a sleeping
// receiver when the power switch channel goes on.
extern device_t WakeBurst_device;
