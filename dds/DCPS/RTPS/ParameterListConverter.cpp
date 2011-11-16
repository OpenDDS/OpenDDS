/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ParameterListConverter.h"
#include "dds/DCPS/GuidUtils.h"

namespace OpenDDS { namespace RTPS {

int
ParameterListConverter::to_param_list(
    const SPDPdiscoveredParticipantData& participant_data,
    ParameterList& param_list) const
{
  // Parameterize ParticipantBuiltinTopicData
  // Ignore participant builtin topic key 
  
  Parameter ud_param;
  ud_param.user_data(participant_data.ddsParticipantData.user_data);
  add_param(param_list, ud_param);
  
  // Parameterize ParticipantProxy_t
  Parameter pv_param;
  pv_param.version(participant_data.participantProxy.protocolVersion);
  add_param(param_list, pv_param);

  // For guid prefix, copy into guid, and force some values
  Parameter gp_param;
  GUID_t guid;
  ACE_OS::memcpy(guid.guidPrefix,
                 participant_data.participantProxy.guidPrefix,
                 sizeof(guid.guidPrefix));
  guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;

  gp_param.guid(guid);
  gp_param._d(PID_PARTICIPANT_GUID);
  add_param(param_list, gp_param);

  Parameter vid_param;
  vid_param.vendor(participant_data.participantProxy.vendorId);
  add_param(param_list, vid_param);

  Parameter eiq_param; // Default is false
  eiq_param.expects_inline_qos(
      participant_data.participantProxy.expectsInlineQos);
  add_param(param_list, eiq_param);

  Parameter abe_param;
  abe_param.builtin_endpoints(
      participant_data.participantProxy.availableBuiltinEndpoints);
  abe_param._d(PID_PARTICIPANT_BUILTIN_ENDPOINTS);
  add_param(param_list, abe_param);

  // Each locator
  add_param_locator_seq(
      param_list, 
      participant_data.participantProxy.metatrafficUnicastLocatorList,
      PID_METATRAFFIC_UNICAST_LOCATOR);
  
  add_param_locator_seq(
      param_list, 
      participant_data.participantProxy.metatrafficMulticastLocatorList,
      PID_METATRAFFIC_MULTICAST_LOCATOR);
  
  add_param_locator_seq(
      param_list, 
      participant_data.participantProxy.defaultUnicastLocatorList,
      PID_DEFAULT_UNICAST_LOCATOR);
  
  add_param_locator_seq(
      param_list, 
      participant_data.participantProxy.defaultMulticastLocatorList,
      PID_DEFAULT_MULTICAST_LOCATOR);
  
  Parameter ml_param;
  ml_param.count(participant_data.participantProxy.manualLivelinessCount);
  add_param(param_list, ml_param);

  // Parameterize Duration_t
  Parameter ld_param;
  ld_param.duration(participant_data.leaseDuration);
  add_param(param_list, ld_param);

  return 0;
}

int
ParameterListConverter::to_param_list(
    const DiscoveredWriterData& writer_data,
    ParameterList& param_list) const
{
  // Ignore builtin topic key

  {
    Parameter param;
    param.string_data(writer_data.ddsPublicationData.topic_name);
    param._d(PID_TOPIC_NAME);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.string_data(writer_data.ddsPublicationData.type_name);
    param._d(PID_TYPE_NAME);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.durability(writer_data.ddsPublicationData.durability);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.durability_service(writer_data.ddsPublicationData.durability_service);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.deadline(writer_data.ddsPublicationData.deadline);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.latency_budget(writer_data.ddsPublicationData.latency_budget);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.liveliness(writer_data.ddsPublicationData.liveliness);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.reliability(writer_data.ddsPublicationData.reliability);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.lifespan(writer_data.ddsPublicationData.lifespan);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.user_data(writer_data.ddsPublicationData.user_data);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.ownership(writer_data.ddsPublicationData.ownership);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.ownership_strength(writer_data.ddsPublicationData.ownership_strength);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.destination_order(writer_data.ddsPublicationData.destination_order);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.presentation(writer_data.ddsPublicationData.presentation);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.partition(writer_data.ddsPublicationData.partition);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.topic_data(writer_data.ddsPublicationData.topic_data);
    add_param(param_list, param);
  }
  {
    Parameter param;
    param.group_data(writer_data.ddsPublicationData.group_data);
    add_param(param_list, param);
  }
  return 0;
}

int
ParameterListConverter::from_param_list(
    const ParameterList& param_list,
    SPDPdiscoveredParticipantData& participant_data) const
{
  // Start by setting defaults
  participant_data.ddsParticipantData.user_data.value.length(0);
  participant_data.participantProxy.expectsInlineQos = false;
  participant_data.leaseDuration.seconds = 100;
  participant_data.leaseDuration.fraction = 0;

  CORBA::ULong length = param_list.length();
  for (CORBA::ULong i = 0; i < length; ++i) {
    Parameter param = param_list[i];
    switch (param._d()) {
      case PID_USER_DATA:
        participant_data.ddsParticipantData.user_data = param.user_data();
        break;
      case PID_PROTOCOL_VERSION:
        participant_data.participantProxy.protocolVersion = param.version();
        break;
      case PID_PARTICIPANT_GUID:
        memcpy(participant_data.participantProxy.guidPrefix,
               param.guid().guidPrefix, sizeof(GuidPrefix_t));
        break;
      case PID_VENDORID:
        memcpy(participant_data.participantProxy.vendorId.vendorId,
               param.vendor().vendorId, sizeof(OctetArray2));
        break;
      case PID_EXPECTS_INLINE_QOS:
        participant_data.participantProxy.expectsInlineQos = 
            param.expects_inline_qos();
        break;
      case PID_PARTICIPANT_BUILTIN_ENDPOINTS:
        participant_data.participantProxy.availableBuiltinEndpoints = 
            param.builtin_endpoints();
        break;
      case PID_METATRAFFIC_UNICAST_LOCATOR:
        append_locator(
            participant_data.participantProxy.metatrafficUnicastLocatorList,
            param.locator());
        break;
      case PID_METATRAFFIC_MULTICAST_LOCATOR:
        append_locator(
            participant_data.participantProxy.metatrafficMulticastLocatorList,
            param.locator());
        break;
      case PID_DEFAULT_UNICAST_LOCATOR:
        append_locator(
            participant_data.participantProxy.defaultUnicastLocatorList,
            param.locator());
        break;
      case PID_DEFAULT_MULTICAST_LOCATOR:
        append_locator(
            participant_data.participantProxy.defaultMulticastLocatorList,
            param.locator());
        break;
      case PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT:
        participant_data.participantProxy.manualLivelinessCount.value = 
            param.count().value;
        break;
      case PID_PARTICIPANT_LEASE_DURATION:
        participant_data.leaseDuration = param.duration();
        break;
      case PID_SENTINEL:
      case PID_PAD:
        // ignore
        break;
      default:
        // If this param is vendor specific, ignore
        if (param._d() & PIDMASK_VENDOR_SPECIFIC) {
          // skip
        // Else if this param is not required for compatibility, ignore
        } else if (!(param._d() & PIDMASK_INCOMPATIBLE)) {
          // skip
        // Else error
        } else {
          return -1;
        }
    }
  }
  return 0;
}

void
ParameterListConverter::add_param(
    ParameterList& param_list,
    const Parameter& param) const
{
  CORBA::ULong length = param_list.length();
  param_list.length(length + 1);
  param_list[length] = param;
}

void
ParameterListConverter::add_param_locator_seq(
    ParameterList& param_list,
    const LocatorSeq& locator_seq,
    const ParameterId_t pid) const
{
  CORBA::ULong length = locator_seq.length();
  for (CORBA::ULong i = 0; i < length; ++i) {
    Parameter param;
    param.locator(locator_seq[i]);
    param._d(pid);
    add_param(param_list, param);
  }
}

void
ParameterListConverter::append_locator(
    LocatorSeq& list, 
    const Locator_t& locator) const
{
  CORBA::ULong length = list.length();
  list.length(length + 1); 
  list[length] = locator;
}

} }

