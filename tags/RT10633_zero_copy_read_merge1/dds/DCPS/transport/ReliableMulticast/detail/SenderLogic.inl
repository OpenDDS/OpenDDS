// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticast::detail::SenderLogic::SenderLogic(
  size_t sender_history_size
  )
  : sender_history_size_(sender_history_size)
  , current_id_(0)
{
}
