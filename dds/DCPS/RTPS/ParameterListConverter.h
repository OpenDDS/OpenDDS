/*
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PARAMETER_LIST_CONVERTER_H
#define PARAMETER_LIST_CONVERTER_H

#include "dds/DCPS/RTPS/rtps_export.h"
#include "dds/DCPS/RTPS/RtpsCoreC.h"
#include "dds/DCPS/RTPS/RtpsSecurityC.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

struct DiscoveredWriterData_SecurityWrapper;
struct DiscoveredReaderData_SecurityWrapper;

namespace ParameterListConverter {

// DDS::ParticipantBuiltinTopicData

OpenDDS_Rtps_Export
int to_param_list(const DDS::ParticipantBuiltinTopicData& pbtd,
                  ParameterList& param_list);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                    DDS::ParticipantBuiltinTopicData& pbtd);

// DDS::Security::ParticipantBuiltinTopicData

OpenDDS_Rtps_Export
int to_param_list(const DDS::Security::ParticipantBuiltinTopicData& pbtd,
                  ParameterList& param_list);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                    DDS::Security::ParticipantBuiltinTopicData& pbtd);

// DDS::Security::ParticipantBuiltinTopicDataSecure

OpenDDS_Rtps_Export
int to_param_list(const DDS::Security::ParticipantBuiltinTopicDataSecure& pbtds,
                  ParameterList& param_list);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                    DDS::Security::ParticipantBuiltinTopicDataSecure& pbtds);

// OpenDDS::RTPS::ParticipantProxy_t

OpenDDS_Rtps_Export
int to_param_list(const ParticipantProxy_t& proxy,
                  ParameterList& param_list);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                    ParticipantProxy_t& proxy);

// OpenDDS::RTPS::Duration_t

OpenDDS_Rtps_Export
int to_param_list(const Duration_t& duration,
                  ParameterList& param_list);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                    Duration_t& duration);

// OpenDDS::RTPS::SPDPdiscoveredParticipantData

OpenDDS_Rtps_Export
int to_param_list(const SPDPdiscoveredParticipantData& participant_data,
                  ParameterList& param_list);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                    SPDPdiscoveredParticipantData& participant_data);

// OpenDDS::Security::SPDPdiscoveredParticipantData

OpenDDS_Rtps_Export
int to_param_list(const OpenDDS::Security::SPDPdiscoveredParticipantData& participant_data,
                  ParameterList& param_list);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                    OpenDDS::Security::SPDPdiscoveredParticipantData& participant_data);

// OpenDDS::DCPS::DiscoveredWriterData

OpenDDS_Rtps_Export
int to_param_list(const DCPS::DiscoveredWriterData& writer_data,
                  ParameterList& param_list,
                  bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                   DCPS::DiscoveredWriterData& writer_data);

// OpenDDS::DCPS::DiscoveredReaderData

OpenDDS_Rtps_Export
int to_param_list(const DCPS::DiscoveredReaderData& reader_data,
                  ParameterList& param_list,
                  bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                   DCPS::DiscoveredReaderData& reader_data);

// DDS::Security::EndpointSecurityInfo

OpenDDS_Rtps_Export
int to_param_list(const DDS::Security::EndpointSecurityInfo& info,
                  ParameterList& param_list);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                    DDS::Security::EndpointSecurityInfo& info);

// DDS::Security::DataTags& data_tags

OpenDDS_Rtps_Export
int to_param_list(const DDS::Security::DataTags& tags,
                  ParameterList& param_list);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                    DDS::Security::DataTags& tags);

// DiscoveredWriterData_SecurityWrapper

OpenDDS_Rtps_Export
int to_param_list(const DiscoveredWriterData_SecurityWrapper& wrapper,
                  ParameterList& param_list,
                  bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                   DiscoveredWriterData_SecurityWrapper& wrapper);

// DiscoveredReaderData_SecurityWrapper

OpenDDS_Rtps_Export
int to_param_list(const DiscoveredReaderData_SecurityWrapper& wrapper,
                  ParameterList& param_list,
                  bool map = false /*map IPV4 to IPV6 addr*/);

OpenDDS_Rtps_Export
int from_param_list(const ParameterList& param_list,
                   DiscoveredReaderData_SecurityWrapper& wrapper);

}
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
