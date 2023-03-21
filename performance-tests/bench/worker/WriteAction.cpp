#include "WriteAction.h"

#include "MemFunHandler.h"
#include "util.h"

namespace Bench {

WriteAction::WriteAction(ACE_Proactor& proactor)
: proactor_(proactor)
, started_(false)
, stopped_(false)
, manual_rescheduling_(false)
, write_period_(1, 0)
, last_scheduled_time_(0, 0)
, max_count_(0)
, new_key_count_(0)
, new_key_probability_(0)
, instance_(0)
, filter_class_start_value_(0)
, filter_class_stop_value_(0)
, filter_class_increment_(0)
, final_wait_for_ack_{DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC}
{
}

bool WriteAction::init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers,
  Builder::WriterMap& writers, const Builder::ContentFilteredTopicMap& cft_map)
{

  std::unique_lock<std::mutex> lock(mutex_);
  Action::init(config, report, readers, writers, cft_map);

  if (writers_by_index_.empty()) {
    std::stringstream ss;
    ss << "WriteAction '" << config.name << "' is missing a writer" << std::flush;
    throw std::runtime_error(ss.str());
  }

  data_dw_ = DataDataWriter::_narrow(writers_by_index_[0]->get_dds_datawriter());
  if (!data_dw_) {
    std::stringstream ss;
    ss << "WriteAction '" << config.name << "' is missing a valid Bench::Data datawriter" << std::flush;
    throw std::runtime_error(ss.str());
  }

  std::string name(config.name.in());

  std::random_device r;
  std::seed_seq seed{r(), one_at_a_time_hash(reinterpret_cast<const uint8_t*>(name.data()), name.size()), r(), r(), r(), r(), r(), r()};
  mt_ = std::mt19937_64(seed);

  data_.id.high = mt_();
  data_.id.low = mt_();

  size_t data_buffer_bytes = 256;
  auto data_buffer_bytes_prop = get_property(config.params, "data_buffer_bytes", Builder::PVK_ULL);
  if (data_buffer_bytes_prop) {
    data_buffer_bytes = static_cast<size_t>(data_buffer_bytes_prop->value.ull_prop());
  }
  data_.buffer.length(static_cast<CORBA::ULong>(data_buffer_bytes));

  size_t max_count = 0;
  auto max_count_prop = get_property(config.params, "max_count", Builder::PVK_ULL);
  if (max_count_prop) {
    max_count = static_cast<size_t>(max_count_prop->value.ull_prop());
  }
  max_count_ = max_count;

  size_t new_key_count = 0;
  auto new_key_count_prop = get_property(config.params, "new_key_count", Builder::PVK_ULL);
  if (new_key_count_prop) {
    new_key_count = static_cast<size_t>(new_key_count_prop->value.ull_prop());
  }
  new_key_count_ = new_key_count;

  bool relative_scheduling = false;
  auto manual_rescheduling_prop = get_property(config.params, "relative_scheduling", Builder::PVK_ULL);
  if (manual_rescheduling_prop) {
    relative_scheduling = manual_rescheduling_prop->value.ull_prop() != 0u;
  }
  manual_rescheduling_ = relative_scheduling;

  double new_key_probability = 0.0;
  auto new_key_probability_prop = get_property(config.params, "new_key_probability", Builder::PVK_DOUBLE);
  if (new_key_probability_prop) {
    new_key_probability = new_key_probability_prop->value.double_prop();
  }
  new_key_probability_ = static_cast<uint64_t>(static_cast<double>(std::numeric_limits<uint64_t>::max()) * new_key_probability);

  data_.msg_count = 0;

  CORBA::ULongLong total_hops = 0;
  auto total_hops_prop = get_property(config.params, "total_hops", Builder::PVK_ULL);
  if (total_hops_prop) {
    total_hops = total_hops_prop->value.ull_prop();
  }
  data_.total_hops = static_cast<CORBA::ULong>(total_hops);

  CORBA::ULongLong hop_count = 1;
  auto hop_count_prop = get_property(config.params, "hop_count", Builder::PVK_ULL);
  if (hop_count_prop) {
    hop_count = hop_count_prop->value.ull_prop();
  }
  data_.hop_count = static_cast<CORBA::ULong>(hop_count);

  // First check frequency as double (seconds)
  auto write_frequency_prop = get_property(config.params, "write_frequency", Builder::PVK_DOUBLE);
  if (write_frequency_prop) {
    double period = 1.0 / write_frequency_prop->value.double_prop();
    int64_t sec = static_cast<int64_t>(period);
    uint64_t usec = static_cast<uint64_t>((period - static_cast<double>(sec)) * 1000000u);
    write_period_ = ACE_Time_Value(sec, static_cast<suseconds_t>(usec));
  }

  // Then check period as double (seconds)
  auto write_period_prop = get_property(config.params, "write_period", Builder::PVK_DOUBLE);
  if (write_period_prop) {
    double period = write_period_prop->value.double_prop();
    int64_t sec = static_cast<int64_t>(period);
    uint64_t usec = static_cast<uint64_t>((period - static_cast<double>(sec)) * 1000000u);
    write_period_ = ACE_Time_Value(sec, static_cast<suseconds_t>(usec));
  }

  // Finally check period as TimeStamp
  write_period_prop = get_property(config.params, "write_period", Builder::PVK_TIME);
  if (write_period_prop) {
    write_period_ = ACE_Time_Value(write_period_prop->value.time_prop().sec, static_cast<suseconds_t>(write_period_prop->value.time_prop().nsec / 1000u));
  }

  auto final_wait_for_ack_prop = get_property(config.params, "final_wait_for_ack", Builder::PVK_DOUBLE);
  if (final_wait_for_ack_prop) {
    double period = final_wait_for_ack_prop->value.double_prop();
    int64_t sec = static_cast<int64_t>(period);
    uint64_t nsec = static_cast<uint64_t>((period - static_cast<double>(sec)) * 1000000000u);
    final_wait_for_ack_ = {static_cast<CORBA::Long>(sec), static_cast<CORBA::ULong>(nsec)};
  }

  // Filter class parameters
  size_t filter_class_start_value = 0;
  auto filter_class_start_value_prop = get_property(config.params, "filter_class_start_value", Builder::PVK_ULL);
  if (filter_class_start_value_prop) {
    filter_class_start_value = static_cast<size_t>(filter_class_start_value_prop->value.ull_prop());
  }
  filter_class_start_value_ = filter_class_start_value;

  size_t filter_class_stop_value = 0;
  auto filter_class_stop_value_prop = get_property(config.params, "filter_class_stop_value", Builder::PVK_ULL);
  if (filter_class_stop_value_prop) {
    filter_class_stop_value = static_cast<size_t>(filter_class_stop_value_prop->value.ull_prop());
  }
  filter_class_stop_value_ = filter_class_stop_value;

  size_t filter_class_increment = 0;
  auto filter_class_increment_prop = get_property(config.params, "filter_class_increment", Builder::PVK_ULL);
  if (filter_class_increment_prop) {
    filter_class_increment = static_cast<size_t>(filter_class_increment_prop->value.ull_prop());
  }
  filter_class_increment_ = filter_class_increment;

  data_.filter_class = static_cast<CORBA::ULong>(filter_class_start_value_);

  handler_.reset(new MemFunHandler<WriteAction>(&WriteAction::do_write, *this));

  return true;
}

