/*

SoftwareSerial.cpp - Implementation of the Arduino software serial for ESP8266/ESP32.
Copyright (c) 2015-2016 Peter Lerup. All rights reserved.
Copyright (c) 2018-2019 Dirk O. Kaar. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "ESPSoftwareSerial.h"
#include <Arduino.h>

#ifdef ESP32
#define xt_rsil(a) (a)
#define xt_wsr_ps(a)
#endif

constexpr uint8_t BYTE_ALL_BITS_SET = ~static_cast<uint8_t>(0);

ESPSoftwareSerial::ESPSoftwareSerial(int8_t txPin, bool invert)
{
    m_txPin = txPin;
    m_invert = invert;
}

ESPSoftwareSerial::~ESPSoftwareSerial() {
    end();
}

bool ESPSoftwareSerial::isValidGPIOpin(int8_t pin) {
#if defined(ESP8266)
    return (pin >= 0 && pin <= 16) && !isFlashInterfacePin(pin);
#elif defined(ESP32)
    return (pin >= 0 && pin <= 5) || (pin >= 12 && pin <= 19) ||
        (pin >= 21 && pin <= 23) || (pin >= 25 && pin <= 27) || (pin >= 32 && pin <= 35);
#else
    return true;
#endif
}

bool ESPSoftwareSerial::isValidRxGPIOpin(int8_t pin) {
    return isValidGPIOpin(pin)
#if defined(ESP8266)
        && (pin != 16)
#endif
        ;
}

void ESPSoftwareSerial::begin(uint32_t baud, SoftwareSerialConfig config,
    int8_t txPin,
    bool invert, int bufCapacity, int isrBufCapacity) {
    if (-1 != txPin) m_txPin = txPin;
    m_invert = invert;
    m_dataBits = 5 + (config & 07);
    m_parityMode = static_cast<SoftwareSerialParity>(config & 070);
    m_stopBits = 1 + ((config & 0300) ? 1 : 0);
    m_pduBits = m_dataBits + static_cast<bool>(m_parityMode) + m_stopBits;
    m_bitCycles = (ESP.getCpuFreqMHz() * 1000000UL + baud / 2) / baud;
    m_intTxEnabled = true;
    if (isValidGPIOpin(m_txPin)) {
        m_txValid = true;
        pinMode(m_txPin, OUTPUT);
        digitalWrite(m_txPin, !m_invert);
    }
}

void ESPSoftwareSerial::end()
{
    m_txValid = false;
}

uint32_t ESPSoftwareSerial::baudRate() {
    return ESP.getCpuFreqMHz() * 1000000UL / m_bitCycles;
}

void ESPSoftwareSerial::setTransmitEnablePin(int8_t txEnablePin) {
    if (isValidGPIOpin(txEnablePin)) {
        m_txEnableValid = true;
        m_txEnablePin = txEnablePin;
        pinMode(m_txEnablePin, OUTPUT);
        digitalWrite(m_txEnablePin, LOW);
    }
    else {
        m_txEnableValid = false;
    }
}

void ESPSoftwareSerial::enableIntTx(bool on) {
    m_intTxEnabled = on;
}

void ESPSoftwareSerial::enableTx(bool on) {
}

void ICACHE_RAM_ATTR ESPSoftwareSerial::preciseDelay(bool sync) {
    if (!sync)
    {
        // Reenable interrupts while delaying to avoid other tasks piling up
        if (!m_intTxEnabled) { xt_wsr_ps(m_savedPS); }
        const auto expired = ESP.getCycleCount() - m_periodStart;
        const auto ms = (m_periodDuration - expired) / ESP.getCpuFreqMHz() / 1000UL;
        if (ms)
        {
            delay(ms);
        }
        else
        {
            do
            {
                optimistic_yield(10000UL);
            }
            while ((ESP.getCycleCount() - m_periodStart) < m_periodDuration);
        }
        // Disable interrupts again
        if (!m_intTxEnabled) { m_savedPS = xt_rsil(15); }
    }
    else
    {
        while ((ESP.getCycleCount() - m_periodStart) < m_periodDuration) {}
    }
    m_periodDuration = 0;
    m_periodStart = ESP.getCycleCount();
}

void ICACHE_RAM_ATTR ESPSoftwareSerial::writePeriod(
    uint32_t dutyCycle, uint32_t offCycle, bool withStopBit) {
    preciseDelay(true);
    if (dutyCycle)
    {
        digitalWrite(m_txPin, HIGH);
        m_periodDuration += dutyCycle;
        if (offCycle || (withStopBit && !m_invert)) preciseDelay(!withStopBit || m_invert);
    }
    if (offCycle)
    {
        digitalWrite(m_txPin, LOW);
        m_periodDuration += offCycle;
        if (withStopBit && m_invert) preciseDelay(false);
    }
}

size_t ESPSoftwareSerial::write(uint8_t byte) {
    return write(&byte, 1);
}

size_t ESPSoftwareSerial::write(uint8_t byte, SoftwareSerialParity parity) {
    return write(&byte, 1, parity);
}

size_t ESPSoftwareSerial::write(const uint8_t* buffer, size_t size) {
    return write(buffer, size, m_parityMode);
}

size_t ICACHE_RAM_ATTR ESPSoftwareSerial::write(const uint8_t* buffer, size_t size, SoftwareSerialParity parity) {
    if (!m_txValid) { return -1; }

    if (m_txEnableValid) {
        digitalWrite(m_txEnablePin, HIGH);
    }
    // Stop bit: if inverted, LOW, otherwise HIGH
    bool b = !m_invert;
    uint32_t dutyCycle = 0;
    uint32_t offCycle = 0;
    if (!m_intTxEnabled) {
        // Disable interrupts in order to get a clean transmit timing
        m_savedPS = xt_rsil(15);
    }
    const uint32_t dataMask = ((1UL << m_dataBits) - 1);
    bool withStopBit = true;
    m_periodDuration = 0;
    m_periodStart = ESP.getCycleCount();
    for (size_t cnt = 0; cnt < size; ++cnt) {
        uint8_t byte = pgm_read_byte(buffer + cnt) & dataMask;
        // push LSB start-data-parity-stop bit pattern into uint32_t
        // Stop bits: HIGH
        uint32_t word = ~0UL;
        // inverted parity bit, performance tweak for xor all-bits-set word
        if (parity && m_parityMode)
        {
            uint32_t parityBit;
            switch (parity)
            {
            case SWSERIAL_PARITY_EVEN:
                // from inverted, so use odd parity
                parityBit = byte;
                parityBit ^= parityBit >> 4;
                parityBit &= 0xf;
                parityBit = (0x9669 >> parityBit) & 1;
                break;
            case SWSERIAL_PARITY_ODD:
                // from inverted, so use even parity
                parityBit = byte;
                parityBit ^= parityBit >> 4;
                parityBit &= 0xf;
                parityBit = (0x6996 >> parityBit) & 1;
                break;
            case SWSERIAL_PARITY_MARK:
                parityBit = 0;
                break;
            case SWSERIAL_PARITY_SPACE:
                // suppresses warning parityBit uninitialized
            default:
                parityBit = 1;
                break;
            }
            word ^= parityBit;
        }
        word <<= m_dataBits;
        word |= byte;
        // Start bit: LOW
        word <<= 1;
        if (m_invert) word = ~word;
        for (int i = 0; i <= m_pduBits; ++i) {
            bool pb = b;
            b = word & (1UL << i);
            if (!pb && b) {
                writePeriod(dutyCycle, offCycle, withStopBit);
                withStopBit = false;
                dutyCycle = offCycle = 0;
            }
            if (b) {
                dutyCycle += m_bitCycles;
            }
            else {
                offCycle += m_bitCycles;
            }
        }
        withStopBit = true;
    }
    writePeriod(dutyCycle, offCycle, true);
    if (!m_intTxEnabled) {
        // restore the interrupt state
        xt_wsr_ps(m_savedPS);
    }
    if (m_txEnableValid) {
        digitalWrite(m_txEnablePin, LOW);
    }
    return size;
}

int ESPSoftwareSerial::peek() {
    return 0;
}

int ESPSoftwareSerial::available() {
    return 0;
}

int ESPSoftwareSerial::read() {
    return 0;
}

