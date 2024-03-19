/*
 * dhtDriver.c
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
 * Thanks to beegee-tokyo and markruys for the Arduino version
 */


/************************************
 * INCLUDES
 ************************************/
#include <math.h>
#include <rom/ets_sys.h>
#include "driver/gpio.h"

#include "dhtDriver.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
static void readSensor(void);
static int32_t millis(void);
static int64_t micros(void);
float distanceTooHot(float temp, float humidity);
float distanceTooHumid(float temp, float humidity);
float distanceTooCold(float temp, float humidity);
float distanceTooDry(float temp, float humidity);
void gpioCfgInput(void);
void gpioCfgOutput(void);

uint64_t dhtPin;
DHT_MODEL_t dhtModel;
ComfortProfile_t m_comfort;
int32_t lastReadTime;
float temperature;
float humidity;
DHT_ERROR_t error;

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
void dhtDriver_setup(uint8_t pin, DHT_MODEL_t model) {
  dhtPin = pin;
  dhtModel = model;

  gpioCfgOutput();

  if (model == AUTO_DETECT) {
    dhtModel = DHT22;
    readSensor();
    if (error == ERROR_TIMEOUT) {
      dhtModel = DHT11;
      // Warning: in case we auto detect a DHT11, you should wait at least 1000 ms
      // before your first read request. Otherwise you will get a time out error.
    }
  }

  //Set default comfort profile.

  //In computing these constants the following reference was used
  //http://epb.apogee.net/res/refcomf.asp
  //It was simplified as 4 straight lines and added very little skew on
  //the vertical lines (+0.1 on x for C,D)
  //The for points used are(from top left, clock wise)
  //A(30%, 30°C) B(70%, 26.2°C) C(70.1%, 20.55°C) D(30.1%, 22.22°C)
  //On the X axis we have the relative humidity in % and on the Y axis the temperature in °C

  //Too hot line AB
  m_comfort.m_tooHot_m = -0.095;
  m_comfort.m_tooHot_b = 32.85;
  //Too humid line BC
  m_comfort.m_tooHumid_m = -56.5;
  m_comfort.m_tooHumid_b = 3981.2;
  //Too cold line DC
  m_comfort.m_tooCold_m = -0.04175;
  m_comfort.m_tooHCold_b = 23.476675;
  //Too dry line AD
  m_comfort.m_tooDry_m = -77.8;
  m_comfort.m_tooDry_b = 2364;
}

void dhtDriver_resetTimer() {
  lastReadTime = millis() - 3000;
}

float dhtDriver_getHumidity() {
  readSensor();
  if (error == ERROR_TIMEOUT) { // Try a second time to read
    readSensor();
  }
  return humidity;
}

float dhtDriver_getTemperature() {
  readSensor();
  if (error == ERROR_TIMEOUT) { // Try a second time to read
    readSensor();
  }
  return temperature;
}

TempAndHumidity_t dhtDriver_getTempAndHumidity() {
  readSensor();
  if (error == ERROR_TIMEOUT) { // Try a second time to read
    readSensor();
  }
  TempAndHumidity_t thValues;
  thValues.temperature = temperature;
  thValues.humidity = humidity;
  return thValues;
}

DHT_ERROR_t dhtDriver_getStatus() {
  return error;
}

#ifndef OPTIMIZE_SRAM_SIZE

const char* dhtDriver_getStatusString() {
  switch (error) {
  case ERROR_TIMEOUT:
    return "TIMEOUT";

  case ERROR_CHECKSUM:
    return "CHECKSUM";

  default:
    return "OK";
  }
}

#else

/* At the expense of 26 bytes of extra PROGMEM, we save 11 bytes of
   SRAM by using the following method: */

prog_char P_OK[] PROGMEM = "OK";
prog_char P_TIMEOUT[] PROGMEM = "TIMEOUT";
prog_char P_CHECKSUM[] PROGMEM = "CHECKSUM";

const char *dhtDriver_getStatusString()
{
  prog_char *c;
  switch (error)
  {
  case ERROR_CHECKSUM:
    c = P_CHECKSUM;
    break;

  case ERROR_TIMEOUT:
    c = P_TIMEOUT;
    break;

  default:
    c = P_OK;
    break;
  }

  static char buffer[9];
  strcpy_P(buffer, c);

  return buffer;
}

#endif

DHT_MODEL_t dhtDriver_getModel() {
  return dhtModel;
}

int dhtDriver_getMinimumSamplingPeriod() {
  return dhtModel == DHT11 ? 1000 : 2000;
}

int8_t dhtDriver_getNumberOfDecimalsTemperature() {
  return dhtModel == DHT11 ? 0 : 1;
}

