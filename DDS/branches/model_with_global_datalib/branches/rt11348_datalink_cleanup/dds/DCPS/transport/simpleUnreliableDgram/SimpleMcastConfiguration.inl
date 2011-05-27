// -*- C++ -*-
//
// $Id$
#include "dds/DCPS/transport/framework/EntryExit.h"
#include <sstream>

ACE_INLINE
OpenDDS::DCPS::SimpleMcastConfiguration::SimpleMcastConfiguration()
  : multicast_group_address_(ACE_DEFAULT_MULTICAST_PORT, ACE_DEFAULT_MULTICAST_ADDR),
    receiver_(false)
{
  multicast_group_address_str_ = ACE_DEFAULT_MULTICAST_ADDR;
  multicast_group_address_str_ += ":";
  std::stringstream out;
  out << ACE_DEFAULT_MULTICAST_PORT;
  multicast_group_address_str_ += out.str ();

  DBG_ENTRY_LVL("SimpleMcastConfiguration","SimpleMcastConfiguration",6);
  this->transport_type_ = ACE_TEXT("SimpleMcast");
}
