/*
 * esp32TempSnsr.ino
 *
 *  Created on: 21 Apr 2024
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

/************************************
 * INCLUDES
 ************************************/
#include <WiFi.h>
#include <DHTesp.h> /** Click here to get the library: http://librarymanager/All#DHTesp */
#include <MQTTClient.h>  /** Click here to get the library: http://librarymanager/All#MQTT */
#include <ArduinoJson.h> /** Click here to get the library: http://librarymanager/All#ArduinoJson */
#include <Preferences.h> /** Click here to get the library: http://librarymanager/All#Preferences */
#include "config.h"
#include "deepSleep.h"
#if (ESP_ARDUINO_VERSION_MAJOR >= 3)
  #include "esp32-hal-periman.h"
#endif /* ESP_ARDUINO_VERSION_MAJOR >= 3 */

/************************************
* Check configurations
 ************************************/
#ifndef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

#ifndef __CONFIG_H_
#pragma message(FILE CONFIG.H NOT ADDED TO THE PROJECT)
#error The file config.h needs to be added.
#endif

/************************************
 * EXTERN VARIABLES
 ************************************/
extern EspClass ESP;

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define UNCONFIGURED            "UNCONFIGURED\0"
#define WIFI_SSID_PSW_MAX_LEN   64
#define APP_DEVICE_NAME_MAX_LEN 64
#define SLEEP_TIME              ((uint16_t) 30)      /* Time ESP32 will go to sleep (in seconds) */
#define INIT_STS_OK             true
#define INIT_STS_NOK            false

/************************************
 * PRIVATE TYPEDEFS
 ************************************/
typedef struct {
  wifi_mode_t wifiMode;
  char wifiSsid[WIFI_SSID_PSW_MAX_LEN];
  char wifiPassword[WIFI_SSID_PSW_MAX_LEN];
  bool configInit;
} wifiConfig_t;

typedef struct {
  uint16_t sleepTime;
} deepSleepConfig_t;

typedef struct {
  char deviceName[APP_DEVICE_NAME_MAX_LEN];
  bool configInit;
} appConfig_t;

typedef struct {
  wifiConfig_t wifiCfg;
  deepSleepConfig_t deepSleepCfg;
  appConfig_t appConfig;
} localConfig_t;

/************************************
 * STATIC VARIABLES
 ************************************/
static uint32_t chipId = 0;
static DHTesp dht;
static ComfortState cf;
static bool connectionUp = true;
static WiFiClient network;
static MQTTClient mqtt = MQTTClient(256);
static Preferences configuration;
static localConfig_t localCfg;

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
bool getTemperature();
void getChipId();
void connectToMQTT();
void sendToMQTT();
void messageHandler(String &topic, String &payload);
void startWiFiConfig(void);
void readWiFiConfig(void);
void startDeviceConfig(void);
void readDeviceConfig(void);
void saveSettings(void);

/************************************
 * MAIN FUNCTION
 ************************************/
void setup()
{
  /* Initialize serial */
  Serial.begin(115200);
  delay(1000); /* wait 1s to let Serial monitor initialize */
  getChipId();
  configuration.begin("esp32TempSensor");

  log_i("Checking wifi configuration");
  log_v("If the WiFi is not configured ask for the configuration through serial communication");
  strcpy(localCfg.wifiCfg.wifiSsid, UNCONFIGURED);
  strcpy(localCfg.wifiCfg.wifiPassword, UNCONFIGURED);
  localCfg.wifiCfg.wifiMode = (wifi_mode_t)configuration.getUChar(KEY_WIFIMODE, (uint8_t)WIFI_STA);
  configuration.getString(KEY_WIFIID, localCfg.wifiCfg.wifiSsid, WIFI_SSID_PSW_MAX_LEN);
  configuration.getString(KEY_WIFIPSW, localCfg.wifiCfg.wifiPassword, WIFI_SSID_PSW_MAX_LEN);
  localCfg.wifiCfg.configInit = INIT_STS_OK;

  if ((strcmp(localCfg.wifiCfg.wifiSsid, UNCONFIGURED) == 0) || (strcmp(localCfg.wifiCfg.wifiPassword, UNCONFIGURED) == 0)) {
    log_w("Wifi not configured");
    localCfg.wifiCfg.configInit = INIT_STS_NOK;
  }

  log_i("Checking deep sleep configuration");
  localCfg.deepSleepCfg.sleepTime = configuration.getUShort(KEY_SLEEPTIME, 0);

  if (localCfg.deepSleepCfg.sleepTime == 0u) {
    log_w("Invalid deep sleep time configuration: %d - set to default (%d)", localCfg.deepSleepCfg.sleepTime, SLEEP_TIME);
    localCfg.deepSleepCfg.sleepTime = SLEEP_TIME;
  }

  strcpy(localCfg.appConfig.deviceName, UNCONFIGURED);
  configuration.getString(KEY_DEVICENAME, localCfg.appConfig.deviceName, APP_DEVICE_NAME_MAX_LEN);
  localCfg.appConfig.configInit = INIT_STS_OK;
  log_i("Checking device condifuration");
  if ((strcmp(localCfg.appConfig.deviceName, UNCONFIGURED) == 0)) {
    log_w("Device name not configured");
    localCfg.appConfig.configInit = INIT_STS_NOK;
  }

  if (INIT_STS_NOK == localCfg.wifiCfg.configInit) {
    startWiFiConfig();
  }

  if (INIT_STS_NOK == localCfg.appConfig.configInit) {
    startDeviceConfig();
  }

  WiFi.mode(localCfg.wifiCfg.wifiMode);
  WiFi.begin(localCfg.wifiCfg.wifiSsid, localCfg.wifiCfg.wifiPassword);

  log_i("ESP32 - Connecting to Wi-Fi: %s", localCfg.wifiCfg.wifiSsid);

  uint8_t tentatives = ATTEMPING_CONNECTION;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    tentatives--;

    if (tentatives == 0u)
    {
      log_e("Unable to connect to %s", localCfg.wifiCfg.wifiSsid);
      connectionUp = false;
      break;
    }
  }

  if (connectionUp)
  {
    connectToMQTT();
  }

  log_i("Boot number: %d", deepSleep_getbootCount());

  /* Initialize temperature sensor */