int8_t dhtDriver_getLowerBoundTemperature() {
  return dhtModel == DHT11 ? 0 : -40;
}

int8_t dhtDriver_getUpperBoundTemperature() {
  return dhtModel == DHT11 ? 50 : 125;
}

int8_t dhtDriver_getNumberOfDecimalsHumidity() {
  return 0;
}

int8_t dhtDriver_getLowerBoundHumidity() {
  return dhtModel == DHT11 ? 20 : 0;
}

int8_t dhtDriver_getUpperBoundHumidity() {
  return dhtModel == DHT11 ? 90 : 100;
}

float dhtDriver_toFahrenheit(float fromCelcius) {
  return 1.8 * fromCelcius + 32.0;
}

float dhtDriver_toCelsius(float fromFahrenheit) {
  return (fromFahrenheit - 32.0) / 1.8;
}

//boolean isFahrenheit: True == Fahrenheit; False == Celsius
float dhtDriver_computeHeatIndex(float temperature, float percentHumidity,
    bool isFahrenheit) {
  // Using both Rothfusz and Steadman's equations
  // http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
  float hi;

  if (!isFahrenheit) {
    temperature = dhtDriver_toFahrenheit(temperature);
  }

  hi = 0.5
      * (temperature + 61.0 + ((temperature - 68.0) * 1.2)
          + (percentHumidity * 0.094));

  if (hi > 79) {
    hi = -42.379 + 2.04901523 * temperature + 10.14333127 * percentHumidity
        + -0.22475541 * temperature * percentHumidity
        + -0.00683783 * pow(temperature, 2)
        + -0.05481717 * pow(percentHumidity, 2)
        + 0.00122874 * pow(temperature, 2) * percentHumidity
        + 0.00085282 * temperature * pow(percentHumidity, 2)
        + -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

    if ((percentHumidity < 13) && (temperature >= 80.0)
        && (temperature <= 112.0))
      hi -= ((13.0 - percentHumidity) * 0.25)
          * sqrt((17.0 - fabs(temperature - 95.0)) * 0.05882);

    else if ((percentHumidity > 85.0) && (temperature >= 80.0)
        && (temperature <= 87.0))
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
  }

  return isFahrenheit ? hi : dhtDriver_toCelsius(hi);
}

//boolean isFahrenheit: True == Fahrenheit; False == Celsius
float dhtDriver_computeDewPoint(float temperature, float percentHumidity, bool isFahrenheit) {
  // reference: http://wahiduddin.net/calc/density_algorithms.htm
  if (isFahrenheit) {
    temperature = dhtDriver_toCelsius(temperature);
  }
  double calc_temp = 373.15 / (273.15 + (double) temperature);
  double calc_sum = -7.90298 * (calc_temp - 1);
  calc_sum += 5.02808 * log10(calc_temp);
  calc_sum += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / calc_temp))) - 1);
  calc_sum += 8.1328e-3 * (pow(10, (-3.49149 * (calc_temp - 1))) - 1);
  calc_sum += log10(1013.246);
  double calc_value = pow(10, calc_sum - 3) * (double) percentHumidity;
  double calc_dew_temp = log(calc_value / 0.61078); // temp var
  calc_dew_temp = (241.88 * calc_dew_temp) / (17.558 - calc_dew_temp);
  return isFahrenheit ? dhtDriver_toFahrenheit(calc_dew_temp) : calc_dew_temp;
}

//boolean isFahrenheit: True == Fahrenheit; False == Celsius
float dhtDriver_getComfortRatio(ComfortState_t *destComfortStatus,
    float temperature, float percentHumidity, bool isFahrenheit) {
  float ratio = 100; //100%
  float distance = 0;
  float kTempFactor = 3;  //take into account the slope of the lines
  float kHumidFactor = 0.1; //take into account the slope of the lines
  uint8_t tempComfort = 0;

  if (isFahrenheit) {
    temperature = dhtDriver_toCelsius(temperature);
  }

  *destComfortStatus = Comfort_OK;

  distance = distanceTooHot(temperature, percentHumidity);
  if (distance > 0) {
    //update the comfort descriptor
    tempComfort += (uint8_t) Comfort_TooHot;
    //decrease the comfort ratio taking the distance into account
    ratio -= distance * kTempFactor;
  }

  distance = distanceTooHumid(temperature, percentHumidity);
  if (distance > 0) {
    //update the comfort descriptor
    tempComfort += (uint8_t) Comfort_TooHumid;
    //decrease the comfort ratio taking the distance into account
    ratio -= distance * kHumidFactor;
  }

  distance = distanceTooCold(temperature, percentHumidity);
  if (distance > 0) {
    //update the comfort descriptor
    tempComfort += (uint8_t) Comfort_TooCold;
    //decrease the comfort ratio taking the distance into account
    ratio -= distance * kTempFactor;
  }

  distance = distanceTooDry(temperature, percentHumidity);
  if (distance > 0) {
    //update the comfort descriptor
    tempComfort += (uint8_t) Comfort_TooDry;
    //decrease the comfort ratio taking the distance into account
    ratio -= distance * kHumidFactor;
  }

  *destComfortStatus = (ComfortState_t) tempComfort;

  if (ratio < 0)
    ratio = 0;

  return ratio;
}

