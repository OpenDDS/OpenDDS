#include "SetCftParametersAction.h"

#include "MemFunHandler.h"
#include "util.h"

namespace Bench {

SetCftParametersAction::SetCftParametersAction(ACE_Proactor& proactor)
: proactor_(proactor)
, started_(false)
, stopped_(false)
, set_period_(1, 0)
, max_count_(0)
, param_count_(0)
, random_order_(false)
, instance_(0)
, set_call_count_(0)
{
}

bool SetCftParametersAction::init(const ActionConfig& config, ActionReport& report,
  Builder::ReaderMap& readers, Builder::WriterMap& writers, const Builder::ContentFilteredTopicMap& cft_map)
{

  std::unique_lock<std::mutex> lock(mutex_);
  Action::init(config, report, readers, writers, cft_map);

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  auto content_filtered_topic_prop = get_property(config.params, "content_filtered_topic_name", Builder::PVK_STRING);
  if (content_filtered_topic_prop) {
    auto it = cft_map.find(static_cast<std::string>(content_filtered_topic_prop->value.string_prop()));
    if (it != cft_map.end()) {
      content_filtered_topic_ = it->second;
    }
  }

  if (!content_filtered_topic_) {
    std::stringstream ss;
    ss << "SetCftParametersAction '" << config.name << "' is missing a valid DDS::ContentFilteredTopic_var content_filtered_topic_" << std::flush;
    throw std::runtime_error(ss.str());
  }
#endif

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

  auto random_order_prop = get_property(config.params, "random_order", Builder::PVK_ULL);
  if (random_order_prop) {
    random_order_ = static_cast<bool>(random_order_prop->value.ull_prop());
  }

  // Acceptable parameter values
  auto acceptable_param_values_prop = get_property(config.params, "acceptable_param_values", Builder::PVK_STRING_SEQ_SEQ);
  if (acceptable_param_values_prop) {
    acceptable_param_values_ = static_cast<Builder::StringSeqSeq>(acceptable_param_values_prop->value.string_seq_seq_prop());
  }

  if (acceptable_param_values_.length() == 0) {
    std::stringstream ss;
    ss << "SetCftParametersAction '" << config.name << "' is missing a valid acceptable_param_values parameter" << std::flush;
    throw std::runtime_error(ss.str());
  }

  for (size_t i = 0; i < acceptable_param_values_.length(); i++) {
    current_acceptable_param_values_index_.push_back(0);
  }

  // Set period
  // First check frequency as double (seconds)
  auto set_frequency_prop = get_property(config.params, "set_frequency", Builder::PVK_DOUBLE);
  if (set_frequency_prop) {
    double period = 1.0 / set_frequency_prop->value.double_prop();
    int64_t sec = static_cast<int64_t>(period);
    uint64_t usec = static_cast<uint64_t>((period - static_cast<double>(sec)) * 1000000u);
    set_period_ = ACE_Time_Value(sec, static_cast<suseconds_t>(usec));
  }

  // Then check period as double (seconds)
  auto set_period_prop = get_property(config.params, "set_period", Builder::PVK_DOUBLE);
  if (set_period_prop) {
    double period = set_period_prop->value.double_prop();
    int64_t sec = static_cast<int64_t>(period);
    uint64_t usec = static_cast<uint64_t>((period - static_cast<double>(sec)) * 1000000u);
    set_period_ = ACE_Time_Value(sec, static_cast<suseconds_t>(usec));
  }

  // Finally check period as TimeStamp
  set_period_prop = get_property(config.params, "set_period", Builder::PVK_TIME);
  if (set_period_prop) {
    set_period_ = ACE_Time_Value(set_period_prop->value.time_prop().sec, static_cast<suseconds_t>(set_period_prop->value.time_prop().nsec / 1000u));
  }

  handler_.reset(new MemFunHandler<SetCftParametersAction>(&SetCftParametersAction::do_set_expression_parameters, *this));

  return true;
}

void SetCftParametersAction::test_start()
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (!started_) {
    started_ = true;
    proactor_.schedule_timer(*handler_, nullptr, ZERO_TIME, set_period_);
  }
}

void SetCftParametersAction::test_stop()
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    stopped_ = true;
    proactor_.cancel_timer(*handler_);
  }
}

void SetCftParametersAction::do_set_expression_parameters()
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_ && !stopped_) {
    if (max_count_ == 0 || set_call_count_ < max_count_) {
      ++set_call_count_;

      DDS::StringSeq params;
      params.length(static_cast<CORBA::ULong>(param_count_));
      for (CORBA::ULong i = 0; i < params.length(); ++i) {
        const int index = random_order_ ? mt_() % acceptable_param_values_[i].length() : current_acceptable_param_values_index_[i]++;
        params[i] = acceptable_param_values_[i][index];
      }

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
      if (content_filtered_topic_->set_expression_parameters(params) != DDS::RETCODE_OK) {
        std::cout << "Error during SetCftParametersAction::do_set_expression_parameters()'s call to set_expression_parameters()" << std::endl;
      }
#endif
    }
  }
}

}
