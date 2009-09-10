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
OpenDDS::DCPS::SimpleUnreliableDgramConfiguration::SimpleUnreliableDgramConfiguration()
  : max_output_pause_period_(-1)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramConfiguration","SimpleUnreliableDgramConfiguration",6);
  this->max_packet_size_ = 62501;
}