ComfortProfile_t dhtDriver_getComfortProfile() {
  return m_comfort;
}

void dhtDriver_setComfortProfile(ComfortProfile_t c) {
  m_comfort = c;
}

bool dhtDriver_isTooHot(float temp, float humidity) {
  return (temp > (humidity * m_comfort.m_tooHot_m + m_comfort.m_tooHot_b));
}

bool dhtDriver_isTooHumid(float temp, float humidity) {
  return (temp > (humidity * m_comfort.m_tooHumid_m + m_comfort.m_tooHumid_b));
}

inline bool dhtDriver_isTooCold(float temp, float humidity) {
  return (temp < (humidity * m_comfort.m_tooCold_m + m_comfort.m_tooHCold_b));
}

inline bool dhtDriver_isTooDry(float temp, float humidity) {
  return (temp < (humidity * m_comfort.m_tooDry_m + m_comfort.m_tooDry_b));
}

//boolean isFahrenheit: True == Fahrenheit; False == Celsius
uint8_t dhtDriver_computePerception(float temperature,
    float percentHumidity, bool isFahrenheit) {
  // Computing human perception from dew point
  // reference: https://en.wikipedia.org/wiki/Dew_point ==> Relationship to human comfort
  // reference: Horstmeyer, Steve (2006-08-15). "Relative Humidity....Relative to What? The Dew Point Temperature...a better approach". Steve Horstmeyer, Meteorologist, WKRC TV, Cincinnati, Ohio, USA. Retrieved 2009-08-20.
  // Using table
  // Return value Dew point    Human perception[6]
  //    7         Over 26 °C   Severely high, even deadly for asthma related illnesses
  //    6         24–26 °C     Extremely uncomfortable, oppressive
  //    5         21–24 °C     Very humid, quite uncomfortable
  //    4         18–21 °C     Somewhat uncomfortable for most people at upper edge
  //    3         16–18 °C     OK for most, but all perceive the humidity at upper edge
  //    2         13–16 °C     Comfortable
  //    1         10–12 °C     Very comfortable
  //    0         Under 10 °C  A bit dry for some

  if (isFahrenheit) {
    temperature = dhtDriver_toCelsius(temperature);
  }

  float dewPoint = dhtDriver_computeDewPoint(temperature, percentHumidity, false);

  if (dewPoint < 10.0f) {
    return (uint8_t)Perception_Dry;
  } else if (dewPoint < 13.0f) {
    return (uint8_t)Perception_VeryComfy;
  } else if (dewPoint < 16.0f) {
    return (uint8_t)Perception_Comfy;
  } else if (dewPoint < 18.0f) {
    return (uint8_t)Perception_Ok;
  } else if (dewPoint < 21.0f) {
    return (uint8_t)Perception_UnComfy;
  } else if (dewPoint < 24.0f) {
    return (uint8_t)Perception_QuiteUnComfy;
  } else if (dewPoint < 26.0f) {
    return (uint8_t)Perception_VeryUnComfy;
  }
  // else dew >= 26.0
  return (uint8_t)Perception_SevereUncomfy;
}

float dhtDriver_computeAbsoluteHumidity(float temperature,
    float percentHumidity, bool isFahrenheit) {
  // Calculate the absolute humidity in g/m³
  // https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
  if (isFahrenheit) {
    temperature = dhtDriver_toCelsius(temperature);
  }

  float absHumidity;
  float absTemperature;
  absTemperature = temperature + 273.15;

  absHumidity = 6.112;
  absHumidity *= exp((17.67 * temperature) / (243.5 + temperature));
  absHumidity *= percentHumidity;
  absHumidity *= 2.1674;
  absHumidity /= absTemperature;

  return absHumidity;
}

uint8_t dhtDriver_getPin() {
  return dhtPin;
}

/************************************
 * STATIC FUNCTIONS
 ************************************/
