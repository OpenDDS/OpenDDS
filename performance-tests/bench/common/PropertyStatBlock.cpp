#include "PropertyStatBlock.h"

#include <cctype>
#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>

namespace Bench {

SimpleStatBlock::SimpleStatBlock()
 : sample_count_(0)
 , min_(std::numeric_limits<double>::max())
 , max_(std::numeric_limits<double>::lowest())
 , mean_(0.0)
 , var_x_sample_count_(0.0)
 , median_buffer_()
 , timestamp_buffer_()
 , median_sample_count_(0)
 , median_sample_overflow_(0)
 , median_(0.0)
 , median_absolute_deviation_(0.0)
{
}

namespace {
char my_toupper(char ch)
{
  return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
}
}

void SimpleStatBlock::pretty_print(std::ostream& os, const std::string& name, const std::string& indent, size_t indent_level) const
{
  std::string i1, i2;
  for (size_t i = 0; i < indent_level; ++i) {
    i1 += indent;
  }
  i2 = i1 + indent;

  std::string uname = name;
  for (size_t i = 0; i < uname.size(); ++i) {
    if (i == 0 || uname[i - 1] == ' ') {
      uname[i] = my_toupper(uname[i]);
    }
  }

  os << i1 << uname << " Statistics:" << std::endl;
  const size_t my_w = sample_count_ ? (median_sample_overflow_ ? 10 : 7) : 5;
  os << i2 << name << std::setw(my_w) << std::setfill(' ') << " count" << " = " << sample_count_ << std::endl;
  if (sample_count_) {
    os << i2 << name << std::setw(my_w) << std::setfill(' ') << " min" << " = " << std::fixed << std::setprecision(6) << min_ << std::endl;
    os << i2 << name << std::setw(my_w) << std::setfill(' ') << " max" << " = " << std::fixed << std::setprecision(6) << max_ << std::endl;
    os << i2 << name << std::setw(my_w) << std::setfill(' ') << " mean" << " = " << std::fixed << std::setprecision(6) << mean_ << std::endl;
    const double stdev = sample_count_ ? std::sqrt(var_x_sample_count_ / static_cast<double>(sample_count_)) : 0.0;
    os << i2 << name << std::setw(my_w) << std::setfill(' ') << " stdev" << " = " << std::fixed << std::setprecision(6) << stdev << std::endl;
    os << i2 << name << std::setw(my_w) << std::setfill(' ') << " median" << " = " << std::fixed << std::setprecision(6) << median_ << std::endl;
    os << i2 << name << std::setw(my_w) << std::setfill(' ') << " madev" << " = " << std::fixed << std::setprecision(6) << median_absolute_deviation_ << std::endl;
    if (median_sample_overflow_) {
      os << i2 << name << std::setw(my_w) << std::setfill(' ') << " overflow" << " = " << median_sample_overflow_ << std::endl;
    }
  }
}

void SimpleStatBlock::to_json_summary(const std::string& name, rapidjson::Value& dst, rapidjson::Value::AllocatorType& alloc) const
{
  rapidjson::Value& stat_val = dst.AddMember(rapidjson::StringRef(name.c_str()), rapidjson::Value(0).Move(), alloc)[name.c_str()].SetObject();
  stat_val.AddMember("sample_count", rapidjson::Value(static_cast<uint64_t>(sample_count_)).Move(), alloc);
  if (sample_count_) {
    stat_val.AddMember("min", rapidjson::Value(min_).Move(), alloc);
    stat_val.AddMember("max", rapidjson::Value(max_).Move(), alloc);
    stat_val.AddMember("mean", rapidjson::Value(mean_).Move(), alloc);
    const double stdev = sample_count_ ? std::sqrt(var_x_sample_count_ / static_cast<double>(sample_count_)) : 0.0;
    stat_val.AddMember("stdev", rapidjson::Value(stdev).Move(), alloc);
    stat_val.AddMember("median", rapidjson::Value(median_).Move(), alloc);
    stat_val.AddMember("madev", rapidjson::Value(median_absolute_deviation_).Move(), alloc);
    stat_val.AddMember("median_sample_count", rapidjson::Value(static_cast<uint64_t>(median_sample_count_)).Move(), alloc);
    stat_val.AddMember("median_sample_overflow", rapidjson::Value(static_cast<uint64_t>(median_sample_overflow_)).Move(), alloc);
  }
}

SimpleStatBlock consolidate(const SimpleStatBlock& sb1, const SimpleStatBlock& sb2)
{
  SimpleStatBlock result;

  result.sample_count_ = sb1.sample_count_ + sb2.sample_count_;
  result.min_ = std::min(sb1.min_, sb2.min_);
  result.max_ = std::max(sb1.max_, sb2.max_);
  if (result.sample_count_ > 0) {
    result.mean_ = (sb1.mean_ * static_cast<double>(sb1.sample_count_) + sb2.mean_ * static_cast<double>(sb2.sample_count_)) / result.sample_count_;
  }
  result.var_x_sample_count_ = sb1.var_x_sample_count_ + sb2.var_x_sample_count_;

  OPENDDS_ASSERT(sb1.median_sample_count_ <= sb1.median_buffer_.size());
  OPENDDS_ASSERT(sb1.timestamp_buffer_.size() == 0 || sb1.median_sample_count_ <= sb1.timestamp_buffer_.size());
  OPENDDS_ASSERT(sb1.timestamp_buffer_.size() == 0 || sb1.median_buffer_.size() == sb1.timestamp_buffer_.size());
  OPENDDS_ASSERT(sb2.median_sample_count_ <= sb2.median_buffer_.size());
  OPENDDS_ASSERT(sb2.timestamp_buffer_.size() == 0 || sb2.median_sample_count_ <= sb2.timestamp_buffer_.size());
  OPENDDS_ASSERT(sb2.timestamp_buffer_.size() == 0 || sb2.median_buffer_.size() == sb2.timestamp_buffer_.size());

  // Consolidate median buffers (no need to include unused / invalid values beyond median sample count)
  result.median_sample_count_ = sb1.median_sample_count_ + sb2.median_sample_count_;
  result.median_buffer_.resize(result.median_sample_count_);
  if (sb1.median_sample_count_) {
    memcpy(&result.median_buffer_[0], &sb1.median_buffer_[0], sb1.median_sample_count_ * sizeof (double));
  }
  if (sb2.median_sample_count_) {
    memcpy(&result.median_buffer_[sb1.median_sample_count_], &sb2.median_buffer_[0], sb2.median_sample_count_ * sizeof (double));
  }
  result.median_sample_overflow_ = result.sample_count_ - result.median_sample_count_;

  // Consolidate timestamp buffers
  result.timestamp_buffer_.resize(result.median_sample_count_);
  if (sb1.timestamp_buffer_.size()) {
    memcpy(&result.timestamp_buffer_[0], &sb1.timestamp_buffer_[0], sb1.median_sample_count_ * sizeof (Builder::TimeStamp));
  } else if (sb1.median_sample_count_) {
    memset(&result.timestamp_buffer_[0], 0, sb1.median_sample_count_ * sizeof (Builder::TimeStamp));
  }
  if (sb2.timestamp_buffer_.size()) {
    memcpy(&result.timestamp_buffer_[sb1.median_sample_count_], &sb2.timestamp_buffer_[0], sb2.median_sample_count_ * sizeof (Builder::TimeStamp));
  } else if (sb2.median_sample_count_) {
    memset(&result.timestamp_buffer_[sb1.median_sample_count_], 0, sb2.median_sample_count_ * sizeof (Builder::TimeStamp));
  }

  if (result.median_sample_count_) {
    // Calculate consolidated median from consolidated median buffer
    {
      std::vector<double> median_buffer = result.median_buffer_;
      std::sort(&median_buffer[0], &median_buffer[0] + result.median_sample_count_);
      if (result.median_sample_count_ % 2 == 0) {
        // even, but not zero
        result.median_ = (median_buffer[(result.median_sample_count_ / 2) - 1] + median_buffer[result.median_sample_count_ / 2]) / 2.0;
      } else {
        // odd
        result.median_ = median_buffer[result.median_sample_count_ / 2];
      }
    }

    // Calculate consolidated median absolute deviation from consolidated median buffer
    {
      std::vector<double> mad_buffer = result.median_buffer_;
      for (size_t i = 0; i < mad_buffer.size(); ++i) {
        mad_buffer[i] = fabs(mad_buffer[i] - result.median_);
      }
      std::sort(&mad_buffer[0], &mad_buffer[0] + result.median_sample_count_);
      if (result.median_sample_count_ % 2 == 0) {
        result.median_absolute_deviation_ = (mad_buffer[(result.median_sample_count_ / 2) - 1] + mad_buffer[result.median_sample_count_ / 2]) / 2.0;
      } else {
        result.median_absolute_deviation_ = mad_buffer[result.median_sample_count_ / 2];
      }
    }
  }

  return result;
}

SimpleStatBlock consolidate(const std::vector<SimpleStatBlock>& vec)
{
  SimpleStatBlock result;

  // Do this in two passes: normal stats and then buffers
  // This is an optimization, since incremental median calculations (from buffers) are wasteful

  for (auto it = vec.begin(); it != vec.end(); ++it) {
    size_t old_sample_count = result.sample_count_;
    result.sample_count_ += it->sample_count_;
    if (it == vec.begin()) {
      result.min_ = it->min_;
      result.max_ = it->max_;
      result.mean_ = it->mean_;
      result.var_x_sample_count_ = it->var_x_sample_count_;
      result.median_sample_count_ = it->median_sample_count_;
    } else {
      result.min_ = std::min(result.min_, it->min_);
      result.max_ = std::max(result.max_, it->max_);
      if (result.sample_count_ > 0) {
        result.mean_ = (result.mean_ * static_cast<double>(old_sample_count) + it->mean_ * static_cast<double>(it->sample_count_)) / result.sample_count_;
      }
      result.var_x_sample_count_ += it->var_x_sample_count_;
      result.median_sample_count_ += it->median_sample_count_;
    }
  }

  result.median_buffer_.resize(result.median_sample_count_);
  result.median_sample_overflow_ = result.sample_count_ - result.median_sample_count_;

  result.timestamp_buffer_.resize(result.median_sample_count_);

  size_t median_buffer_pos = 0;
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    OPENDDS_ASSERT(it->median_sample_count_ <= it->median_buffer_.size());
    OPENDDS_ASSERT(it->timestamp_buffer_.size() == 0 || it->median_sample_count_ <= it->timestamp_buffer_.size());
    OPENDDS_ASSERT(it->timestamp_buffer_.size() == 0 || it->median_buffer_.size() == it->timestamp_buffer_.size());

    // Consolidate median buffers (no need to include unused / invalid values beyond median sample count)
    memcpy(&result.median_buffer_[median_buffer_pos], &it->median_buffer_[0], it->median_sample_count_ * sizeof (double));
    if (it->timestamp_buffer_.size()) {
      memcpy(&result.timestamp_buffer_[median_buffer_pos], &it->timestamp_buffer_[0], it->median_sample_count_ * sizeof (Builder::TimeStamp));
    } else {
      memset(&result.timestamp_buffer_[median_buffer_pos], 0, it->median_sample_count_ * sizeof (Builder::TimeStamp));
    }
    median_buffer_pos += it->median_sample_count_;
  }

