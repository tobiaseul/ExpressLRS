#pragma once

#include "device.h"

// Model finder buzzer: a passive buzzer on misc_buzzer, beeps while the buzzer channel is on.
extern device_t Buzzer_device;

// Tone off and pin released, e.g. before deep sleep (the external pull-down keeps the driver off).
void Buzzer_off();
