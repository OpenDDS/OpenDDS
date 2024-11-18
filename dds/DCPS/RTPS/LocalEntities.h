/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef OPENDDS_DCPS_RTPS_LOCAL_ENTITIES_H
#define OPENDDS_DCPS_RTPS_LOCAL_ENTITIES_H

#include <dds/OpenDDSConfigWrapper.h>
#include <dds/Versioned_Namespace.h>

#include <dds/DCPS/DataReaderCallbacks.h>
#include <dds/DCPS/DataWriterCallbacks.h>
#include <dds/DCPS/Discovery.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/SequenceNumber.h>
#include <dds/DCPS/Time_Helper.h>

#include <dds/DCPS/XTypes/TypeObject.h>

#include <dds/DdsDcpsGuidC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#if OPENDDS_CONFIG_SECURITY
#  include <dds/DCPS/Ice.h>
#  include <dds/DdsSecurityCoreC.h>
#endif

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

struct LocalEntity {
  LocalEntity()
    : topic_id_(DCPS::GUID_UNKNOWN)
    , participant_discovered_at_(DCPS::monotonic_time_zero())
    , transport_context_(0)
    , sequence_(DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN())
#if OPENDDS_CONFIG_SECURITY
    , have_ice_agent_info(false)
  {
    security_attribs_.base.is_read_protected = false;
    security_attribs_.base.is_write_protected = false;
    security_attribs_.base.is_discovery_protected = false;
    security_attribs_.base.is_liveliness_protected = false;
    security_attribs_.is_submessage_protected = false;
    security_attribs_.is_payload_protected = false;
    security_attribs_.is_key_protected = false;
    security_attribs_.plugin_endpoint_attributes = 0;
  }
#else
  {}
#endif

  bool isDiscoveryProtected() const
  {
#if OPENDDS_CONFIG_SECURITY
    return security_attribs_.base.is_discovery_protected;
#else
    return false;
#endif
  }

  const XTypes::TypeInformation* typeInfoFor(const DCPS::GUID_t& remote,
                                             bool* usedFlexible = 0) const
  {
    const FlexibleTypeMap::const_iterator found = flexible_types_.find(remote);
    if (found == flexible_types_.end()) {
      return &type_info_.xtypes_type_info_;
    }
    if (usedFlexible) {
      *usedFlexible = true;
    }
    return &found->second;
  }

  DCPS::GUID_t topic_id_;
  DCPS::TransportLocatorSeq trans_info_;
  DCPS::MonotonicTime_t participant_discovered_at_;
  ACE_CDR::ULong transport_context_;
  DCPS::RepoIdSet matched_endpoints_;
  // This is the sequence number assigned to this "sample" for durable replay.
  DCPS::SequenceNumber sequence_;
  DCPS::RepoIdSet remote_expectant_opendds_associations_;
  DCPS::TypeInformation type_info_;
#if OPENDDS_CONFIG_SECURITY
  bool have_ice_agent_info;
  ICE::AgentInfo ice_agent_info;
  DDS::Security::EndpointSecurityAttributes security_attribs_;
#endif
  typedef OPENDDS_MAP_CMP(DCPS::GUID_t, XTypes::TypeInformation, DCPS::GUID_tKeyLessThan) FlexibleTypeMap;
  FlexibleTypeMap flexible_types_;
};

struct LocalPublication : LocalEntity {
  DCPS::DataWriterCallbacks_wrch publication_;
  DDS::DataWriterQos qos_;
  DDS::PublisherQos publisher_qos_;
};

struct LocalSubscription : LocalEntity {
  DCPS::DataReaderCallbacks_wrch subscription_;
  DDS::DataReaderQos qos_;
  DDS::SubscriberQos subscriber_qos_;
  DCPS::ContentFilterProperty_t filterProperties;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_RTPS_LOCAL_ENTITIES_H
