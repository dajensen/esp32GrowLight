//#include <Arduino.h>
#include "TimeSpec.h"
#include "wifi_config.h"
#include "timer_config.h"
#include "blinker.h"
#include <WiFi.h>
#include "WifiComm.h"
#include <PubSubClient.h>
#include "MqttPublisher.h"
#include <ezTime.h>

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
Timezone MountainTime;

String uniqueId;
boolean light_state = false;
boolean prev_desired_state = false;
boolean manual_override = false;

static const char *ONLINE_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Online";
static const char *OFFLINE_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Offline";
static const char *LIGHTON_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Lighton";
static const char *LIGHTOFF_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Lightoff";

void setup_network() {
  
	// We start by connecting to a WiFi network
	wifi_comm.Connect(cfg.ssid, cfg.password);

	// This fills uniqueId with a string that includes a few bytes of the MAC address, so it's pretty unique.
	wifi_comm.GetUniqueId(uniqueId, "growlight");
  Serial.println("UniqueID: " + uniqueId);

  publisher = new MqttPublisher(wifi_comm.GetClient(), blinkStatus);
	publisher->Configure(cfg.mqtt_server, cfg.mqtt_port, uniqueId);
  publisher->SetWill(String(OFFLINE_TOPIC), uniqueId);
  waitForSync();  // from ezTime, means wait for NTP to get date and time.
  Serial.println("UTC: " + UTC.dateTime());
    
  MountainTime.setLocation("America/Denver");
  Serial.println("Mountain time: " + MountainTime.dateTime());
  Serial.println("Hour: " + String(MountainTime.hour()) + " Minute: " + String(MountainTime.minute()));
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
  return uniqueId + ", " + light_state;
}


void setup() {
  Serial.begin(115200);
  delay(5000); // This is necessary for the ESP32
  Serial.println("..... STARTING .....");
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  blinkStatus.setup(true);
  setLightState(false);
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
  const int PRINT_FREQUENCY = 60000;
  static int lastPrintTime = 0;
  int currentTime = millis();
  if(currentTime - lastPrintTime > PRINT_FREQUENCY || currentTime < PRINT_FREQUENCY) {
    Serial.println("Mountain time: " + MountainTime.dateTime());
    int current_second_of_day = MountainTime.hour() * SECONDS_PER_HOUR + MountainTime.minute() * SECONDS_PER_MINUTE + MountainTime.second();
    Serial.print("Second Of Day: " + String(current_second_of_day));
    Serial.println("");
    lastPrintTime = currentTime;

    boolean desiredState = current_second_of_day > start_time.secondval() && current_second_of_day < stop_time.secondval();
    Serial.println("Desired light state: " + String(desiredState));
    if(desiredState != light_state && (!manual_override || desiredState != prev_desired_state)){
      handleButtonPress();
      Serial.println("Set light_state to " + String(light_state));
    }

    if(desiredState != prev_desired_state) {
      Serial.println("Clearing manual override");
      manual_override = false;      
    }
      
    prev_desired_state = desiredState;
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
  light_state = !light_state;
  if(light_state) {
    publisher->Publish(LIGHTON_TOPIC, uniqueId.c_str());
    setLightState(true);
  }
  else {
    publisher->Publish(LIGHTOFF_TOPIC, uniqueId.c_str());  
    setLightState(false);
  }
  manual_override = true;
}

void onLightOn(byte *msg, unsigned int len) {
  Serial.println("OnLightOn");
  if(strncmp((const char *)msg, uniqueId.c_str(), uniqueId.length()) == 0) {
    setLightState(true);
    manual_override = true;
  }
}

void onLightOff(byte *msg, unsigned int len) {
  Serial.println("OnLightOff");
  if(strncmp((const char *)msg, uniqueId.c_str(), uniqueId.length()) == 0) {
    setLightState(false);
    manual_override = true;
  }
}

void setLightState(bool onOff) {
  if(onOff)
    Serial.println("Light state: On");
  else
    Serial.println("Light state: Off");
  
  digitalWrite(POWER_PIN, onOff);
  light_state = onOff;
}
