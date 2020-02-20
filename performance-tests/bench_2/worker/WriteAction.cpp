#include "WriteAction.h"

#include "MemFunHandler.h"

namespace {

uint32_t one_at_a_time_hash(const uint8_t* key, size_t length) {
  size_t i = 0;
  uint32_t hash = 0;
  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

const ACE_Time_Value ZERO(0, 0);

}

namespace Bench {

WriteAction::WriteAction(ACE_Proactor& proactor)
 : proactor_(proactor), started_(false), stopped_(false)
 , write_period_(1, 0), max_count_(0), new_key_count_(0), new_key_probability_(0)
{
}

bool WriteAction::init(const ActionConfig& config, ActionReport& report, Builder::ReaderMap& readers, Builder::WriterMap& writers) {

  std::unique_lock<std::mutex> lock(mutex_);
  Action::init(config, report, readers, writers);

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
    data_buffer_bytes = data_buffer_bytes_prop->value.ull_prop();
  }
  data_.buffer.length(data_buffer_bytes);

  size_t max_count = 0;
  auto max_count_prop = get_property(config.params, "max_count", Builder::PVK_ULL);
  if (max_count_prop) {
    max_count = max_count_prop->value.ull_prop();
  }
  max_count_ = max_count;

  size_t new_key_count = 0;
  auto new_key_count_prop = get_property(config.params, "new_key_count", Builder::PVK_ULL);
  if (new_key_count_prop) {
    new_key_count = new_key_count_prop->value.ull_prop();
  }
  new_key_count_ = new_key_count;

  uint64_t new_key_probability = 0;
  auto new_key_probability_prop = get_property(config.params, "new_key_probability", Builder::PVK_DOUBLE);
  if (new_key_probability_prop) {
    new_key_probability = new_key_probability_prop->value.double_prop();
  }
  new_key_probability_ = static_cast<uint64_t>(static_cast<double>(std::numeric_limits<uint64_t>::max()) * new_key_probability);

  data_.msg_count = 0;

  size_t total_hops = 0;
  auto total_hops_prop = get_property(config.params, "total_hops", Builder::PVK_ULL);
  if (total_hops_prop) {
    total_hops = total_hops_prop->value.ull_prop();
  }
  data_.total_hops = total_hops;

  size_t hop_count = 1;
  auto hop_count_prop = get_property(config.params, "hop_count", Builder::PVK_ULL);
  if (hop_count_prop) {
    hop_count = hop_count_prop->value.ull_prop();
  }
  data_.hop_count = hop_count;

  // First check frequency as double (seconds)
  auto write_frequency_prop = get_property(config.params, "write_frequency", Builder::PVK_DOUBLE);
  if (write_frequency_prop) {
    double period = 1.0 / write_frequency_prop->value.double_prop();
    int64_t sec = static_cast<int64_t>(period);
    uint64_t usec = static_cast<uint64_t>((period - static_cast<double>(sec)) * 1e6);
    write_period_ = ACE_Time_Value(sec, usec);
  }

  // Then check period as double (seconds)
  auto write_period_prop = get_property(config.params, "write_period", Builder::PVK_DOUBLE);
  if (write_period_prop) {
    double period = write_period_prop->value.double_prop();
    int64_t sec = static_cast<int64_t>(period);
    uint64_t usec = static_cast<uint64_t>((period - static_cast<double>(sec)) * 1e6);
    write_period_ = ACE_Time_Value(sec, usec);
  }

  // Finally check period as TimeStamp
  write_period_prop = get_property(config.params, "write_period", Builder::PVK_TIME);
  if (write_period_prop) {
    write_period_ = ACE_Time_Value(write_period_prop->value.time_prop().sec, write_period_prop->value.time_prop().nsec / 1e3);
  }

  handler_.reset(new MemFunHandler<WriteAction>(&WriteAction::do_write, *this));

  return true;
}

void WriteAction::start() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!started_) {
    instance_ = data_dw_->register_instance(data_);
    started_ = true;
    proactor_.schedule_timer(*handler_, nullptr, ZERO, write_period_);
  }
}

void WriteAction::stop() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    stopped_ = true;
    proactor_.cancel_timer(*handler_);
    const DDS::Duration_t timeout = { 0, 0 };
    data_dw_->wait_for_acknowledgments(timeout);
    data_dw_->unregister_instance(data_, instance_);
    data_dw_->wait_for_acknowledgments(timeout);
  }
}

void WriteAction::do_write() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    if (max_count_ == 0 || data_.msg_count < max_count_) {
      ++(data_.msg_count);
      if ((new_key_count_ != 0 && (data_.msg_count % new_key_count_) == 0)
          || (new_key_probability_ != 0 && mt_() <= new_key_probability_)) {
        data_.id.high = mt_();
        data_.id.low = mt_();
      }
      data_.created_time = data_.sent_time = Builder::get_time();
      DDS::ReturnCode_t result = data_dw_->write(data_, 0);
      if (result != DDS::RETCODE_OK) {
        --(data_.msg_count);
        std::cout << "Error during WriteAction::do_write()'s call to datawriter::write()" << std::endl;
      }
    }
  }
}

}

