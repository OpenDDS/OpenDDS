/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RTPS_BASEMESSAGEUTILS_H
#define RTPS_BASEMESSAGEUTILS_H

#include "RtpsCoreTypeSupportImpl.h"
#include "rtps_export.h"
#include "BaseMessageTypes.h"

#include "dds/DCPS/Message_Block_Ptr.h"
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
  using DCPS::Serializer;

  DCPS::KeyOnly<const T> ko(msg);

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

inline DCPS::RepoId make_id(const GuidPrefix_t& prefix, const EntityId_t& entity)
{
  DCPS::RepoId id;
  assign(id.guidPrefix, prefix);
  id.entityId = entity;
  return id;
}

inline DCPS::RepoId make_id(const DCPS::RepoId& participant_id, const EntityId_t& entity)
{
  DCPS::RepoId id = participant_id;
  id.entityId = entity;
  return id;
}

inline void assign(DCPS::OctetArray16& dest,
                   const DCPS::OctetArray16& src)
{
  std::memcpy(&dest[0], &src[0], sizeof(DCPS::OctetArray16));
}

inline void assign(DCPS::OctetArray16& dest,
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
address_to_bytes(DCPS::OctetArray16& dest, const ACE_INET_Addr& addr)
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

OpenDDS_Rtps_Export
int locator_to_address(ACE_INET_Addr& dest,
                       const DCPS::Locator_t& locator,
                       bool map /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
DDS::ReturnCode_t blob_to_locators(const DCPS::TransportBLOB& blob,
                                   DCPS::LocatorSeq& locators,
                                   bool* requires_inline_qos = 0,
                                   unsigned int* pBytesRead = 0);

OpenDDS_Rtps_Export
void locators_to_blob(const DCPS::LocatorSeq& locators,
                      DCPS::TransportBLOB& blob);

OpenDDS_Rtps_Export
DCPS::LocatorSeq transport_locator_to_locator_seq(const DCPS::TransportLocator& info);

template <typename T>
void message_block_to_sequence(const ACE_Message_Block& mb_locator, T& out)
{
  out.length (CORBA::ULong(mb_locator.length()));
  std::memcpy (out.get_buffer(), mb_locator.rd_ptr(), mb_locator.length());
}

#ifndef OPENDDS_SAFETY_PROFILE

inline bool operator==(const Duration_t& x, const Duration_t& y)
{
  return x.seconds == y.seconds && x.fraction == y.fraction;
}

inline bool operator==(const VendorId_t& v1, const VendorId_t& v2)
{
  return (v1.vendorId[0] == v2.vendorId[0] && v1.vendorId[1] == v2.vendorId[1]);
}

#endif

inline bool operator<(const ProtocolVersion_t& v1, const ProtocolVersion_t& v2)
{
  return (v1.major < v2.major || (v1.major == v2.major && v1.minor < v2.minor));
}

OpenDDS_Rtps_Export
DCPS::TimeDuration rtps_duration_to_time_duration(const Duration_t& rtps_duration, const ProtocolVersion_t& version, const VendorId_t& vendor);

/// Utility for iterating through a contiguous buffer (either really contiguous
/// or virtually contiguous using message block chaining) of RTPS Submessages
/// optionally prefixed by the RTPS Header
class OpenDDS_Rtps_Export MessageParser {
public:
  explicit MessageParser(const ACE_Message_Block& in);
  explicit MessageParser(const DDS::OctetSeq& in);

  bool parseHeader();
  bool parseSubmessageHeader();
  bool hasNextSubmessage() const;
  bool skipToNextSubmessage();
  bool skipSubmessageContent();

  const Header& header() const { return header_; }
  SubmessageHeader submessageHeader() const { return sub_; }
  size_t remaining() const { return in_ ? in_->total_length() : fromSeq_.length(); }
  const char* current() const { return ser_.pos_rd(); }

  DCPS::Serializer& serializer() { return ser_; }

  template <typename T>
  bool operator>>(T& rhs) { return ser_ >> rhs; }

private:
  ACE_Message_Block fromSeq_;
  DCPS::Message_Block_Ptr in_;
  DCPS::Serializer ser_;
  Header header_;
  SubmessageHeader sub_;
  size_t smContentStart_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* RTPS_BASEMESSAGETYPES_H */
