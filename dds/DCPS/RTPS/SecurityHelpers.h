/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_SECURITYHELPERS_H
#define OPENDDS_DCPS_RTPS_SECURITYHELPERS_H

#include <dds/DCPS/Ice.h>
#include <dds/DCPS/XTypes/TypeObject.h>

#include <dds/DdsSecurityCoreC.h>
#include <dds/DdsDcpsGuidC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using OpenDDS::DCPS::EntityId_t;

/**
 * The below entities are from the security spec. V1.1
 * section 7.3.7.1 "Mapping of the EntityIds for the Builtin DataWriters and DataReaders"
 */
///@{
const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER = {{0xff, 0x00, 0x03}, 0xc2};
const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER = {{0xff, 0x00, 0x03}, 0xc7};
const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER = {{0xff, 0x00, 0x04}, 0xc2};
const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER = {{0xff, 0x00, 0x04}, 0xc7};
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER = {{0xff, 0x02, 0x00}, 0xc2};
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER = {{0xff, 0x02, 0x00}, 0xc7};
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER = {{0x00, 0x02, 0x01}, 0xc3};
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER = {{0x00, 0x02, 0x01}, 0xc4};
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER = {{0xff, 0x02, 0x02}, 0xc3};
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER = {{0xff, 0x02, 0x02}, 0xc4};
const EntityId_t ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER = {{0xff, 0x01, 0x01}, 0xc2};
const EntityId_t ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER = {{0xff, 0x01, 0x01}, 0xc7};
// https://issues.omg.org/browse/DDSSEC12-87
const EntityId_t ENTITYID_TL_SVC_REQ_WRITER_SECURE = {{0xff, 0x03, 0x00}, 0xc3 };
const EntityId_t ENTITYID_TL_SVC_REQ_READER_SECURE = {{0xff, 0x03, 0x00}, 0xc4 };
const EntityId_t ENTITYID_TL_SVC_REPLY_WRITER_SECURE = {{0xff, 0x03, 0x01}, 0xc3 };
const EntityId_t ENTITYID_TL_SVC_REPLY_READER_SECURE = {{0xff, 0x03, 0x01}, 0xc4 };
///@}

const DDS::Security::ParticipantSecurityInfo PARTICIPANT_SECURITY_ATTRIBUTES_INFO_DEFAULT = {0, 0};
const DDS::Security::EndpointSecurityInfo ENDPOINT_SECURITY_ATTRIBUTES_INFO_DEFAULT = {0, 0};

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

struct DiscoveredPublication_SecurityWrapper {
  DCPS::DiscoveredWriterData data;
  DDS::Security::EndpointSecurityInfo security_info;
  DDS::Security::DataTags data_tags;
  bool have_ice_agent_info;
  ICE::AgentInfo ice_agent_info;
  XTypes::TypeInformation type_info;

  DiscoveredPublication_SecurityWrapper()
    : have_ice_agent_info(false) {
    security_info.endpoint_security_attributes = 0;
    security_info.plugin_endpoint_security_attributes = 0;
  }
};

struct DiscoveredSubscription_SecurityWrapper {
  DCPS::DiscoveredReaderData data;
  DDS::Security::EndpointSecurityInfo security_info;
  DDS::Security::DataTags data_tags;
  bool have_ice_agent_info;
  ICE::AgentInfo ice_agent_info;
  XTypes::TypeInformation type_info;

  DiscoveredSubscription_SecurityWrapper()
    : have_ice_agent_info(false) {
    security_info.endpoint_security_attributes = 0;
    security_info.plugin_endpoint_security_attributes = 0;
  }
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
