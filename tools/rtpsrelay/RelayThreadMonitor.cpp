#include "RelayThreadMonitor.h"

#include <dds/DCPS/ReactorTask.h>
#include <ace/Thread.h>
#include <ace/OS_NS_strings.h>

#include <stdexcept>

using namespace OpenDDS::DCPS;
namespace RtpsRelay {

RelayThreadMonitor::RelayThreadMonitor(TimeDuration perd, size_t depth)
: summarizer_(*this, perd)
, history_depth_(depth)
{
  ThreadMonitor::installed_monitor_ = this;
}

RelayThreadMonitor::~RelayThreadMonitor() noexcept
{
  for (auto r : reporters_) delete r;
}
void RelayThreadMonitor::preset(ThreadStatusManager *tsm, const char *alias)
{
  pending_reg_[alias] = tsm;
}

void RelayThreadMonitor::update(ThreadMonitor::UpdateMode mode,
                                const char* alias)
{
  MonotonicTimePoint tnow = MonotonicTimePoint::now();
  Sample s = { mode, tnow };
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

double RelayThreadMonitor::get_utilization(const char* key) const
{
  try {
    return busy_map_.at(key);
  } catch (const std::out_of_range&) {
    return 0.0;
  }
}

ThreadDescriptor::ThreadDescriptor(const char* alias, MonotonicTimePoint tnow)
: alias_(alias)
, tsm_(nullptr)
, last_(tnow)
{
}

void RelayThreadMonitor::summarize()
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
      ACE_UINT64* bucket = s.mode_.idle_ && s.mode_.stacked_ ? &ls.accum_busy_ : &ls.accum_idle_;
      if (s.mode_.stacked_) {
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
      UpdateMode mode = {true, true, false};
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

    double pbusy = 0.0;
    double uspan = static_cast<double>(ls.accum_idle_ + ls.accum_busy_);
    if (uspan != 0.0) {
      if (td->tsm_) {
        td->tsm_->update_busy(td->alias_.c_str(), pbusy);
      } else {
        busy_map_[td->alias_.c_str()] = pbusy;
      }
    }

    if (td->summaries_.size() >= history_depth_) {
      td->summaries_.pop_front();
    }
    td->summaries_.emplace_back(std::move(ls));
  }
}

ThreadLoadReporter::~ThreadLoadReporter() noexcept
{
}

void ThreadLoadReporter::report_header(ThreadMonitor&) const
{
}

void ThreadLoadReporter::report_thread(ThreadDescriptor_rch) const
{
}

void ThreadLoadReporter::report_footer(ThreadMonitor&) const
{
}

AceLogReporter::~AceLogReporter() noexcept
{
}

void AceLogReporter::report_header(ThreadMonitor& tlm) const
{
  ACE_DEBUG((LM_INFO, "%T TLM: Reporting on %d threads\n", tlm.thread_count()));
}

void AceLogReporter::report_thread(ThreadDescriptor_rch td) const
{
  if (td->summaries_.empty()) {
    ACE_DEBUG((LM_INFO, "     TLM thread \"%s\" busy:  n/a    idle: n/a", td->alias_.c_str()));
    return;
  }
  const LoadSummary& ls = td->summaries_.back();
  double pbusy = 0.0;
  double uspan = static_cast<double>(ls.accum_idle_ + ls.accum_busy_);
  if (uspan != 0.0) {
    pbusy = ls.accum_busy_ / uspan;
    double pidle = ls.accum_idle_ / uspan;
    ACE_DEBUG((LM_INFO, "     TLM thread \"%s\" busy:%6.4F%% (%d usec) idle: %6.4F%% measured interval: %F sec and %d samples\n",
               td->alias_.c_str(), 100.0 * pbusy, ls.accum_busy_, 100.0 * pidle, uspan / 1000000.0, ls.samples_));
  } else {
    ACE_DEBUG((LM_INFO, "     TLM thread \"%s\" busy:  N/A idle: N/A  measured interval: N/A\n",
               td->alias_.c_str()));
  }
  if (td->nested_.size()) {
    ACE_DEBUG((LM_INFO, "     TLM nesting level is %d\n", td->nested_.size()));
  }
}

size_t RelayThreadMonitor::thread_count()
{
  return descs_.size();
}

int RelayThreadMonitor::add_reporter(const char* name)
{
  ACE_DEBUG((LM_DEBUG,"(%P|%t) RelayThreadMonitor::add_reporter adding \'%C\'\n", name));
  if (ACE_OS::strcasecmp(name, "AceLog") == 0) {
    reporters_.push_back(new AceLogReporter);
    return 0;
  } else {
    ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) ERROR: RelayThreadMonitor::add_reporter \'%C\' not recognized\n"), name));
    return -1;
  }
}

void RelayThreadMonitor::report()
{
  if (reporters_.empty()) {
    return;
  }
  for(auto r : reporters_) {
    r->report_header(*this);
  }
  for (auto d = descs_.begin(); d != descs_.end(); d++) {
    for (auto r : reporters_) {
      r->report_thread(d->second);
    }
  }
  for (auto r : reporters_) {
    r->report_footer(*this);
  }
}

int RelayThreadMonitor::start()
{
  return summarizer_.start();
}

void RelayThreadMonitor::stop()
{
  summarizer_.stop();
}

Summarizer::Summarizer(ThreadMonitor& owner, TimeDuration perd)
: running_(false)
, period_(perd)
, lock_()
, condition_(lock_)
, owner_(owner)
{
}

Summarizer::~Summarizer()
{
}


int Summarizer::svc()
{
  GuardType guard(lock_);
  running_ = true;
  condition_.notify_all();
  while (running_) {
    MonotonicTimePoint expire = MonotonicTimePoint::now() + period_;
    condition_.wait_until(expire);
    if (running_) {
      owner_.summarize();
      owner_.report();
    }
  }
  return 0;
}

int Summarizer::start()
{
  GuardType guard(lock_);
  if (running_) {
    return 0;
  }
  if (activate(THR_NEW_LWP | THR_JOINABLE, 1) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                     "(%P|%t) ERROR: RelayThreadMonitor::Summarizer Failed to activate "
                     "itself.\n"),
                     -1);
  }
  while (!running_) {
    condition_.wait();
  }
  return 0;
}

void Summarizer::stop()
{
  if (running_) {
    GuardType guard(lock_);
    running_ = false;
    condition_.notify_all();
  }
  wait();
}
}
