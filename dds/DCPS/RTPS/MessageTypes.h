/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_MESSAGETYPES_H
#define OPENDDS_DCPS_RTPS_MESSAGETYPES_H

#include "RtpsCoreC.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace RTPS {

    using OpenDDS::DCPS::EntityId_t;

    /**
     * The below entities 
     are from the security spec. V1.1
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
    #ifdef OPENDDS_SECURITY
    const DDS::Security::ParticipantSecurityInfo PARTICIPANT_SECURITY_ATTRIBUTES_INFO_DEFAULT = {0, 0};
    const DDS::Security::EndpointSecurityInfo ENDPOINT_SECURITY_ATTRIBUTES_INFO_DEFAULT = {0, 0};
    #endif 
    // end of EntityId section

    // For messages we create, the "octetsToInlineQoS" value will be constant.
    const ACE_CDR::UShort DATA_OCTETS_TO_IQOS = 16;
    const ACE_CDR::UShort DATA_FRAG_OCTETS_TO_IQOS = 28;

    const ACE_CDR::UShort RTPSHDR_SZ = 20, // size of RTPS Message Header
      SMHDR_SZ = 4, // size of SubmessageHeader
      HEARTBEAT_SZ = 28, // size (octetsToNextHeader) of HeartBeatSubmessage
      INFO_DST_SZ = 12, // size (octetsToNextHeader) of InfoDestSubmessage
      INFO_TS_SZ = 8, // size of InfoTimestampSubmessage with FLAG_I == 0
      INFO_SRC_SZ = 20; // size (octetsToNextHeader) of InfoSourceSubmessage

    /// Alignment of RTPS Submessage
    const size_t SM_ALIGN = 4;

    const DCPS::EntityId_t PARTICIPANT_MESSAGE_DATA_KIND_UNKNOWN =
      {{0x00, 0x00, 0x00}, 0x00};
    const DCPS::EntityId_t PARTICIPANT_MESSAGE_DATA_KIND_AUTOMATIC_LIVELINESS_UPDATE =
      {{0x00, 0x00, 0x00}, 0x01};
    const DCPS::EntityId_t PARTICIPANT_MESSAGE_DATA_KIND_MANUAL_LIVELINESS_UPDATE =
      {{0x00, 0x00, 0x00}, 0x02};
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* RTPS_MESSAGETYPES_H */
