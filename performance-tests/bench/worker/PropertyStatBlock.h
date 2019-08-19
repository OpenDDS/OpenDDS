#pragma once

#include "Common.h"
#include "BenchTypeSupportImpl.h"

#include <vector>

namespace Bench {

class PropertyStatBlock {
public:
  PropertyStatBlock(Builder::PropertySeq& seq, const std::string& prefix, size_t median_buffer_size);
  void update(double value);
  void write_median(bool write_buffer = false);

private:
  Builder::PropertyIndex sample_count_;
  Builder::PropertyIndex min_;
  Builder::PropertyIndex max_;
  Builder::PropertyIndex mean_;
  Builder::PropertyIndex var_x_sample_count_;

  std::vector<double> median_buffer_;
  Builder::PropertyIndex median_;
  Builder::PropertyIndex median_sample_count_;
};

}