  if (result.median_sample_count_) {
    // Calculate consolidated median from consolidated median buffer
    {
      std::vector<double> median_buffer = result.median_buffer_;
      std::sort(&median_buffer[0], &median_buffer[0] + result.median_sample_count_);
      if (result.median_sample_count_ % 2 == 0) {
        // even, but not zero
        result.median_ = (median_buffer[(result.median_sample_count_ / 2) - 1] + median_buffer[result.median_sample_count_ / 2]) / 2.0;
      } else {
        // odd
        result.median_ = median_buffer[result.median_sample_count_ / 2];
      }
    }

    // Calculate consolidated median absolute deviation from consolidated median buffer
    {
      std::vector<double> mad_buffer = result.median_buffer_;
      for (size_t i = 0; i < mad_buffer.size(); ++i) {
        mad_buffer[i] = fabs(mad_buffer[i] - result.median_);
      }
      std::sort(&mad_buffer[0], &mad_buffer[0] + result.median_sample_count_);
      if (result.median_sample_count_ % 2 == 0) {
        result.median_absolute_deviation_ = (mad_buffer[(result.median_sample_count_ / 2) - 1] + mad_buffer[result.median_sample_count_ / 2]) / 2.0;
      } else {
        result.median_absolute_deviation_ = mad_buffer[result.median_sample_count_ / 2];
      }
    }
  }

