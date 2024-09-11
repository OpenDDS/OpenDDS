/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_MESSAGEUTILS_H
#define OPENDDS_DCPS_RTPS_MESSAGEUTILS_H

#include "RtpsCoreTypeSupportImpl.h"
#include "rtps_export.h"

#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/Hash.h>
#include <dds/DCPS/Message_Block_Ptr.h>
#include <dds/DCPS/SequenceNumber.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TimeTypes.h>
#include <dds/DCPS/TypeSupportImpl.h>
#include <dds/DCPS/Util.h>

#include <dds/DdsDcpsInfoUtilsC.h>
#include <dds/DdsDcpsInfoUtilsTypeSupportImpl.h>
#include <dds/OpenDDSConfigWrapper.h>

#include <ace/INET_Addr.h>
#include <ace/Message_Block.h>

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::GuidPrefix_t;
using DCPS::GUID_t;
using DCPS::assign;
using DCPS::EntityId_t;
using DCPS::GUID_tKeyLessThan;
using DCPS::LogGuid;
using DCPS::String;
using DCPS::SequenceNumber;
using DCPS::TimeDuration;
using DCPS::MonotonicTimePoint;
using DCPS::SystemTimePoint;
using DCPS::DCPS_debug_level;
using DCPS::RcHandle;

template <typename T>
void marshal_key_hash(const T& msg, KeyHash_t& hash)
{
  using DCPS::Serializer;
  using DCPS::Encoding;
  typedef DCPS::MarshalTraits<T> Traits;

  DCPS::KeyOnly<const T> ko(msg);

  static const size_t hash_limit = 16;
  std::memset(hash.value, 0, hash_limit);

  const Encoding encoding(Encoding::KIND_XCDR1, DCPS::ENDIAN_BIG);
  const OpenDDS::DCPS::SerializedSizeBound bound =
    Traits::key_only_serialized_size_bound(encoding);

  if (bound && bound.get() <= hash_limit) {
    // If it is bounded and can always fit in 16 bytes, we will use the
    // marshaled key
    ACE_Message_Block mb(hash_limit);
    Serializer out_serializer(&mb, encoding);
    out_serializer << ko;
    std::memcpy(hash.value, mb.rd_ptr(), mb.length());

  } else {
    // We will use the hash of the marshaled key
    ACE_Message_Block mb(serialized_size(encoding, ko));
    Serializer out_serializer(&mb, encoding);
    out_serializer << ko;

    DCPS::MD5Hash(hash.value, mb.rd_ptr(), mb.length());
  }
}

OpenDDS_Rtps_Export
const DCPS::Encoding& get_locators_encoding();

OpenDDS_Rtps_Export
DDS::ReturnCode_t blob_to_locators(const DCPS::TransportBLOB& blob,
                                   DCPS::LocatorSeq& locators,
                                   VendorId_t& vendor_id,
                                   bool* requires_inline_qos = 0,
                                   unsigned int* pBytesRead = 0);

