#include "RelayThreadMonitor.h"
#include <ace/Thread.h>
#include <stdexcept>

using namespace OpenDDS::DCPS;
namespace RtpsRelay {

Relay_Thread_Monitor::Relay_Thread_Monitor(int perd, size_t depth) : running_(false),
                                                         modlock_(),
                                                         moderator_(
                                                         modlock_), period_(
static_cast<time_t>(perd)), history_depth_(depth)
{
  Thread_Monitor::installed_monitor_ = this;
}

void Relay_Thread_Monitor::update(Thread_Monitor::UpdateMode mode,
                            const char* alias)
{
  MonotonicTimePoint tnow = MonotonicTimePoint::now();
  struct Sample s = { mode, tnow };
  ACE_thread_t key = ACE_OS::thr_self();
  if (mode.implicit_ && !mode.idle_) {
    ACE_DEBUG((LM_DEBUG, "TLM Got an implicit busy on thread %x\n", key));
  }
  try {
    Thread_Descriptor_rch& td = descs_.at(key);
    ACE_Guard <ACE_Thread_Mutex> g(td->queue_lock_);
    td->samples_.emplace_back(std::move(s));
  } catch (const std::out_of_range&) {
    Load_Samples samps;
    Load_History hist;
    Thread_Descriptor_rch td = make_rch<Thread_Descriptor>(alias, tnow);
    td->samples_.emplace_back(std::move(s));
    descs_.emplace(key, std::move(td));
  }
}

Relay_Thread_Monitor::Thread_Descriptor::Thread_Descriptor(const char* alias,
                                                     MonotonicTimePoint tnow)
: alias_(alias), last_(tnow)
{
}

void Relay_Thread_Monitor::summarize(void)
{
  MonotonicTimePoint tnow = MonotonicTimePoint::now();
  for (auto d = descs_.begin(); d != descs_.end(); d++) {
    std::deque <Sample> local;
    auto& td = d->second;
    {
      ACE_Guard <ACE_Thread_Mutex> g(td->queue_lock_);
      local.swap(td->samples_);
    }
    struct Load_Summary ls;
    ls.accum_idle_ = ls.accum_busy_ = 0;
    ls.recorded_ = tnow;
    ls.samples_ = local.size();

    while (!local.empty()) {
      auto& s = local.front();
      ACE_UINT64* bucket = s.mode_.idle_ ? &ls.accum_busy_
                                         : &ls.accum_idle_;
      if (td->nested_.size()) {
        UpdateMode prev = td->nested_.top();
        if (s.mode_.implicit_) {
          td->nested_.pop();
        } else {
          if (prev.idle_ == s.mode_.idle_) {
            bucket = s.mode_.idle_ ? &ls.accum_idle_ : &ls.accum_busy_;
          }
          td->nested_.push(s.mode_);
        }
      } else {
        td->nested_.push(s.mode_);
      }
      if (bucket) {
        ACE_UINT64 udiff = 0;
        TimeDuration tdiff = s.at_ - td->last_;
        tdiff.value().to_usec(udiff);
        *bucket += udiff;
        td->last_ = s.at_;
      }
      local.pop_front();
    }
    {
      UpdateMode mode = {true, true};
      if (td->nested_.size()) {
        mode = td->nested_.top();
      }
      ACE_UINT64* bucket = mode.idle_ ? &ls.accum_idle_ : &ls.accum_busy_;
      ACE_UINT64 udiff = 0;
      TimeDuration tdiff = tnow - td->last_;
      tdiff.value().to_usec(udiff);
      ++ls.samples_;
      *bucket += udiff;
      td->last_ = tnow;
    }
    if (td->summaries_.size() >= history_depth_) {
      td->summaries_.pop_front();
    }
    td->summaries_.emplace_back(std::move(ls));
  }
}

void Relay_Thread_Monitor::report_thread(ACE_thread_t key)
{
  try {
    Thread_Descriptor_rch& td = descs_.at(key);
    if (td->summaries_.empty()) {
      ACE_DEBUG((LM_INFO, "     TLM thread: 0x%x \"%s\" busy:  n/a    idle: n/a", key, td->alias_.c_str()));
      return;
    }
    const struct Load_Summary& ls = td->summaries_.back();
    ACE_UINT64 uspan = ls.accum_idle_ + ls.accum_busy_;
    if (uspan > 0) {
      double pbusy = 100.0 * ls.accum_busy_ / uspan;
      double pidle = 100.0 * ls.accum_idle_ / uspan;
      ACE_DEBUG((LM_INFO, "     TLM thread: 0x%x \"%s\" busy:%6.4F%% (%d usec) idle: %6.4F%% measured interval: %F sec and %d samples\n",
                 key, td->alias_.c_str(), pbusy, ls.accum_busy_, pidle, uspan / 1000000.0, ls.samples_));
    } else {
      ACE_DEBUG((LM_INFO, "     TLM thread: 0x%x \"%s\" busy:  N/A idle: N/A  measured interval: N/A\n",
                 key, td->alias_.c_str()));
    }
    if (td->nested_.size()) {
      ACE_DEBUG((LM_INFO, "     TLM thread: 0x%x nesting level is %d\n", key, td->nested_.size()));
    }
  } catch (const std::out_of_range&) {
    ACE_DEBUG((LM_INFO, "     TLM: No entry available for thread id 0x%x\n", key));
  }
}

void Relay_Thread_Monitor::report(void)
{
  ACE_DEBUG((LM_INFO, "%T TLM: Reporting on %d threads\n", descs_.size()));
  for (auto d = descs_.begin(); d != descs_.end(); d++) {
    report_thread(d->first);
  }
}

void Relay_Thread_Monitor::active_monitor(void)
{
  while (running_) {
    MonotonicTimePoint expire = MonotonicTimePoint::now() + period_;
    moderator_.wait(&expire.value());
    if (running_) {
      summarize();
      report();
    }
  }
}

ACE_THR_FUNC_RETURN loadmonfunction(void* arg)
{
  Relay_Thread_Monitor* tmon = reinterpret_cast<Relay_Thread_Monitor*>(arg);
  tmon->active_monitor();
  return 0;
}

void Relay_Thread_Monitor::start()
{
  if (running_) {
    return;
  }
  running_ = true;
  ACE_Thread::spawn(&loadmonfunction, this);
}

void Relay_Thread_Monitor::stop()
{
  if (running_) {
    running_ = false;
    moderator_.broadcast();
  }
}
}