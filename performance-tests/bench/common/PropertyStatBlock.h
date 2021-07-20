#pragma once

#include "Bench_Common_Export.h"
#include "Common.h"
#include "BenchTypeSupportImpl.h"

#include <vector>

namespace Bench {

constexpr size_t DEFAULT_STAT_BLOCK_BUFFER_SIZE = 1000u;

struct Bench_Common_Export SimpleStatBlock {
  SimpleStatBlock();

  size_t sample_count_;
  double min_;
  double max_;
  double mean_;
  double var_x_sample_count_;

  std::vector<double> median_buffer_;
  std::vector<Builder::TimeStamp> timestamp_buffer_;
  size_t median_sample_count_;
  size_t median_sample_overflow_;
  double median_;
  double median_absolute_deviation_;

  void pretty_print(std::ostream& os, const std::string& prefix, const std::string& indentation = "  ", size_t indentation_level = 0) const;
  void to_json_summary(const std::string& name, rapidjson::Value& dst, rapidjson::Value::AllocatorType& alloc) const;
};

Bench_Common_Export SimpleStatBlock consolidate(const SimpleStatBlock& sb1, const SimpleStatBlock& sb2);
Bench_Common_Export SimpleStatBlock consolidate(const std::vector<SimpleStatBlock>& vec);

class Bench_Common_Export PropertyStatBlock {
public:
  // Constructor for initializing / writing PropertyStatBlock
  PropertyStatBlock(Builder::PropertySeq& seq, const std::string& prefix, size_t median_buffer_size, bool timestamps = false);

  void update(double value, const Builder::TimeStamp& time = Builder::ZERO);
  void finalize();

  void to_simple_stat_block(SimpleStatBlock&) const;
  SimpleStatBlock to_simple_stat_block() const;

private:

  Builder::PropertyIndex sample_count_;
  Builder::PropertyIndex min_;
  Builder::PropertyIndex max_;
  Builder::PropertyIndex mean_;
  Builder::PropertyIndex var_x_sample_count_;

  std::vector<double> median_buffer_;
  std::vector<Builder::TimeStamp> timestamp_buffer_;
  Builder::PropertyIndex median_sample_count_;
  Builder::PropertyIndex median_;
  Builder::PropertyIndex median_absolute_deviation_;
};

class Bench_Common_Export ConstPropertyStatBlock {
public:
  // Constructor for reading PropertyStatBlock
  ConstPropertyStatBlock(const Builder::PropertySeq& seq, const std::string& prefix);

  inline explicit operator bool() const noexcept { return static_cast<bool>(sample_count_); }
  inline bool operator!() const noexcept { return !static_cast<bool>(sample_count_); }

  void to_simple_stat_block(SimpleStatBlock&) const;
  SimpleStatBlock to_simple_stat_block() const;

private:

  Builder::ConstPropertyIndex sample_count_;
  Builder::ConstPropertyIndex min_;
  Builder::ConstPropertyIndex max_;
  Builder::ConstPropertyIndex mean_;
  Builder::ConstPropertyIndex var_x_sample_count_;

  std::vector<double> median_buffer_;
  std::vector<Builder::TimeStamp> timestamp_buffer_;
  Builder::ConstPropertyIndex median_sample_count_;
  Builder::ConstPropertyIndex median_;
  Builder::ConstPropertyIndex median_absolute_deviation_;
};

}