OpenDDS_Rtps_Export
void locators_to_blob(const DCPS::LocatorSeq& locators,
                      const VendorId_t& vendor_id,
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

OpenDDS_Rtps_Export
bool bitmapNonEmpty(const SequenceNumberSet& snSet);

inline DCPS::SequenceNumber to_opendds_seqnum(const RTPS::SequenceNumber_t& rtps_seqnum)
{
  DCPS::SequenceNumber opendds_seqnum;
  opendds_seqnum.setValue(rtps_seqnum.high, rtps_seqnum.low);
  return opendds_seqnum;
}

inline RTPS::SequenceNumber_t to_rtps_seqnum(const DCPS::SequenceNumber& opendds_seqnum)
{
  RTPS::SequenceNumber_t rtps_seqnum;
  rtps_seqnum.high = opendds_seqnum.getHigh();
  rtps_seqnum.low = opendds_seqnum.getLow();
  return rtps_seqnum;
}

inline void append_submessage(RTPS::Message& message, const RTPS::InfoDestinationSubmessage& submessage)
{
  RTPS::Submessage sm;
  sm.info_dst_sm(submessage);
  DCPS::push_back(message.submessages, sm);
}

inline void append_submessage(RTPS::Message& message, const RTPS::InfoTimestampSubmessage& submessage)
{
  RTPS::Submessage sm;
  sm.info_ts_sm(submessage);
  DCPS::push_back(message.submessages, sm);
}

inline void append_submessage(RTPS::Message& message, const RTPS::DataSubmessage& submessage)
{
  RTPS::Submessage sm;
  sm.data_sm(submessage);
  DCPS::push_back(message.submessages, sm);
}

inline void append_submessage(RTPS::Message& message, const RTPS::DataFragSubmessage& submessage)
{
  RTPS::Submessage sm;
  sm.data_frag_sm(submessage);
  DCPS::push_back(message.submessages, sm);
}

#if OPENDDS_CONFIG_SECURITY
inline DDS::Security::ParticipantSecurityAttributesMask
security_attributes_to_bitmask(const DDS::Security::ParticipantSecurityAttributes& sec_attr)
{
  using namespace DDS::Security;

  ParticipantSecurityAttributesMask result = PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;
  if (sec_attr.is_rtps_protected) {
    result |= PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED;
  }
  if (sec_attr.is_discovery_protected) {
    result |= PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED;
  }
  if (sec_attr.is_liveliness_protected) {
    result |= PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED;
  }
  return result;
}

inline DDS::Security::EndpointSecurityAttributesMask
security_attributes_to_bitmask(const DDS::Security::EndpointSecurityAttributes& sec_attr)
{
  using namespace DDS::Security;

  EndpointSecurityAttributesMask result = ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;

  if (sec_attr.base.is_read_protected)
    result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_READ_PROTECTED;

  if (sec_attr.base.is_write_protected)
    result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_WRITE_PROTECTED;

  if (sec_attr.base.is_discovery_protected)
    result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED;

  if (sec_attr.base.is_liveliness_protected)
    result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED;

  if (sec_attr.is_submessage_protected)
    result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED;

  if (sec_attr.is_payload_protected)
    result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED;

  if (sec_attr.is_key_protected)
    result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_KEY_PROTECTED;

  return result;
}

inline DDS::OctetSeq
handle_to_octets(DDS::Security::NativeCryptoHandle handle)
{
  DDS::OctetSeq handleOctets(sizeof handle);
  handleOctets.length(handleOctets.maximum());
  unsigned char* rawHandleOctets = handleOctets.get_buffer();
  unsigned int handleTmp = handle;
  for (unsigned int j = sizeof handle; j > 0; --j) {
    rawHandleOctets[j - 1] = handleTmp & 0xff;
    handleTmp >>= 8;
  }
  return handleOctets;
}
#endif

// Default values for spec-defined parameters for determining what ports RTPS
// uses.
const DDS::UInt16 default_port_base = 7400; // (PB)
const DDS::UInt16 default_domain_gain = 250; // (DG)
const DDS::UInt16 default_part_gain = 2; // (PG)
const DDS::UInt16 default_spdp_multicast_offset = 0; // (D0)
const DDS::UInt16 default_spdp_unicast_offset = 10; // (D1)
const DDS::UInt16 default_user_multicast_offset = 1; // (D2)
const DDS::UInt16 default_user_unicast_offset = 11; // (D3)

// Default values for OpenDDS-specific parameters for determining what ports
// RTPS uses.
const DDS::UInt16 default_sedp_multicast_offset = 2; // (DX)
const DDS::UInt16 default_sedp_unicast_offset = 12; // (DY)

OpenDDS_Rtps_Export
bool get_rtps_port(DDS::UInt16& port_result, const char* what,
  DDS::UInt16 port_base, DDS::UInt16 offset,
  DDS::UInt16 domain, DDS::UInt16 domain_gain,
  DDS::UInt16 part = 0, DDS::UInt16 part_gain = 0);

enum PortMode {
  PortMode_System,
  PortMode_Probe
};

OpenDDS_Rtps_Export
PortMode get_port_mode(const String& key, PortMode default_value);

OpenDDS_Rtps_Export
void set_port_mode(const String& key, PortMode value);

OpenDDS_Rtps_Export
bool set_rtps_multicast_port(
  DCPS::NetworkAddress& addr, const char* what,
  DDS::UInt16 port_base, DDS::UInt16 offset,
  DDS::UInt16 domain, DDS::UInt16 domain_gain);

OpenDDS_Rtps_Export
bool set_rtps_unicast_port(
  DCPS::NetworkAddress& addr, bool& fixed_port,
  const char* what, PortMode port_mode,
  DDS::UInt16 port_base, DDS::UInt16 offset,
  DDS::UInt16 domain, DDS::UInt16 domain_gain,
  DDS::UInt16 part, DDS::UInt16 part_gain);

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_RTPS_MESSAGEUTILS_H */
