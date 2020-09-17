#ifndef RTPSRELAY_STATS_SCHEDULER_H_
#define RTPSRELAY_STATS_SCHEDULER_H_

#include "StatisticsReporter.h"

#include "dds/DCPS/TimeDuration.h"

#include <ace/Event_Handler.h>
#include <ace/Reactor.h>
#include <ace/Time_Value.h>


namespace RtpsRelay {

class StatsScheduler : public ACE_Event_Handler {
public:
  static const long INVALID_TIMER_ID = -1l;

  StatsScheduler(const OpenDDS::DCPS::TimeDuration& interval_sec,
                 StatisticsReporter& reporter,
                 ACE_Reactor* reactor)
    : ACE_Event_Handler(reactor)
    , statistics_interval_(interval_sec)
    , stats_reporter_(reporter)
    , timer_id_(INVALID_TIMER_ID)
  {}

  virtual ~StatsScheduler() { stop(); }
  void start()
  {
    if (!statistics_interval_.is_zero()) {
      // Register for a timer callback at the given interval
      timer_id_ = reactor()->schedule_timer(this, 0, statistics_interval_.value(), statistics_interval_.value());
      if (INVALID_TIMER_ID == timer_id_) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: StatsScheduler::start failed to register schedule statistics timer\n")));
      } else {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) %N:%l ERROR: StatsScheduler::start no interval set - no stats will be reported\n")));
      }
    }
  }

  void stop()
  {
    // Cleanup the handler
    if (INVALID_TIMER_ID != timer_id_ ) {
      reactor()->cancel_timer(timer_id_);
      timer_id_ = INVALID_TIMER_ID;
    }
  }

  int handle_timeout(const ACE_Time_Value& ace_now, const void* arg) override
  {
    ACE_UNUSED_ARG(arg);

    // Report the stats
    const OpenDDS::DCPS::MonotonicTimePoint now(ace_now);
    stats_reporter_.report(now);
    stats_reporter_.reset_stats();

    return 0;
  }

private:
  OpenDDS::DCPS::TimeDuration statistics_interval_;
  StatisticsReporter& stats_reporter_;
  long timer_id_;
};

}
#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
