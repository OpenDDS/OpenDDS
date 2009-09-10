/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ReliableMulticast::detail::SenderLogic::SenderLogic(
  size_t sender_history_size)
  : sender_history_size_(sender_history_size)
  , current_id_(0)
{
}
