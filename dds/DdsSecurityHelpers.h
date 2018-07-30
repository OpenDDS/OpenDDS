/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DDS_SECURITY_HELPERS_H
#define DDS_SECURITY_HELPERS_H

#include "dds/DdsSecurityCoreC.h"

namespace DDS {
namespace Security {

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

inline void
security_bitmask_to_attributes(const DDS::Security::ParticipantSecurityAttributesMask& mask,
                               DDS::Security::ParticipantSecurityAttributes& sec_attr)
{
  using namespace DDS::Security;

  sec_attr.is_rtps_protected = (mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED);
  sec_attr.is_discovery_protected = (mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED);
  sec_attr.is_liveliness_protected = (mask & PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED);
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

inline void
security_bitmask_to_attributes(const DDS::Security::EndpointSecurityAttributesMask& mask,
                               DDS::Security::EndpointSecurityAttributes& sec_attr)
{
  using namespace DDS::Security;

  sec_attr.base.is_read_protected = (mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_READ_PROTECTED);
  sec_attr.base.is_write_protected = (mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_WRITE_PROTECTED);
  sec_attr.base.is_discovery_protected = (mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED);
  sec_attr.base.is_liveliness_protected = (mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED);
  sec_attr.is_submessage_protected = (mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED);
  sec_attr.is_payload_protected = (mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED);
  sec_attr.is_key_protected = (mask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_KEY_PROTECTED);
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

}
}

#endif
