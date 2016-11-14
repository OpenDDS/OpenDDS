/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RTPS_BASEMESSAGETYPES_H
#define RTPS_BASEMESSAGETYPES_H

#include "RtpsCoreC.h"
#include "dds/DCPS/GuidUtils.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace RTPS {

    const Time_t TIME_ZERO     = { 0, 0 };
    const Time_t TIME_INVALID  = { -1, 0xffffffff };
    const Time_t TIME_INFINITE = { 0x7fffffff, 0xffffffff };

    // conversion factor from nanoseconds to NTP fractional (2^-32) seconds
    const double NANOS_TO_RTPS_FRACS = 4.294967296;

    const VendorId_t VENDORID_UNKNOWN = { { 0 } };
    const VendorId_t VENDORID_OPENDDS =
      { { OpenDDS::DCPS::VENDORID_OCI[0], OpenDDS::DCPS::VENDORID_OCI[1] } };
    // --> see http://portals.omg.org/dds/content/page/dds-rtps-vendor-ids

    using OpenDDS::DCPS::GUIDPREFIX_UNKNOWN;
    using OpenDDS::DCPS::GUID_UNKNOWN;

    using OpenDDS::DCPS::ENTITYID_UNKNOWN;
    using OpenDDS::DCPS::ENTITYID_PARTICIPANT;
    using OpenDDS::DCPS::ENTITYID_SEDP_BUILTIN_TOPIC_WRITER;
    using OpenDDS::DCPS::ENTITYID_SEDP_BUILTIN_TOPIC_READER;
    using OpenDDS::DCPS::ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    using OpenDDS::DCPS::ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    using OpenDDS::DCPS::ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    using OpenDDS::DCPS::ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    using OpenDDS::DCPS::ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER;
    using OpenDDS::DCPS::ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER;
    using OpenDDS::DCPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    using OpenDDS::DCPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;

    const SequenceNumber_t SEQUENCENUMBER_UNKNOWN = { -1, 0 };

    const OpenDDS::DCPS::Locator_t LOCATOR_INVALID =
      { LOCATOR_KIND_INVALID, LOCATOR_PORT_INVALID, { 0 } };
    const OpenDDS::DCPS::OctetArray16 LOCATOR_ADDRESS_INVALID = { 0 };

    const LocatorUDPv4_t LOCATORUDPv4_INVALID = { 0, 0 };

    const ProtocolVersion_t PROTOCOLVERSION_1_0 = { 1, 0 };
    const ProtocolVersion_t PROTOCOLVERSION_1_1 = { 1, 1 };
    const ProtocolVersion_t PROTOCOLVERSION_2_0 = { 2, 0 };
    const ProtocolVersion_t PROTOCOLVERSION_2_1 = { 2, 1 };
    const ProtocolVersion_t PROTOCOLVERSION_2_2 = { 2, 2 };
    const ProtocolVersion_t PROTOCOLVERSION = PROTOCOLVERSION_2_2;
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* RTPS_BASEMESSAGETYPES_H */
