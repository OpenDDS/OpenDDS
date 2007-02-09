// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticast::detail::ReceiverLogic::ReceiverLogic(
  size_t max_size,
  const ReliabilityMode& reliability
  )
  : max_size_(max_size)
  , reliability_(reliability)
  , seen_last_delivered_(false)
  , last_delivered_id_(0)
{
}
