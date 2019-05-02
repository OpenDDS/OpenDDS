#include "Common.h"

#include <iomanip>

namespace Builder {

const TimeStamp ZERO = {0, 0};

TimeStamp get_time() {
  auto now = std::chrono::system_clock::now();
  auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
  TimeStamp result = {static_cast<CORBA::Long>(millisecs.count() / 1000), static_cast<CORBA::ULong>((millisecs.count() % 1000) * 10000000)};
  return result;
}

double to_seconds_double(const TimeStamp& ts) {
  return static_cast<double>(ts.sec) + (static_cast<double>(ts.nsec) * 1e-9);
}

std::chrono::milliseconds get_duration(const TimeStamp& ts) {
  return std::chrono::milliseconds(static_cast<int64_t>((static_cast<int64_t>(ts.sec) * 1000) + (static_cast<int64_t>(ts.nsec) / 1000)));
}

TimeStamp operator-(const TimeStamp& lhs, const TimeStamp& rhs) {
  TimeStamp result;
  result.sec = lhs.sec - rhs.sec;
  if (lhs.nsec >= rhs.nsec) {
    result.nsec = lhs.nsec - rhs.nsec;
  } else {
    --result.sec;
    result.nsec = 1e9 - (rhs.nsec - lhs.nsec);
  }
  return result;
}

bool operator<(const TimeStamp& lhs, const TimeStamp& rhs) {
  return lhs.sec < rhs.sec || (lhs.sec == rhs.sec && lhs.nsec < rhs.nsec);
}

bool operator<=(const TimeStamp& lhs, const TimeStamp& rhs) {
  return lhs.sec < rhs.sec || (lhs.sec == rhs.sec && lhs.nsec <= rhs.nsec);
}

bool operator==(const TimeStamp& lhs, const TimeStamp& rhs) {
  return lhs.sec == rhs.sec && lhs.sec == rhs.sec;
}

std::ostream& operator<<(std::ostream& out, const TimeStamp& ts) {
  out << std::setprecision(3) << std::fixed << static_cast<double>(ts.sec) + (static_cast<double>(ts.nsec) / 1.0e9) << std::flush;
  return out;
}

}

