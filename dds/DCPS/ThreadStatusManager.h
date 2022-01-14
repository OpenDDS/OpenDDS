/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADSTATUSMANAGER_H
#define OPENDDS_DCPS_THREADSTATUSMANAGER_H

#include "dcps_export.h"
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

    Thread(const String& bit_key)
      : bit_key_(bit_key)
      , timestamp_(SystemTimePoint::now())
      , status_(ThreadStatus_Active)
      , last_update_(MonotonicTimePoint::now())
      , current_bucket_(0)
      , nesting_depth_(0)
    {}

    const String& bit_key() const { return bit_key_; }
    const SystemTimePoint& timestamp() const { return timestamp_; }
    const MonotonicTimePoint& last_update() const { return last_update_; }

    void update(const MonotonicTimePoint& m_now,
                const SystemTimePoint& s_now,
                ThreadStatus next_status,
                const TimeDuration& bucket_limit,
                bool nested);
    double utilization(const MonotonicTimePoint& now) const;

    static const size_t bucket_count = 8;

  private:
    const String bit_key_;
    SystemTimePoint timestamp_;
    ThreadStatus status_;

    struct OpenDDS_Dcps_Export Bucket {
      TimeDuration active_time;
      TimeDuration idle_time;
    };
    MonotonicTimePoint last_update_;
    Bucket total_;
    Bucket bucket_[bucket_count];
    size_t current_bucket_;
    size_t nesting_depth_;
  };
  typedef OPENDDS_MAP(ThreadId, Thread) Map;
  typedef OPENDDS_LIST(Thread) List;

  void thread_status_interval(const TimeDuration& thread_status_interval)
  {
    thread_status_interval_ = thread_status_interval;
    bucket_limit_ = thread_status_interval / Thread::bucket_count;
  }

  const TimeDuration& thread_status_interval() const
  {
    return thread_status_interval_;
  }

  bool update_thread_status() const
  {
    return thread_status_interval_ > TimeDuration::zero_value;
  }

  /// Add the calling thread with the manager.
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
    Event(ThreadStatusManager& thread_status_manager)
      : thread_status_manager_(thread_status_manager)
    {
      thread_status_manager_.active(true);
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
    Sleeper(ThreadStatusManager& thread_status_manager)
      : thread_status_manager_(thread_status_manager)
    {
      thread_status_manager_.idle();
    }

    ~Sleeper()
    {
      thread_status_manager_.active();
    }

  private:
    ThreadStatusManager& thread_status_manager_;
  };

  /// Copy active and idle threads to running and finished threads to
  /// finished.  Only threads updated after start are considered.
  void harvest(const MonotonicTimePoint& start,
               List& running,
               List& finished) const;

#ifdef ACE_HAS_GETTID
  static inline pid_t gettid()
  {
    return syscall(SYS_gettid);
  }
#endif

private:
  static ThreadId get_thread_id();
  void add_thread(const String& name);
  void active(bool nested = false);
  void idle(bool nested = false);
  void finished();

  void cleanup(const MonotonicTimePoint& now);

  TimeDuration thread_status_interval_;
  TimeDuration bucket_limit_;
  Map map_;
  List list_;

  mutable ACE_Thread_Mutex lock_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_THREADSTATUSMANAGER_H */