#if (ESP_ARDUINO_VERSION_MAJOR >= 3)
  if (!perimanSetPinBus(DHT_PIN, ESP32_BUS_TYPE_GPIO, (void *)(DHT_PIN + 1), -1, -1)) {
    log_e("Failed to initialize pin %d", DHT_PIN);
  }
#endif /* ESP_ARDUINO_VERSION_MAJOR >= 3 */

  dht.setup(DHT_PIN, DHTesp::DHT11);
  log_i("DHT initiated");

  /* Configure the wake up source */
  log_i("Setup ESP32 to sleep for %d Seconds", localCfg.deepSleepCfg.sleepTime);
  deepSleep_Init(localCfg.deepSleepCfg.sleepTime);
}

void loop()
{
  getTemperature();
  log_i("Save settings");
  saveSettings();
  log_i("Going to sleep");
  Serial.flush();
  deepSleep_goToSleep();
}

/************************************
 * STATIC FUNCTIONS
 ************************************/

void startWiFiConfig(void)
{
  Serial.println("***************************");
  Serial.println("Starting wifi configuration");
  Serial.println("");
  Serial.println("Insert settings in this format:");
  Serial.println("SSID: <Your WiFi name>");
  Serial.println("PSW: <Your password>");

  Serial.onReceive(readWiFiConfig);
  while (!localCfg.wifiCfg.configInit) {
    delay(500);
  }
  Serial.onReceive(NULL);
}

void readWiFiConfig(void)
{
  String serialRXString;
  size_t available = Serial.available();
  while (available--) {
    serialRXString += (char)Serial.read();
  }
  Serial.print("Received: ");
  Serial.println(serialRXString);
  if (serialRXString.startsWith("SSID: ")) {
    serialRXString.remove(0, (sizeof("SSID: ") - 1));
    serialRXString.toCharArray(localCfg.wifiCfg.wifiSsid, WIFI_SSID_PSW_MAX_LEN);
  } else if (serialRXString.startsWith("PSW: ")) {
    serialRXString.remove(0, (sizeof("PSW: ") - 1));
    serialRXString.toCharArray(localCfg.wifiCfg.wifiPassword, WIFI_SSID_PSW_MAX_LEN);
  } else {
    Serial.println("Wrong WiFi config input");
  }
  if ((strcmp(localCfg.wifiCfg.wifiSsid, UNCONFIGURED) != 0) && (strcmp(localCfg.wifiCfg.wifiPassword, UNCONFIGURED) != 0)) {
    log_i("Wifi configured");
    localCfg.wifiCfg.configInit = INIT_STS_OK;
  }
}

void startDeviceConfig(void)
{
  Serial.println("***************************");
  Serial.println("Starting device configuration");
  Serial.println("");
  Serial.println("Insert settings in this format:");
  Serial.println("NAME: <Your device name>");

  Serial.onReceive(readDeviceConfig);
  while (!localCfg.appConfig.configInit) {
    delay(500);
  }
  Serial.onReceive(NULL);
}

void readDeviceConfig(void)
{
  String serialRXString;
  size_t available = Serial.available();
  while (available--) {
    serialRXString += (char)Serial.read();
  }
  Serial.print("Received: ");
  Serial.println(serialRXString);
  if (serialRXString.startsWith("NAME: ")) {
    serialRXString.remove(0, (sizeof("NAME: ") - 1));
    serialRXString.toCharArray(localCfg.appConfig.deviceName, APP_DEVICE_NAME_MAX_LEN);
  } else {
    Serial.println("Wrong device config input");
  }
  if (strcmp(localCfg.appConfig.deviceName, UNCONFIGURED) != 0) {
    log_i("Device configured");
    localCfg.appConfig.configInit = INIT_STS_OK;
  }
}

