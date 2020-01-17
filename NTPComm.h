#include <NTPClient.h>
#include <WiFiUdp.h>

class NTPComm {

  WiFiUDP ntpUDP;
  NTPClient timeClient;

public:
  NTPComm(int timezone_offset);
  void update();
  int getHours();
  int getMinutes();
  int getSeconds();
  String getFormattedTime();
  int getSecondOfDay();
};
