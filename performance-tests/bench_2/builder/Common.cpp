#include "Common.h"

#include <iomanip>
#include <sstream>

namespace Builder {

const TimeStamp ZERO = {0, 0};

TimeStamp get_time()
{
  auto now = std::chrono::high_resolution_clock::now();
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
  auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>((now - seconds).time_since_epoch());
  TimeStamp result = {static_cast<CORBA::Long>(seconds.count()), static_cast<CORBA::ULong>(nanoseconds.count())};
  return result;
}

TimeStamp from_seconds(int32_t sec)
{
  TimeStamp result = { sec, 0 };
  return result;
}

double to_seconds_double(const TimeStamp& ts)
{
  return static_cast<double>(ts.sec) + (static_cast<double>(ts.nsec) * 1e-9);
}

std::chrono::milliseconds
get_duration(const TimeStamp& ts)
{
  return std::chrono::milliseconds(static_cast<int64_t>((static_cast<int64_t>(ts.sec) * 1000) +
    (static_cast<int64_t>(ts.nsec) / 1e6)));
}

TimeStamp operator+(const TimeStamp& lhs, const TimeStamp& rhs)
{
  TimeStamp result;
  result.sec = lhs.sec + rhs.sec;
  uint64_t nsec = static_cast<uint64_t>(lhs.nsec) + static_cast<uint64_t>(rhs.nsec);
  if (nsec >= static_cast<uint64_t>(1e9)) {
    result.sec += nsec / static_cast<uint64_t>(1e9);
    result.nsec = nsec % static_cast<uint64_t>(1e9);
  } else {
    result.nsec = nsec;
  }
  return result;
}

TimeStamp operator-(const TimeStamp& lhs, const TimeStamp& rhs)
{
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

bool operator<(const TimeStamp& lhs, const TimeStamp& rhs)
{
  return lhs.sec < rhs.sec || (lhs.sec == rhs.sec && lhs.nsec < rhs.nsec);
}

bool operator<=(const TimeStamp& lhs, const TimeStamp& rhs)
{
  return lhs.sec < rhs.sec || (lhs.sec == rhs.sec && lhs.nsec <= rhs.nsec);
}

bool operator==(const TimeStamp& lhs, const TimeStamp& rhs)
{
  return lhs.sec == rhs.sec && lhs.nsec == rhs.nsec;
}

std::ostream&
operator<<(std::ostream& out, const TimeStamp& ts)
{
  std::streamsize ssize = out.precision();
  out << std::setprecision(3) << std::fixed <<
    static_cast<double>(ts.sec) + (static_cast<double>(ts.nsec) / 1.0e9) << std::setprecision(ssize) << std::flush;
  return out;
}

PropertyIndex::PropertyIndex()
: seq_(0)
, index_(0)
{
}

PropertyIndex::PropertyIndex(PropertySeq& seq, uint32_t index)
: seq_(&seq)
, index_(index)
{
}

const Property* PropertyIndex::operator->() const
{
  return &((*seq_)[index_]);
}

Property* PropertyIndex::operator->()
{
  return &((*seq_)[index_]);
}

ConstPropertyIndex::ConstPropertyIndex()
: seq_(0)
, index_(0)
{
}

ConstPropertyIndex::ConstPropertyIndex(const PropertySeq& seq, uint32_t index)
: seq_(&seq)
, index_(index)
{
}

const Property* ConstPropertyIndex::operator->() const
{
  return &((*seq_)[index_]);
}

Builder::PropertyIndex
get_or_create_property(
  Builder::PropertySeq& seq,
  const std::string& name, Builder::PropertyValueKind kind)
{
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

Builder::ConstPropertyIndex
get_property(
  const Builder::PropertySeq& seq,
  const std::string& name, Builder::PropertyValueKind kind)
{
  for (uint32_t i = 0; i < seq.length(); ++i) {
    if (std::string(seq[i].name.in()) == name) {
      if (seq[i].value._d() == kind) {
        return ConstPropertyIndex(seq, i);
      } else {
        return ConstPropertyIndex();
      }
    }
  }
  return ConstPropertyIndex();
}

int
NullStream::overflow(int c)
{
  return c;
}

std::ostream* Log::stream = nullptr;

std::ostream&
Log::log()
{
  static NullStream null_stream_buf;
  static std::ostream null_stream(&null_stream_buf);
  return stream ? *stream : null_stream;
}

}
