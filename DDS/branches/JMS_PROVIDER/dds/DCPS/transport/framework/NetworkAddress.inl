// -*- C++ -*-
//
// $Id$
#include "EntryExit.h"

#include <ace/CDR_Base.h>

ACE_INLINE
OpenDDS::DCPS::NetworkAddress::NetworkAddress()
  : reserved_ (0)
{
  DBG_ENTRY_LVL("NetworkAddress","NetworkAddress",6);
}

ACE_INLINE
OpenDDS::DCPS::NetworkAddress::~NetworkAddress()
{
}

ACE_INLINE
OpenDDS::DCPS::NetworkAddress::NetworkAddress(const std::string& addr)
: reserved_ (0)
{
  DBG_ENTRY_LVL("NetworkAddress","NetworkAddress",6);

  addr_ = addr;
}


ACE_INLINE
void OpenDDS::DCPS::NetworkAddress::to_addr(ACE_INET_Addr& addr) const
{
  DBG_ENTRY_LVL("NetworkAddress","to_addr",6);
  addr.set (addr_.c_str ());
}


ACE_INLINE
void OpenDDS::DCPS::NetworkAddress::dump ()
{
  ACE_DEBUG ((LM_DEBUG, "(%P|%t)NetworkAddress addr: %s reserved: %d\n", addr_.c_str (), reserved_));
}




