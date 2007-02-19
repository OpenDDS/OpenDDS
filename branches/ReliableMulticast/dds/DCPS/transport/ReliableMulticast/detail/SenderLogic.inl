// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticast::detail::SenderLogic::SenderLogic(
  size_t max_retry_buffer_size
  )
  : max_retry_buffer_size_(max_retry_buffer_size)
  , current_id_(0)
{
}