static void readSensor(void) {
  // Make sure we don't poll the sensor too often
  // - Max sample rate DHT11 is 1 Hz   (duty cycle 1000 ms)
  // - Max sample rate DHT22 is 0.5 Hz (duty cycle 2000 ms)
  unsigned long startTime = millis();
  if ((unsigned long) (startTime - lastReadTime)
      < (dhtModel == DHT11 ? 999L : 1999L)) {
    return;
  }
  lastReadTime = startTime;

  temperature = NAN;
  humidity = NAN;

  uint16_t rawHumidity = 0;
  uint16_t rawTemperature = 0;
  uint16_t data = 0;

  // Request sample
  gpioCfgOutput();
  gpio_set_level(dhtPin, LOW);  // Send start signal

  if (dhtModel == DHT11) {
    ets_delay_us(18 * 1000);     //Stalls execution for #us
  } else {
    // This will fail for a DHT11 - that's how we can detect such a device
    ets_delay_us(2 * 1000);     //Stalls execution for #us
  }

  gpio_set_level(dhtPin, HIGH);  // Switch bus to receive data
  gpioCfgInput();

  // We're going to read 83 edges:
  // - First a FALLING, RISING, and FALLING edge for the start bit
  // - Then 40 bits: RISING and then a FALLING edge per bit
  // To keep our code simple, we accept any HIGH or LOW reading if it's max 85 us long
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&mux);
  for (int8_t i = -3; i < 2 * 40; i++) {
    uint8_t age;
    startTime = micros();

    do {
      age = (unsigned long) (micros() - startTime);
      if (age > 90) {
        error = ERROR_TIMEOUT;
        portEXIT_CRITICAL(&mux);
        return;
      }
    } while (gpio_get_level(dhtPin) == (i & 1) ? HIGH : LOW);

    if (i >= 0 && (i & 1)) {
      // Now we are being fed our 40 bits
      data <<= 1;

      // A zero max 30 us, a one at least 68 us.
      if (age > 30) {
        data |= 1; // we got a one
      }
    }

    switch (i) {
    case 31:
      rawHumidity = data;
      break;
    case 63:
      rawTemperature = data;
      data = 0;
      break;
    }
  }

  portEXIT_CRITICAL(&mux);

  // Verify checksum

  if ((uint8_t)(
      ((uint8_t) rawHumidity) + (rawHumidity >> 8) + ((uint8_t) rawTemperature)
          + (rawTemperature >> 8)) != data) {
    error = ERROR_CHECKSUM;
    return;
  }

  // Store readings

  if (dhtModel == DHT11) {
    humidity = (rawHumidity >> 8) + ((rawHumidity & 0x00FF) * 0.1);
    temperature = (rawTemperature >> 8) + ((rawTemperature & 0x007F) * 0.1);
    if (rawTemperature & 0x0080) {
      temperature = -temperature;
    }
  } else {
    humidity = rawHumidity * 0.1;

    if (rawTemperature & 0x8000) {
      rawTemperature = -(int16_t)(rawTemperature & 0x7FFF);
    }
    temperature = ((int16_t) rawTemperature) * 0.1;
  }

  error = ERROR_NONE;
}

static int32_t millis(void) {
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  int64_t time_us = (int64_t) tv_now.tv_sec * 1000000L + (int64_t) tv_now.tv_usec;
  return (int32_t) time_us / 1000;
}

static int64_t micros(void) {
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  int64_t time_us = (int64_t) tv_now.tv_sec * 1000000L + (int64_t) tv_now.tv_usec;
  return time_us;
}

float distanceTooHot(float temp, float humidity) {
  return temp - (humidity * m_comfort.m_tooHot_m + m_comfort.m_tooHot_b);
}

float distanceTooHumid(float temp, float humidity) {
  return temp - (humidity * m_comfort.m_tooHumid_m + m_comfort.m_tooHumid_b);
}

float distanceTooCold(float temp, float humidity) {
  return (humidity * m_comfort.m_tooCold_m + m_comfort.m_tooHCold_b) - temp;
}

float distanceTooDry(float temp, float humidity) {
  return (humidity * m_comfort.m_tooDry_m + m_comfort.m_tooDry_b) - temp;
}

void gpioCfgInput(void)
{
  //zero-initialize the config structure.
  gpio_config_t io_conf = { };
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //bit mask of the pins
  io_conf.pin_bit_mask = (1ULL<<dhtPin);
  //set as input mode
  io_conf.mode = GPIO_MODE_INPUT;
  //enable pull-up mode
  io_conf.pull_up_en = 1;
  //configure GPIO with the given settings
  gpio_config(&io_conf);
}

void gpioCfgOutput(void)
{
  //zero-initialize the config structure.
  gpio_config_t io_conf = { };
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins
  io_conf.pin_bit_mask = (1ULL<<dhtPin);
  //disable pull-down mode
  io_conf.pull_down_en = 0;
  //disable pull-up mode
  io_conf.pull_up_en = 0;
  //configure GPIO with the given settings
  gpio_config(&io_conf);
}

