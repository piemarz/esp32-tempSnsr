/*
 * dhtDriver.h
 *
 *  Created on: 5 Mar 2024
 *      Author: piemarz
 *
 *  MIT License
 *
 * Copyright (c) 2024 piemarz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * Thanks to beegee-tokyo and markruys for the arduino version
 */

#ifndef DHT_DHTDRIVER_H_
#define DHT_DHTDRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * INCLUDES
 ************************************/
#include "main.h"

/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/
// Reference: http://epb.apogee.net/res/refcomf.asp (References invalid)
typedef enum ComfortState_e {
  Comfort_OK = 0,
  Comfort_TooHot = 1,
  Comfort_TooCold = 2,
  Comfort_TooDry = 4,
  Comfort_TooHumid = 8,
  Comfort_HotAndHumid = 9,
  Comfort_HotAndDry = 5,
  Comfort_ColdAndHumid = 10,
  Comfort_ColdAndDry = 6
}ComfortState_t;

// References https://en.wikipedia.org/wiki/Dew_point ==> Relationship to human comfort
typedef enum PerceptionState_e {
  Perception_Dry = 0,
  Perception_VeryComfy = 1,
  Perception_Comfy = 2,
  Perception_Ok = 3,
  Perception_UnComfy = 4,
  Perception_QuiteUnComfy = 5,
  Perception_VeryUnComfy = 6,
  Perception_SevereUncomfy = 7
}PerceptionState_t;

typedef struct TempAndHumidity_s {
  float temperature;
  float humidity;
} TempAndHumidity_t;

typedef enum DHT_MODEL_e {
  AUTO_DETECT,
  DHT11,
  DHT22,
  AM2302,  // Packaged DHT22
  RHT03    // Equivalent to DHT22
} DHT_MODEL_t;

typedef enum DHT_ERROR_e {
  ERROR_NONE = 0,
  ERROR_TIMEOUT,
  ERROR_CHECKSUM
} DHT_ERROR_t;

typedef struct ComfortProfile_s
{
  //Represent the 4 line equations:
  //dry, humid, hot, cold, using the y = mx + b formula
  float m_tooHot_m, m_tooHot_b;
  float m_tooCold_m, m_tooHCold_b;
  float m_tooDry_m, m_tooDry_b;
  float m_tooHumid_m, m_tooHumid_b;
} ComfortProfile_t;

/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
void dhtDriver_setup(uint8_t pin, DHT_MODEL_t model);
void dhtDriver_resetTimer();

float dhtDriver_getHumidity();
float dhtDriver_getTemperature();
TempAndHumidity_t dhtDriver_getTempAndHumidity();

DHT_ERROR_t dhtDriver_getStatus();
const char* dhtDriver_getStatusString();

DHT_MODEL_t dhtDriver_getModel();

int dhtDriver_getMinimumSamplingPeriod();

int8_t dhtDriver_getNumberOfDecimalsTemperature();
int8_t dhtDriver_getLowerBoundTemperature();
int8_t dhtDriver_getUpperBoundTemperature();

int8_t dhtDriver_getNumberOfDecimalsHumidity();
int8_t dhtDriver_getLowerBoundHumidity();
int8_t dhtDriver_getUpperBoundHumidity();

float dhtDriver_toFahrenheit(float fromCelcius);
float dhtDriver_toCelsius(float fromFahrenheit);

float dhtDriver_computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit);
float dhtDriver_computeDewPoint(float temperature, float percentHumidity, bool isFahrenheit);
float dhtDriver_getComfortRatio(ComfortState_t *destComfStatus, float temperature, float percentHumidity, bool isFahrenheit);
ComfortProfile_t dhtDriver_getComfortProfile();
void dhtDriver_setComfortProfile(ComfortProfile_t c);
bool dhtDriver_isTooHot(float temp, float humidity);
bool dhtDriver_isTooHumid(float temp, float humidity);
bool dhtDriver_isTooCold(float temp, float humidity);
bool dhtDriver_isTooDry(float temp, float humidity);
uint8_t dhtDriver_computePerception(float temperature, float percentHumidity, bool isFahrenheit);
float dhtDriver_computeAbsoluteHumidity(float temperature, float percentHumidity, bool isFahrenheit);
uint8_t dhtDriver_getPin();

#ifdef __cplusplus
}
#endif

#endif /* DHT_DHTDRIVER_H_ */
