#include "ProgressIndicator.h"
#include <ace/Log_Msg.h>
#include <iostream>

ProgressIndicator::ProgressIndicator(const char* format, size_t max, size_t grad)
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
  size_t pct = static_cast<size_t>(curr_ / static_cast<double>(max_) * 100.0);
  if (pct > last_) {
    ACE_DEBUG((LM_INFO, format_, pct, curr_));
    last_ += grad_;
  }
  return *this;
}
