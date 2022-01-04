#ifndef RelayThreadMonitor_H_
#define RelayThreadMonitor_H_

#include <dds/DCPS/ConditionVariable.h>
#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/ThreadMonitor.h>
#include <dds/DCPS/TimeTypes.h>

#include <ace/Task.h>
#include <ace/Synch_Traits.h>
#include <ace/Log_Msg.h>

#include <map>
#include <deque>
#include <string>
#include <stack>

namespace RtpsRelay {

struct Sample {
  OpenDDS::DCPS::ThreadMonitor::UpdateMode mode_;
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
//typedef std::map<std::string, OpenDDS::DCPS::ThreadStatusManager*> registrants;

class ThreadDescriptor : public OpenDDS::DCPS::RcObject
{
public:
  ThreadDescriptor(const char *alias, OpenDDS::DCPS::MonotonicTimePoint tnow);
  bool add_sample(Sample s);
  void reset_current();
  mutable ACE_Thread_Mutex queue_lock_;
  std::string alias_;
  LoadSamples samples_;
  std::stack<OpenDDS::DCPS::ThreadMonitor::UpdateMode> nested_;
  OpenDDS::DCPS::MonotonicTimePoint last_;
  LoadHistory summaries_;
  ACE_UINT64 current_busy_;
  ACE_UINT64 current_idle_;
  static double high_water_mark;
  static double low_water_mark;
};


typedef OpenDDS::DCPS::RcHandle<ThreadDescriptor> ThreadDescriptor_rch;
typedef std::map<ACE_thread_t, ThreadDescriptor_rch> DescriptorMap;

class ThreadLoadReporter
{
public:
  virtual ~ThreadLoadReporter() noexcept;
  virtual void report_header(OpenDDS::DCPS::ThreadMonitor& tlm) const;
  virtual void report_thread(ThreadDescriptor_rch td) const;
  virtual void report_footer(OpenDDS::DCPS::ThreadMonitor& tlm) const;
};

typedef std::list<ThreadLoadReporter*> ReporterList;

class AceLogReporter : public ThreadLoadReporter
{
public:
  virtual ~AceLogReporter() noexcept;
  void report_header(OpenDDS::DCPS::ThreadMonitor& tlm) const;
  void report_thread(ThreadDescriptor_rch td) const;
};

class Summarizer : public virtual ACE_Task_Base {
public:
  explicit Summarizer(OpenDDS::DCPS::ThreadMonitor& owner, OpenDDS::DCPS::TimeDuration perd);
  virtual ~Summarizer();
  virtual int svc();
  int start();
  void stop();
  void force_update();
private:
  bool running_;
  bool forced_;
  OpenDDS::DCPS::TimeDuration period_;

  typedef ACE_SYNCH_MUTEX LockType;
  typedef ACE_Guard<LockType> GuardType;
  typedef OpenDDS::DCPS::ConditionVariable<LockType> ConditionVariableType;

  LockType lock_;
  ConditionVariableType condition_;
  OpenDDS::DCPS::ThreadMonitor& owner_;
};

/**
 * @class RelayThreadMonitor
 *
 * @brief Defines the means of tracking thread utilization by measuring
 * time spent in event handling vs idle
 */
class RelayThreadMonitor : public OpenDDS::DCPS::ThreadMonitor {
public:
  explicit RelayThreadMonitor (OpenDDS::DCPS::TimeDuration perd,
                               OpenDDS::DCPS::ThreadStatusManager* tsm);
  ~RelayThreadMonitor() noexcept;
  //virtual void preset(OpenDDS::DCPS::ThreadStatusManager*, const char*);
  virtual void update(UpdateMode, const char* = "");
  virtual double get_utilization(const char* key) const;
  virtual void set_levels(double hwm, double lwm);
  size_t thread_count();
  int add_reporter (const char* name);
  void summarize();
  void summarize_thread(ThreadDescriptor_rch& );
  void report();

  int start();
  void stop();

protected:

  Summarizer summarizer_;
  //registrants pending_reg_;
  DescriptorMap descs_;
  ThreadDescriptor_rch forced_;
  size_t history_depth_;
  BusyMap busy_map_;
  ReporterList reporters_;
  OpenDDS::DCPS::ThreadStatusManager *tsm_;
};

}

#endif // RelayThreadMonitor_H_
