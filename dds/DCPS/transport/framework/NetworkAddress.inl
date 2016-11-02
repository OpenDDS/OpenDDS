/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

#include "ace/CDR_Stream.h"
#include "ace/OS_NS_netdb.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE
OpenDDS::DCPS::NetworkAddress::NetworkAddress()
  : reserved_(0)
{
  DBG_ENTRY_LVL("NetworkAddress","NetworkAddress",6);
}

ACE_INLINE
OpenDDS::DCPS::NetworkAddress::NetworkAddress(const ACE_INET_Addr& addr,
                                              bool use_hostname)
  : reserved_(0)
{
  DBG_ENTRY_LVL("NetworkAddress","NetworkAddress",6);

  ACE_TCHAR addr_s[MAXHOSTNAMELEN + 1];
  addr.addr_to_string(addr_s, sizeof (addr_s), !use_hostname);

  this->addr_ = ACE_TEXT_ALWAYS_CHAR(addr_s);
}

ACE_INLINE
OpenDDS::DCPS::NetworkAddress::NetworkAddress(const OPENDDS_STRING& addr)
  : reserved_(0)
{
  DBG_ENTRY_LVL("NetworkAddress","NetworkAddress",6);

  addr_ = addr;
}

ACE_INLINE
OpenDDS::DCPS::NetworkAddress::~NetworkAddress()
{
}

ACE_INLINE
void OpenDDS::DCPS::NetworkAddress::to_addr(ACE_INET_Addr& addr) const
{
  DBG_ENTRY_LVL("NetworkAddress","to_addr",6);
  addr.set(addr_.c_str());
}

ACE_INLINE
void OpenDDS::DCPS::NetworkAddress::dump()
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) NetworkAddress addr: %s reserved: %d\n", addr_.c_str(), reserved_));
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