  return result;
}

PropertyStatBlock::PropertyStatBlock(Builder::PropertySeq& seq, const std::string& prefix, size_t median_buffer_size, bool timestamps)
{
  sample_count_ = get_or_create_property(seq, prefix + "_sample_count", Builder::PVK_ULL);
  sample_count_->value.ull_prop(0);

  min_ = get_or_create_property(seq, prefix + "_min", Builder::PVK_DOUBLE);
  min_->value.double_prop(std::numeric_limits<double>::max());

  max_ = get_or_create_property(seq, prefix + "_max", Builder::PVK_DOUBLE);
  max_->value.double_prop(std::numeric_limits<double>::lowest());

  mean_ = get_or_create_property(seq, prefix + "_mean", Builder::PVK_DOUBLE);
  mean_->value.double_prop(0.0);

  var_x_sample_count_ = get_or_create_property(seq, prefix + "_var_x_sample_count", Builder::PVK_DOUBLE);
  var_x_sample_count_->value.double_prop(0.0);

  median_buffer_.resize(median_buffer_size, 0.0);
  if (timestamps) {
    timestamp_buffer_.resize(median_buffer_size, Builder::ZERO);
  }

  median_sample_count_ = get_or_create_property(seq, prefix + "_median_sample_count", Builder::PVK_ULL);
  median_sample_count_->value.ull_prop(0);

  median_ = get_or_create_property(seq, prefix + "_median", Builder::PVK_DOUBLE);
  median_->value.double_prop(0.0);

  median_absolute_deviation_ = get_or_create_property(seq, prefix + "_median_absolute_deviation", Builder::PVK_DOUBLE);
  median_absolute_deviation_->value.double_prop(0.0);
}

