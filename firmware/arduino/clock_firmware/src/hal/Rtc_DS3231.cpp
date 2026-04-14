#include "Rtc_DS3231.h"

Rtc_DS3231::Rtc_DS3231() : present_(false), lostPower_(false) {}

bool Rtc_DS3231::begin() {
  present_ = rtc_.begin();
  if (present_) {
    lostPower_ = rtc_.lostPower();
  } else {
    lostPower_ = false;
  }
  return present_;
}

bool Rtc_DS3231::isPresent() const {
  return present_;
}

bool Rtc_DS3231::lostPower() const {
  return present_ && lostPower_;
}

DateTime Rtc_DS3231::now() const {
  if (!present_) {
    return DateTime(2026, 1, 1, 0, 0, 0);
  }
  return rtc_.now();
}

void Rtc_DS3231::adjust(const DateTime& dateTime) {
  if (!present_) {
    return;
  }
  rtc_.adjust(dateTime);
  lostPower_ = false;
}
