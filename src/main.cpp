
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <Losant.h>
#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_VL6180X.h>

//States KLOALA

// WiFi credentials.
const char* WIFI_SSID = "Hofmayer_Keller";
const char* WIFI_PASS = "J0p@ul21!";

// Losant credentials.
const char* LOSANT_DEVICE_ID = "5fc6b0c1d428370007e12b2f";
const char* LOSANT_ACCESS_KEY = "d7cbb9f2-3c05-42d1-be0f-a007740e6346";
const char* LOSANT_ACCESS_SECRET = "744103ac1613c555e0b8bc25407d1d18e3478f2d44a386ef0ee49134b7fa4e42";

// LEDs credeentials.

#define LED_PIN     13
#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define NUM_LEDS    2
#define BRIGHTNESS  200
#define FRAMES_PER_SECOND 60



// Cert taken from 
// https://github.com/Losant/losant-mqtt-ruby/blob/master/lib/losant_mqtt/RootCA.crt
static const char digicert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

BearSSL::WiFiClientSecure wifiClient;

LosantDevice device(LOSANT_DEVICE_ID);


CRGB leds[NUM_LEDS];
void initLed(){
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
}
void setColour(CRGB colour ){
  leds[0] = colour;
  leds[1] = colour;
  FastLED.show();
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
Adafruit_VL6180X disatanceSensor = Adafruit_VL6180X();
int refZeroDistance = 100;  //Von Losant gesendet nach PowerUp
int refFullDistance = 50; // Von Losant gesendet nach PowerUp
int aktDistance = 75;
int aktProzent = 0;

void initDistanceSensor(){
  disatanceSensor.begin();
}

int getRefZero(){
  //...
  return 0;
}

int getRefFull(){
  //...
  return 0;
}

void setRefZero(int newZero){
    StaticJsonDocument<200> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();
  root["RefZero"] = newZero;
  // Send the state to Losant.
  device.sendState(root);
}

void setRefFull(int newFull){
    StaticJsonDocument<200> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();
  root["RefFull"] = newFull;
  // Send the state to Losant.
  device.sendState(root);
}

int distance2percent(int distanceMM){
  int distancePercent;
  distancePercent= ((distanceMM - refZeroDistance) * 100) / (refFullDistance - refZeroDistance); //umrechnung in Prozent
  return distancePercent;
}
//------------------------------------------------------------------------
//------------------------------------------------------------------------

// Set time via NTP, as required for x.509 validation
void setClock() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    now = time(nullptr);
  }
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
}

// Called whenever the device receives a command from the Losant platform.
void handleCommand(LosantCommand *command) {
  Serial.print("Command received: ");
  Serial.println(command->name);
  // Optional command payload. May not be present on all commands.
  //JsonObject payload = *command->payload;
  
  // Perform action specific to the command received.
  if(strcmp(command->name, "toggle") == 0) {
    /**
    * In Losant, including a payload along with your
    * command is optional. This is an example of how
    * to parse a JSON payload from Losant and print
    * the value of a key called "temperature".
    */
    // int temperature = payload["temperature"];
    // Serial.println(temperature);
  }
  else if(strcmp(command->name, "SerialPrint") == 0){
    Serial.println("Hey ein Losant befehl");
  }
  else if(strcmp(command->name, "LedColourRed") == 0){
    setColour(CRGB::Red);
  }
  else if(strcmp(command->name, "LedColourBlue") == 0){
    setColour(CRGB::Blue);
  }
  else if(strcmp(command->name, "LedColourGreen") == 0){
    setColour(CRGB::Green);
  }
}

void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Validating  the SSL for for a secure connection you must
  // set trust anchor, as well as set the time.
  BearSSL::X509List cert(digicert);
  wifiClient.setTrustAnchors(&cert);
  setClock();
  

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to Losant.
  Serial.println();
  Serial.print("Connecting to Losant...");

  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  while(!device.connected()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected!");
}

void commands() {
  Serial.println("Sending Command!");
  bool isIdel = true;
  // Losant uses a JSON protocol. Construct the simple state object.
  // { "button" : true }
  StaticJsonDocument<200> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();
  root["aktProzent"] = 99;
  root["isIdel"] = isIdel;

  // Send the state to Losant.
  device.sendState(root);
}

void sendAktprozent(int aktProzent){
  StaticJsonDocument<200> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();
  root["aktProzent"] = aktProzent;
  device.sendState(root);
  Serial.println(aktProzent);
}




void setup() {
  Serial.begin(115200);
  //while(!Serial) { }


  // Register the command handler to be called when a command is received
  // from the Losant platform.
  device.onCommand(&handleCommand);

//
  
  initLed();
  setColour(CRGB::Blue);

  

  
  connect();

  initDistanceSensor();

  aktProzent = distance2percent(aktDistance);
  Serial.println(aktProzent);

  commands();
  sendAktprozent(69);
}


int buttonState = 0;

void loop() {

  bool toReconnect = false;

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if(!device.connected()) {
    Serial.println("Disconnected from Losant");
    toReconnect = true;
  }

  if(toReconnect) {
    connect();
  }

  device.loop();
}