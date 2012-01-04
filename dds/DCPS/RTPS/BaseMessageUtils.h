/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RTPS_BASEMESSAGEUTILS_H
#define RTPS_BASEMESSAGEUTILS_H

#include "RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/Serializer.h"
#include "RtpsBaseMessageTypesC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "md5.h"
#include "ace/INET_Addr.h"
#include "ace/Message_Block.h"

#include <cstring>

namespace OpenDDS {
namespace RTPS {

template <typename T>
void marshal_key_hash(const T& msg, KeyHash_t& hash) {
  using OpenDDS::DCPS::Serializer;

  OpenDDS::DCPS::KeyOnly<const T> ko(msg);

  static const size_t HASH_LIMIT = 16;
  std::memset(hash.value, 0, HASH_LIMIT);

  // Key Hash must use big endian ordering.
  // Native==Little endian means we need to swap
#if defined ACE_LITTLE_ENDIAN
  static const bool swap_bytes = true;
#else
  static const bool swap_bytes = false;
#endif

  if (gen_is_bounded_size(ko) &&
      gen_max_marshaled_size(ko, true /*align*/) <= HASH_LIMIT) {
    // If it is bounded and can always fit in 16 bytes, we will use the
    // marshaled key
    ACE_Message_Block mb(HASH_LIMIT);
    Serializer out_serializer(&mb, swap_bytes, Serializer::ALIGN_INITIALIZE);
    out_serializer << ko;
    std::memcpy(hash.value, mb.rd_ptr(), mb.length());

  } else {
    // We will use the hash of the marshaled key
    size_t size = 0, padding = 0;
    gen_find_size(ko, size, padding);
    ACE_Message_Block mb(size + padding);
    Serializer out_serializer(&mb, swap_bytes, Serializer::ALIGN_INITIALIZE);
    out_serializer << ko;

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, mb.rd_ptr(), static_cast<unsigned long>(mb.length()));
    MD5_Final(hash.value, &ctx);
  }
}

inline void assign(OpenDDS::RTPS::GuidPrefix_t& dest,
                   const OpenDDS::RTPS::GuidPrefix_t& src)
{
  std::memcpy(&dest[0], &src[0], sizeof(GuidPrefix_t));
}

inline void assign(OpenDDS::RTPS::OctetArray16& dest,
                   const OpenDDS::RTPS::OctetArray16& src)
{
  std::memcpy(&dest[0], &src[0], sizeof(OpenDDS::RTPS::OctetArray16));
}

inline void assign(OpenDDS::RTPS::OctetArray16& dest,
                   const ACE_CDR::ULong& ipv4addr_be)
{
  std::memset(&dest[0], 0, 12);
  dest[12] = ipv4addr_be >> 24;
  dest[13] = ipv4addr_be >> 16;
  dest[14] = ipv4addr_be >> 8;
  dest[15] = ipv4addr_be;
}

inline void assign(DCPS::EntityKey_t& lhs, unsigned int rhs)
{
  lhs[0] = static_cast<CORBA::Octet>(rhs);
  lhs[1] = static_cast<CORBA::Octet>(rhs >> 8);
  lhs[2] = static_cast<CORBA::Octet>(rhs >> 16);
}


inline void
address_to_bytes(OpenDDS::RTPS::OctetArray16& dest, const ACE_INET_Addr& addr)
{
  const void* raw = addr.get_addr();
  if (addr.get_type() == AF_INET6) {
    const sockaddr_in6* in = static_cast<const sockaddr_in6*>(raw);
    std::memcpy(&dest[0], &in->sin6_addr, 16);
  } else {
    const sockaddr_in* in = static_cast<const sockaddr_in*>(raw);
    std::memset(&dest[0], 0, 12);
                std::memcpy(&dest[12], &in->sin_addr, 4);
  }
}

inline int
locator_to_address(ACE_INET_Addr& dest, const Locator_t& locator)
{
  switch (locator.kind) {
#ifdef ACE_HAS_IPV6
  case LOCATOR_KIND_UDPv6:
    dest.set_type(AF_INET6);
    if (dest.set_address(reinterpret_cast<const char*>(locator.address),
                         16) == -1) {
      return -1;
    }
    dest.set_port_number(locator.port);
    return 0;
    break;
#endif
  case LOCATOR_KIND_UDPv4:
    dest.set_type(AF_INET);
    if (dest.set_address(reinterpret_cast<const char*>(locator.address)
                         + 12, 4, 0 /*network order*/) == -1) {
      return -1;
    }
    dest.set_port_number(locator.port);
    return 0;
    break;
  default:
    return -1;  // Unknown kind
  }

  return -1;
}

inline DDS::ReturnCode_t
blob_to_locators(
    const OpenDDS::DCPS::TransportBLOB& blob, 
    LocatorSeq& locators)
{
  ACE_Data_Block db(blob.length(), ACE_Message_Block::MB_DATA,
      reinterpret_cast<const char*>(blob.get_buffer()),
      0 /*alloc*/, 0 /*lock*/, ACE_Message_Block::DONT_DELETE, 0 /*db_alloc*/);
  ACE_Message_Block mb(&db, ACE_Message_Block::DONT_DELETE, 0 /*mb_alloc*/);
  mb.wr_ptr(mb.space());

  using OpenDDS::DCPS::Serializer;
  Serializer ser(&mb, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  if (!(ser >> locators)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) blob_to_locators: ")
                      ACE_TEXT(" Failed to deserialize blob\n")), 
                      DDS::RETCODE_ERROR);
  }
  return DDS::RETCODE_OK;
}

}
}

#endif /* RTPS_BASEMESSAGETYPES_H */
