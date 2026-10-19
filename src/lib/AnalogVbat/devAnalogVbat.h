#pragma once

#if defined(TARGET_RX)

#include "common.h"
#include "device.h"

void Vbat_enableSlowUpdate(bool enable);
void Vbat_setCalibrationActive(bool active);
// last measured battery voltage in mV (first voltage source), 0 before the first reading
int32_t Vbat_getMilliVolts();

extern device_t AnalogVbat_device;

#endif
