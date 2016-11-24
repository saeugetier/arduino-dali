/*

ExtSoftwareSerial.cpp - Implementation of the Arduino software serial for ESP8266.
Copyright (c) 2015-2016 Peter Lerup. All rights reserved.

Modified by Timm Eversmeyer 2016

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

#include <Arduino.h>

// The Arduino standard GPIO routines are not enough,
// must use some from the Espressif SDK as well
extern "C" {
#include "gpio.h"
}

#include <ExtSoftwareSerial.h>

#define MAX_PIN 15

// As the Arduino attachInterrupt has no parameter, lists of objects
// and callbacks corresponding to each possible GPIO pins have to be defined
ExtSoftwareSerial *ObjList[MAX_PIN+1];

void ICACHE_RAM_ATTR sws_isr_0() { ObjList[0]->rxRead(); };
void ICACHE_RAM_ATTR sws_isr_1() { ObjList[1]->rxRead(); };
void ICACHE_RAM_ATTR sws_isr_2() { ObjList[2]->rxRead(); };
void ICACHE_RAM_ATTR sws_isr_3() { ObjList[3]->rxRead(); };
void ICACHE_RAM_ATTR sws_isr_4() { ObjList[4]->rxRead(); };
void ICACHE_RAM_ATTR sws_isr_5() { ObjList[5]->rxRead(); };
// Pin 6 to 11 can not be used
void ICACHE_RAM_ATTR sws_isr_12() { ObjList[12]->rxRead(); };
void ICACHE_RAM_ATTR sws_isr_13() { ObjList[13]->rxRead(); };
void ICACHE_RAM_ATTR sws_isr_14() { ObjList[14]->rxRead(); };
void ICACHE_RAM_ATTR sws_isr_15() { ObjList[15]->rxRead(); };

static void (*ISRList[MAX_PIN+1])() = {
      sws_isr_0,
      sws_isr_1,
      sws_isr_2,
      sws_isr_3,
      sws_isr_4,
      sws_isr_5,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      sws_isr_12,
      sws_isr_13,
      sws_isr_14,
      sws_isr_15
};

ExtSoftwareSerial::ExtSoftwareSerial(int receivePin, int transmitPin, bool inverse_logic, unsigned int buffSize) {
   m_rxValid = m_txValid = m_txEnableValid = false;
   m_buffer = NULL;
   m_invert = inverse_logic;
   if (isValidGPIOpin(receivePin)) {
      m_rxPin = receivePin;
      m_buffSize = buffSize;
      m_buffer = (uint8_t*)malloc(m_buffSize);
      if (m_buffer != NULL) {
         m_rxValid = true;
         m_inPos = m_outPos = 0;
         pinMode(m_rxPin, INPUT_PULLUP);
         ObjList[m_rxPin] = this;
         enableRx(true);
      }
   }
   if (isValidGPIOpin(transmitPin)) {
      m_txValid = true;
      m_txPin = transmitPin;
      pinMode(m_txPin, OUTPUT);
      digitalWrite(m_txPin, !m_invert);
   }
   // Default speed
   begin(9600);
}

ExtSoftwareSerial::~ExtSoftwareSerial() {
   enableRx(false);
   if (m_rxValid)
      ObjList[m_rxPin] = NULL;
   if (m_buffer)
      free(m_buffer);
}

bool ExtSoftwareSerial::isValidGPIOpin(int pin) {
   return (pin >= 0 && pin <= 5) || (pin >= 12 && pin <= MAX_PIN);
}

void ExtSoftwareSerial::begin(long speed, uint8_t bitlength_rx, uint8_t bitlength_tx, bool manchester) {
   // Use getCycleCount() loop to get as exact timing as possible
   if(manchester)
   {
     m_bitTime = ESP.getCpuFreqMHz()*500000/speed;
   }
   else
   {
     m_bitTime = ESP.getCpuFreqMHz()*1000000/speed;
   }
   m_manchester = manchester;
   if(bitlength_rx > 7 && bitlength_rx < 17)
      m_bitlength_rx = bitlength_rx;
   else
      m_bitlength_rx = 8;
   if(bitlength_tx > 7 && bitlength_tx < 17)
      m_bitlength_tx = bitlength_tx;
   else
      m_bitlength_tx = 8;
}

long ExtSoftwareSerial::baudRate() {
   // Use getCycleCount() loop to get as exact timing as possible
  long speed = ESP.getCpuFreqMHz()*1000000/m_bitTime;
  if(m_manchester)
  {
    speed = speed / 2;
  }
   return speed;
}

void ExtSoftwareSerial::setTransmitEnablePin(int transmitEnablePin) {
  if (isValidGPIOpin(transmitEnablePin)) {
     m_txEnableValid = true;
     m_txEnablePin = transmitEnablePin;
     pinMode(m_txEnablePin, OUTPUT);
     digitalWrite(m_txEnablePin, LOW);
  } else {
     m_txEnableValid = false;
  }
}

void ExtSoftwareSerial::enableRx(bool on) {
   if (m_rxValid) {
      if (on)
         attachInterrupt(m_rxPin, ISRList[m_rxPin], m_invert ? RISING : FALLING);
      else
         detachInterrupt(m_rxPin);
   }
}

int ExtSoftwareSerial::read() {
   if (!m_rxValid || (m_inPos == m_outPos)) return -1;
   uint8_t ch = m_buffer[m_outPos];
   m_outPos = (m_outPos+1) % m_buffSize;
   return ch;
}

int ExtSoftwareSerial::available() {
   if (!m_rxValid) return 0;
   int avail = m_inPos - m_outPos;
   if (avail < 0) avail += m_buffSize;
   return avail;
}

#define WAIT { while (ESP.getCycleCount()-start < wait); wait += m_bitTime; }

size_t ExtSoftwareSerial::write(uint8_t b) {
   write((uint16_t) b);
}

size_t ExtSoftwareSerial::write(uint16_t w) {
   if (!m_txValid) return 0;

   if (m_invert) w = ~w;
   // Disable interrupts in order to get a clean transmit
   cli();
   unsigned long wait = m_bitTime;
   digitalWrite(m_txPin, LOW);
   unsigned long start = ESP.getCycleCount();
   WAIT;
   if (m_txEnableValid) digitalWrite(m_txEnablePin, HIGH);
   WAIT;
   digitalWrite(m_txPin, HIGH);
   WAIT;
    // Start bit;
    if(m_manchester)
    {
      uint32_t buffer = 0;
      digitalWrite(m_txPin, LOW);
      for(int i = 0; i < m_bitlength_tx; i++)
    	{
    		buffer <<= 2;
    		buffer |= (w & 0x01) ? 0x02 : 0x01;
    		w >>= 1;
    	}
      WAIT;
      digitalWrite(m_txPin, HIGH);
      WAIT;
      for (int i = 0; i < m_bitlength_tx * 2; i++) {
        digitalWrite(m_txPin, (buffer & 1) ? HIGH : LOW);
        WAIT;
        buffer >>= 1;
      }
    }
    else
    {
      digitalWrite(m_txPin, LOW);
      WAIT;
      for (int i = 0; i < m_bitlength_tx; i++) {
        digitalWrite(m_txPin, (w & 1) ? HIGH : LOW);
        WAIT;
        w >>= 1;
      }
    }

   // Stop bit
   digitalWrite(m_txPin, HIGH);
   WAIT;
   WAIT;
   WAIT;
   WAIT;
   if (m_txEnableValid) digitalWrite(m_txEnablePin, LOW);
   sei();
   return 1;
}

void ExtSoftwareSerial::flush() {
   m_inPos = m_outPos = 0;
}

int ExtSoftwareSerial::peek() {
   if (!m_rxValid || (m_inPos == m_outPos)) return -1;
   return m_buffer[m_outPos];
}

void ICACHE_RAM_ATTR ExtSoftwareSerial::rxRead() {
   // Advance the starting point for the samples but compensate for the
   // initial delay which occurs before the interrupt is delivered
   unsigned long wait = m_bitTime + m_bitTime/3 - 500;
   unsigned long start = ESP.getCycleCount();
   uint8_t rec = 0;
   for (int i = 0; i < m_bitlength_rx; i++) {
     WAIT;
     rec >>= 1;
     if (digitalRead(m_rxPin))
       rec |= 0x80;
   }
   if (m_invert) rec = ~rec;
   // Stop bit
   WAIT;
   // Store the received value in the buffer unless we have an overflow
   int next = (m_inPos+1) % m_buffSize;
   if (next != m_inPos) {
      m_buffer[m_inPos] = rec;
      m_inPos = next;
   }
   // Must clear this bit in the interrupt register,
   // it gets set even when interrupts are disabled
   GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, 1 << m_rxPin);
}
