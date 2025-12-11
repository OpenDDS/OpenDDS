/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADSTATUSMANAGER_H
#define OPENDDS_DCPS_THREADSTATUSMANAGER_H

#include "dcps_export.h"
#include "RcEventHandler.h"
#include "TimeTypes.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ThreadStatusManager {
public:

#if defined (ACE_WIN32)
  typedef unsigned ThreadId;
#else

#  ifdef ACE_HAS_GETTID
  typedef pid_t ThreadId;
#  else
  typedef String ThreadId;
#  endif
#endif /* ACE_WIN32 */

  class OpenDDS_Dcps_Export Thread {
  public:
    enum ThreadStatus {
      ThreadStatus_Active,
      ThreadStatus_Idle,
    };

    explicit Thread(const String& bit_key)
      : bit_key_(bit_key)
      , timestamp_(SystemTimePoint::now())
      , last_update_(MonotonicTimePoint::now())
      , last_status_change_(MonotonicTimePoint::now())
      , status_(ThreadStatus_Active)
      , nesting_depth_(0)
      , detail1_(0)
      , detail2_(0)
      , current_bucket_(0)
    {}

    const String& bit_key() const { return bit_key_; }
    const SystemTimePoint& timestamp() const { return timestamp_; }
    const MonotonicTimePoint& last_update() const { return last_update_; }
    int detail1() const { return detail1_; }
    int detail2() const { return detail2_; }

    void update(const MonotonicTimePoint& m_now,
                const SystemTimePoint& s_now,
                ThreadStatus next_status,
                const TimeDuration& bucket_limit,
                bool nested, int detail1, int detail2);
    double utilization(const MonotonicTimePoint& now) const;

    static const size_t bucket_count = 8;

  private:
    const String bit_key_;
    SystemTimePoint timestamp_;
    MonotonicTimePoint last_update_, last_status_change_;
    ThreadStatus status_;
    size_t nesting_depth_;
    int detail1_, detail2_;

    struct OpenDDS_Dcps_Export Bucket {
      TimeDuration active_time;
      TimeDuration idle_time;
      TimeDuration total_time() const { return active_time + idle_time; }
    };
    Bucket total_;
    Bucket buckets_[bucket_count];
    size_t current_bucket_;
  };
  typedef OPENDDS_MAP(ThreadId, Thread) Map;
  typedef OPENDDS_LIST(Thread) List;

  void thread_status_interval(const TimeDuration& thread_status_interval)
  {
    thread_status_interval_ = thread_status_interval;
    bucket_limit_ = thread_status_interval / static_cast<double>(Thread::bucket_count);
  }

  const TimeDuration& thread_status_interval() const
  {
    return thread_status_interval_;
  }

  bool update_thread_status() const
  {
    return thread_status_interval_ > TimeDuration::zero_value;
  }

  /// Add the calling thread to the manager.
  /// name is for a more human-friendly name that will be appended to the BIT key.
  /// Implicitly makes the thread active and finishes the thread on destruction.
  class Start {
  public:
    Start(ThreadStatusManager& thread_status_manager, const String& name)
      : thread_status_manager_(thread_status_manager)
    {
      thread_status_manager_.add_thread(name);
    }

    ~Start()
    {
      thread_status_manager_.finished();
    }

  private:
    ThreadStatusManager& thread_status_manager_;
  };

  class Event {
  public:
    explicit Event(ThreadStatusManager& thread_status_manager, int detail1 = 0, int detail2 = 0)
      : thread_status_manager_(thread_status_manager)
    {
      thread_status_manager_.active(true, detail1, detail2);
    }

    ~Event()
    {
      thread_status_manager_.idle(true);
    }

  private:
    ThreadStatusManager& thread_status_manager_;
  };

  class Sleeper {
  public:
    explicit Sleeper(ThreadStatusManager* thread_status_manager)
      : thread_status_manager_(thread_status_manager)
    {
      if (thread_status_manager_) {
        thread_status_manager_->idle();
      }
    }

    explicit Sleeper(ThreadStatusManager& thread_status_manager)
      : thread_status_manager_(&thread_status_manager)
    {
      thread_status_manager_->idle();
    }

    ~Sleeper()
    {
      if (thread_status_manager_) {
        thread_status_manager_->active();
      }
    }

  private:
    ThreadStatusManager* const thread_status_manager_;
  };

  struct Updater : RcEventHandler {
    int handle_timeout(const ACE_Time_Value&, const void* arg)
    {
      const ThreadStatusManager* const tsmConst = static_cast<const ThreadStatusManager*>(arg);
      ThreadStatusManager* const tsm = const_cast<ThreadStatusManager*>(tsmConst);
      tsm->idle();
      return 0;
    }
  };

  /// Copy active and idle threads to running and finished threads to
  /// finished.  Only threads updated after start are considered.
  void harvest(const MonotonicTimePoint& start,
               List& running,
               List& finished) const;

private:
  static ThreadId get_thread_id();
  void add_thread(const String& name);

  void update_current_thread(Thread::ThreadStatus status, bool nested = false, int detail1 = 0, int detail2 = 0)
  {
    update_i(status, false, nested, detail1, detail2);
  }

  void finished() { update_i(Thread::ThreadStatus_Idle, true, false); }

  void update_i(Thread::ThreadStatus status, bool finished = false,
                bool nested = false, int detail1 = 0, int detail2 = 0);

  void active(bool nested = false, int detail1 = 0, int detail2 = 0)
  {
    update_current_thread(Thread::ThreadStatus_Active, nested, detail1, detail2);
  }

  void idle(bool nested = false) { update_current_thread(Thread::ThreadStatus_Idle, nested); }

  void cleanup(const MonotonicTimePoint& now);

  TimeDuration thread_status_interval_;
  TimeDuration bucket_limit_;
  Map map_;
  List finished_;

  mutable ACE_Thread_Mutex lock_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_THREADSTATUSMANAGER_H */
