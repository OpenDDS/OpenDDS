/*
 * $Id$
 */

#include <iostream>

#include <ace/Log_Msg.h>

#include "ProgressIndicator.h"

ProgressIndicator::ProgressIndicator(const char* format,
                                     const std::size_t max,
                                     const std::size_t inc)
  : format_(format),
    max_(max),
    inc_(inc),
    curr_(0),
    last_(10)
{}

ProgressIndicator::~ProgressIndicator()
{}

ProgressIndicator&
ProgressIndicator::operator++()
{
  ++curr_;

  std::size_t pct = curr_ / double(max_) * 100;
  if (pct > last_)
  {
    ACE_DEBUG((LM_DEBUG, format_, last_, curr_));
    last_ += inc_;
  }
  else if (curr_ == max_)
  {
    ACE_DEBUG((LM_DEBUG, format_, 100, curr_));
  }
  
  return *this;
}

