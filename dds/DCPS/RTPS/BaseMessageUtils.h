/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RTPS_BASEMESSAGEUTILS_H
#define RTPS_BASEMESSAGEUTILS_H

#include "RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfoUtilsTypeSupportImpl.h"
#include "md5.h"
#include "ace/INET_Addr.h"
#include "ace/Message_Block.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {
  using DCPS::GuidPrefix_t;
  using DCPS::GUID_t;
  using DCPS::EntityId_t;

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

  if (DCPS::MarshalTraits<T>::gen_is_bounded_key_size() &&
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

inline void assign(GuidPrefix_t& dest, const GuidPrefix_t& src)
{
  std::memcpy(&dest[0], &src[0], sizeof(GuidPrefix_t));
}

inline void assign(OpenDDS::DCPS::OctetArray16& dest,
                   const OpenDDS::DCPS::OctetArray16& src)
{
  std::memcpy(&dest[0], &src[0], sizeof(OpenDDS::DCPS::OctetArray16));
}

inline void assign(OpenDDS::DCPS::OctetArray16& dest,
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
address_to_bytes(OpenDDS::DCPS::OctetArray16& dest, const ACE_INET_Addr& addr)
{
  const void* raw = addr.get_addr();
#ifdef ACE_HAS_IPV6
  if (addr.get_type() == AF_INET6) {
    const sockaddr_in6* in = static_cast<const sockaddr_in6*>(raw);
    std::memcpy(&dest[0], &in->sin6_addr, 16);
  } else {
#else
  {
#endif
    const sockaddr_in* in = static_cast<const sockaddr_in*>(raw);
    std::memset(&dest[0], 0, 12);
    std::memcpy(&dest[12], &in->sin_addr, 4);
  }
}

inline int
address_to_kind(const ACE_INET_Addr& addr)
{
#ifdef ACE_HAS_IPV6
  return addr.get_type() == AF_INET6 ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
#else
  ACE_UNUSED_ARG(addr);
  return LOCATOR_KIND_UDPv4;
#endif
}

inline int
locator_to_address(ACE_INET_Addr& dest,
                   const OpenDDS::DCPS::Locator_t& locator,
                   bool map /*map IPV4 to IPV6 addr*/)
{
  switch (locator.kind) {
#ifdef ACE_HAS_IPV6
  case LOCATOR_KIND_UDPv6:
    dest.set_type(AF_INET6);
    if (dest.set_address(reinterpret_cast<const char*>(locator.address),
                         16, 0 /*encode*/) == -1) {
      return -1;
    }
    dest.set_port_number(locator.port);
    return 0;
    break;
#endif
  case LOCATOR_KIND_UDPv4:
#if !defined (ACE_HAS_IPV6) || !defined (IPV6_V6ONLY)
    ACE_UNUSED_ARG(map);
#endif
    dest.set_type(AF_INET);
    if (dest.set_address(reinterpret_cast<const char*>(locator.address)
                         + 12, 4, 0 /*network order*/
#if defined (ACE_HAS_IPV6) && defined (IPV6_V6ONLY)
                         , map ? 1 : 0 /*map IPV4 to IPV6 addr*/
#endif
                         ) == -1) {
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
    OpenDDS::DCPS::LocatorSeq& locators,
    bool& requires_inline_qos)
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
                      ACE_TEXT("Failed to deserialize blob's locators\n")),
                      DDS::RETCODE_ERROR);
  }
  if (!(ser >> ACE_InputCDR::to_boolean(requires_inline_qos))) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) blob_to_locators: ")
                      ACE_TEXT("Failed to deserialize blob's inline QoS flag\n")),
                      DDS::RETCODE_ERROR);
  }
  return DDS::RETCODE_OK;
}

template <typename T>
void
  message_block_to_sequence(const ACE_Message_Block& mb_locator, T& out)
{
  out.length (CORBA::ULong(mb_locator.length()));
  std::memcpy (out.get_buffer(), mb_locator.rd_ptr(), mb_locator.length());
}

inline void
locators_to_blob(const OpenDDS::DCPS::LocatorSeq& locators, DCPS::TransportBLOB& blob)
{
  using OpenDDS::DCPS::Serializer;
  size_t size_locator = 0, padding_locator = 0;
  DCPS::gen_find_size(locators, size_locator, padding_locator);
  ACE_Message_Block mb_locator(size_locator + padding_locator + 1);
  Serializer ser_loc(&mb_locator, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  ser_loc << locators;
  // Add a bool for 'requires inline qos', see Sedp::set_inline_qos():
  // if the bool is no longer the last octet of the sequence then that function
  // must be changed as well.
  ser_loc << ACE_OutputCDR::from_boolean(false);
  message_block_to_sequence(mb_locator, blob);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* RTPS_BASEMESSAGETYPES_H */
