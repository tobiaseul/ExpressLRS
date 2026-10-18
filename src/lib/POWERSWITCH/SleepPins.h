#pragma once

#include <stdint.h>

#define SLEEP_PINS_MAX 16

// Signal pins that go high impedance before deep sleep, so servos and sensors without
// supply are not fed through their ESD diodes. Undefined pins (-1) are skipped; the
// power switch pin is not passed in, it stays held.
static inline void SleepPins_collect(const int16_t *pwm, uint8_t pwmCount, int serialTx, int serialRx,
                                     int16_t *out, uint8_t &outCount)
{
    outCount = 0;
    for (uint8_t i = 0; i < pwmCount && outCount < SLEEP_PINS_MAX; i++)
    {
        if (pwm[i] >= 0)
        {
            out[outCount++] = pwm[i];
        }
    }
    const int serial[] = {serialTx, serialRx};
    for (int pin : serial)
    {
        if (pin >= 0 && outCount < SLEEP_PINS_MAX)
        {
            out[outCount++] = pin;
        }
    }
}
