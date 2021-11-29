#include "ReadAction.h"

#include "MemFunHandler.h"
#include "util.h"

#include "dds/DCPS/WaitSet.h"

namespace Bench {

ReadAction::ReadAction(ACE_Proactor& proactor)
: proactor_(proactor)
, started_(false)
, stopped_(false)
, stop_condition_(new DDS::GuardCondition())
, dr_listener_(0)
, read_period_(1, 0)
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

  handler_.reset(new MemFunHandler<ReadAction>(&ReadAction::do_read, *this));

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
    proactor_.schedule_timer(*handler_, nullptr, ZERO_TIME, ZERO_TIME);
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
    proactor_.cancel_timer(*handler_);
    ws_->detach_condition(stop_condition_);
    ws_->detach_condition(read_condition_);
    data_dr_->delete_readcondition(read_condition_);
    stop_condition_->set_trigger_value(true);
  }
}

void ReadAction::do_read()
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    DDS::ConditionSeq active;
    const DDS::Duration_t duration = {static_cast<CORBA::Long>(read_period_.sec()), static_cast<CORBA::ULong>(read_period_.usec() * 1000)};
    DDS::ReturnCode_t ret = ws_->wait(active, duration);

    if (stopped_) {
      return;
    }

    if (ret == DDS::RETCODE_OK) {
      for (CORBA::ULong i = 0; i < active.length(); ++i) {
        if (active[i] == read_condition_) {
          Bench::Data data;
          DDS::SampleInfo si;
          while ((ret = data_dr_->take_next_sample(data, si)) == DDS::RETCODE_OK) {
            if (si.valid_data && dr_listener_) {
              dr_listener_->on_valid_data(data, si);
            }
          }
        }
      }
    }

    proactor_.schedule_timer(*handler_, nullptr, ZERO_TIME, ZERO_TIME);
  }
}

}
