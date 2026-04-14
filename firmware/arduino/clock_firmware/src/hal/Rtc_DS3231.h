#pragma once

#include <RTClib.h>

class Rtc_DS3231 {
 public:
  Rtc_DS3231();

  bool begin();
  bool isPresent() const;
  bool lostPower() const;
  DateTime now() const;
  void adjust(const DateTime& dateTime);

 private:
  RTC_DS3231 rtc_;
  bool present_;
  bool lostPower_;
};
