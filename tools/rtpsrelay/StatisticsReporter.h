#ifndef RTPSRELAY_STATISTICS_REPORTER_H_
#define RTPSRELAY_STATISTICS_REPORTER_H_

#include "dds/DCPS/TimeTypes.h"

namespace RtpsRelay {

class StatisticsReporter {
public:
  virtual ~StatisticsReporter() {}

  virtual void report(const OpenDDS::DCPS::MonotonicTimePoint& time_now) = 0;
  virtual void reset_stats() = 0;
protected:

  explicit StatisticsReporter() {}
};

}

#endif // RTPSRELAY_STATISTICS_REPORTER_H_
