#include "SetCftParametersAction.h"

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

  SetCftParametersAction::SetCftParametersAction(ACE_Proactor& proactor)
 : proactor_(proactor), started_(false), stopped_(false)
 , write_period_(1, 0), max_count_(0), set_call_count_(0)
{
}

bool SetCftParametersAction::init(const ActionConfig& config, ActionReport& report,
  Builder::ReaderMap& readers, Builder::WriterMap& writers, const Builder::ContentFilteredTopicMap& cft_map) {

  std::unique_lock<std::mutex> lock(mutex_);
  Action::init(config, report, readers, writers, cft_map);

  if (!content_filtered_topic_) {
    std::stringstream ss;
    ss << "SetCftParametersAction '" << config.name << "' is missing a valid DDS::ContentFilteredTopic_var content_filtered_topic_" << std::flush;
    throw std::runtime_error(ss.str());
  }

  std::string name(config.name.in());

  std::random_device r;
  std::seed_seq seed{r(), one_at_a_time_hash(reinterpret_cast<const uint8_t*>(name.data()), name.size()), r(), r(), r(), r(), r(), r()};
  mt_ = std::mt19937_64(seed);

  size_t max_count = 0;
  auto max_count_prop = get_property(config.params, "max_count", Builder::PVK_ULL);
  if (max_count_prop) {
    max_count = static_cast<size_t>(max_count_prop->value.ull_prop());
  }
  max_count_ = max_count;

  size_t param_count = 0;
  auto param_count_prop = get_property(config.params, "param_count", Builder::PVK_ULL);
  if (param_count_prop) {
    param_count = static_cast<size_t>(param_count_prop->value.ull_prop());
  }
  param_count_ = param_count;

  if (!param_count_) {
    std::stringstream ss;
    ss << "SetCftParametersAction '" << config.name << "' is missing a valid param_count parameter" << std::flush;
    throw std::runtime_error(ss.str());
  }

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

  handler_.reset(new MemFunHandler<SetCftParametersAction>(&SetCftParametersAction::do_set_expression_parameters, *this));

  return true;
}

void SetCftParametersAction::start() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!started_) {
    started_ = true;
    proactor_.schedule_timer(*handler_, nullptr, ZERO, write_period_);
  }
}

void SetCftParametersAction::stop() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    stopped_ = true;
    proactor_.cancel_timer(*handler_);
  }
}

void SetCftParametersAction::do_set_expression_parameters() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    if (max_count_ == 0 || set_call_count_ < max_count_) {
      ++set_call_count_;

      DDS::StringSeq params(param_count_);
      params.length(param_count_);
      for (size_t i = 0; i < param_count_; i++) {
        int new_val = mt_();
        params[i] = std::to_string(new_val).c_str();
      }

      if (content_filtered_topic_->set_expression_parameters(params) != DDS::RETCODE_OK) {
        std::cout << "Error during SetCftParametersAction::do_set_expression_parameters()'s call to set_expression_parameters()" << std::endl;
      }
    }
  }
}

}

