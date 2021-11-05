
#include "ThreadMonitor.h"

using namespace RtpsRelay;
using namespace std;

Thread_Monitor::Thread_Monitor (int perd, size_t depth)
: running_ (false),
  thr_func_ (0),
  modlock_(),
  moderator_(modlock_),
  period_(static_cast<time_t>(perd)),
  history_depth_(depth)
  {

  }

void Thread_Monitor::update(OpenDDS::DCPS::Thread_Monitor::UpdateMode mode, const char *alias) {
  struct timespec now;
  timespec_get(&now, TIME_UTC);
  struct Sample s({mode, now});
  ACE_thread_t key = ACE_OS::thr_self();
  try {
    Thr_Desc td = descs_.at(key);
    ACE_Guard <ACE_Thread_Mutex> g(*td->queue_lock_);
    td->samples_.emplace_back(s);
  } catch (const std::out_of_range & ) {
    Thr_Desc td = new struct Thread_Descriptor;
    td->queue_lock_ = new ACE_Thread_Mutex;
    td->last_[0].set(s.at_);
    td->last_[1].set(s.at_);
    td->alias_ = alias;
    td->samples_.emplace_back(s);

    descs_.emplace(key,td);
  }
}

void Thread_Monitor::summarize(void)
{
  for (auto d = this->descs_.begin(); d != this->descs_.end(); d++ ) {
    std::deque <Sample> local;
    auto &td = d->second;
    {
      ACE_Guard<ACE_Thread_Mutex> g(*td->queue_lock_);
      local.swap(td->samples_);
    }
    struct Load_Summary ls;
    ls.recorded_.set(timespec_get(0,TIME_UTC));
    ls.last_state_ = -1;
    if (local.empty()) {
      // emit warning of empty thread samples
    }
    while (!local.empty()) {
      auto &s = local.front();
      ACE_Time_Value diff(s.at_);
      int ndx = (s.mode_ == IMPLICIT_IDLE || s.mode_ == EXPLICIT_IDLE)  ? 1 : 0;
      if (s.mode_ == ls.last_state_) {
        // emit warning of consecutive samples in the same state
      }
      diff -= td->last_[ndx];
      ls.accum_[ndx] += diff;
      ls.last_state_ = s.mode_;
      td->last_[1-ndx].set(s.at_);
      local.pop_front();
    }
    if (td->summaries_.size() >= this->history_depth_) {
      td->summaries_.pop_front();
    }
    td->summaries_.emplace_back(ls);
  }
}

void Thread_Monitor::report_thread(ACE_thread_t key)
{
  try {
    Thr_Desc td = this->descs_.at(key);
    if (td->summaries_.empty()) {
      ACE_DEBUG((LM_DEBUG,"thread: 0x%x \"%s\" busy:  n/a    idle: n/a", key, td->alias_.c_str()));
      return;
    }
    const struct Load_Summary &ls = td->summaries_.back();
    ACE_Time_Value tspan = ls.accum_[0] + ls.accum_[1];
    ACE_UINT64 uspan, uidle, ubusy;
    tspan.to_usec(uspan);
    ls.accum_[0].to_usec(uidle);
    ls.accum_[2].to_usec(ubusy);
    double pbusy = 100.0 * ubusy/uspan;
    double pidle = 100.0 * uidle/uspan;
    ACE_DEBUG((LM_DEBUG,"thread: 0x%x \"%s\" busy:  %F%% idle: %F%%  measured interval: %F\n",
    key, td->alias_.c_str(), pbusy, pidle, uspan/1000000.0));
  }
  catch (const out_of_range& ) {
    ACE_DEBUG ((LM_DEBUG, "No entry available for thread id 0x%x\n", key));
  }
}

void Thread_Monitor::report(void)
{
  for (auto d = this->descs_.begin(); d != this->descs_.end(); d++ ) {
    this->report_thread(d->first);
  }
}

void Thread_Monitor::active_monitor(void)
{
  while (this->running_) {
    ACE_Time_Value expire(time(0)+this->period_);
    this->moderator_.wait (&expire);
    if (this->running_) {
      this->summarize();
      this->report();
    }
  }
}

ACE_THR_FUNC_RETURN loadmonfunction (void *arg) {
  Thread_Monitor *tmon = reinterpret_cast<Thread_Monitor *>(arg);
  tmon->active_monitor();
  return 0;
}

void Thread_Monitor::start()
{
  if (this->running_) {
    return;
  }
  this->running_ = true;
  this->thr_func_ = new ACE_Thread_Adapter(&loadmonfunction, this);
  thr_func_->invoke();
}

void Thread_Monitor::stop () {
  if (!this->running_) {
    this->running_ = false;
    this->moderator_.broadcast();
  }
}