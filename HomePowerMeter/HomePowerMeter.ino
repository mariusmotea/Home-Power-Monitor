#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <FS.h>
#include <Adafruit_ADS1015.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define demultiplier 6.15
#define samples 500
int8_t ads, input_pin, current_input;
int counter;
float measurements[8];
int16_t val_min[8], val_max[8], current_val;

#define INFLUXDB_URL "https://eu-central-1-1.aws.cloud2.influxdata.com"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "ayIsLDXI63f8bPfVqVJyOV5P6exxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxfbNWXJJxjA3OTHVS4YbuvHow=="
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "marius.motea@example.com"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "marius.motea's Bucket"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

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




void InfluxDB_Push (int8_t input) {
  //Data point
  Point sensor("ADS1115");
  sensor.addTag("device", "Power");
  sensor.addField("fuse" + String(input + 1), measurements[input]);
  client.writePoint(sensor);
}

void setup(void)
{
  //
  //Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

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
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setCursor(5, 0);
  display.print(F("Searching for Wi-Fi..."));
  display.display();

  WiFi.mode(WIFI_STA);

  WiFi.hostname("powermeter");
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(40);
  wifiManager.autoConnect("powermeter");

  if (WiFi.status() != WL_CONNECTED) {
    display.print(F("Connection Fail"));
    display.display();
    delay(1000);
    ESP.reset();
  } else {
    display.clearDisplay();
    display.print(F("Connected..."));
  }

  display.display();
  delay(1000);
  display.clearDisplay();

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
  if (counter == samples) { // nr of samples per sensor
    //display_values(input);
    counter = 0;
    input_pin++;
    if (input_pin == 4) {
      input_pin = 0;
      ads++;
      if (ads == 2) {
        ads = 0;
      }
    }
    current_input = ads * 4 + input_pin;
    display_mesurement(current_input);
  }
  if (ads == 0) {
    current_val = ads1.readADC_SingleEnded(input_pin);
  } else {
    current_val = ads2.readADC_SingleEnded(input_pin);
  }

  if (counter == 0) {
    val_max[current_input] = current_val;
    val_min[current_input] = current_val;
  }
  if (current_val > val_max[current_input]) {
    val_max[current_input] = current_val;
  } else if (current_val < val_min[current_input]) {
    val_min[current_input] = current_val;
  }
  if (counter == samples - 1) {
    measurements[current_input] = (val_max[current_input] - val_min[current_input]) / demultiplier;
    display_values(current_input);
    InfluxDB_Push(current_input);
  }

  counter++;
}

void display_mesurement (int8_t input) {
    if (input < 4) {
    display.fillRect(30, input * 10, 30, 10, 0);
    display.setCursor(30, input * 10);
    display.print("read");
  } else {
    display.fillRect(94, (input - 4) * 10, 30, 10, 0);
    display.setCursor(94, (input - 4) * 10);
    display.print("read");
  }
  display.display();
}


void display_values(int8_t input) {
  //display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  int total = 0;
  if (input < 4) {
    display.fillRect(20, input * 10, 44, 10, 0);
    display.setCursor(0, input * 10);
  } else {
    display.fillRect(80, (input - 4) * 10, 44, 10, 0);
    display.setCursor(64, (input - 4) * 10);
  }
  if (measurements[input] < 1) {
    display.println("In" + (String)(input + 1) + ":   0W");
  } else if (measurements[input] < 10) {
    display.println("In" + (String)(input + 1) + ":   " + (String)(int)measurements[input] + "W");
  } else if (measurements[input] < 100) {
    display.println("In" + (String)(input + 1) + ":  " + (String)(int)measurements[input] + "W");
  } else {
    display.println("In" + (String)(input + 1) + ": " + (String)(int)measurements[input] + "W");
  }

  for (int8_t fuse = 0; fuse < 8; fuse++) {
    total += measurements[fuse];
  }
  display.fillRect(64, 55, 64, 11, 0);
  display.setCursor(0, 55);
  if (total < 10) {
    display.println(" TOTAL:           " + (String)(int)total + "W");
  } else if (total < 100) {
    display.println(" TOTAL:          " + (String)(int)total + "W");
  } else {
    display.println(" TOTAL:         " + (String)(int)total + "W");
  }


  display.display();
}
