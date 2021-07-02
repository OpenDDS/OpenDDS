#include "ProgressIndicator.h"
#include <ace/Log_Msg.h>
#include <iostream>

ProgressIndicator::ProgressIndicator(const char* format, const std::size_t max, const std::size_t grad)
  : format_(format)
  , max_(max)
  , grad_(grad)
  , last_(grad_ - 1)
  , curr_(0)
{}

ProgressIndicator::~ProgressIndicator()
{}

ProgressIndicator& ProgressIndicator::operator++()
{
  ++curr_;
  std::size_t pct = std::size_t(curr_ / double(max_) * 100);
  if (pct > last_) {
    ACE_DEBUG((LM_INFO, format_, pct, curr_));
    last_ += grad_;
  }
  return *this;
}
