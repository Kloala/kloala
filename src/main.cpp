
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <Losant.h>
#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_VL6180X.h>

//Time credentials
unsigned long aktTime = 0;


// WiFi credentials.
const char* WIFI_SSID = "Hofmayer_Keller";
const char* WIFI_PASS = "J0p@ul21!";

// Losant credentials.
const char* LOSANT_DEVICE_ID = "5fdb7d2e2aa0d500073c1599";
const char* LOSANT_ACCESS_KEY = "0b936153-082c-4ff8-8bf4-3f5bea38d6b5";
const char* LOSANT_ACCESS_SECRET = "fd8536019cde72bdae7c9fcb6faadae8853bd4ee86c6fda2ad31a31f4e378e87";

// LEDs credeentials.

#define LED_PIN     13
#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define NUM_LEDS    2
#define BRIGHTNESS  100
#define FRAMES_PER_SECOND 60

// Sensor cerdiancials.
#define AVERAGE_SMOOTHING 10 

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
  FastLED.clear();
  FastLED.show();
}
void setColour(CRGB colour ){
  leds[0] = colour;
  leds[1] = colour;
  FastLED.show();
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------

Adafruit_VL6180X disatanceSensor = Adafruit_VL6180X();
unsigned int refZeroDistance = 0;  //Von Losant gesendet nach PowerUp
unsigned refFullDistance = 0; // Von Losant gesendet nach PowerUp
unsigned int aktDistance = 0;
int aktProzent = 0;
unsigned long median = 0;
unsigned int counterRunsSensor = 0;

//inits DistanceSensor
void initDistanceSensor(){
  Serial.println("Adafruit VL6180x test!");
  if (! disatanceSensor.begin()) {
    Serial.println("Failed to find sensor");
    while (1){
      setColour(CRGB::Red);
    }
  }
  Serial.println("Sensor found!");
}

//reads Distance from Sensor, add it to the median, increments counterRunsSensor
void readDistance(){ 
  int tempDistance = (int)disatanceSensor.readRange();
  if(disatanceSensor.readRangeStatus() == 0){
    median = median + tempDistance;
    counterRunsSensor ++; 
    /*
    if(counterRunsSensor >=AVERAGE_SMOOTHING){
      aktDistance = median / AVERAGE_SMOOTHING;
      counterRunsSensor = 0;
      median = 0;
    }
    */
  }
}

//sets aktDistance as an Average of the past CounterRunsSensor and resets medain & counterruns
void getNewDistance(){
  aktDistance = median / counterRunsSensor;
      counterRunsSensor = 0;
      median = 0;
}

//Sets Reference distance Zero and sents it to losant
void setRefZero(int newZero){
  refZeroDistance = newZero;
  Serial.println("in functoin value:");
  Serial.println(newZero);
  StaticJsonDocument<200> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();
  root["RefZero"] = newZero;
  // Send the state to Losant.
  device.sendState(root);
}

//Sets Reference distance Full and sents it to losant
void setRefFull(int newFull){
  refFullDistance = newFull;
  StaticJsonDocument<200> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();
  root["RefFull"] = newFull;
  // Send the state to Losant.
  device.sendState(root);
}

//Processes mm into %, retuns percent as an int
int distance2percent(int distanceMM, int refZero, int refFull){
  int distancePercent = 0;
  distancePercent= 100 - ((distanceMM - refFull) * 100)  / (refZero - refFull); //umrechnung in Prozent
  return distancePercent;
}

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
  JsonObject payload = *command->payload;

  if(strcmp(command->name, "setReferenceDistance") == 0) {
    refZeroDistance = payload["refZero"];
    refFullDistance = payload["refFull"];
    Serial.println(refZeroDistance);
    Serial.println(refFullDistance);
    if((refFullDistance == 0) || (refZeroDistance == 0)){
      Serial.println("Error Reciving Data from Losant: Restart or Calibrate manually");
      setColour(CRGB::OrangeRed);
      refFullDistance = -1;
      refZeroDistance = -1;
    }  
  }
  else if(strcmp(command->name, "setRefFull") == 0){
    setRefFull(aktDistance);
  }
  else if(strcmp(command->name, "setRefZero") == 0){
    setRefZero(aktDistance);
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
   else if(strcmp(command->name, "LedColourBlack") == 0){
    setColour(CRGB::Black);
  }
  else if(strcmp(command->name, "LedColourYellow") == 0){
    setColour(CRGB::Yellow);
  }
}

//Connects ESP with Wifi and Losant
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

//sends the given value to Losant as the actPercent Deviceparameter
void sendAktProzent(int aktProzent){
  StaticJsonDocument<200> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();
  root["actPercent"] = aktProzent;
  device.sendState(root);
}

void setup() {
  Serial.begin(115200);

  device.onCommand(&handleCommand);

  initLed();
  initDistanceSensor();

  setColour(CRGB::Blue);
  connect();

  aktTime = millis();

  setColour(CRGB::Yellow);
  Serial.println("Kalibrierung benötigt!");
  while((refFullDistance == 0) || (refZeroDistance == 0)){
    readDistance();
    if((millis()-aktTime) > 250){ //process distance every 250ms
      aktTime = millis();
      Serial.print("runs =");
      Serial.println(counterRunsSensor);
      getNewDistance();
    }
    device.loop();
  }
  Serial.println("Kalibrierung komplett");
  setColour(CRGB::Black);
}

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
    setColour(CRGB::Blue);
    connect();
    setColour(CRGB::Black);
  }
  
  readDistance();

  if((millis()-aktTime) > 1000){ //send once per second
      aktTime = millis();
      Serial.print("runs =");
      Serial.print(counterRunsSensor);

      getNewDistance();
      aktProzent = distance2percent(aktDistance,refZeroDistance,refFullDistance);

      Serial.print(" | aktuelle Distanze [mm]=");
      Serial.print(aktDistance);
      Serial.print(" |  Prozent [%] = ");
      Serial.println(aktProzent);
      sendAktProzent(aktProzent);
  }
  device.loop();
}

