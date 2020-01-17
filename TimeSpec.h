
const int SECONDS_PER_HOUR = 60*60;
const int SECONDS_PER_MINUTE = 60;

struct TimeSpec{
  int hour;
  int minute;
  int second;
  int secondval() {return hour * SECONDS_PER_HOUR + minute * SECONDS_PER_MINUTE + second;}
  TimeSpec(int h, int m, int s) : hour(h), minute(m), second(s) {}
};
