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

}

