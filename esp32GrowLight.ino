#include "wifi_config.h"
#include <WiFi.h>
//#include <PubSubClient.h>

#include "TimeSpec.h"
#include "timer_config.h"
#include <ezTime.h>

#include <M5StickC.h>
  
WiFiClient wifiClient;
String uniqueId;
//PubSubClient mqttClient(wifiClient);
Timezone LocalTime;
#define LOCAL_TIMEZONE "America/Denver"
boolean light_state = false;
boolean prev_desired_state = false;
boolean manual_override = false;

const int STATUS_LED = 10;
const int POWER_PIN = 26;
const int BUTTON_PIN = 37;

/*
static const char *ENUMERATE_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Enumerate";
static const char *STATUS_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Status";
static const char *OFFLINE_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/Offline";
static const char *LIGHTON_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/LightOn";
static const char *LIGHTOFF_TOPIC = "616b7b49-aab4-4cbb-a7a8-ba7ed744dc11/LightOff";

void mqttCallback(char *topic, byte* payload, unsigned int len) {
  if(strcmp(topic, ENUMERATE_TOPIC) == 0) {
    onEnumerate(payload, len);
  }
  else if(strcmp(topic, LIGHTON_TOPIC) == 0) {
    onLightOn(payload, len);
  }
  else if(strcmp(topic, LIGHTOFF_TOPIC) == 0) {
    onLightOff(payload, len);
  }
}
*/

void PrintMacAddr(const unsigned char *mac) {
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
}

void GetUniqueId(String &id, const char *prefix)
{
const int WL_MAC_ADDR_LENGTH = 12;

   // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "<prefix>-"
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  String macID = "";
  PrintMacAddr(mac);

  if(mac[4] < 0x10)
    macID += '0';
  macID += String(mac[4], HEX);
  
  if(mac[5] < 0x10)
    macID += '0';
  macID += String(mac[5], HEX);
  
  macID.toUpperCase();
  id = String(prefix) + "-" + macID;
}

void create_status_message(String &dest, String id, boolean status) {
  dest = id + "," + String(status);
}
/*
void connect_mqtt(PubSubClient &client, const char *server, const unsigned int port, const char *id) {
  client.setServer(server, port);
  delay(5000);    // This is required for the esp32 mqtt library.  Otherwise the whole device sometimes hangs when a reconnect happens.  Others are having the same trouble.
  client.connect(id);

  while(!client.connected()) {
    // This is a bad block of code because it doesn't do anything to reconnect WiFi.  That's something that should be done.
    if(WiFi.status() != WL_CONNECTED)
        printStatus("DISCONNECTED FROM WiFi ...", "");
    delay(500);
    Serial.print(":");
  }
 Serial.println("After connecting: " + String(client.connected()));
 Serial.println("Mqtt State: " + String(client.state()));

  String msg;
  create_status_message(msg, id, light_state);
 client.publish(STATUS_TOPIC, msg.c_str());
 Serial.println("Published: " + msg);
 client.subscribe(ENUMERATE_TOPIC);
 client.subscribe(LIGHTON_TOPIC);
 client.subscribe(LIGHTOFF_TOPIC);
}
*/

void check_timer() {
  const int CHECK_FREQUENCY = 5000;
  static int lastPrintTime = 0;
  int currentTime = millis();
  if(currentTime - lastPrintTime > CHECK_FREQUENCY || lastPrintTime == 0) {
    Serial.println("Local time: " + LocalTime.dateTime());
    int current_second_of_day = LocalTime.hour() * SECONDS_PER_HOUR + LocalTime.minute() * SECONDS_PER_MINUTE + LocalTime.second();
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

void initLCD() {
    // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(32, 0, 2);
  M5.Lcd.println("Grow Lights");
}

void printStatus(const char *wifiStatus, const char *mqttStatus) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(32, 0, 2);
  M5.Lcd.println("Grow Lights");

  M5.Lcd.setCursor(0, 30);
  M5.Lcd.println(wifiStatus);
  M5.Lcd.println(mqttStatus);
}

void setup() {
  digitalWrite(POWER_PIN, 0);
  digitalWrite(STATUS_LED, 1);    // Inverted for the built-in light
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(POWER_PIN, OUTPUT);  
  pinMode(STATUS_LED, OUTPUT);

  initLCD();
    
  Serial.begin(115200);
  //delay(5000); // This is necessary for the ESP32 to get serial messages
  Serial.println("..... STARTING .....");
  printStatus("Connecting to WiFi ...", "");
  WiFi.begin(cfg.ssid, cfg.password);
  while (WiFi.status() != WL_CONNECTED) {
		delay(100);
	}
  GetUniqueId(uniqueId, "Growlight");
  Serial.println("UniqueID: " + uniqueId);
  printStatus("Connected to WiFi", "NOT Connecting to MQTT ...");
/* 
  mqttClient.setCallback(mqttCallback);
  connect_mqtt(mqttClient, cfg.mqtt_server, cfg.mqtt_port, uniqueId.c_str());
  printStatus("Connected to WiFi", "Connected to MQTT");
*/
  waitForSync();  // from ezTime, means wait for NTP to get date and time.
  
  LocalTime.setLocation(LOCAL_TIMEZONE);
  Serial.println("Local time: " + LocalTime.dateTime());
  Serial.println("Hour: " + String(LocalTime.hour()) + " Minute: " + String(LocalTime.minute()));      
}

void loop() {
/*
	mqttClient.loop();
  if(!mqttClient.connected()){
    Serial.print("-");
    printStatus("Connected to WiFi", "Connecting to MQTT ...");
    connect_mqtt(mqttClient, cfg.mqtt_server, cfg.mqtt_port, uniqueId.c_str());
    printStatus("Connected to wifi", "Connected to MQTT");
  }
*/
  check_timer();
  checkButtonPress();
} 

void onEnumerate(byte *msg, unsigned int len) {
  Serial.println("Enumerate");
  String topicMsg;
  create_status_message(topicMsg, uniqueId, light_state);
//  mqttClient.publish(STATUS_TOPIC, topicMsg.c_str());
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
  setLightState(!light_state);
  manual_override = true;
}

 void setLightState(bool onOff) {
  if(onOff)
    Serial.println("Light state: On");
  else
    Serial.println("Light state: Off");
  
  digitalWrite(POWER_PIN, onOff);
  digitalWrite(STATUS_LED, !onOff);    // Inverted for the built-in light
  light_state = onOff;

  String msg;
  create_status_message(msg, uniqueId, light_state);
//  mqttClient.publish(STATUS_TOPIC, msg.c_str());
}
