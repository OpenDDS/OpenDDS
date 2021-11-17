#include "RelayThreadMonitor.h"

#include <dds/DCPS/ReactorTask.h>
#include <ace/Thread.h>

#include <stdexcept>

using namespace OpenDDS::DCPS;
namespace RtpsRelay {

RelayThreadMonitor::RelayThreadMonitor(int perd, size_t depth)
: running_(false)
, modlock_()
, moderator_(modlock_)
, period_(static_cast<time_t>(perd))
, history_depth_(depth)
{
  ThreadMonitor::installed_monitor_ = this;
}

void RelayThreadMonitor::preset(ThreadStatusManager *tsm, const char *alias)
{
  pending_reg_[alias] = tsm;
}

void RelayThreadMonitor::update(ThreadMonitor::UpdateMode mode,
                                const char* alias)
{
  MonotonicTimePoint tnow = MonotonicTimePoint::now();
  struct Sample s = { mode, tnow };
  ACE_thread_t key = ACE_OS::thr_self();
  try {
    ThreadDescriptor_rch& td = descs_.at(key);
    ACE_Guard<ACE_Thread_Mutex> g(td->queue_lock_);
    td->samples_.emplace_back(std::move(s));
  } catch (const std::out_of_range&) {
    LoadSamples samps;
    LoadHistory hist;
    ThreadDescriptor_rch td = make_rch<ThreadDescriptor>(alias, tnow);
    td->tsm_ = pending_reg_[alias];
    pending_reg_.erase(alias);
    td->samples_.emplace_back(std::move(s));
    descs_.emplace(key, std::move(td));
  }
}

double RelayThreadMonitor::get_busy_pct(const char* key) const
{
  try {
    return busy_map_.at(key);
  } catch (const std::out_of_range&) {
  }
  return 0.0;
}

RelayThreadMonitor::ThreadDescriptor::ThreadDescriptor(const char* alias,
                                                     MonotonicTimePoint tnow)
: alias_(alias)
, tsm_(nullptr)
, last_(tnow)
{
}

void RelayThreadMonitor::summarize(void)
{
  MonotonicTimePoint tnow = MonotonicTimePoint::now();
  for (auto d = descs_.begin(); d != descs_.end(); d++) {
    std::deque<Sample> local;
    auto& td = d->second;
    {
      ACE_Guard<ACE_Thread_Mutex> g(td->queue_lock_);
      local.swap(td->samples_);
    }
    LoadSummary ls;
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
      UpdateMode mode = {true, true, true};
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

void RelayThreadMonitor::report_thread(ACE_thread_t key)
{
  try {
    ThreadDescriptor_rch& td = descs_.at(key);
    if (td->summaries_.empty()) {
      ACE_DEBUG((LM_INFO, "     TLM thread: 0x%x \"%s\" busy:  n/a    idle: n/a", key, td->alias_.c_str()));
      return;
    }
    const LoadSummary& ls = td->summaries_.back();
    double pbusy = 0.0;
    ACE_UINT64 uspan = ls.accum_idle_ + ls.accum_busy_;
    if (uspan > 0) {
      pbusy = 100.0 * ls.accum_busy_ / uspan;
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
    if (td->tsm_) {
      td->tsm_->update_busy(td->alias_.c_str(), pbusy);
    } else {
      busy_map_[td->alias_.c_str()] = pbusy;
    }
  } catch (const std::out_of_range&) {
    ACE_DEBUG((LM_INFO, "     TLM: No entry available for thread id 0x%x\n", key));
  }
}

void RelayThreadMonitor::report(void)
{
  ACE_DEBUG((LM_INFO, "%T TLM: Reporting on %d threads\n", descs_.size()));
  for (auto d = descs_.begin(); d != descs_.end(); d++) {
    report_thread(d->first);
  }
}

void RelayThreadMonitor::active_monitor(void)
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
  RelayThreadMonitor* tmon = reinterpret_cast<RelayThreadMonitor*>(arg);
  tmon->active_monitor();
  return 0;
}

void RelayThreadMonitor::start()
{
  if (running_) {
    return;
  }
  running_ = true;
  ACE_Thread::spawn(&loadmonfunction, this);
}

void RelayThreadMonitor::stop()
{
  if (running_) {
    running_ = false;
    moderator_.broadcast();
  }
}
}
