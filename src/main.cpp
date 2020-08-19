/***************************************************************************
  This is a script of code I wrote that mostly uses the bme680 library example
  from arduino with some wifi enabling and Losant IoT connectivity.

  It reads temp, and posts every 5 seconds to the Losant platform to easily
  visualize the data sent.

  I would say copyright me, but there's no way I can take credit for any of this

 ***************************************************************************/

// TODO: figure out why it stops sending data to losant after X time
// * probably a good idea to find out how long
// * getting keepalive timeout from losant -- might be because of packet size?

// * don't forget to install arduino json, it's necessary for `JsonObject`

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <WiFiClientSecure.h>
#include <Losant.h>

//wifi credentials
const char *WIFI_SSID = "";
const char *WIFI_PASS = "";

//losant credentials
const char *LOSANT_DEVICE_ID = "";
const char *LOSANT_ACCESS_KEY = "";
const char *LOSANT_ACCESS_SECRET = "";

WiFiClientSecure wifiClient;

LosantDevice device(LOSANT_DEVICE_ID);

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C
//Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println();
  Serial.print("Connecting to Losant...");

  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  while (!device.connected()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected!");
  Serial.println("This device is now ready for use!");
} 

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;
  Serial.println(F("BME680 test"));

  if (!bme.begin())
  {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1)
      ;
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  connect();
}
/**
 * @ reportTemp
 * optionally add double bmeTemp and uncomment root["tempC"] = bmeTemp in loop
 * optionally add double pressureInches and uncomment root["pressureInches"] in loop
 */
void reportTemp(double bmeTemp, double bmePress, double bmeHumidity)
{
  StaticJsonDocument<200> root;
  // JsonObject &root = jsonBuffer.createObject();
  // DynamicJsonDocument root(1024);
  root["temperature"] = bmeTemp;
  root["humidity"] = bmeHumidity;
  root["pressure"] = bmePress;
  JsonObject object = root.as<JsonObject>();
  device.sendState(object);
}

unsigned long check_wifi = 30000;

void loop()
{
  // make sure wifi is connected
  //* Maybe this is the issue? could check_wifi be getting too big?
  if ((WiFi.status() != WL_CONNECTED) && (millis() > check_wifi))
  {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    connect();
    check_wifi = millis() + 30000;
  }

  if (!bme.performReading())
  {
    Serial.println("Failed to perform reading :(");
    return;
  }
  double bmeTemp = bme.temperature;
  double bmePress = bme.pressure / 100.0;
  double bmeHumidity = bme.humidity;

  reportTemp(bmeTemp, bmePress, bmeHumidity);

  //* report temp && check wifi connectivity every 5 seconds
  delay(5000);
}
