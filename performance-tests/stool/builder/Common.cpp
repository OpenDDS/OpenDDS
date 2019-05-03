#include "Common.h"

#include <iomanip>
#include <sstream>

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

PropertyIndex::PropertyIndex() : seq_(0), index_(0) {
}

PropertyIndex::PropertyIndex(PropertySeq& seq, uint32_t index) : seq_(&seq), index_(index) {
}

const Property* PropertyIndex::operator->() const {
  return &((*seq_)[index_]);
}

Property* PropertyIndex::operator->() {
  return &((*seq_)[index_]);
}

Builder::PropertyIndex get_or_create_property(Builder::PropertySeq& seq, const std::string& name, Builder::PropertyValueKind kind) {
  for (uint32_t i = 0; i < seq.length(); ++i) {
    if (std::string(seq[i].name.in()) == name) {
      if (seq[i].value._d() == kind) {
        return PropertyIndex(seq, i);
      } else {
        std::stringstream ss;
        ss << "Property with name '" << name << " already defined with a different type." << std::flush;
        throw std::runtime_error(ss.str());
      }
    }
  }

  uint32_t idx = seq.length();
  seq.length(idx + 1);
  seq[idx].name = name.c_str();
  seq[idx].value._d(kind);
  return PropertyIndex(seq, idx);
}

}

