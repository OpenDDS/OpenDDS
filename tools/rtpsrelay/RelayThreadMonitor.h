#ifndef RelayThreadMonitor_H_
#define RelayThreadMonitor_H_

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

namespace RtpsRelay {

/**
 * @class RelayThreadMonitor
 *
 * @brief Defines the means of tracking thread utilization by measuring
 * time spent in event handling vs idle
 */
class RelayThreadMonitor : public OpenDDS::DCPS::ThreadMonitor {
public:
  RelayThreadMonitor (OpenDDS::DCPS::TimeDuration perd = 5, size_t depth = 1);
  virtual void preset(OpenDDS::DCPS::ThreadStatusManager *, const char *);
  virtual void update(UpdateMode, const char* = "");
  virtual double get_busy_pct(const char* key) const;

  void summarize(void);
  void report_thread(ACE_thread_t key);

  void report(void);

  void active_monitor(void);
  void start(void);
  void stop(void);

protected:
  bool running_;
  typedef std::map<std::string, OpenDDS::DCPS::ThreadStatusManager *> registrants;
  registrants pending_reg_;

  ACE_Thread_Mutex modlock_;
  ACE_Condition_Thread_Mutex moderator_;

  struct Sample {
    UpdateMode mode_;
    OpenDDS::DCPS::MonotonicTimePoint at_;
  };

  struct LoadSummary {
    ACE_UINT64 accum_busy_;
    ACE_UINT64 accum_idle_;
    size_t samples_;
    OpenDDS::DCPS::MonotonicTimePoint recorded_;
  };

  typedef std::deque<struct Sample> LoadSamples;
  typedef std::deque<struct LoadSummary> LoadHistory;
  typedef std::map<const char*, double> BusyMap;

  class ThreadDescriptor : public OpenDDS::DCPS::RcObject
  {
  public:
    ThreadDescriptor(const char *alias, OpenDDS::DCPS::MonotonicTimePoint tnow);
    mutable ACE_Thread_Mutex queue_lock_;
    std::string alias_;
    OpenDDS::DCPS::ThreadStatusManager *tsm_;
    LoadSamples samples_;
    std::stack<UpdateMode> nested_;
    OpenDDS::DCPS::MonotonicTimePoint last_;
    LoadHistory summaries_;
  };

  typedef OpenDDS::DCPS::RcHandle<ThreadDescriptor> ThreadDescriptor_rch;
  std::map<ACE_thread_t, ThreadDescriptor_rch> descs_;
  OpenDDS::DCPS::TimeDuration period_;
  size_t history_depth_;
  BusyMap busy_map_;
};

}

#endif // RelayThreadMonitor_H_
