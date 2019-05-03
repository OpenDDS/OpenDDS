#pragma once

#include "Stool_Builder_Export.h"
#include "BuilderC.h"

#include <chrono>

#define APPLY_QOS_MASK(QOS_OUT, QOS_IN, MASK, PATH, FIELD) do { if (MASK.PATH.has_ ## FIELD) { QOS_OUT.PATH.FIELD = QOS_IN.PATH.FIELD; } } while (0);

namespace Builder {

Stool_Builder_Export extern const TimeStamp ZERO;

Stool_Builder_Export TimeStamp get_time();

Stool_Builder_Export double to_seconds_double(const TimeStamp& ts);

Stool_Builder_Export std::chrono::milliseconds get_duration(const TimeStamp& ts);

Stool_Builder_Export TimeStamp operator-(const TimeStamp& lhs, const TimeStamp& rhs);

Stool_Builder_Export bool operator<(const TimeStamp& lhs, const TimeStamp& rhs);

Stool_Builder_Export bool operator<=(const TimeStamp& lhs, const TimeStamp& rhs);

Stool_Builder_Export bool operator==(const TimeStamp& lhs, const TimeStamp& rhs);

Stool_Builder_Export std::ostream& operator<<(std::ostream& out, const TimeStamp& ts);

// This class is intended to provide constant-time access into a potentially-growing sequence of properties
// We want the names and content of the properties to be dynamic (set by the users of the builder API), but
// for performance reasons it would be good to stay away from something with log-n access times (maps, etc)
class Stool_Builder_Export PropertyIndex {
public:
  PropertyIndex();
  PropertyIndex(PropertySeq& seq, uint32_t index);
  const Property* operator->() const;
  Property* operator->();

  inline explicit operator bool() const { return seq_ != 0; }
  inline bool operator!() const { return seq_ == 0; }

protected:
  PropertySeq* seq_;
  uint32_t index_;
};

Stool_Builder_Export PropertyIndex get_or_create_property(PropertySeq& seq, const std::string& name, PropertyValueKind kind);

}

