/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DDS_SECURITY_ENTITIES
#define DDS_SECURITY_ENTITIES

#include "dds/DdsDcpsGuidC.h"

using OpenDDS::DCPS::EntityId_t;

namespace DDS {
namespace Security {

/*
 * The below entities are from the security spec. V1.1
 * section 7.3.7.1 "Mapping of the EntityIds for the Builtin DataWriters and DataReaders"
 */
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

const ParticipantSecurityInfo PARTICIPANT_SECURITY_ATTRIBUTES_INFO_DEFAULT = {0, 0};
const EndpointSecurityInfo ENDPOINT_SECURITY_ATTRIBUTES_INFO_DEFAULT = {0, 0};

}} /* DDS::Security */

#endif
