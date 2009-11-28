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
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::ReceiverLogic(
  size_t receiver_buffer_size,
  const ReliabilityMode& reliability)
  : receiver_buffer_size_(receiver_buffer_size)
  , reliability_(reliability)
  , seen_last_delivered_(false)
  , last_delivered_id_(0)
{
}
