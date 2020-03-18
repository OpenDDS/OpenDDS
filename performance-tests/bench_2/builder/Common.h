#pragma once

#include "Bench_Builder_Export.h"
#include "BuilderC.h"

#include <chrono>
#include <ostream>
#include <string>

#define APPLY_QOS_MASK(QOS_OUT, QOS_IN, MASK, PATH, FIELD) do { \
  if (MASK.PATH.has_ ## FIELD) { \
    QOS_OUT.PATH.FIELD = QOS_IN.PATH.FIELD; \
  } } while (0);

namespace Builder {

Bench_Builder_Export extern const TimeStamp ZERO;

Bench_Builder_Export TimeStamp get_time();

Bench_Builder_Export TimeStamp from_seconds(int32_t sec);

Bench_Builder_Export double to_seconds_double(const TimeStamp& ts);

Bench_Builder_Export std::chrono::milliseconds get_duration(const TimeStamp& ts);

Bench_Builder_Export TimeStamp operator+(const TimeStamp& lhs, const TimeStamp& rhs);

Bench_Builder_Export TimeStamp operator-(const TimeStamp& lhs, const TimeStamp& rhs);

Bench_Builder_Export bool operator<(const TimeStamp& lhs, const TimeStamp& rhs);

Bench_Builder_Export bool operator<=(const TimeStamp& lhs, const TimeStamp& rhs);

Bench_Builder_Export bool operator==(const TimeStamp& lhs, const TimeStamp& rhs);

Bench_Builder_Export std::ostream& operator<<(std::ostream& out, const TimeStamp& ts);

/**
 * This class is intended to provide constant-time access into a
 * potentially-growing sequence of properties. We want the names and content of
 * the properties to be dynamic (set by the users of the builder API), but for
 * performance reasons it would be good to stay away from something with log-n
 * access times (maps, etc)
 */
class Bench_Builder_Export PropertyIndex {
public:
  PropertyIndex();
  PropertyIndex(PropertySeq& seq, uint32_t index);

  const Property* operator->() const;
  Property* operator->();

  inline explicit operator bool() const { return seq_ != 0; }
  inline bool operator!() const { return seq_ == 0; }

  PropertySeq* get_seq() { return seq_; };
  const PropertySeq* get_seq() const { return seq_; };

protected:
  PropertySeq* seq_;
  uint32_t index_;
};

class Bench_Builder_Export ConstPropertyIndex {
public:
  ConstPropertyIndex();
  ConstPropertyIndex(const PropertySeq& seq, uint32_t index);

  const Property* operator->() const;

  inline explicit operator bool() const { return seq_ != 0; }
  inline bool operator!() const { return seq_ == 0; }

  const PropertySeq* get_seq() const { return seq_; };

protected:
  const PropertySeq* seq_;
  uint32_t index_;
};

Bench_Builder_Export PropertyIndex get_or_create_property(
  PropertySeq& seq, const std::string& name, PropertyValueKind kind);

Bench_Builder_Export ConstPropertyIndex get_property(
  const PropertySeq& seq, const std::string& name, PropertyValueKind kind);

class Bench_Builder_Export NullStream : public std::streambuf {
public:
  int overflow(int c);
};

class Bench_Builder_Export Log {
public:
  static std::ostream& log();
  static std::ostream* stream;
};

}
