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
ParameterListConverter::from_param_list(
    const ParameterList& param_list,
    SPDPdiscoveredParticipantData& participant_data) const
{
  size_t length = param_list.length();
  for (size_t i = 0; i < length; ++i) {
    Parameter param = param_list[i];
    switch (param._d()) {
      case PID_USER_DATA:
        participant_data.ddsParticipantData.user_data = param.user_data();
        break;
      case PID_PROTOCOL_VERSION:
        participant_data.participantProxy.protocolVersion = param.version();
        break;
      default:
        return -1;
    }
  }
  return 0;
}

void
ParameterListConverter::add_param(
    ParameterList& param_list,
    const Parameter& param) const
{
  size_t length = param_list.length();
  param_list.length(length + 1);
  param_list[length] = param;
}

void
ParameterListConverter::add_param_locator_seq(
    ParameterList& param_list,
    const LocatorSeq& locator_seq,
    const ParameterId_t pid) const
{
  size_t length = locator_seq.length();
  for (size_t i = 0; i < length; ++i) {
    Parameter param;
    param.locator(locator_seq[i]);
    param._d(pid);
    add_param(param_list, param);
  }
}

} }

