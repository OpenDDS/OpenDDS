/*
 * $Id$
 */

#include <iostream>

#include <ace/Log_Msg.h>

#include "ProgressIndicator.h"

ProgressIndicator::ProgressIndicator(const char* format,
                                     const std::size_t max,
                                     unsigned grad)
  : format_(format),
    max_(max),
    grad_(grad),
    curr_(0),
    last_(grad_)
{}

ProgressIndicator::~ProgressIndicator()
{}

ProgressIndicator&
ProgressIndicator::operator++()
{
  ++curr_;

  unsigned pct = curr_ / double(max_) * 100;
  if (pct > last_)
  {
    ACE_DEBUG((LM_DEBUG, format_, pct, curr_));
    last_ += grad_;
  }
  else if (curr_ >= max_)
  {
    ACE_DEBUG((LM_DEBUG, format_, 100, curr_));
  }
  
  return *this;
}

