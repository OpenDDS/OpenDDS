// -*- C++ -*-
//
// $Id$
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleMcastConfiguration::SimpleMcastConfiguration()
  : receiver_(false),
    max_output_pause_period_ (-1)
{
  DBG_ENTRY_LVL("SimpleMcastConfiguration","SimpleMcastConfiguration",5);
  this->max_packet_size_ = 62501;
  this->transport_type_ = "SimpleMcast";
}

