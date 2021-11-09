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
    OpenDDS::DCPS::Thread_Monitor::installed_monitor_ = this;
  }

void Thread_Monitor::update(OpenDDS::DCPS::Thread_Monitor::UpdateMode mode, const char *alias)
{
  OpenDDS::DCPS::MonotonicTimePoint tnow;
  tnow = tnow.now();
  struct Sample s({mode, tnow});
  ACE_thread_t key = ACE_OS::thr_self();
  try {
    Thr_Desc td = descs_.at(key);
    ACE_Guard <ACE_Thread_Mutex> g(*td->queue_lock_);
    td->samples_.emplace_back(std::move(s));
  } catch (const out_of_range & ) {
    Thr_Desc td = new struct Thread_Descriptor;
    td->queue_lock_ = new ACE_Thread_Mutex;
    td->last_ = tnow;
    td->alias_ = alias;
    td->samples_.emplace_back(std::move(s));

    descs_.emplace(key,td);
  }
}

void Thread_Monitor::summarize(void)
{
  OpenDDS::DCPS::MonotonicTimePoint tnow;
  tnow = tnow.now();
  for (auto d = this->descs_.begin(); d != this->descs_.end(); d++ ) {
    std::deque <Sample> local;
    auto &td = d->second;
    {
      ACE_Guard<ACE_Thread_Mutex> g(*td->queue_lock_);
      local.swap(td->samples_);
    }
    struct Load_Summary ls;
    ls.accum_[0] = ls.accum_[1] = 0;
    ls.recorded_ = tnow.now();
    ls.last_state_ = -1;
    if (local.empty()) {
      ACE_DEBUG ((LM_DEBUG,"TLM: sample queue for 0x%x is empty\n", d->first));
      // emit warning of empty thread samples
    }

    while (!local.empty()) {
      auto &s = local.front();
      int ndx = (s.mode_ == IMPLICIT_IDLE || s.mode_ == EXPLICIT_IDLE)  ? 1 : 0;
      if (s.mode_ == ls.last_state_) {
        // emit warning of consecutive samples in the same state
      }
      OpenDDS::DCPS::TimeDuration durr = s.at_ - td->last_;
      ls.accum_[ndx] += durr;

      ls.last_state_ = s.mode_;
      td->last_ = s.at_;
      local.pop_front();
    }
    if (td->summaries_.size() >= this->history_depth_) {
      td->summaries_.pop_front();
    }
    td->summaries_.emplace_back(std::move(ls));
  }
}

void Thread_Monitor::report_thread(ACE_thread_t key)
{
  try {
    Thr_Desc td = this->descs_.at(key);
    if (td->summaries_.empty()) {
      ACE_DEBUG((LM_DEBUG,"%T TLM thread: 0x%x \"%s\" busy:  n/a    idle: n/a", key, td->alias_.c_str()));
      return;
    }
    const struct Load_Summary &ls = td->summaries_.back();
    ACE_UINT64 uspan, uidle, ubusy;
    ls.accum_[0].value().to_usec(uidle);
    ls.accum_[1].value().to_usec(ubusy);
    uspan = uidle + ubusy;
    if (uspan > 0) {
      double pbusy = 100.0 * ubusy/uspan;
      double pidle = 100.0 * uidle/uspan;
      ACE_DEBUG((LM_DEBUG,"%T TLM thread: 0x%x \"%s\" busy:  %F%% idle: %F%%  measured interval: %F sec\n",
      key, td->alias_.c_str(), pbusy, pidle, uspan/1000000.0));
    } else {
      ACE_DEBUG((LM_DEBUG,"%T TLM thread: 0x%x \"%s\" busy:  N/A idle: N/A  measured interval: N/A\n",
      key, td->alias_.c_str()));
    }
  }
  catch (const out_of_range& ) {
    ACE_DEBUG ((LM_DEBUG, "%T TLM: No entry available for thread id 0x%x\n", key));
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
    OpenDDS::DCPS::MonotonicTimePoint expire;
    expire = expire.now() + this->period_;
    this->moderator_.wait (&expire.value());
    if (this->running_) {
      this->summarize();
      this->report();
    }
  }
}

ACE_THR_FUNC_RETURN loadmonfunction (void* arg)
{
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

void Thread_Monitor::stop ()
{
  if (this->running_) {
    this->running_ = false;
    this->moderator_.broadcast();
  }
}
