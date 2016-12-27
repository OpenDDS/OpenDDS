/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/RTPS/GuidGenerator.h"

#include "dds/DCPS/GuidUtils.h"

#include "dds/DdsDcpsGuidTypeSupportImpl.h"

#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_netdb.h"
#include "ace/OS_NS_sys_socket.h"
#include "ace/OS_NS_sys_time.h"

#include "ace/os_include/net/os_if.h"

#include <cstring>

#ifdef ACE_LINUX
# include <sys/types.h>
# include <ifaddrs.h>
#endif

#if defined ACE_WIN32 && !defined ACE_HAS_WINCE
# include <WinSock2.h>
# include <Iphlpapi.h>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace RTPS {

GuidGenerator::GuidGenerator()
  : pid_(ACE_OS::getpid()),
    counter_(0)
{

  if (pid_ == -1) {
    unsigned seed = static_cast<unsigned>(ACE_OS::gettimeofday().usec());
    pid_ = static_cast<pid_t>(ACE_OS::rand_r(&seed));
  }

  ACE_OS::macaddr_node_t macaddress;
  const int result = ACE_OS::getmacaddress(&macaddress);

  if (-1 != result) {
    ACE_OS::memcpy(node_id_, macaddress.node, NODE_ID_SIZE);
  } else {
    for (int i = 0; i < NODE_ID_SIZE; ++i) {
      node_id_[i] = static_cast<unsigned char>(ACE_OS::rand());
    }
  }
}

ACE_UINT16
GuidGenerator::getCount()
{
  ACE_Guard<ACE_SYNCH_MUTEX> guard(counter_lock_);
  return counter_++;
}

int
GuidGenerator::interfaceName(const char* iface)
{
  if (interface_name_ == iface) {
    return 0;
  }
  interface_name_ = iface;
  // See ace/OS_NS_netdb.cpp ACE_OS::getmacaddress()
#if defined ACE_WIN32 && !defined ACE_HAS_WINCE
  ULONG size;
  if (::GetAdaptersAddresses(AF_UNSPEC, 0, 0, 0, &size)
      != ERROR_BUFFER_OVERFLOW) {
    return -1;
  }
  ACE_Allocator* const alloc = DCPS::SafetyProfilePool::instance();
  IP_ADAPTER_ADDRESSES* const addrs =
    static_cast<IP_ADAPTER_ADDRESSES*>(alloc->malloc(size));
  if (!addrs) {
    return -1;
  }
  if (::GetAdaptersAddresses(AF_UNSPEC, 0, 0, addrs, &size) != NO_ERROR) {
    alloc->free(addrs);
    return -1;
  }

  bool found = false;
  for (IP_ADAPTER_ADDRESSES* iter = addrs; iter && !found; iter = iter->Next) {
    if (std::strcmp(iter->AdapterName, iface) == 0) {
      std::memcpy(node_id_, iter->PhysicalAddress,
                  std::min(static_cast<size_t>(iter->PhysicalAddressLength),
                           sizeof node_id_));
      found = true;
    }
  }

  alloc->free(addrs);
  return found ? 0 : -1;
#elif defined sun
  //TODO: Solaris
  return -1;
#elif defined ACE_LINUX

  ifaddrs* addrs;
  if (::getifaddrs(&addrs) != 0) {
    return -1;
  }

  bool found = false;
  for (ifaddrs* addr = addrs; addr && !found; addr = addr->ifa_next) {
    if (addr->ifa_addr == 0) {
      continue;
    }
    if (std::strcmp(addr->ifa_name, iface) == 0) {
      found = true;
      ifreq ifr;
      std::strncpy(ifr.ifr_name, iface, IFNAMSIZ);
      const ACE_HANDLE h = ACE_OS::socket(PF_INET, SOCK_DGRAM, 0);
      if (h == ACE_INVALID_HANDLE) {
        return -1;
      }
      if (ACE_OS::ioctl(h, SIOCGIFHWADDR, &ifr) < 0) {
        ACE_OS::close(h);
        return -1;
      }
      ACE_OS::close(h);
      std::memcpy(node_id_, ifr.ifr_addr.sa_data, sizeof node_id_);
    }
  }

  ::freeifaddrs(addrs);
  return found ? 0 : -1;

#elif defined ACE_HAS_SIOCGIFCONF
  //TODO: MacOSX uses this
  return -1;
#else
  return -1;
#endif
}

void
GuidGenerator::populate(DCPS::GUID_t &container)
{
  container.guidPrefix[0] = DCPS::VENDORID_OCI[0];
  container.guidPrefix[1] = DCPS::VENDORID_OCI[1];

  const ACE_UINT16 count = getCount();
  ACE_OS::memcpy(&container.guidPrefix[2], node_id_, NODE_ID_SIZE);
  container.guidPrefix[8] = static_cast<CORBA::Octet>(pid_ >> 8);
  container.guidPrefix[9] = static_cast<CORBA::Octet>(pid_ & 0xFF);
  container.guidPrefix[10] = static_cast<CORBA::Octet>(count >> 8);
  container.guidPrefix[11] = static_cast<CORBA::Octet>(count & 0xFF);
}

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
