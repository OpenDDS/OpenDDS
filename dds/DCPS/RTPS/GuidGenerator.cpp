/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/RTPS/GuidGenerator.h"

#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/TimeTypes.h"

#include "dds/DdsDcpsGuidTypeSupportImpl.h"

#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_netdb.h"
#include "ace/OS_NS_sys_socket.h"
#include "ace/OS_NS_sys_time.h"
#include "ace/Log_Msg.h"

#include "ace/os_include/net/os_if.h"

#include <cstring>

#ifdef ACE_HAS_CPP11
#include <random>
#endif

#ifdef ACE_LINUX
# include <sys/types.h>
# include <ifaddrs.h>
#endif

#if defined ACE_WIN32 && !defined ACE_HAS_WINCE
# include <winsock2.h>
# include <iphlpapi.h>
# include "ace/Version.h"
// older versions of ACE don't link to IPHlpApi.Lib, see acedefaults.mpb
# if ACE_MAJOR_VERSION == 6 && ACE_MINOR_VERSION == 0 && defined _MSC_VER
#  pragma comment(lib, "IPHlpApi.Lib")
# endif
#endif

#ifdef ACE_VXWORKS
# include "ace/os_include/sys/os_sysctl.h"
# include <net/route.h>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::SystemTimePoint;

GuidGenerator::GuidGenerator()
  : pid_(ACE_OS::getpid())
{
  unsigned seed = static_cast<unsigned>(SystemTimePoint::now().value().usec());

  if (pid_ == -1) {
    pid_ = static_cast<pid_t>(ACE_OS::rand_r(&seed));
  }

#ifdef ACE_HAS_CPP11
  std::mt19937 generator(seed);
  std::uniform_int_distribution<ACE_UINT16> distribution(0, ACE_UINT16_MAX);
  counter_ = distribution(generator);
#else
  counter_ = static_cast<ACE_UINT16>(ACE_OS::rand_r(&seed));
#endif

  ACE_OS::macaddr_node_t macaddress;
  const int result = ACE_OS::getmacaddress(&macaddress);

#ifndef ACE_HAS_IOS
  if (-1 != result) {
    ACE_OS::memcpy(node_id_, macaddress.node, NODE_ID_SIZE);
  } else {
    for (int i = 0; i < NODE_ID_SIZE; ++i) {
      node_id_[i] = static_cast<unsigned char>(ACE_OS::rand());
    }
  }
#else
// iOS has non-unique MAC addresses
  ACE_UNUSED_ARG(result);

  for (int i = 0; i < NODE_ID_SIZE; ++i) {
    node_id_[i] = static_cast<unsigned char>(ACE_OS::rand());
  }
#endif /* ACE_HAS_IOS */
}

ACE_UINT16
GuidGenerator::getCount()
{
  ACE_Guard<ACE_Thread_Mutex> guard(counter_lock_);
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
    if (ACE_Wide_To_Ascii(iter->FriendlyName).char_rep() == interface_name_) {
      std::memcpy(node_id_, iter->PhysicalAddress,
                  std::min(static_cast<size_t>(iter->PhysicalAddressLength),
                           sizeof node_id_));
      found = true;
    }
  }

  alloc->free(addrs);
  return found ? 0 : -1;
#elif defined ACE_LINUX || defined ACE_ANDROID
  ifreq ifr;
  // Guarantee that iface will fit in ifr.ifr_name and still be null terminated
  // ifr.ifr_name is sized to IFNAMSIZ
  if (std::strlen(iface) >= sizeof(ifr.ifr_name)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("Interface name %C exceeds max allowable length, must be < %d."),
      iface, IFNAMSIZ));
    return -1;
  }
  std::strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name));
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
  return 0;
#elif defined ACE_HAS_SIOCGIFCONF || defined ACE_HAS_MAC_OSX
  const ACE_HANDLE h = ACE_OS::socket(AF_INET, SOCK_DGRAM, 0);
  if (h == ACE_INVALID_HANDLE) {
    return -1;
  }

  const int BUFFERSIZE = 4000;
  char buffer[BUFFERSIZE];
  ifconf ifc;
  ifc.ifc_len = BUFFERSIZE;
  ifc.ifc_buf = buffer;

  if (ACE_OS::ioctl(h, SIOCGIFCONF, &ifc) < 0) {
    ACE_OS::close(h);
    return -1;
  }

  bool found = false;
  for (const char* ptr = buffer; !found && ptr < buffer + ifc.ifc_len;) {
    const ifreq* ifr = reinterpret_cast<const ifreq*>(ptr);
    if (ifr->ifr_addr.sa_family == AF_LINK && ifr->ifr_name == interface_name_) {
      const sockaddr_dl* sdl =
        reinterpret_cast<const sockaddr_dl*>(&ifr->ifr_addr);
      std::memcpy(node_id_, LLADDR(sdl), sizeof node_id_);
      found = true;
    }

    ptr += sizeof ifr->ifr_name + std::max(sizeof ifr->ifr_addr,
      static_cast<size_t>(ifr->ifr_addr.sa_len));
  }

  ACE_OS::close(h);
  return found ? 0 : -1;
#elif defined ACE_VXWORKS
  int name[] = {CTL_NET, AF_ROUTE, 0, 0, NET_RT_IFLIST, 0};
  static const size_t name_elts = sizeof name / sizeof name[0];

  size_t result_sz = 0u;
  if (sysctl(name, name_elts, 0, &result_sz, 0, 0u) != 0) {
    return -1;
  }

  ACE_Allocator* const alloc = DCPS::SafetyProfilePool::instance();
  char* const result = static_cast<char*>(alloc->malloc(result_sz));

  if (sysctl(name, name_elts, result, &result_sz, 0, 0u) != 0) {
    alloc->free(result);
    return -1;
  }

  for (size_t pos = 0, n; pos + sizeof(if_msghdr) < result_sz; pos += n) {
    if_msghdr* const hdr = reinterpret_cast<if_msghdr*>(result + pos);
    n = hdr->ifm_msglen;
    sockaddr_dl* const addr =
      reinterpret_cast<sockaddr_dl*>(result + pos + sizeof(if_msghdr));

    if (hdr->ifm_type == RTM_IFINFO && (hdr->ifm_addrs & RTA_IFP)
        && std::memcmp(addr->sdl_data, iface, addr->sdl_nlen) == 0
        && addr->sdl_alen >= sizeof node_id_) {
      std::memcpy(node_id_, LLADDR(addr), sizeof node_id_);
      alloc->free(result);
      return 0;
    }

    while (pos + n < result_sz) {
      if_msghdr* const nxt = reinterpret_cast<if_msghdr*>(result + pos + n);
      if (nxt->ifm_type != RTM_NEWADDR) {
        break;
      }
      n += nxt->ifm_msglen;
    }
  }

  alloc->free(result);
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
