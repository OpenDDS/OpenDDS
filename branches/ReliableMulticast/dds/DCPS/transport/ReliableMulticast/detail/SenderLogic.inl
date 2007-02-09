// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticast::detail::SenderLogic::SenderLogic(
  size_t max_size
  )
  : max_size_(max_size)
  , current_id_(0)
{
}
