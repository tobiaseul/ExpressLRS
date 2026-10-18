#pragma once

// Radio setup for the wake-on-radio PHY, shared by RX and TX (SX1280 only).
#include "common.h"
#include "FHSS.h"
#include "OTA.h"
#include "WakeRadio.h"

static inline uint32_t WakeRadio_freq()
{
    const uint8_t ch = WakeRadio::channel(OtaGetUidSeed(), FHSSgetChannelCount());
    return FHSSconfig->freq_start + (freq_spread * ch / FREQ_SPREAD_SCALE);
}

static inline void WakeRadio_configure()
{
    Radio.Config(SX1280_LORA_BW_0800, SX1280_LORA_SF7, SX1280_LORA_CR_LI_4_8, WakeRadio_freq(),
                 WakeRadio::preambleReg, false, WakeRadio::payloadLength,
                 0, 0, RadioBandMod::Combined::LORA_2G4);
}
