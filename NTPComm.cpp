#include <NTPClient.h>
//#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "NTPComm.h"

WiFiUDP ntpUDP;

const int SECONDS_PER_HOUR = 3600;
const int SECONDS_PER_MINUTE = 60;

NTPComm::NTPComm(int timezone_offset) : ntpUDP(), timeClient(ntpUDP) {
  const int TIME_OFFSET = timezone_offset * SECONDS_PER_HOUR;

  timeClient.begin();
  timeClient.setTimeOffset(TIME_OFFSET);
}

void NTPComm::update() {
  timeClient.update();
}

int NTPComm::getHours() {
  return timeClient.getHours();
}

int NTPComm::getMinutes() {
  return timeClient.getMinutes();
}

int NTPComm::getSeconds() {
  return timeClient.getSeconds();
}

String NTPComm::getFormattedTime() {
  return timeClient.getFormattedTime();
}

int NTPComm::getSecondOfDay() {
  return getHours() * SECONDS_PER_HOUR + getMinutes() * SECONDS_PER_MINUTE + getSeconds();
}
