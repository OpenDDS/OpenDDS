// -*- C++ -*-
//
// $Id$
#include "EntryExit.h"

#include "ace/OS_NS_arpa_inet.h"

ACE_INLINE
OpenDDS::DCPS::NetworkAddress::NetworkAddress()
  : ip_(0),
    port_(0)
{
  DBG_ENTRY_LVL("NetworkAddress","NetworkAddress",6);
}


ACE_INLINE
OpenDDS::DCPS::NetworkAddress::NetworkAddress(const ACE_INET_Addr& addr)
{
  DBG_ENTRY_LVL("NetworkAddress","NetworkAddress",6);
  this->ip_   = htonl(addr.get_ip_address());
  this->port_ = htons(addr.get_port_number());
}


ACE_INLINE
void
OpenDDS::DCPS::NetworkAddress::to_addr(ACE_INET_Addr& addr) const
{
  DBG_ENTRY_LVL("NetworkAddress","to_addr",6);
  addr.set(this->port_, this->ip_, 0);
}
