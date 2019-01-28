#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <ArduinoJson.h>

#define demultiplier 6.15
int8_t counter, ads, input_pin;
float measurements[8];
int16_t val_min[8], val_max[8], current_val;

Adafruit_ADS1115 ads1(0x48);
Adafruit_ADS1115 ads2(0x49);

ESP8266WebServer server(80);

void setup(void)
{
  Serial.begin(115200);
  WiFi.hostname("powermeter");
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.autoConnect("powermeter");
  Serial.println();
  Serial.println();
  Serial.print("Connecting...");

  if (!wifiManager.autoConnect()) {
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  ArduinoOTA.begin();
  ArduinoOTA.setHostname("powermeter");

  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV


  ads1.setGain(GAIN_FOUR);
  ads2.setGain(GAIN_FOUR);

  ads1.begin();
  ads2.begin();
  server.on("/", []() {
    DynamicJsonBuffer newBuffer;
    JsonObject& root = newBuffer.createObject();
    for (int8_t input = 0; input < 8; input++) {
      if (measurements[input] < 1) {
        root["in" + (String)(input + 1)] = 0;
      } else {
        root["in" + (String)(input + 1)] = (int)measurements[input];
      }
    }
    String output;
    root.printTo(output);
    server.send(200, "text/plain", output);
  });

  server.on("/reset", []() {
    server.send(200, "text/plain", "reset");
    ESP.reset();
  });
  server.begin();
}



void loop(void)
{
  ArduinoOTA.handle();
  server.handleClient();
  
  ///begin sensors read
  if (counter == 100) { // 100 samples per sensor
     counter = 0;
     input_pin++;
     if (input_pin == 4){
        input_pin = 0;
        ads++;
        if (ads == 2) {
          ads = 0;
        }
     }
  }
  int8_t input = ads * 4 + input_pin;
      if (ads == 0) {
      current_val = ads1.readADC_SingleEnded(input_pin);
    } else {
      current_val = ads2.readADC_SingleEnded(input_pin);
    }

    if (counter == 0) {
      val_max[input] = current_val;
      val_min[input] = current_val;
    }
    if (current_val > val_max[input]) {
      val_max[input] = current_val;
    } else if (current_val < val_min[input]) {
      val_min[input] = current_val;
    }
     measurements[input] = (val_max[input] - val_min[input]) / demultiplier;
     
    counter++;
}
