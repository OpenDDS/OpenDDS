/*
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PARAMETER_LIST_CONVERTER_H
#define PARAMETER_LIST_CONVERTER_H

#include "dds/DCPS/RTPS/rtps_export.h"
#include "dds/DCPS/RTPS/RtpsCoreC.h"

#include "dds/DCPS/RTPS/ICE/Ice.h"

#ifdef OPENDDS_SECURITY
#include "dds/DCPS/RTPS/RtpsSecurityC.h"
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

#ifdef OPENDDS_SECURITY
struct DiscoveredPublication_SecurityWrapper;
struct DiscoveredSubscription_SecurityWrapper;
#endif

namespace ParameterListConverter {

// DDS::ParticipantBuiltinTopicData

OpenDDS_Rtps_Export
bool to_param_list(const DDS::ParticipantBuiltinTopicData& pbtd,
                   ParameterList& param_list);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DDS::ParticipantBuiltinTopicData& pbtd);

#ifdef OPENDDS_SECURITY
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

#ifdef OPENDDS_SECURITY
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
bool to_param_list(const DCPS::DiscoveredWriterData& writer_data,
                   ParameterList& param_list,
                   bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                    DCPS::DiscoveredWriterData& writer_data);

// OpenDDS::DCPS::DiscoveredReaderData

OpenDDS_Rtps_Export
bool to_param_list(const DCPS::DiscoveredReaderData& reader_data,
                   ParameterList& param_list,
                   bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DCPS::DiscoveredReaderData& reader_data);

#ifdef OPENDDS_SECURITY
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
                   bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DiscoveredPublication_SecurityWrapper& wrapper);

// DiscoveredSubscription_SecurityWrapper

OpenDDS_Rtps_Export
bool to_param_list(const DiscoveredSubscription_SecurityWrapper& wrapper,
                   ParameterList& param_list,
                   bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
bool from_param_list(const ParameterList& param_list,
                     DiscoveredSubscription_SecurityWrapper& wrapper);

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
