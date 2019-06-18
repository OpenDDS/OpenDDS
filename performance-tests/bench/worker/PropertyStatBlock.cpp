#include "PropertyStatBlock.h"

namespace Bench {

PropertyStatBlock::PropertyStatBlock(Builder::PropertySeq& seq, const std::string& prefix, size_t median_buffer_size)
{
  sample_count_ = get_or_create_property(seq, prefix + "_sample_count", Builder::PVK_ULL);
  sample_count_->value.ull_prop(0);

  min_ = get_or_create_property(seq, prefix + "_min", Builder::PVK_DOUBLE);
  min_->value.double_prop(std::numeric_limits<double>::max());

  max_ = get_or_create_property(seq, prefix + "_max", Builder::PVK_DOUBLE);
  max_->value.double_prop(std::numeric_limits<double>::min());

  mean_ = get_or_create_property(seq, prefix + "_mean", Builder::PVK_DOUBLE);
  mean_->value.double_prop(0.0);

  var_x_sample_count_ = get_or_create_property(seq, prefix + "_var_x_sample_count", Builder::PVK_DOUBLE);
  var_x_sample_count_->value.double_prop(0.0);

  median_buffer_.resize(median_buffer_size, 0.0);

  median_ = get_or_create_property(seq, prefix + "_median", Builder::PVK_DOUBLE);
  median_->value.double_prop(0.0);

  median_sample_count_ = get_or_create_property(seq, prefix + "_median_sample_count", Builder::PVK_ULL);
  median_sample_count_->value.ull_prop(0);
}

void PropertyStatBlock::update(double value)
{
  auto prev_mean = mean_->value.double_prop();
  auto prev_var_x_sample_count = var_x_sample_count_->value.double_prop();

  auto next_median_buffer_index = sample_count_->value.ull_prop() % median_buffer_.size();

  sample_count_->value.ull_prop(sample_count_->value.ull_prop() + 1);
  if (sample_count_->value.ull_prop() < median_buffer_.size()) {
    median_sample_count_->value.ull_prop(median_sample_count_->value.ull_prop() + 1);
  }

  if (value < min_->value.double_prop()) {
    min_->value.double_prop(value);
  }
  if (max_->value.double_prop() < value) {
    max_->value.double_prop(value);
  }
  if (sample_count_->value.ull_prop() == 0) {
    mean_->value.double_prop(value);
    var_x_sample_count_->value.double_prop(value);
  } else {
    // Incremental mean calculation (doesn't require storing all the data)
    mean_->value.double_prop(prev_mean + ((value - prev_mean) / static_cast<double>(sample_count_->value.ull_prop())));
    // Incremental (variance * sample_count) calculation (doesn't require storing all the data, can be used to easily find variance / standard deviation)
    var_x_sample_count_->value.double_prop(prev_var_x_sample_count + ((value - prev_mean) * (value - mean_->value.double_prop())));
  }

  median_buffer_[next_median_buffer_index] = value;
}

void PropertyStatBlock::write_median()
{
  size_t count = median_sample_count_->value.ull_prop();
  std::sort(&median_buffer_[0], &median_buffer_[count - 1]);
  double result = 0.0;
  if (count % 2) {
    result = (median_buffer_[count / 2] + median_buffer_[(count / 2) + 1]) / 2.0;
  } else {
    result = median_buffer_[(count / 2) + 1];
  }
  median_->value.double_prop(result);
}

}

