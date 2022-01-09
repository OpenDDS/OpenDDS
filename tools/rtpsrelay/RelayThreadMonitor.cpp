#include "RelayThreadMonitor.h"

#include <dds/DCPS/ThreadStatusManager.h>
#include <dds/DCPS/Service_Participant.h>
#include <ace/Thread.h>
#include <ace/OS_NS_strings.h>
#include <ace/Arg_Shifter.h>
#include <ace/Configuration_Import_Export.h>
#include <ace/Argv_Type_Converter.h>

#include <stdexcept>

using namespace OpenDDS::DCPS;
namespace RtpsRelay {

static const ACE_TCHAR RELAY_SECTION_NAME[] = ACE_TEXT("relay");
static bool got_rtps_thread_hwm = false;
static bool got_rtps_thread_lwm = false;

RelayThreadMonitor::RelayThreadMonitor(int &argc, ACE_TCHAR* argv[])
: summarizer_(*this, TheServiceParticipant->get_thread_status_interval())
, history_depth_(1)
, tsm_(TheServiceParticipant->get_thread_status_manager())
{
  parse_args(argc, argv);
  load_configuration(TheServiceParticipant->get_configuration());
  ThreadMonitor::installed_monitor_ = this;
}

RelayThreadMonitor::~RelayThreadMonitor() noexcept
{
  for (auto r : reporters_) delete r;
}

int RelayThreadMonitor::parse_args(int &argc, ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left()) {
    const ACE_TCHAR* currentArg = 0;
    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-ThreadHighWaterMark"))) != 0) {
      ThreadDescriptor::high_water_mark = ACE_OS::atof(currentArg);
      arg_shifter.consume_arg();
      got_rtps_thread_hwm = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-ThreadLowWaterMark"))) != 0) {
      ThreadDescriptor::low_water_mark = ACE_OS::atof(currentArg);
      arg_shifter.consume_arg();
      got_rtps_thread_lwm = true;

    } else if ((currentArg = arg_shifter.get_the_parameter("-ThreadMonitorOutput"))) {
      add_reporter(ACE_TEXT_ALWAYS_CHAR(currentArg));
      arg_shifter.consume_arg();
    } else {
      arg_shifter.ignore_arg();
    }
  }

  return 0;
}

int RelayThreadMonitor::load_configuration(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key sect;

  if (cf.open_section(root, RELAY_SECTION_NAME, false, sect) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // a common section. The code default configuration will be used.
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) NOTICE: Service_Participant::load_common_configuration ")
      ACE_TEXT("failed to open section %s\n"), RELAY_SECTION_NAME));
    }

    return 0;

  } else {
    const ACE_TCHAR* message = ACE_TEXT("(%P|%t) NOTICE: using %s value from command option (overrides value if it's in config file)\n");

    if (got_rtps_thread_hwm) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("ThreadHighWaterMark")));
    } else {
      double hwm = 0.5;
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("ThreadHighWaterMark"), hwm, double)
      ThreadDescriptor::high_water_mark = hwm;
    }

    if (got_rtps_thread_hwm) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("ThreadLowWaterMark")));
    } else {
      double lwm = 0.5;
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("ThreadLowWaterMark"), lwm, double)
      ThreadDescriptor::low_water_mark = lwm;
    }
  }
  return 0;
}

