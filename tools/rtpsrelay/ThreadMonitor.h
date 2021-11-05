#ifndef RTPSRELAY_THREAD_MONITOR_H_
#define RTPSRELAY_THREAD_MONITOR_H_

#include <dds/DCPS/ThreadMonitor.h>

#include <ace/Guard_T.h>
#include <ace/Condition_Thread_Mutex.h>
#include <ace/Time_Value.h>
#include <ace/Log_Msg.h>
#include <ace/Thread_Adapter.h>

#include <ctime>
#include <map>

using namespace std;

namespace RtpsRelay {

/**
 * @class ACE_Thread_Monitor
 *
 * @brief Defines the means of tracking thread utilization by measuring
 * time spent in event handling vs idle
 */
  class Thread_Monitor : public OpenDDS::DCPS::Thread_Monitor
  {
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
    ACE_Thread_Adapter *thr_func_;

    ACE_Thread_Mutex modlock_;
    ACE_Condition_Thread_Mutex moderator_;

    struct Sample {
      UpdateMode mode_;
      struct timespec at_;
    };

    struct Load_Summary {
      ACE_Time_Value accum_[2];
      int last_state_;
      ACE_Time_Value recorded_;
    };

    typedef struct Thread_Descriptor {
      ACE_Thread_Mutex *queue_lock_;
      std::string alias_;
      std::deque<struct Sample> samples_;
      ACE_Time_Value last_[2];
      std::deque<struct Load_Summary> summaries_;
    } *Thr_Desc;

    std::map<ACE_thread_t, Thr_Desc> descs_;
    time_t period_;
    size_t history_depth_;

  };


}

#endif // RTPSRELAY_THREAD_MONITOR_H_
