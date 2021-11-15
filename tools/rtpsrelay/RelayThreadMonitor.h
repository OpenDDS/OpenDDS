#ifndef RELAY_THREAD_MONITOR_H_
#define RELAY_THREAD_MONITOR_H_

#include <dds/DCPS/ThreadMonitor.h>
#include <dds/DCPS/TimeTypes.h>
#include <ace/Guard_T.h>
#include <ace/Condition_Thread_Mutex.h>
#include <ace/Log_Msg.h>
#include <dds/DCPS/RcObject.h>

#include <map>
#include <deque>
#include <string>
#include <stack>

using namespace OpenDDS::DCPS;

namespace RtpsRelay {

/**
 * @class Relay_Thread_Monitor
 *
 * @brief Defines the means of tracking thread utilization by measuring
 * time spent in event handling vs idle
 */
class Relay_Thread_Monitor : public Thread_Monitor {
public:
  Relay_Thread_Monitor (int perd = 5, size_t depth = 1);

  virtual void update(UpdateMode, const char* = "");
  void summarize(void);
  void report_thread(ACE_thread_t key);

  void report(void);

  void active_monitor(void);
  void start(void);
  void stop(void);

protected:
  bool running_;

  ACE_Thread_Mutex modlock_;
  ACE_Condition_Thread_Mutex moderator_;

  struct Sample {
    UpdateMode mode_;
    MonotonicTimePoint at_;
  };

  struct Load_Summary {
    ACE_UINT64 accum_busy_;
    ACE_UINT64 accum_idle_;
    size_t samples_;
    MonotonicTimePoint recorded_;
  };

  typedef std::deque<struct Sample> Load_Samples;
  typedef std::deque<struct Load_Summary> Load_History;

  class Thread_Descriptor : public OpenDDS::DCPS::RcObject
  {
  public:
    Thread_Descriptor(const char *alias, OpenDDS::DCPS::MonotonicTimePoint tnow);
    mutable ACE_Thread_Mutex queue_lock_;
    std::string alias_;
    Load_Samples samples_;
    std::stack<UpdateMode> nested_;
    MonotonicTimePoint last_;
    Load_History summaries_;
  };

  typedef OpenDDS::DCPS::RcHandle<Thread_Descriptor> Thread_Descriptor_rch;
  std::map<ACE_thread_t, Thread_Descriptor_rch> descs_;
  TimeDuration period_;
  size_t history_depth_;

};

}

#endif // RELAY_THREAD_MONITOR_H_
