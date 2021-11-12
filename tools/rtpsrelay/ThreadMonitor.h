#ifndef RTPSRELAY_THREAD_MONITOR_H_
#define RTPSRELAY_THREAD_MONITOR_H_

#include <dds/DCPS/ThreadMonitor.h>
#include <dds/DCPS/TimeTypes.h>
#include <ace/Guard_T.h>
#include <ace/Condition_Thread_Mutex.h>
#include <ace/Log_Msg.h>

#include <map>
#include <deque>
#include <string>

using namespace std;

namespace RtpsRelay {

/**
 * @class ACE_Thread_Monitor
 *
 * @brief Defines the means of tracking thread utilization by measuring
 * time spent in event handling vs idle
 */
  class Thread_Monitor : public OpenDDS::DCPS::Thread_Monitor {
  public:
    Thread_Monitor (int perd = 5, size_t depth = 1);

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
      OpenDDS::DCPS::MonotonicTimePoint at_;
    };

    struct Load_Summary {
      OpenDDS::DCPS::TimeDuration accum_[2];
      int last_state_;
      OpenDDS::DCPS::MonotonicTimePoint recorded_;
    };

    typedef std::deque<struct Sample> Load_Samples;
    typedef std::deque<struct Load_Summary> Load_History;

    typedef struct Thread_Descriptor {
      ~Thread_Descriptor()
      {
        delete queue_lock_;
      }
      ACE_Thread_Mutex *queue_lock_;
      std::string alias_;
      Load_Samples samples_;
      OpenDDS::DCPS::MonotonicTimePoint last_;
      Load_History summaries_;
    } Thr_Desc;

    std::map<ACE_thread_t, Thr_Desc> descs_;
    OpenDDS::DCPS::TimeDuration period_;
    size_t history_depth_;

  };


}

#endif // RTPSRELAY_THREAD_MONITOR_H_
