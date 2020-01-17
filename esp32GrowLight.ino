//#include <Arduino.h>
#include "wifi_config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "blinker.h"
#include "WifiComm.h"
#include "MqttPublisher.h"
#include "NTPComm.h"
#include "timer_config.h"


// I could look up the time zone using an API from TimezoneDB.  Got an API key to do it.
// http://api.timezonedb.com/v2.1/list-time-zone?key=I0K9WRO2BVRO&format=json&country=US&zone=America/Denver
// Returns this, which gives the gmt offset in seconds:
/*

{
status: "OK",
message: "",
zones: [
{
countryCode: "US",
countryName: "United States",
zoneName: "America/Denver",
gmtOffset: -25200,
timestamp: 1577909137
}
]
}

 */

const int TIMEZONE = -7;

const int POWER_PIN = 26;
const int BUTTON_PIN = 37;

const int STATUS_LED_PIN = 10;
blinker blinkStatus(STATUS_LED_PIN);
WifiComm wifi_comm(blinkStatus);
MqttPublisher *publisher = NULL;
NTPComm ntp_comm(TIMEZONE);
String uniqueId;
boolean light_on = false;

static const char *ONLINE_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Online";
static const char *OFFLINE_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Offline";
static const char *LIGHTON_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Lighton";
static const char *LIGHTOFF_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Lightoff";

void setup_network() {
  

	// We start by connecting to a WiFi network
	wifi_comm.Connect(cfg.ssid, cfg.password);

	// This fills uniqueId with a string that includes a few bytes of the MAC address, so it's pretty unique.
	wifi_comm.GetUniqueId(uniqueId, "growlight");

  publisher = new MqttPublisher(wifi_comm.GetClient(), blinkStatus);
	publisher->Configure(cfg.mqtt_server, cfg.mqtt_port, uniqueId);
  publisher->SetWill(String(OFFLINE_TOPIC), uniqueId);
}

void loop_network() {
	if (!publisher->Connected()) {
		publisher->Reconnect(LIGHTON_TOPIC, getOnlineMessage().c_str());
    publisher->Subscribe(LIGHTON_TOPIC, onLightOn);
    publisher->Subscribe(LIGHTOFF_TOPIC, onLightOff);
	}
	publisher->Loop();
}

String getOnlineMessage() {
  return uniqueId + ", " + light_on;
}

void setup() {
  Serial.begin(115200);
  delay(500); // This is necessary for the ESP32
  Serial.println("..... STARTING .....");
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  blinkStatus.setup(true);
  setLightState(true);
  pinMode(POWER_PIN, OUTPUT);

	setup_network();
}

void loop() {
	loop_network();
  blinkStatus.check();
  checkButtonPress();
  checkLightState();
} 

void checkLightState() {
  const int PRINT_FREQUENCY = 30000;
  static int lastPrintTime = 0;
  int currentTime = millis();
  if(currentTime - lastPrintTime > PRINT_FREQUENCY) {
    ntp_comm.update();
    int current_second_of_day = ntp_comm.getSecondOfDay();
    Serial.println(ntp_comm.getFormattedTime());
    Serial.print("Second Of Day: " + String(current_second_of_day));
    Serial.println("");
    lastPrintTime = currentTime;

    boolean desiredState = current_second_of_day > start_time.secondval() && current_second_of_day < stop_time.secondval();
    Serial.println("Desired light state: " + String(desiredState));
    if(desiredState != light_on)
      handleButtonPress();
  }
}

void checkButtonPress() {

  static unsigned long lastDebounceTime = 0;
  static unsigned long debounceDelay = 100;
  static int lastReading = digitalRead(BUTTON_PIN);

  int reading = digitalRead(BUTTON_PIN);
  if(reading != lastReading) {
    if(millis() - lastDebounceTime > debounceDelay) {
      handleButtonPress();
    }
    lastDebounceTime = millis();
  }  
}

void handleButtonPress() {
  light_on = !light_on;
  if(light_on) {
    publisher->Publish(LIGHTON_TOPIC, uniqueId.c_str());
    setLightState(true);
  }
  else {
    publisher->Publish(LIGHTOFF_TOPIC, uniqueId.c_str());  
    setLightState(false);
  }
}

void onLightOn(byte *msg, unsigned int len) {
  Serial.println("OnLightOn");
  if(strncmp((const char *)msg, uniqueId.c_str(), uniqueId.length()) == 0) {
    setLightState(true);
  }
}

void onLightOff(byte *msg, unsigned int len) {
  Serial.println("OnLightOff");
  if(strncmp((const char *)msg, uniqueId.c_str(), uniqueId.length()) == 0) {
    setLightState(false);
  }
}

void setLightState(bool onOff) {
  if(onOff)
    Serial.println("Light state: On");
  else
    Serial.println("Light state: Off");
  
  digitalWrite(POWER_PIN, onOff);
  light_on = onOff;
}
