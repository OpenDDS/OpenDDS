// -*- C++ -*-
//
// $Id$
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::SimpleUnreliableDgramConfiguration::SimpleUnreliableDgramConfiguration()
: max_output_pause_period_ (-1)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramConfiguration","SimpleUnreliableDgramConfiguration",5);
  this->max_packet_size_ = 62501;
}

