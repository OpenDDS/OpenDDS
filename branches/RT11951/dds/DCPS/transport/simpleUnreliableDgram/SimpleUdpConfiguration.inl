// -*- C++ -*-
//
// $Id$
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::SimpleUdpConfiguration::SimpleUdpConfiguration()
{
  DBG_ENTRY_LVL("SimpleUdpConfiguration","SimpleUdpConfiguration",6);
  this->transport_type_ = ACE_TEXT("SimpleUdp");
}
