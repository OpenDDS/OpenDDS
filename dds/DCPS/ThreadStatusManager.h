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

#if defined (ACE_WIN32)
typedef unsigned ThreadId;
#else
#  ifdef ACE_HAS_GETTID
typedef pid_t ThreadId;
#  else
typedef String ThreadId;
#  endif
#endif /* ACE_WIN32 */

struct OpenDDS_Dcps_Export ThreadInfo {
  /// What the thread is being used for
  String name;
  /// ID used to help generate thread status BIT key
  ThreadId thread_id;
  /// This is p_thread_t on Linux and thread handle on Windows
  ACE_thread_t handle;

  String get_bit_key() const;
};

class OpenDDS_Dcps_Export ThreadStatusListener {
public:
  virtual ~ThreadStatusListener() {}
  virtual void on_thread_started(const ThreadInfo& thread) = 0;
  virtual void on_thread_finished(const ThreadInfo& thread) = 0;
};

class OpenDDS_Dcps_Export ThreadStatusManager {
public:
  class OpenDDS_Dcps_Export Thread {
  public:
    enum ThreadStatus {
      ThreadStatus_Active,
      ThreadStatus_Idle,
    };

    Thread(const String& name, const ThreadId& thread_id, ACE_thread_t handle);

    const ThreadInfo& info() const { return info_; }
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

    static const size_t BUCKET_COUNT = 8;

  private:
    const ThreadInfo info_;
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
    Bucket buckets_[BUCKET_COUNT];
    size_t current_bucket_;
  };

#ifdef ACE_HAS_CPP11
  typedef OPENDDS_UNORDERED_MAP(ThreadId, Thread) Map;
#else
  typedef OPENDDS_MAP(ThreadId, Thread) Map;
#endif

  typedef OPENDDS_LIST(Thread) List;

  void set_thread_status_listener(ThreadStatusListener* listener);
  void thread_status_interval(const TimeDuration& thread_status_interval);
  TimeDuration thread_status_interval() const;

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

  /// Store copies of these from the parent to avoid needing to access it.
  class ManagerInfo {
  public:
    ManagerInfo()
      : thread_status_listener_(0)
    {
    }

    const TimeDuration& thread_status_interval() const
    {
      return thread_status_interval_;
    }

    const TimeDuration& bucket_limit() const
    {
      return bucket_limit_;
    }

    void thread_status_interval(const TimeDuration& interval)
    {
      thread_status_interval_ = interval;
      bucket_limit_ = interval / static_cast<double>(Thread::BUCKET_COUNT);
    }

    void set_thread_status_listener(ThreadStatusListener* listener)
    {
      thread_status_listener_ = listener;
    }

    bool update_thread_status() const
    {
      return thread_status_interval_ || thread_status_listener_;
    }

    void on_thread_started(const Thread& thread) const
    {
      if (thread_status_listener_) {
        thread_status_listener_->on_thread_started(thread.info());
      }
    }

    void on_thread_finished(const Thread& thread) const
    {
      if (thread_status_listener_) {
        thread_status_listener_->on_thread_finished(thread.info());
      }
    }

  protected:
    TimeDuration thread_status_interval_;
    TimeDuration bucket_limit_;
    ThreadStatusListener* thread_status_listener_;
  };

  class ThreadContainer {
  public:
    void set_manager_info(const ManagerInfo& manager_info);
    void add_thread(const Thread& thread);
    void update(Thread::ThreadStatus status, bool finished, bool nested, int detail1, int detail2,
      const MonotonicTimePoint& m_now, const SystemTimePoint& s_now, const ThreadId& thread_id);
    void harvest(const MonotonicTimePoint& start, List& running, List& finished) const;

  protected:
    ManagerInfo manager_info_;
    Map map_;
    List finished_;
    mutable ACE_Thread_Mutex mutex_;
  };

  ThreadContainer& get_container(const ThreadId& tid);
  void update_manager_info(const ManagerInfo& copy);
  void add_thread(const String& name);

  void update_i(Thread::ThreadStatus status, bool finished = false,
                bool nested = false, int detail1 = 0, int detail2 = 0);

  void update_current_thread(Thread::ThreadStatus status, bool nested = false, int detail1 = 0, int detail2 = 0)
  {
    update_i(status, false, nested, detail1, detail2);
  }

  void finished()
  {
    update_i(Thread::ThreadStatus_Idle, true, false);
  }

  void active(bool nested = false, int detail1 = 0, int detail2 = 0)
  {
    update_current_thread(Thread::ThreadStatus_Active, nested, detail1, detail2);
  }

  void idle(bool nested = false)
  {
    update_current_thread(Thread::ThreadStatus_Idle, nested);
  }

  ManagerInfo manager_info_;
  mutable ACE_Thread_Mutex lock_;

  static const size_t NUM_CONTAINERS = 11;
  ThreadContainer containers_[NUM_CONTAINERS];
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_THREADSTATUSMANAGER_H */
