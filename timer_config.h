struct TimeSpec{
  int hour;
  int minute;
  int second;
  int secondval() {return hour * 3600 + minute * 60 + second;}
  TimeSpec(int h, int m, int s) : hour(h), minute(m), second(s) {}
};


TimeSpec start_time = TimeSpec(7,00, 0);   // 3:30 PM
TimeSpec stop_time = TimeSpec(22, 00, 0);   // 4:00 PM
