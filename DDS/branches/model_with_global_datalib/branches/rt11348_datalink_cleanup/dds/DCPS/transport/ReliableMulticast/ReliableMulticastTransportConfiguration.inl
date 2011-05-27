// -*- C++ -*-
//
// $Id$

#include "ReliableMulticastTransportConfiguration.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include <sstream>

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastTransportConfiguration::ReliableMulticastTransportConfiguration()
  : multicast_group_address_(ACE_DEFAULT_MULTICAST_PORT, ACE_DEFAULT_MULTICAST_ADDR)
  , receiver_(false)
  , sender_history_size_(1024)
  , receiver_buffer_size_(256)
{
  transport_type_ = ACE_TEXT("ReliableMulticast");
  multicast_group_address_str_ = ACE_DEFAULT_MULTICAST_ADDR;
  multicast_group_address_str_ += ":";
  std::stringstream out;
  out << ACE_DEFAULT_MULTICAST_PORT;
  multicast_group_address_str_ += out.str ();
}

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastTransportConfiguration::~ReliableMulticastTransportConfiguration()
{
}