void PropertyStatBlock::update(double value, const Builder::TimeStamp& time)
{
  auto prev_mean = mean_->value.double_prop();
  auto prev_var_x_sample_count = var_x_sample_count_->value.double_prop();

  size_t next_median_buffer_index = static_cast<size_t>(sample_count_->value.ull_prop() % median_buffer_.size());

  if (sample_count_->value.ull_prop() < median_buffer_.size()) {
    median_sample_count_->value.ull_prop(median_sample_count_->value.ull_prop() + 1);
  }
  sample_count_->value.ull_prop(sample_count_->value.ull_prop() + 1);

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

  if (timestamp_buffer_.size()) {
    timestamp_buffer_[next_median_buffer_index] = time == Builder::ZERO ? Builder::get_sys_time() : time;
  }
}

void PropertyStatBlock::finalize()
{
  double median_result = 0.0;
  double mad_result = 0.0;
  size_t count = static_cast<size_t>(median_sample_count_->value.ull_prop());

  // write buffer
  Builder::PropertyIndex buff_prop = get_or_create_property(*(median_.get_seq()), std::string(median_->name) + "_buffer", Builder::PVK_DOUBLE_SEQ);
  Builder::DoubleSeq ds;
  ds.length(static_cast<CORBA::ULong>(count));
  for (size_t i = 0; i < count; ++i) {
    size_t pos = (count < median_buffer_.size() ? i : ((sample_count_->value.ull_prop() + 1 + i) % median_buffer_.size()));
    ds[static_cast<CORBA::ULong>(i)] = median_buffer_[pos];
  }
  buff_prop->value.double_seq_prop(ds);
  if (timestamp_buffer_.size()) {
    Builder::PropertyIndex ts_buff_prop = get_or_create_property(*(median_.get_seq()), std::string(median_->name) + "_timestamp_buffer", Builder::PVK_TIME_SEQ);
    Builder::TimeStampSeq tss;
    tss.length(static_cast<CORBA::ULong>(count));
    for (size_t i = 0; i < count; ++i) {
      size_t pos = (count < timestamp_buffer_.size() ? i : ((sample_count_->value.ull_prop() + 1 + i) % timestamp_buffer_.size()));
      tss[static_cast<CORBA::ULong>(i)] = timestamp_buffer_[pos];
    }
    ts_buff_prop->value.time_seq_prop(tss);
  }

  if (count) {
    // calculate median
    std::sort(&median_buffer_[0], &median_buffer_[count - 1]);
    if (count % 2 == 0) {
      median_result = (median_buffer_[(count / 2) - 1] + median_buffer_[count / 2]) / 2.0;
    } else {
      median_result = median_buffer_[(count / 2)];
    }
    // calculate median absolute deviation (median of absolute value of data deviation from median)
    std::vector<double> mad_buffer(median_buffer_);
    for (size_t i = 0; i < count; ++i) {
      mad_buffer[i] = fabs(median_buffer_[i] - median_result);
    }
    std::sort(&median_buffer_[0], &median_buffer_[count - 1]);
    if (count % 2 == 0) {
      mad_result = (mad_buffer[(count / 2) - 1] + mad_buffer[count / 2]) / 2.0;
    } else {
      mad_result = mad_buffer[(count / 2)];
    }
  }
  median_->value.double_prop(median_result);
  median_absolute_deviation_->value.double_prop(mad_result);
}

