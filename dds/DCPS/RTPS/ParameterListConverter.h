/*
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_PARAMETERLISTCONVERTER_H
#define OPENDDS_DCPS_RTPS_PARAMETERLISTCONVERTER_H

#include "rtps_export.h"
#include "RtpsCoreC.h"

#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY
#include "RtpsSecurityC.h"
#endif

#include "ICE/Ice.h"

#include "dds/DCPS/XTypes/TypeObject.h"
#include "dds/DCPS/BuiltInTopicUtils.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {
namespace ParameterListConverter {

#if OPENDDS_CONFIG_SECURITY
struct DiscoveredPublication_SecurityWrapper {
  DCPS::DiscoveredWriterData data;
  DDS::Security::EndpointSecurityInfo security_info;
  DDS::Security::DataTags data_tags;
  bool have_ice_agent_info;
  ICE::AgentInfo ice_agent_info;
  XTypes::TypeInformation type_info;

  DiscoveredPublication_SecurityWrapper()
    : have_ice_agent_info(false)
  {
    data.ddsPublicationData.key = DCPS::BUILTIN_TOPIC_KEY_UNKNOWN;
    data.ddsPublicationData.participant_key = DCPS::BUILTIN_TOPIC_KEY_UNKNOWN;
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
    : have_ice_agent_info(false)
  {
    data.ddsSubscriptionData.key = DCPS::BUILTIN_TOPIC_KEY_UNKNOWN;
    data.ddsSubscriptionData.participant_key = DCPS::BUILTIN_TOPIC_KEY_UNKNOWN;
    security_info.endpoint_security_attributes = 0;
    security_info.plugin_endpoint_security_attributes = 0;
  }
};
#endif

// DDS::ParticipantBuiltinTopicData

OpenDDS_Rtps_Export
bool to_param_list(const DDS::ParticipantBuiltinTopicData& pbtd,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DDS::ParticipantBuiltinTopicData& pbtd);

#if OPENDDS_CONFIG_SECURITY
// DDS::Security::ParticipantBuiltinTopicData

OpenDDS_Rtps_Export
bool to_param_list(const DDS::Security::ParticipantBuiltinTopicData& pbtd,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DDS::Security::ParticipantBuiltinTopicData& pbtd);

// DDS::Security::ParticipantBuiltinTopicDataSecure

OpenDDS_Rtps_Export
bool to_param_list(const DDS::Security::ParticipantBuiltinTopicDataSecure& pbtds,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DDS::Security::ParticipantBuiltinTopicDataSecure& pbtds);
#endif

// OpenDDS::RTPS::ParticipantProxy_t

OpenDDS_Rtps_Export
bool to_param_list(const ParticipantProxy_t& proxy,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     ParticipantProxy_t& proxy);

// OpenDDS::RTPS::Duration_t

OpenDDS_Rtps_Export
bool to_param_list(const Duration_t& duration,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     Duration_t& duration);

// OpenDDS::RTPS::SPDPdiscoveredParticipantData

OpenDDS_Rtps_Export
bool to_param_list(const SPDPdiscoveredParticipantData& participant_data,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     SPDPdiscoveredParticipantData& participant_data);

#if OPENDDS_CONFIG_SECURITY
// OpenDDS::Security::SPDPdiscoveredParticipantData

OpenDDS_Rtps_Export
bool to_param_list(const OpenDDS::Security::SPDPdiscoveredParticipantData& participant_data,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     OpenDDS::Security::SPDPdiscoveredParticipantData& participant_data);
#endif

// OpenDDS::DCPS::DiscoveredWriterData

OpenDDS_Rtps_Export
void add_DataRepresentationQos(ParameterList& param_list, const DDS::DataRepresentationIdSeq& ids);

OpenDDS_Rtps_Export
bool to_param_list(const DCPS::DiscoveredWriterData& writer_data,
                   ParameterList& param_list,
                   bool use_xtypes,
                   const XTypes::TypeInformation& type_info,
                   bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DCPS::DiscoveredWriterData& writer_data,
                     bool use_xtypes,
                     XTypes::TypeInformation& type_info);

// OpenDDS::DCPS::DiscoveredReaderData

OpenDDS_Rtps_Export
bool to_param_list(const DCPS::DiscoveredReaderData& reader_data,
                   ParameterList& param_list,
                   bool use_xtypes,
                   const XTypes::TypeInformation& type_info,
                   bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DCPS::DiscoveredReaderData& reader_data,
                     bool use_xtypes,
                     XTypes::TypeInformation& type_info);

#if OPENDDS_CONFIG_SECURITY
// DDS::Security::EndpointSecurityInfo

OpenDDS_Rtps_Export
bool to_param_list(const DDS::Security::EndpointSecurityInfo& info,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DDS::Security::EndpointSecurityInfo& info);

// DDS::Security::DataTags data_tags

OpenDDS_Rtps_Export
bool to_param_list(const DDS::Security::DataTags& tags,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DDS::Security::DataTags& tags);

// DiscoveredPublication_SecurityWrapper

OpenDDS_Rtps_Export
bool to_param_list(const DiscoveredPublication_SecurityWrapper& wrapper,
                   ParameterList& param_list,
                   bool use_xtypes,
                   const XTypes::TypeInformation& type_info,
                   bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DiscoveredPublication_SecurityWrapper& wrapper,
                     bool use_xtypes,
                     XTypes::TypeInformation& type_info);

// DiscoveredSubscription_SecurityWrapper

OpenDDS_Rtps_Export
bool to_param_list(const DiscoveredSubscription_SecurityWrapper& wrapper,
                   ParameterList& param_list,
                   bool use_xtypes,
                   const XTypes::TypeInformation& type_info,
                   bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DiscoveredSubscription_SecurityWrapper& wrapper,
                     bool use_xtypes,
                     XTypes::TypeInformation& type_info);

// Extensions for ICE

OpenDDS_Rtps_Export
bool to_param_list(const ICE::AgentInfoMap& ai_map,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     ICE::AgentInfoMap& ai_map);
#endif

}
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