/*TEST*/
struct TEST
{
  float temperature;
  float humidity;
  float heatIndex;
  float dewPoint;
  String comfort;
};
struct TEST TESTT;

/**
 * getTemperature
 * Reads temperature from DHT11 sensor
 * @return bool
 *    true if temperature could be aquired
 *    false if aquisition failed
 */
bool getTemperature()
{
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
  // Check if any reads failed and exit early (to try again).
  if (dht.getStatus() != 0)
  {
    Serial.println("DHT11 error status: " + String(dht.getStatusString()));
    return false;
  }

  float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  String comfortStatus;
  switch (cf)
  {
  case Comfort_OK:
    comfortStatus = "Comfort_OK";
    break;
  case Comfort_TooHot:
    comfortStatus = "Comfort_TooHot";
    break;
  case Comfort_TooCold:
    comfortStatus = "Comfort_TooCold";
    break;
  case Comfort_TooDry:
    comfortStatus = "Comfort_TooDry";
    break;
  case Comfort_TooHumid:
    comfortStatus = "Comfort_TooHumid";
    break;
  case Comfort_HotAndHumid:
    comfortStatus = "Comfort_HotAndHumid";
    break;
  case Comfort_HotAndDry:
    comfortStatus = "Comfort_HotAndDry";
    break;
  case Comfort_ColdAndHumid:
    comfortStatus = "Comfort_ColdAndHumid";
    break;
  case Comfort_ColdAndDry:
    comfortStatus = "Comfort_ColdAndDry";
    break;
  default:
    comfortStatus = "Unknown:";
    break;
  };

  Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
  TESTT.temperature = newValues.temperature;
  TESTT.humidity = newValues.humidity;
  TESTT.heatIndex = heatIndex;
  TESTT.dewPoint = dewPoint;
  TESTT.comfort = comfortStatus;

  if (connectionUp)
  {
    sendToMQTT();
  }

  return true;
}

void getChipId()
{
  log_v("Show chip ID");
  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  log_v("ESP32 Chip model = %s Rev %d", ESP.getChipModel(), ESP.getChipRevision());
  log_v("This chip has %d cores", ESP.getChipCores());
  log_v("Chip ID: %d", chipId);
}

void connectToMQTT()
{
  // Connect to the MQTT broker
  mqtt.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, network);

  // Create a handler for incoming messages
  mqtt.onMessage(messageHandler);

  Serial.print("ESP32 - Connecting to MQTT broker");

  uint8_t tentatives = ATTEMPING_CONNECTION;

  while (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
  {
    delay(100);
    tentatives--;

    if(0 == tentatives) {
      log_e("Unable to connect to %s", MQTT_CLIENT_ID);
    }
  }

  if (!mqtt.connected())
  {
    log_e("ESP32 - MQTT broker Timeout!");
    return;
  }

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqtt.subscribe(SUBSCRIBE_TOPIC))
    Serial.print("ESP32 - Subscribed to the topic: ");
  else
    Serial.print("ESP32 - Failed to subscribe to the topic: ");

  Serial.println(SUBSCRIBE_TOPIC);
  Serial.println("ESP32 - MQTT broker Connected!");
}

void sendToMQTT()
{
  JsonDocument message;
  message["timestamp"] = millis();
  message["Temperature"] = TESTT.temperature;
  message["humidity"] = TESTT.humidity;
  message["heatIndex"] = TESTT.heatIndex;
  message["dewPoint"] = TESTT.dewPoint;
  message["comfort"] = TESTT.comfort;
  char messageBuffer[1024];
  serializeJson(message, messageBuffer);

  mqtt.publish(PUBLISH_TOPIC, messageBuffer);

  Serial.println("ESP32 - sent to MQTT:");
  Serial.print("- topic: ");
  Serial.println(PUBLISH_TOPIC);
  Serial.print("- payload:");
  Serial.println(messageBuffer);
}

void messageHandler(String &topic, String &payload)
{
  Serial.println("ESP32 - received from MQTT:");
  Serial.println("- topic: " + topic);
  Serial.println("- payload:");
  Serial.println(payload);
}

void saveSettings(void)
{
  /* Wifi settings */
  configuration.putUChar(KEY_WIFIMODE, (uint8_t)localCfg.wifiCfg.wifiMode);
  configuration.putString(KEY_WIFIID, localCfg.wifiCfg.wifiSsid);
  configuration.putString(KEY_WIFIPSW, localCfg.wifiCfg.wifiPassword);

  /* Deep sleep settings */
  configuration.putUShort(KEY_SLEEPTIME, localCfg.deepSleepCfg.sleepTime);

  /* Device settings */
  configuration.putString(KEY_DEVICENAME, localCfg.appConfig.deviceName);

}