SimpleStatBlock PropertyStatBlock::to_simple_stat_block() const
{
  SimpleStatBlock result;
  to_simple_stat_block(result);
  return result;
}

void PropertyStatBlock::to_simple_stat_block(SimpleStatBlock& result) const
{
  result.sample_count_ = static_cast<size_t>(sample_count_->value.ull_prop());
  result.min_ = min_->value.double_prop();
  result.max_ = max_->value.double_prop();
  result.mean_ = mean_->value.double_prop();
  result.var_x_sample_count_ = var_x_sample_count_->value.double_prop();

  result.median_buffer_ = median_buffer_;
  result.timestamp_buffer_ = timestamp_buffer_;
  result.median_sample_count_ = static_cast<size_t>(median_sample_count_->value.ull_prop());
  result.median_ = median_->value.double_prop();
  result.median_absolute_deviation_ = median_absolute_deviation_->value.double_prop();
}

ConstPropertyStatBlock::ConstPropertyStatBlock(const Builder::PropertySeq& seq, const std::string& prefix)
{
  sample_count_ = get_property(seq, prefix + "_sample_count", Builder::PVK_ULL);

  min_ = get_property(seq, prefix + "_min", Builder::PVK_DOUBLE);

  max_ = get_property(seq, prefix + "_max", Builder::PVK_DOUBLE);

  mean_ = get_property(seq, prefix + "_mean", Builder::PVK_DOUBLE);

  var_x_sample_count_ = get_property(seq, prefix + "_var_x_sample_count", Builder::PVK_DOUBLE);

  Builder::ConstPropertyIndex median_buffer = get_property(seq, prefix + "_median_buffer", Builder::PVK_DOUBLE_SEQ);

  if (median_buffer) {
    median_buffer_.resize(median_buffer->value.double_seq_prop().length());
    for (size_t i = 0; i < median_buffer_.size(); ++i) {
      median_buffer_[i] = median_buffer->value.double_seq_prop()[static_cast<CORBA::ULong>(i)];
    }
  }

  Builder::ConstPropertyIndex timestamp_buffer = get_property(seq, prefix + "_median_timestamp_buffer", Builder::PVK_TIME_SEQ);

  if (timestamp_buffer) {
    timestamp_buffer_.resize(timestamp_buffer->value.time_seq_prop().length());
    for (size_t i = 0; i < timestamp_buffer_.size(); ++i) {
      timestamp_buffer_[i] = timestamp_buffer->value.time_seq_prop()[static_cast<CORBA::ULong>(i)];
    }
  }

  median_sample_count_ = get_property(seq, prefix + "_median_sample_count", Builder::PVK_ULL);

  median_ = get_property(seq, prefix + "_median", Builder::PVK_DOUBLE);

  median_absolute_deviation_ = get_property(seq, prefix + "_median_absolute_deviation", Builder::PVK_DOUBLE);
}

SimpleStatBlock ConstPropertyStatBlock::to_simple_stat_block() const
{
  SimpleStatBlock result;
  to_simple_stat_block(result);
  return result;
}

void ConstPropertyStatBlock::to_simple_stat_block(SimpleStatBlock& result) const
{
  if (sample_count_) {
    result.sample_count_ = static_cast<size_t>(sample_count_->value.ull_prop());
    result.min_ = min_->value.double_prop();
    result.max_ = max_->value.double_prop();
    result.mean_ = mean_->value.double_prop();
    result.var_x_sample_count_ = var_x_sample_count_->value.double_prop();

    result.median_buffer_ = median_buffer_;
    result.timestamp_buffer_ = timestamp_buffer_;
    result.median_sample_count_ = static_cast<size_t>(median_sample_count_->value.ull_prop());
    result.median_ = median_->value.double_prop();
    result.median_absolute_deviation_ = median_absolute_deviation_->value.double_prop();
  } else {
    result = SimpleStatBlock();
  }
}

}
