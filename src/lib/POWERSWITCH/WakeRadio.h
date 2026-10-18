#pragma once

#include <stdint.h>

// Wake-on-radio PHY shared by the TX (wake burst) and the sleeping RX
// (SX1280 RX duty cycle). The preamble is longer than one RX duty cycle,
// so a sleeping receiver always sees it in one of its short RX windows.
namespace WakeRadio
{
    // LoRa BW 800 (812.5 kHz), SF7, CR 4/8, implicit header, 8 byte payload
    constexpr uint32_t bandwidthHz = 812500;
    constexpr uint8_t spreadingFactor = 7;
    constexpr uint32_t symbolNs = (uint32_t)((1ull << spreadingFactor) * 1000000000ull / bandwidthHz);

    // SX1280 LoRa preamble register: length = mantissa * 2^exponent
    constexpr uint8_t preambleMantissa = 7;
    constexpr uint8_t preambleExponent = 9;
    constexpr uint8_t preambleReg = (preambleExponent << 4) | preambleMantissa;
    constexpr uint32_t preambleSymbols = (uint32_t)preambleMantissa << preambleExponent;
    constexpr uint32_t preambleUs = (uint32_t)((uint64_t)preambleSymbols * symbolNs / 1000);

    constexpr uint8_t payloadLength = 8;
    constexpr uint32_t payloadSymbols = 16; // SF7, CR 4/8, implicit header, no CRC, 8 bytes
    // time on air = (preamble + 4.25 + payload) symbols
    constexpr uint32_t airtimeUs = (uint32_t)((uint64_t)(4 * (preambleSymbols + payloadSymbols) + 17) * symbolNs / 4000);

    // SX1280 SetRxDutyCycle with a 62.5 us period base
    constexpr uint8_t dutyPeriodBase = 0x01;
    constexpr uint32_t dutyTickNs = 62500;
    constexpr uint16_t dutyRxTicks = 24;
    constexpr uint16_t dutySleepTicks = 8000;
    constexpr uint32_t dutyRxUs = dutyRxTicks * dutyTickNs / 1000;
    constexpr uint32_t dutySleepUs = dutySleepTicks * dutyTickNs / 1000;

    // SX1280 driver starts TX with timeout 0xFFFF * 15.625 us
    constexpr uint32_t txTimeoutUs = 65535u * 15625u / 1000u;

    static_assert(preambleUs > dutyRxUs + dutySleepUs + 20000, "preamble must span one RX duty cycle");
    static_assert(airtimeUs < txTimeoutUs, "wake burst longer than the SX1280 TX timeout");

    // Wake channel derived from the binding UID, never the sync channel (count / 2),
    // so bursts of other pilots rarely wake this receiver.
    inline uint8_t channel(uint32_t uidSeed, uint8_t channelCount)
    {
        const uint8_t sync = channelCount / 2;
        uint8_t ch = uidSeed % (channelCount - 1);
        if (ch >= sync)
        {
            ch++;
        }
        return ch;
    }
}
