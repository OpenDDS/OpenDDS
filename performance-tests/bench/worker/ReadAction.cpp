#include "ReadAction.h"

#include "MemFunEvent.h"
#include "util.h"

#include "dds/DCPS/WaitSet.h"

namespace Bench {

ReadAction::ReadAction(OpenDDS::DCPS::EventDispatcher_rch event_dispatcher)
: event_dispatcher_(event_dispatcher)
, started_(false)
, stopped_(false)
, stop_condition_(new DDS::GuardCondition())
, dr_listener_(0)
, read_period_(1, 0)
, in_do_read_(false)
{
}

bool ReadAction::init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers,
  Builder::WriterMap& writers, const Builder::ContentFilteredTopicMap& cft_map)
{

  std::unique_lock<std::mutex> lock(mutex_);
  Action::init(config, report, readers, writers, cft_map);

  if (readers_by_index_.empty()) {
    std::stringstream ss;
    ss << "ReadAction '" << config.name << "' is missing a reader";
    throw std::runtime_error(ss.str());
  }

  data_dr_ = DataDataReader::_narrow(readers_by_index_[0]->get_dds_datareader());
  if (!data_dr_) {
    std::stringstream ss;
    ss << "ReadAction '" << config.name << "' is missing a valid Bench::Data datareader";
    throw std::runtime_error(ss.str());
  }

  dr_listener_ = dynamic_cast<WorkerDataReaderListener*>(readers_by_index_[0]->get_dds_datareaderlistener().in());

  std::string name(config.name.in());

  // First check frequency as double (seconds)
  const auto read_frequency_prop = get_property(config.params, "read_frequency", Builder::PVK_DOUBLE);
  if (read_frequency_prop) {
    double period = 1.0 / read_frequency_prop->value.double_prop();
    int64_t sec = static_cast<int64_t>(period);
    uint64_t usec = static_cast<uint64_t>((period - static_cast<double>(sec)) * 1000000u);
    read_period_ = ACE_Time_Value(sec, static_cast<suseconds_t>(usec));
  }

  // Then check period as double (seconds)
  auto read_period_prop = get_property(config.params, "read_period", Builder::PVK_DOUBLE);
  if (read_period_prop) {
    double period = read_period_prop->value.double_prop();
    int64_t sec = static_cast<int64_t>(period);
    uint64_t usec = static_cast<uint64_t>((period - static_cast<double>(sec)) * 1000000u);
    read_period_ = ACE_Time_Value(sec, static_cast<suseconds_t>(usec));
  }

  // Finally check period as TimeStamp
  read_period_prop = get_property(config.params, "read_period", Builder::PVK_TIME);
  if (read_period_prop) {
    read_period_ = ACE_Time_Value(read_period_prop->value.time_prop().sec, static_cast<suseconds_t>(read_period_prop->value.time_prop().nsec / 1000u));
  }

  event_ = OpenDDS::DCPS::make_rch<MemFunEvent<ReadAction> >(shared_from_this(), &ReadAction::do_read);

  return true;
}

void ReadAction::test_start()
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (!started_) {
    started_ = true;
    read_condition_ = data_dr_->create_readcondition(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    ws_ = new DDS::WaitSet();
    ws_->attach_condition(stop_condition_);
    ws_->attach_condition(read_condition_);
    event_dispatcher_->dispatch(event_);
  }
}

void ReadAction::test_stop()
{
}

void ReadAction::action_stop()
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    stopped_ = true;
    event_dispatcher_->cancel(event_);
    stop_condition_->set_trigger_value(true);
    while (in_do_read_) {
      cv_.wait(lock);
    }
    ws_->detach_condition(stop_condition_);
    ws_->detach_condition(read_condition_);
    data_dr_->delete_readcondition(read_condition_);
  }
}

namespace {

struct bool_guard {
  bool_guard(bool& val, std::condition_variable& cv) : val_(val), cv_(cv) { val_ = true; }
  ~bool_guard() { val_ = false; cv_.notify_all(); }
  bool& val_;
  std::condition_variable& cv_;
};

struct reverse_guard {
  explicit reverse_guard(std::unique_lock<std::mutex>& val) : val_(val) { val_.unlock(); }
  ~reverse_guard() { val_.lock(); }
  std::unique_lock<std::mutex>& val_;
};

}

void ReadAction::do_read()
{
  std::unique_lock<std::mutex> lock(mutex_);
  bool_guard bg(in_do_read_, cv_);
  if (started_ && !stopped_) {
    DDS::ConditionSeq active;
    const DDS::Duration_t duration = {static_cast<CORBA::Long>(read_period_.sec()), static_cast<CORBA::ULong>(read_period_.usec() * 1000)};
    DDS::WaitSet_var ws_copy= ws_;
    DDS::ReturnCode_t ret;

    {
      reverse_guard rg(lock);
      ret = ws_->wait(active, duration);
    }

    if (stopped_) {
      return;
    }

    if (ret == DDS::RETCODE_OK) {
      for (CORBA::ULong i = 0; !stopped_ && i < active.length(); ++i) {
        if (active[i] == read_condition_) {
          Bench::Data data;
          DDS::SampleInfo si;
          while (!stopped_ && (ret = data_dr_->take_next_sample(data, si)) == DDS::RETCODE_OK) {
            if (si.valid_data && dr_listener_) {
              reverse_guard rg(lock);
              dr_listener_->on_valid_data(data, si);
            }
          }
        }
      }
    }

    if (!stopped_) {
      event_dispatcher_->dispatch(event_);
    }
  }
}

}
