#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <FS.h>
#include <Adafruit_ADS1015.h>
#include <ArduinoJson.h>

//#define demultiplier 6.15
#define demultiplier 2.43
int8_t counter, ads, input_pin;
float measurements[8];
int16_t val_min[8], val_max[8], current_val;

File fsUploadFile;

Adafruit_ADS1115 ads1(0x48);
Adafruit_ADS1115 ads2(0x49);

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdateServer;

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  //Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    //Serial.println(String("\tSent file: ") + path);
    return true;
  }
  //Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    //Serial.print("handleFileUpload Name: "); //Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      //Serial.print("handleFileUpload Size: "); //Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void setup(void)
{
  //Serial.begin(115200);
  
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      //Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
    }
    //Serial.printf("\n");
  }

  WiFi.hostname("powermeter");
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.autoConnect("powermeter");

  //Serial.println();
  //Serial.println();
  //Serial.print("Connecting...");
  if (!wifiManager.autoConnect()) {
      delay(3000);
      ESP.reset();
      delay(3000);
  }

  httpUpdateServer.setup(&server);

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
  server.on("/json", []() {
    DynamicJsonDocument root(1024);
    for (int8_t input = 0; input < 8; input++) {
      if (measurements[input] < 1) {
        root["in" + (String)(input + 1)] = 0;
      } else {
        root["in" + (String)(input + 1)] = (int)measurements[input];
      }
    }
    String output;
    serializeJson(root, output);
    server.send(200, "text/plain", output);
  });

  server.on("/reset", []() {
    server.send(200, "text/plain", "reset");
    ESP.reset();
  });

  server.on("/file-upload", HTTP_POST,                       // if the client posts to the upload page
  []() {
    server.send(200);
  },                          // Send status 200 (OK) to tell the client we are ready to receive
  handleFileUpload                                    // Receive and save the file
           );

  server.on("/file-upload", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/", []() {
    server.sendHeader("Location", String("/index.html"), true);
    server.send ( 302, "text/plain", "");
  });

  server.serveStatic("/", SPIFFS, "/", "max-age=86400");

  server.begin();
}



void loop(void)
{
  server.handleClient();

  ///begin sensors read
  if (counter == 100) { // 100 samples per sensor
    counter = 0;
    input_pin++;
    if (input_pin == 4) {
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