void WriteAction::test_start()
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (!started_) {
    instance_ = data_dw_->register_instance(data_);
    started_ = true;
    if (manual_rescheduling_) {
      const auto now = Builder::get_hr_time();
      last_scheduled_time_ = ACE_Time_Value(now.sec, static_cast<suseconds_t>(now.nsec / 1000u));
      proactor_.schedule_timer(*handler_, nullptr, ZERO_TIME, ZERO_TIME);
    } else {
      proactor_.schedule_timer(*handler_, nullptr, ZERO_TIME, write_period_);
    }
  }
}

void WriteAction::test_stop()
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    stopped_ = true;
    proactor_.cancel_timer(*handler_);
    data_dw_->wait_for_acknowledgments(final_wait_for_ack_);
    data_dw_->unregister_instance(data_, instance_);
    data_dw_->wait_for_acknowledgments(final_wait_for_ack_);
  }
}

void WriteAction::do_write()
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    if (max_count_ == 0 || data_.msg_count < max_count_) {
      ++(data_.msg_count);
      if ((new_key_count_ != 0 && (data_.msg_count % new_key_count_) == 0)
          || (new_key_probability_ != 0 && mt_() <= new_key_probability_)) {
        data_.id.high = mt_();
        data_.id.low = mt_();
      }

      data_.created_time = data_.sent_time = Builder::get_sys_time();
      const DDS::ReturnCode_t result = data_dw_->write(data_, 0);
      if (result != DDS::RETCODE_OK) {
        --(data_.msg_count);
        std::cout << "Error during WriteAction::do_write()'s call to datawriter::write()" << std::endl;
      } else {
        if ((data_.filter_class += static_cast<CORBA::ULong>(filter_class_increment_)) > static_cast<CORBA::ULong>(filter_class_stop_value_)) {
          data_.filter_class = static_cast<CORBA::ULong>(filter_class_start_value_);
        }
      }

      if (manual_rescheduling_) {
        const auto now = Builder::get_hr_time();
        const ACE_Time_Value atv_now(now.sec, static_cast<suseconds_t>(now.nsec / 1000u));
        const ACE_Time_Value diff = atv_now - last_scheduled_time_;

        if (diff < write_period_) {
          const ACE_Time_Value remaining = write_period_ - diff;
          last_scheduled_time_ = atv_now + remaining;
          proactor_.schedule_timer(*handler_, nullptr, remaining, ZERO_TIME);
        } else {
          last_scheduled_time_ = atv_now;
          proactor_.schedule_timer(*handler_, nullptr, ZERO_TIME, ZERO_TIME);
        }
      }
    }
  }
}

}