void RelayThreadMonitor::update(ThreadMonitor::UpdateMode mode, const char* alias)
{
  MonotonicTimePoint tnow = MonotonicTimePoint::now();
  Sample s = { mode, tnow };
  ACE_thread_t key = ACE_OS::thr_self();
  bool force_update = false;
  try {
    ThreadDescriptor_rch& td = descs_.at(key);
    ACE_Guard<ACE_Thread_Mutex> g(td->queue_lock_);
    force_update = td->add_sample(s);
    if (force_update) {
      forced_ = td;
    }
  } catch (const std::out_of_range&) {
    LoadSamples samps;
    LoadHistory hist;
    ThreadDescriptor_rch td = make_rch<ThreadDescriptor>(alias, tnow);
    ACE_DEBUG((LM_DEBUG,"RTM::upddate creating new entry for alias %s\n", alias));
    td->add_sample(s);
    descs_.emplace(key, std::move(td));
  }
  if (force_update) {
    summarizer_.force_update();
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

double ThreadDescriptor::high_water_mark = 0.5;
double ThreadDescriptor::low_water_mark = 0.5;

ThreadDescriptor::ThreadDescriptor(const char* alias, MonotonicTimePoint tnow)
: alias_(alias)
, last_(tnow)
, current_busy_(0)
, current_idle_(0)
{
}

void ThreadDescriptor::reset_current()
{
  current_busy_ = current_idle_ = 0;
}

bool ThreadDescriptor::add_sample(Sample s)
{
  bool trigger_update = false;
  if (samples_.size() > 0) {
    auto prior = samples_.rbegin();
    ACE_UINT64 udiff = 0;
    TimeDuration tdiff = s.at_ - prior->at_;
    tdiff.value().to_usec(udiff);
    if (prior->mode_.idle_) {
      current_idle_ += udiff;
    } else {
      current_busy_ += udiff;
    }
    trigger_update = (current_busy_ > current_idle_);
  } else {
    auto prior = summaries_.rbegin();
    if (prior != summaries_.rend()) {
      ACE_UINT64 udiff = 0;
      TimeDuration tdiff = s.at_ - prior->recorded_;
      tdiff.value().to_usec(udiff);
      if (s.mode_.idle_) {
        current_busy_ = udiff;
      } else {
        current_idle_ = udiff;
      }
    }
  }
  samples_.emplace_back(std::move(s));
  return trigger_update;
}

void RelayThreadMonitor::summarize_thread(ThreadDescriptor_rch &td)
{
  MonotonicTimePoint tnow = MonotonicTimePoint::now();
  std::deque<Sample> local;
  {
    ACE_Guard<ACE_Thread_Mutex> g(td->queue_lock_);
    td->reset_current();
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

  double dbusy = static_cast<double>(ls.accum_busy_);
  double uspan = static_cast<double>(ls.accum_idle_ + ls.accum_busy_);
  if (uspan != 0.0) {
    double pbusy = dbusy/uspan;
    bool sat = td->saturated_;
    if (tsm_) {
      if (pbusy >= ThreadDescriptor::high_water_mark) {
        sat = true;
      } else if (pbusy <= ThreadDescriptor::low_water_mark) {
        sat = false;
      }
      td->saturated_ = sat;
      tsm_->update_busy(td->alias_.c_str(), pbusy, sat);
    } else {
      ACE_DEBUG((LM_DEBUG,"TLM::summarie_thread, cannot update thread %C, pbusy = %g\n", td->alias_.c_str(), pbusy));
      busy_map_[td->alias_.c_str()] = pbusy;
    }
  }

  if (td->summaries_.size() >= history_depth_) {
    td->summaries_.pop_front();
  }
  td->summaries_.emplace_back(std::move(ls));
}

void RelayThreadMonitor::summarize()
{
  if (forced_) {
    summarize_thread(forced_);
    forced_.reset();
  } else {
    for (auto d = descs_.begin(); d != descs_.end(); d++) {
      summarize_thread(d->second);
    }
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
, forced_(false)
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
  running_ = !period_.is_zero();
  condition_.notify_all();
  while (running_) {
    MonotonicTimePoint expire = MonotonicTimePoint::now() + period_;
    condition_.wait_until(expire);
    if (running_) {
      owner_.summarize();
      if (!forced_) {
        owner_.report();
      }
      forced_ = false;
    }
  }
  return 0;
}

void Summarizer::force_update()
{
  GuardType guoard(lock_);
  forced_ = true;
  condition_.notify_all();
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
  while (!running_ && !period_.is_zero()) {
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
