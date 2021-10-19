/*
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/GuidBuilder.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/GuidGenerator.h"
#include "dds/DCPS/RTPS/ParameterListConverter.h"
#include "dds/DCPS/RTPS/RtpsCoreC.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "gtest/gtest.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>

using namespace DDS;
using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS::ParameterListConverter;
using namespace OpenDDS::RTPS;

#ifdef OPENDDS_SECURITY
using namespace DDS::Security;
using namespace OpenDDS::Security;
#endif

using namespace std;

namespace {
  GuidGenerator guid_generator;

  namespace Factory {
    Locator_t locator(long kind,
                      unsigned long port,
                      unsigned char addr0,
                      unsigned char addr1,
                      unsigned char addr2,
                      unsigned char addr3)
    {
      Locator_t result;
      result.kind = kind;
      result.port = port;
      memset(result.address, 0, sizeof(result.address));
      result.address[12] = addr0;
      result.address[13] = addr1;
      result.address[14] = addr2;
      result.address[15] = addr3;

      return result;
    }

    OpenDDS::RTPS::SPDPdiscoveredParticipantData spdp_participant(
      const void* user_data = NULL,
      CORBA::ULong user_data_len = 0,
      char major_protocol_version = 0,
      char minor_protocol_version = 0,
      char* vendor_id = NULL,
      GUID_t* guid = NULL,
      bool expects_inline_qos = false,
      unsigned long builtin_endpoints = 0,
      Locator_t* mtu_locs = NULL,
      CORBA::ULong num_mtu_locs = 0,
      Locator_t* mtm_locs = NULL,
      CORBA::ULong num_mtm_locs = 0,
      Locator_t* du_locs = NULL,
      CORBA::ULong num_du_locs = 0,
      Locator_t* dm_locs = NULL,
      CORBA::ULong num_dm_locs = 0,
      long liveliness_count = 0,
      long lease_dur_seconds = 100,
      unsigned long lease_dur_fraction = 0
    )
    {
      OpenDDS::RTPS::SPDPdiscoveredParticipantData result;
      if (user_data_len && user_data) {
        result.ddsParticipantData.user_data.value.length(user_data_len);
        for (CORBA::ULong i = 0; i < user_data_len; ++i) {
          result.ddsParticipantData.user_data.value[i] = ((char*)user_data)[i];
        }
      }
      if (major_protocol_version && minor_protocol_version) {
        result.participantProxy.protocolVersion.major = major_protocol_version;
        result.participantProxy.protocolVersion.minor = minor_protocol_version;
      }
      if (vendor_id) {
        result.participantProxy.vendorId.vendorId[0] = vendor_id[0];
        result.participantProxy.vendorId.vendorId[1] = vendor_id[1];
      }
      if (guid) {
        memcpy(result.participantProxy.guidPrefix,
               guid->guidPrefix,
               sizeof(guid->guidPrefix));
      }
      result.participantProxy.expectsInlineQos = expects_inline_qos;
      result.participantProxy.availableBuiltinEndpoints = builtin_endpoints;

      if (num_mtu_locs && mtu_locs) {
        result.participantProxy.metatrafficUnicastLocatorList.length(num_mtu_locs);
        for (CORBA::ULong i = 0; i < num_mtu_locs; ++i) {
          result.participantProxy.metatrafficUnicastLocatorList[i] =
              mtu_locs[i];
        }
      }

      if (num_mtm_locs && mtm_locs) {
        result.participantProxy.metatrafficMulticastLocatorList.length(num_mtm_locs);
        for (CORBA::ULong i = 0; i < num_mtm_locs; ++i) {
          result.participantProxy.metatrafficMulticastLocatorList[i] =
              mtm_locs[i];
        }
      }

      if (num_du_locs && du_locs) {
        result.participantProxy.defaultUnicastLocatorList.length(num_du_locs);
        for (CORBA::ULong i = 0; i < num_du_locs; ++i) {
          result.participantProxy.defaultUnicastLocatorList[i] = du_locs[i];
        }
      }

      if (num_dm_locs && dm_locs) {
        result.participantProxy.defaultMulticastLocatorList.length(num_dm_locs);
        for (CORBA::ULong i = 0; i < num_dm_locs; ++i) {
          result.participantProxy.defaultMulticastLocatorList[i] = dm_locs[i];
        }
      }

      if (liveliness_count) {
        result.participantProxy.manualLivelinessCount.value = liveliness_count;
      }

      result.leaseDuration.seconds = lease_dur_seconds;
      result.leaseDuration.fraction = lease_dur_fraction;

      return result;
    }

    OpenDDS::RTPS::SPDPdiscoveredParticipantData default_participant_data()
    {
      OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data;
      part_data.ddsParticipantData.user_data =
          TheServiceParticipant->initial_UserDataQosPolicy();
      part_data.participantProxy.expectsInlineQos = false;
      part_data.leaseDuration.seconds = 100;
      part_data.leaseDuration.fraction = 0;
      return part_data;
    }

    DiscoveredWriterData default_writer_data()
    {
      DiscoveredWriterData writer_data;
      writer_data.ddsPublicationData.durability =
          TheServiceParticipant->initial_DurabilityQosPolicy();
      writer_data.ddsPublicationData.durability_service =
          TheServiceParticipant->initial_DurabilityServiceQosPolicy();
      writer_data.ddsPublicationData.deadline =
          TheServiceParticipant->initial_DeadlineQosPolicy();
      writer_data.ddsPublicationData.latency_budget =
          TheServiceParticipant->initial_LatencyBudgetQosPolicy();
      writer_data.ddsPublicationData.liveliness =
          TheServiceParticipant->initial_LivelinessQosPolicy();
      writer_data.ddsPublicationData.reliability =
          TheServiceParticipant->initial_DataWriterQos().reliability;
      writer_data.ddsPublicationData.lifespan =
          TheServiceParticipant->initial_LifespanQosPolicy();
      writer_data.ddsPublicationData.user_data =
          TheServiceParticipant->initial_UserDataQosPolicy();
      writer_data.ddsPublicationData.ownership =
          TheServiceParticipant->initial_OwnershipQosPolicy();
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      writer_data.ddsPublicationData.ownership_strength =
          TheServiceParticipant->initial_OwnershipStrengthQosPolicy();
#endif
      writer_data.ddsPublicationData.destination_order =
          TheServiceParticipant->initial_DestinationOrderQosPolicy();
      writer_data.ddsPublicationData.presentation =
          TheServiceParticipant->initial_PresentationQosPolicy();
      writer_data.ddsPublicationData.partition =
          TheServiceParticipant->initial_PartitionQosPolicy();
      writer_data.ddsPublicationData.topic_data =
          TheServiceParticipant->initial_TopicDataQosPolicy();
      writer_data.ddsPublicationData.group_data =
          TheServiceParticipant->initial_GroupDataQosPolicy();

      memset (&writer_data.writerProxy.remoteWriterGuid, 0, sizeof (GUID_t));

      return writer_data;
    }

    DiscoveredReaderData default_reader_data()
    {
      DiscoveredReaderData reader_data;
      reader_data.ddsSubscriptionData.durability =
          TheServiceParticipant->initial_DurabilityQosPolicy();
      reader_data.ddsSubscriptionData.deadline =
          TheServiceParticipant->initial_DeadlineQosPolicy();
      reader_data.ddsSubscriptionData.latency_budget =
          TheServiceParticipant->initial_LatencyBudgetQosPolicy();
      reader_data.ddsSubscriptionData.liveliness =
          TheServiceParticipant->initial_LivelinessQosPolicy();
      reader_data.ddsSubscriptionData.reliability =
          TheServiceParticipant->initial_DataReaderQos().reliability;
      reader_data.ddsSubscriptionData.ownership =
          TheServiceParticipant->initial_OwnershipQosPolicy();
      reader_data.ddsSubscriptionData.destination_order =
          TheServiceParticipant->initial_DestinationOrderQosPolicy();
      reader_data.ddsSubscriptionData.user_data =
          TheServiceParticipant->initial_UserDataQosPolicy();
      reader_data.ddsSubscriptionData.time_based_filter =
          TheServiceParticipant->initial_TimeBasedFilterQosPolicy();
      reader_data.ddsSubscriptionData.partition =
          TheServiceParticipant->initial_PartitionQosPolicy();
      reader_data.ddsSubscriptionData.presentation =
          TheServiceParticipant->initial_PresentationQosPolicy();
      reader_data.ddsSubscriptionData.topic_data =
          TheServiceParticipant->initial_TopicDataQosPolicy();
      reader_data.ddsSubscriptionData.group_data =
          TheServiceParticipant->initial_GroupDataQosPolicy();

      return reader_data;
    }

    DiscoveredWriterData
    writer_data(
        const char* topic_name = NULL,
        const char* type_name  = NULL,
        DurabilityQosPolicyKind durability
                               = VOLATILE_DURABILITY_QOS,
        long svc_del_sec = 0L,
        unsigned long svc_del_nsec = 0L,
        HistoryQosPolicyKind hist = KEEP_LAST_HISTORY_QOS,
        long hist_depth = 1L,
        long max_samples = 1L,
        long max_instances = 1L,
        long max_samples_per_instance = 1L,
        long deadline_sec = 0L, unsigned long deadline_nsec = 0L,
        long lb_sec = 0L, unsigned long lb_nsec = 0L,
        LivelinessQosPolicyKind liveliness =
              AUTOMATIC_LIVELINESS_QOS,
        long ld_sec = 0L, unsigned long ld_nsec = 0L,
        ReliabilityQosPolicyKind reliability =
              BEST_EFFORT_RELIABILITY_QOS,
        long mbt_sec = 0L, unsigned long mbt_nsec = 0L,
        long ls_sec = 0L, unsigned long ls_nsec = 0L,
        const void* user_data = NULL, CORBA::ULong user_data_len = 0,
        OwnershipQosPolicyKind ownership =
              SHARED_OWNERSHIP_QOS,
        long ownership_strength = 0L,
        DestinationOrderQosPolicyKind destination_order =
              BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS,
        PresentationQosPolicyAccessScopeKind presentation =
              INSTANCE_PRESENTATION_QOS,
        bool coherent = false, bool ordered = false,
        const char* partition = NULL,
        const void* topic_data = NULL, CORBA::ULong topic_data_len = 0,
        const void* group_data = NULL, CORBA::ULong group_data_len = 0,
        Locator_t* uc_locs = NULL, CORBA::ULong num_uc_locs = 0,
        Locator_t* mc_locs = NULL, CORBA::ULong num_mc_locs = 0
    ) {
      DiscoveredWriterData result;
      if (topic_name) {
        result.ddsPublicationData.topic_name = topic_name;
      }
      if (type_name) {
        result.ddsPublicationData.type_name  = type_name;
      }
      result.ddsPublicationData.durability.kind = durability;
      result.ddsPublicationData.durability_service.service_cleanup_delay.sec = svc_del_sec;
      result.ddsPublicationData.durability_service.service_cleanup_delay.nanosec = svc_del_nsec;
      result.ddsPublicationData.durability_service.history_kind = hist;
      result.ddsPublicationData.durability_service.history_depth = hist_depth;
      result.ddsPublicationData.durability_service.max_samples = max_samples;
      result.ddsPublicationData.durability_service.max_instances = max_instances;
      result.ddsPublicationData.durability_service.max_samples_per_instance = max_samples_per_instance;
      result.ddsPublicationData.deadline.period.sec = deadline_sec;
      result.ddsPublicationData.deadline.period.nanosec = deadline_nsec;
      result.ddsPublicationData.latency_budget.duration.sec = lb_sec;
      result.ddsPublicationData.latency_budget.duration.nanosec = lb_nsec;
      result.ddsPublicationData.liveliness.kind = liveliness;
      result.ddsPublicationData.liveliness.lease_duration.sec = ld_sec;
      result.ddsPublicationData.liveliness.lease_duration.nanosec = ld_nsec;
      result.ddsPublicationData.reliability.kind = reliability;
      result.ddsPublicationData.reliability.max_blocking_time.sec = mbt_sec;
      result.ddsPublicationData.reliability.max_blocking_time.nanosec = mbt_nsec;
      result.ddsPublicationData.lifespan.duration.sec = ls_sec;
      result.ddsPublicationData.lifespan.duration.nanosec = ls_nsec;
      if (user_data_len && user_data) {
        result.ddsPublicationData.user_data.value.length(user_data_len);
        for (CORBA::ULong i = 0; i < user_data_len; ++i) {
          result.ddsPublicationData.user_data.value[i] = ((char*)user_data)[i];
        }

      }
      result.ddsPublicationData.ownership.kind = ownership;
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      result.ddsPublicationData.ownership_strength.value = ownership_strength;
#else
      ACE_UNUSED_ARG(ownership_strength);
#endif
      result.ddsPublicationData.destination_order.kind = destination_order;
      result.ddsPublicationData.presentation.access_scope = presentation;
      result.ddsPublicationData.presentation.coherent_access = coherent;
      result.ddsPublicationData.presentation.ordered_access = ordered;
      if (partition) {
        result.ddsPublicationData.partition.name.length(1);
        result.ddsPublicationData.partition.name[0] = partition;
      }
      if (topic_data && topic_data_len) {
        result.ddsPublicationData.topic_data.value.length(topic_data_len);
        for (CORBA::ULong i = 0; i < topic_data_len; ++i) {
          result.ddsPublicationData.topic_data.value[i] = ((char*)topic_data)[i];
        }
      }
      if (group_data && group_data_len) {
        result.ddsPublicationData.group_data.value.length(group_data_len);
        for (CORBA::ULong i = 0; i < group_data_len; ++i) {
          result.ddsPublicationData.group_data.value[i] = ((char*)group_data)[i];
        }
      }
      guid_generator.populate(result.writerProxy.remoteWriterGuid);
      // One DCPS locator for all unicast and multicast locators
      if ((num_uc_locs && uc_locs) || (num_mc_locs || mc_locs)) {
        CORBA::ULong len = result.writerProxy.allLocators.length();
        result.writerProxy.allLocators.length(len + 1);
        OpenDDS::DCPS::TransportLocator& loc = result.writerProxy.allLocators[len];
        // Combine unicast and multicast locators into one seq
        LocatorSeq rtps_locators;
        CORBA::ULong i;
        for (i = 0; i < num_uc_locs; ++i) {
          CORBA::ULong rtps_len = rtps_locators.length();
          rtps_locators.length(rtps_len +1);
          rtps_locators[rtps_len] = uc_locs[i];
        }
        for (i = 0; i < num_mc_locs; ++i) {
          CORBA::ULong rtps_len = rtps_locators.length();
          rtps_locators.length(rtps_len +1);
          rtps_locators[rtps_len] = mc_locs[i];
        }
        loc.transport_type = "rtps_udp";
        // Add that seq to the blob
        locators_to_blob(rtps_locators, loc.data);
      }
      return result;
    }

    DiscoveredReaderData
    reader_data(
        const char* topic_name = NULL,
        const char* type_name  = NULL,
        DurabilityQosPolicyKind durability
                               = VOLATILE_DURABILITY_QOS,
        long deadline_sec = 0L, unsigned long deadline_nsec = 0L,
        long lb_sec = 0L, unsigned long lb_nsec = 0L,
        LivelinessQosPolicyKind liveliness =
              AUTOMATIC_LIVELINESS_QOS,
        long ld_sec = 0L, unsigned long ld_nsec = 0L,
        ReliabilityQosPolicyKind reliability =
              BEST_EFFORT_RELIABILITY_QOS,
        long mbt_sec = 0L, unsigned long mbt_nsec = 0L,
        const void* user_data = NULL, CORBA::ULong user_data_len = 0,
        OwnershipQosPolicyKind ownership =
              SHARED_OWNERSHIP_QOS,
        DestinationOrderQosPolicyKind destination_order =
              BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS,
        PresentationQosPolicyAccessScopeKind presentation =
              INSTANCE_PRESENTATION_QOS,
        bool coherent = false, bool ordered = false,
        const char* partition = NULL,
        const void* topic_data = NULL, CORBA::ULong topic_data_len = 0,
        const void* group_data = NULL, CORBA::ULong group_data_len = 0,
        Locator_t* uc_locs = NULL, CORBA::ULong num_uc_locs = 0,
        Locator_t* mc_locs = NULL, CORBA::ULong num_mc_locs = 0,
        const char* cf_topic_name = NULL, const char* rel_topic_name = NULL,
        const char* filter_name = NULL, const char* filter_expr = NULL,
        const char** params = NULL, CORBA::ULong num_params = 0
    ) {
      DiscoveredReaderData result;
      if (topic_name) {
        result.ddsSubscriptionData.topic_name = topic_name;
      }
      if (type_name) {
        result.ddsSubscriptionData.type_name  = type_name;
      }
      result.ddsSubscriptionData.durability.kind = durability;
      result.ddsSubscriptionData.deadline.period.sec = deadline_sec;
      result.ddsSubscriptionData.deadline.period.nanosec = deadline_nsec;
      result.ddsSubscriptionData.latency_budget.duration.sec = lb_sec;
      result.ddsSubscriptionData.latency_budget.duration.nanosec = lb_nsec;
      result.ddsSubscriptionData.liveliness.kind = liveliness;
      result.ddsSubscriptionData.liveliness.lease_duration.sec = ld_sec;
      result.ddsSubscriptionData.liveliness.lease_duration.nanosec = ld_nsec;
      result.ddsSubscriptionData.reliability.kind = reliability;
      result.ddsSubscriptionData.reliability.max_blocking_time.sec = mbt_sec;
      result.ddsSubscriptionData.reliability.max_blocking_time.nanosec = mbt_nsec;
      if (user_data_len && user_data) {
        result.ddsSubscriptionData.user_data.value.length(user_data_len);
        for (CORBA::ULong i = 0; i < user_data_len; ++i) {
          result.ddsSubscriptionData.user_data.value[i] = ((char*)user_data)[i];
        }

      }
      result.ddsSubscriptionData.ownership.kind = ownership;
      result.ddsSubscriptionData.destination_order.kind = destination_order;
      result.ddsSubscriptionData.presentation.access_scope = presentation;
      result.ddsSubscriptionData.presentation.coherent_access = coherent;
      result.ddsSubscriptionData.presentation.ordered_access = ordered;
      if (partition) {
        result.ddsSubscriptionData.partition.name.length(1);
        result.ddsSubscriptionData.partition.name[0] = partition;
      }
      if (topic_data && topic_data_len) {
        result.ddsSubscriptionData.topic_data.value.length(topic_data_len);
        for (CORBA::ULong i = 0; i < topic_data_len; ++i) {
          result.ddsSubscriptionData.topic_data.value[i] = ((char*)topic_data)[i];
        }
      }
      if (group_data && group_data_len) {
        result.ddsSubscriptionData.group_data.value.length(group_data_len);
        for (CORBA::ULong i = 0; i < group_data_len; ++i) {
          result.ddsSubscriptionData.group_data.value[i] = ((char*)group_data)[i];
        }
      }
      guid_generator.populate(result.readerProxy.remoteReaderGuid);
      if (cf_topic_name) {
        result.contentFilterProperty.contentFilteredTopicName = cf_topic_name;
      }
      if (rel_topic_name) {
        result.contentFilterProperty.relatedTopicName = rel_topic_name;
      }
      if (filter_name) {
        result.contentFilterProperty.filterClassName = filter_name;
      }
      if (filter_expr) {
        result.contentFilterProperty.filterExpression = filter_expr;
      }
      result.contentFilterProperty.expressionParameters.length(num_params);
      if (params && num_params) {
        for (CORBA::ULong i = 0; i < num_params; ++i) {
          result.contentFilterProperty.expressionParameters[i] = params[i];
        }
      }
      // One DCPS locator for all unicast and multicast locators
      if ((num_uc_locs && uc_locs) || (num_mc_locs || mc_locs)) {
        CORBA::ULong len = result.readerProxy.allLocators.length();
        result.readerProxy.allLocators.length(len + 1);
        OpenDDS::DCPS::TransportLocator& loc = result.readerProxy.allLocators[len];
        // Combine unicast and multicast locators into one seq
        LocatorSeq rtps_locators;
        CORBA::ULong i;
        for (i = 0; i < num_uc_locs; ++i) {
          CORBA::ULong rtps_len = rtps_locators.length();
          rtps_locators.length(rtps_len +1);
          rtps_locators[rtps_len] = uc_locs[i];
        }
        for (i = 0; i < num_mc_locs; ++i) {
          CORBA::ULong rtps_len = rtps_locators.length();
          rtps_locators.length(rtps_len +1);
          rtps_locators[rtps_len] = mc_locs[i];
        }
        loc.transport_type = "rtps_udp";
        // Add that seq to the blob
        locators_to_blob(rtps_locators, loc.data);
      }
      return result;
    } // method
  } // Factory namespace

#ifdef OPENDDS_SECURITY
  OpenDDS::RTPS::SPDPdiscoveredParticipantData spdp_participant(
    const void* user_data = NULL,
    CORBA::ULong user_data_len = 0,
    char major_protocol_version = 0,
    char minor_protocol_version = 0,
    char* vendor_id = NULL,
    OpenDDS::DCPS::GUID_t* guid = NULL,
    bool expects_inline_qos = false,
    unsigned long builtin_endpoints = 0,
    OpenDDS::DCPS::Locator_t* mtu_locs = NULL,
    CORBA::ULong num_mtu_locs = 0,
    OpenDDS::DCPS::Locator_t* mtm_locs = NULL,
    CORBA::ULong num_mtm_locs = 0,
    OpenDDS::DCPS::Locator_t* du_locs = NULL,
    CORBA::ULong num_du_locs = 0,
    OpenDDS::DCPS::Locator_t* dm_locs = NULL,
    CORBA::ULong num_dm_locs = 0,
    long liveliness_count = 0,
    long lease_dur_seconds = 100,
    unsigned long lease_dur_fraction = 0
  )
  {
    OpenDDS::RTPS::SPDPdiscoveredParticipantData result;
    if (user_data_len && user_data) {
      result.ddsParticipantData.user_data.value.length(user_data_len);
      for (CORBA::ULong i = 0; i < user_data_len; ++i) {
        result.ddsParticipantData.user_data.value[i] = ((char*)user_data)[i];
      }
    }
    if (major_protocol_version && minor_protocol_version) {
      result.participantProxy.protocolVersion.major = major_protocol_version;
      result.participantProxy.protocolVersion.minor = minor_protocol_version;
    }
    if (vendor_id) {
      result.participantProxy.vendorId.vendorId[0] = vendor_id[0];
      result.participantProxy.vendorId.vendorId[1] = vendor_id[1];
    }
    if (guid) {
      memcpy(result.participantProxy.guidPrefix,
             guid->guidPrefix,
             sizeof(guid->guidPrefix));
    }
    result.participantProxy.expectsInlineQos = expects_inline_qos;
    result.participantProxy.availableBuiltinEndpoints = builtin_endpoints;

    if (num_mtu_locs && mtu_locs) {
      result.participantProxy.metatrafficUnicastLocatorList.length(num_mtu_locs);
      for (CORBA::ULong i = 0; i < num_mtu_locs; ++i) {
        result.participantProxy.metatrafficUnicastLocatorList[i] =
            mtu_locs[i];
      }
    }

    if (num_mtm_locs && mtm_locs) {
      result.participantProxy.metatrafficMulticastLocatorList.length(num_mtm_locs);
      for (CORBA::ULong i = 0; i < num_mtm_locs; ++i) {
        result.participantProxy.metatrafficMulticastLocatorList[i] =
            mtm_locs[i];
      }
    }

    if (num_du_locs && du_locs) {
      result.participantProxy.defaultUnicastLocatorList.length(num_du_locs);
      for (CORBA::ULong i = 0; i < num_du_locs; ++i) {
        result.participantProxy.defaultUnicastLocatorList[i] = du_locs[i];
      }
    }

    if (num_dm_locs && dm_locs) {
      result.participantProxy.defaultMulticastLocatorList.length(num_dm_locs);
      for (CORBA::ULong i = 0; i < num_dm_locs; ++i) {
        result.participantProxy.defaultMulticastLocatorList[i] = dm_locs[i];
      }
    }

    if (liveliness_count) {
      result.participantProxy.manualLivelinessCount.value = liveliness_count;
    }

    result.leaseDuration.seconds = lease_dur_seconds;
    result.leaseDuration.fraction = lease_dur_fraction;

    return result;
  }

  Token token(string classid = "Test-Class-Id",
              CORBA::ULong proplen = 1,
              CORBA::ULong bproplen = 1,
              bool propagate = true)
  {
    Token t;

    t.class_id = classid.c_str();

    t.properties.length(proplen);
    for (CORBA::ULong i = 0; i < proplen; ++i) {
        stringstream name, value;
        name << "Property " << i;
        value << "PropertyValue " << i;

        t.properties[i].name = name.str().c_str();
        t.properties[i].value = value.str().c_str();
        t.properties[i].propagate = propagate;
    }

    t.binary_properties.length(bproplen);
    for (CORBA::ULong i = 0; i < bproplen; ++i) {
        stringstream name, value;
        name << "BinaryProperty " << i;
        value << "BinaryPropertyValue " << i;

        t.binary_properties[i].name = name.str().c_str();

        const CORBA::ULong vlen = static_cast<CORBA::ULong>(value.str().length());
        t.binary_properties[i].value.length(vlen);
        memcpy(t.binary_properties[i].value.get_buffer(),  value.str().c_str(),  vlen);

        t.binary_properties[i].propagate = propagate;
    }

    return t;
  }

#if 0
  void participant_security_attribs(ParticipantSecurityAttributes& a,
                                    size_t plen = 1,
                                    bool allow_unauthenticated_participants = false,
                                    bool is_access_protected = false,
                                    bool is_discovery_protected = false,
                                    bool is_liveliness_protected = false,
                                    PluginParticipantSecurityAttributesMask plugin_participant_attribs = 0u,
                                    bool propagate = true)
  {
    a.ac_endpoint_properties.length(plen);

    for (size_t i = 0; i < plen; ++i) {
        stringstream name, value;
        name << "Property " << i;
        value << "Value " << i;

        a.ac_endpoint_properties[i].name = name.str().c_str();
        a.ac_endpoint_properties[i].value = value.str().c_str();
        a.ac_endpoint_properties[i].propagate = propagate;
    }

    a.allow_unauthenticated_participants = allow_unauthenticated_participants;
    a.is_access_protected = is_access_protected;
    a.is_discovery_protected = is_discovery_protected;
    a.is_liveliness_protected = is_liveliness_protected;
    a.plugin_participant_attributes = plugin_participant_attribs;
  }
#endif

#endif
} // anon namespace

bool is_present(const ParameterList& param_list, const OpenDDS::RTPS::ParameterId_t pid)
{
  CORBA::ULong length = param_list.length();
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      return true;
    }
  }
  return false;
}

bool is_missing(const ParameterList& param_list, const OpenDDS::RTPS::ParameterId_t pid)
{
  CORBA::ULong length = param_list.length();
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      return false;
    }
  }
  return true;
}

Parameter get(const ParameterList& param_list,
              const OpenDDS::RTPS::ParameterId_t pid,
              const CORBA::ULong instance_num = 0)
{

  const CORBA::ULong length = param_list.length();
  CORBA::ULong count = 0;
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      if (count++ == instance_num) {
        return param_list[i];
      }
    }
  }
  EXPECT_TRUE(false); // Not found
  return Parameter();
}

#ifdef OPENDDS_SECURITY
TEST(dds_DCPS_RTPS_ParameterListConverter, From_SPDPdiscoveredParticipantData_IdentityStatusToken)
{
  OpenDDS::Security::SPDPdiscoveredParticipantData w1, w2;

  OpenDDS::RTPS::SPDPdiscoveredParticipantData temp = spdp_participant("Test-Test-Test", 10);

  w1.dataKind = OpenDDS::Security::DPDK_SECURE;
  w1.ddsParticipantDataSecure.identity_status_token = token();
  w1.ddsParticipantDataSecure.base.base = temp.ddsParticipantData;
  w1.participantProxy = temp.participantProxy;
  w1.leaseDuration = temp.leaseDuration;

  ParameterList p;
  ASSERT_EQ(true, ParameterListConverter::to_param_list(w1, p));
  ASSERT_EQ(true, ParameterListConverter::from_param_list(p, w2));

  ASSERT_EQ(0, strcmp("Property 0", w2.ddsParticipantDataSecure.identity_status_token.properties[0].name));
  ASSERT_EQ(0, strcmp("PropertyValue 0", w2.ddsParticipantDataSecure.identity_status_token.properties[0].value));
  ASSERT_EQ(0, strcmp("BinaryProperty 0", w2.ddsParticipantDataSecure.identity_status_token.binary_properties[0].name));
  ASSERT_EQ(0, memcmp("BinaryPropertyValue 0", w2.ddsParticipantDataSecure.identity_status_token.binary_properties[0].value.get_buffer(), strlen("BinaryPropertyValue 0")));
}
#endif

TEST(dds_DCPS_RTPS_ParameterListConverter, maintest)
{
  { // Should encode participant data with 1 locator to param list properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data = Factory::default_participant_data();
    ParameterList param_list;
    participant_data.participantProxy.metatrafficUnicastLocatorList.length(1);
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    EXPECT_TRUE(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_METATRAFFIC_MULTICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }

  { // Should encode participant data with 2 locators to param list properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data = Factory::default_participant_data();
    ParameterList param_list;
    participant_data.participantProxy.metatrafficUnicastLocatorList.length(1);
    participant_data.participantProxy.metatrafficMulticastLocatorList.length(1);
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    EXPECT_TRUE(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    EXPECT_TRUE(is_present(param_list, PID_METATRAFFIC_MULTICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }

  { // Should encode participant data with 3 locators to param list properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data = Factory::default_participant_data();
    ParameterList param_list;
    participant_data.participantProxy.metatrafficUnicastLocatorList.length(1);
    participant_data.participantProxy.metatrafficMulticastLocatorList.length(1);
    participant_data.participantProxy.defaultUnicastLocatorList.length(1);
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    EXPECT_TRUE(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    EXPECT_TRUE(is_present(param_list, PID_METATRAFFIC_MULTICAST_LOCATOR));
    EXPECT_TRUE(is_present(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }

  { // Should encode participant data with 4 locators to param list properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data = Factory::default_participant_data();
    ParameterList param_list;
    participant_data.participantProxy.metatrafficUnicastLocatorList.length(1);
    participant_data.participantProxy.metatrafficMulticastLocatorList.length(1);
    participant_data.participantProxy.defaultUnicastLocatorList.length(1);
    participant_data.participantProxy.defaultMulticastLocatorList.length(1);
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    EXPECT_TRUE(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    EXPECT_TRUE(is_present(param_list, PID_METATRAFFIC_MULTICAST_LOCATOR));
    EXPECT_TRUE(is_present(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    EXPECT_TRUE(is_present(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    EXPECT_TRUE(is_missing(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }

  { // Should encode participant user data properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant("hello user", 10);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_USER_DATA));
    Parameter param = get(param_list, PID_USER_DATA);
    UserDataQosPolicy ud_qos = param.user_data();
    EXPECT_TRUE(ud_qos.value.length() == 10);
    EXPECT_TRUE(ud_qos.value[0] == 'h');
    EXPECT_TRUE(ud_qos.value[1] == 'e');
    EXPECT_TRUE(ud_qos.value[2] == 'l');
    EXPECT_TRUE(ud_qos.value[3] == 'l');
    EXPECT_TRUE(ud_qos.value[4] == 'o');
    EXPECT_TRUE(ud_qos.value[5] == ' ');
    EXPECT_TRUE(ud_qos.value[6] == 'u');
    EXPECT_TRUE(ud_qos.value[7] == 's');
    EXPECT_TRUE(ud_qos.value[8] == 'e');
    EXPECT_TRUE(ud_qos.value[9] == 'r');
  }

  { // Should decode participant user data properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant("hello user", 10);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[0] ==
                part_data_out.ddsParticipantData.user_data.value[0]);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[1] ==
                part_data_out.ddsParticipantData.user_data.value[1]);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[2] ==
                part_data_out.ddsParticipantData.user_data.value[2]);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[3] ==
                part_data_out.ddsParticipantData.user_data.value[3]);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[4] ==
                part_data_out.ddsParticipantData.user_data.value[4]);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[5] ==
                part_data_out.ddsParticipantData.user_data.value[5]);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[6] ==
                part_data_out.ddsParticipantData.user_data.value[6]);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[7] ==
                part_data_out.ddsParticipantData.user_data.value[7]);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[8] ==
                part_data_out.ddsParticipantData.user_data.value[8]);
    EXPECT_TRUE(participant_data.ddsParticipantData.user_data.value[9] ==
                part_data_out.ddsParticipantData.user_data.value[9]);
  }

  { // Should encode participant protocol version properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 3, 8);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_PROTOCOL_VERSION));
    Parameter param = get(param_list, PID_PROTOCOL_VERSION);
    OpenDDS::RTPS::ProtocolVersion_t pv = param.version();
    EXPECT_TRUE(pv.major == 3);
    EXPECT_TRUE(pv.minor == 8);
  }

  { // Should decode participant protocol version properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 9, 1);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(participant_data.participantProxy.protocolVersion.major ==
                part_data_out.participantProxy.protocolVersion.major);
    EXPECT_TRUE(participant_data.participantProxy.protocolVersion.minor ==
                part_data_out.participantProxy.protocolVersion.minor);
  }

  { // Should encode participant vendor id properly
    char vendor_id[] = {7, 9};
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, vendor_id);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_VENDORID));
    Parameter param = get(param_list, PID_VENDORID);
    OpenDDS::RTPS::VendorId_t vid = param.vendor();
    EXPECT_TRUE(vid.vendorId[0] == 7);
    EXPECT_TRUE(vid.vendorId[1] == 9);
  }

  { // Should decode participant vendor id properly
    char vendor_id[] = {7, 9};
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, vendor_id);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(!memcmp(participant_data.participantProxy.vendorId.vendorId,
                        part_data_out.participantProxy.vendorId.vendorId,
                        sizeof(OctetArray2)));
  }

  { // Should encode participant guid prefix properly
    GUID_t guid_in;
    memcpy(guid_in.guidPrefix, "GUID-ABC                 ", 12);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, &guid_in);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_PARTICIPANT_GUID));
    Parameter param = get(param_list, PID_PARTICIPANT_GUID);
    GUID_t guid_out = param.guid();
    EXPECT_TRUE(memcmp(guid_out.guidPrefix, "GUID-ABC    ", 12) == 0);
  }

  { // Should decode participant guid prefix properly
    GUID_t guid_in;
    memcpy(guid_in.guidPrefix, "GUID-ABC                 ", 12);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, &guid_in);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(memcmp(participant_data.participantProxy.guidPrefix,
                       part_data_out.participantProxy.guidPrefix,
                       sizeof(GuidPrefix_t)) == 0);
  }

  { // Should encode participant expects inline qos properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, true);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_EXPECTS_INLINE_QOS));
    Parameter param = get(param_list, PID_EXPECTS_INLINE_QOS);
    EXPECT_TRUE(param.expects_inline_qos() == true);
  }

  { // Should decode participant expects inline qos properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, true);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(part_data_out.participantProxy.expectsInlineQos == true);
    // Change to false
    participant_data.participantProxy.expectsInlineQos = false;
    param_list.length(0);
    to_param_list(participant_data, param_list);
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(part_data_out.participantProxy.expectsInlineQos == false);
  }

  { // Should encode participant builtin endpoints properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 72393L);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    Parameter param = get(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS);
    EXPECT_TRUE(param.participant_builtin_endpoints() == 72393L);
    EXPECT_TRUE(is_present(param_list, PID_BUILTIN_ENDPOINT_SET));
    Parameter param2 = get(param_list, PID_BUILTIN_ENDPOINT_SET);
    EXPECT_TRUE(param2.builtin_endpoints() == 72393L);
  }

  { // Should decode participant builtin endpoints properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 72393L);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(
                participant_data.participantProxy.availableBuiltinEndpoints ==
                part_data_out.participantProxy.availableBuiltinEndpoints);
  }

  { // Should encode participant meta unicast locators properly
    Locator_t locators[2];
    Locator_t locator_out;
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                locators, 2);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    Parameter param = get(param_list, PID_METATRAFFIC_UNICAST_LOCATOR, 0);
    locator_out = param.locator();
    EXPECT_TRUE(locator_out.kind == locators[0].kind);
    EXPECT_TRUE(locator_out.port == locators[0].port);
    EXPECT_TRUE(memcmp(locator_out.address, locators[0].address, 16) == 0);

    param = get(param_list, PID_METATRAFFIC_UNICAST_LOCATOR, 1);
    locator_out = param.locator();
    EXPECT_TRUE(locator_out.kind == locators[1].kind);
    EXPECT_TRUE(locator_out.port == locators[1].port);
    EXPECT_TRUE(memcmp(locator_out.address, locators[1].address, 16) == 0);
  }

  { // Should decode participant meta unicast locators properly
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                locators, 2);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficUnicastLocatorList[0];
      EXPECT_TRUE(locators[0].kind == locator.kind);
      EXPECT_TRUE(locators[0].port == locator.port);
      EXPECT_TRUE(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficUnicastLocatorList[1];
      EXPECT_TRUE(locators[1].kind == locator.kind);
      EXPECT_TRUE(locators[1].port == locator.port);
      EXPECT_TRUE(memcmp(locators[1].address, locator.address, 16) == 0);
    }
  }

  { // Should encode participant meta multicast locators properly
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, locators, 2);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficMulticastLocatorList[0];
      EXPECT_TRUE(locators[0].kind == locator.kind);
      EXPECT_TRUE(locators[0].port == locator.port);
      EXPECT_TRUE(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficMulticastLocatorList[1];
      EXPECT_TRUE(locators[1].kind == locator.kind);
      EXPECT_TRUE(locators[1].port == locator.port);
      EXPECT_TRUE(memcmp(locators[1].address, locator.address, 16) == 0);
    }
  }

  { // Should decode participant meta multicast locators properly
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, locators, 2);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficMulticastLocatorList[0];
      EXPECT_TRUE(locators[0].kind == locator.kind);
      EXPECT_TRUE(locators[0].port == locator.port);
      EXPECT_TRUE(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficMulticastLocatorList[1];
      EXPECT_TRUE(locators[1].kind == locator.kind);
      EXPECT_TRUE(locators[1].port == locator.port);
      EXPECT_TRUE(memcmp(locators[1].address, locator.address, 16) == 0);
    }
  }

  { // Should encode participant default unicast locators properly
    Locator_t locators[2];
    Locator_t locator_out;
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, NULL, 0, locators, 2);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    Parameter param = get(param_list, PID_DEFAULT_UNICAST_LOCATOR, 0);
    locator_out = param.locator();
    EXPECT_TRUE(locator_out.kind == locators[0].kind);
    EXPECT_TRUE(locator_out.port == locators[0].port);
    EXPECT_TRUE(memcmp(locator_out.address, locators[0].address, 16) == 0);

    param = get(param_list, PID_DEFAULT_UNICAST_LOCATOR, 1);
    locator_out = param.locator();
    EXPECT_TRUE(locator_out.kind == locators[1].kind);
    EXPECT_TRUE(locator_out.port == locators[1].port);
    EXPECT_TRUE(memcmp(locator_out.address, locators[1].address, 16) == 0);
  }

  { // Should decode participant default unicast locators properly
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, NULL, 0, locators, 2);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    {
      Locator_t& locator = part_data_out.participantProxy.defaultUnicastLocatorList[0];
      EXPECT_TRUE(locators[0].kind == locator.kind);
      EXPECT_TRUE(locators[0].port == locator.port);
      EXPECT_TRUE(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.defaultUnicastLocatorList[1];
      EXPECT_TRUE(locators[1].kind == locator.kind);
      EXPECT_TRUE(locators[1].port == locator.port);
      EXPECT_TRUE(memcmp(locators[1].address, locator.address, 16) == 0);
    }
  }

  { // Should encode participant default multicast locators properly
    Locator_t locators[2];
    Locator_t locator_out;
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, NULL, 0, NULL, 0,
                                locators, 2);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    Parameter param = get(param_list, PID_DEFAULT_MULTICAST_LOCATOR, 0);
    locator_out = param.locator();
    EXPECT_TRUE(locator_out.kind == locators[0].kind);
    EXPECT_TRUE(locator_out.port == locators[0].port);
    EXPECT_TRUE(memcmp(locator_out.address, locators[0].address, 16) == 0);

    param = get(param_list, PID_DEFAULT_MULTICAST_LOCATOR, 1);
    locator_out = param.locator();
    EXPECT_TRUE(locator_out.kind == locators[1].kind);
    EXPECT_TRUE(locator_out.port == locators[1].port);
    EXPECT_TRUE(memcmp(locator_out.address, locators[1].address, 16) == 0);
  }

  { // Should decode participant default multicast locators properly
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, NULL, 0, NULL, 0,
                                locators, 2);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    {
      Locator_t& locator = part_data_out.participantProxy.defaultMulticastLocatorList[0];
      EXPECT_TRUE(locators[0].kind == locator.kind);
      EXPECT_TRUE(locators[0].port == locator.port);
      EXPECT_TRUE(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.defaultMulticastLocatorList[1];
      EXPECT_TRUE(locators[1].kind == locator.kind);
      EXPECT_TRUE(locators[1].port == locator.port);
      EXPECT_TRUE(memcmp(locators[1].address, locator.address, 16) == 0);
    }
  }

  { // Should encode participant liveliness count properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                7);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT));
    Parameter param = get(param_list, PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT);
    EXPECT_TRUE(param.count().value == 7);
  }

  { // Should decode participant liveliness count properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                6);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(
                part_data_out.participantProxy.manualLivelinessCount.value == 6);
  }

  { // Should encode participant lease duration properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, NULL, 0, NULL, 0, NULL, 0, 7,
                                12, 300);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(is_present(param_list, PID_PARTICIPANT_LEASE_DURATION));
    Parameter param = get(param_list, PID_PARTICIPANT_LEASE_DURATION);
    EXPECT_TRUE(param.duration().seconds == 12);
    EXPECT_TRUE(param.duration().fraction == 300);
  }

  { // Should decode participant lease duration properly
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                NULL, 0, NULL, 0, NULL, 0, NULL, 0, 7,
                                12, 300);
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    status = from_param_list(param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(part_data_out.leaseDuration.seconds ==
                participant_data.leaseDuration.seconds);
    EXPECT_TRUE(part_data_out.leaseDuration.fraction ==
                participant_data.leaseDuration.fraction);
  }

  { // Should set participant user data qos to default if not present in param list
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    part_data_out.ddsParticipantData.user_data.value.length(4);
    ParameterList empty_param_list;
    bool status = from_param_list(empty_param_list, part_data_out);
    EXPECT_TRUE(status == true);
    CORBA::ULong length = part_data_out.ddsParticipantData.user_data.value.length();
    EXPECT_TRUE(length == 0);
  }

  // There is no default protocol version
  // There is no default guid prefix
  // There is no defaul vendor id

  { // Should set participant expects inline qos to default if not present in param list
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    part_data_out.participantProxy.expectsInlineQos = true;
    ParameterList empty_param_list;
    bool status = from_param_list(empty_param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(part_data_out.participantProxy.expectsInlineQos == false);
  }

  // There is no default available builtin endpoints
  // There is no default metatraffic unicast locator list
  // There is no default metatraffic multicast locator list
  // There is no default default unicast locator list
  // There is no default default multicast locator list
  // There is no default manual liveliness count
  // The lease duration default is { 100, 0 }

  { // Should set participant lease duration to default if not present in param list
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    ParameterList empty_param_list;
    bool status = from_param_list(empty_param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(part_data_out.leaseDuration.seconds == 100);
    EXPECT_TRUE(part_data_out.leaseDuration.fraction == 0);
  }

  { // Should ignore participant vendor-specific parameters
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(0x8001);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    bool status = from_param_list(vs_param_list, part_data_out);
    EXPECT_TRUE(status == true);
  }

  { // Should not fail on participant optional parameters
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(0x3FFF);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    bool status = from_param_list(vs_param_list, part_data_out);
    EXPECT_TRUE(status == true);
  }

  { // Should fail on participant required parameters
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(0x4001);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    bool status = from_param_list(vs_param_list, part_data_out);
    EXPECT_TRUE(status == false);
  }

  { // Foreign DDS's have been observed using PID_BUILTIN_ENDPOINT_SET when
    // they should be using PID_PARTICIPANT_BUILTIN_ENDPOINTS.
    Parameter vs_param;
    vs_param.builtin_endpoints(0x12345678);
    ParameterList vs_param_list;
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    const bool status = from_param_list(vs_param_list, part_data_out);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(part_data_out.participantProxy.availableBuiltinEndpoints
                == 0x12345678);
  }

  { // Should ignore participant SENTINEL
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(PID_SENTINEL);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    bool status = from_param_list(vs_param_list, part_data_out);
    EXPECT_TRUE(status == true);
  }

  { // Should ignore participant PAD
    OpenDDS::RTPS::SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(PID_PAD);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    bool status = from_param_list(vs_param_list, part_data_out);
    EXPECT_TRUE(status == true);
  }

  { // Should encode writer data
    DiscoveredWriterData writer_data = Factory::default_writer_data();
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
  }

  { // Should encode writer topic name
    DiscoveredWriterData writer_data =
      Factory::writer_data("TOPIC NAME TEST");
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_TOPIC_NAME));
    Parameter param = get(param_list, PID_TOPIC_NAME);
    EXPECT_TRUE(!strncmp(param.string_data(), "TOPIC NAME TEST", 15));
  }

  { // Should decode writer topic name
    DiscoveredWriterData writer_data =
      Factory::writer_data("TOPIC NAME TEST");
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(!strcmp(writer_data.ddsPublicationData.topic_name,
                        writer_data_out.ddsPublicationData.topic_name));
  }

  { // Should set writer topic name to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.topic_name = "TEST TOPIC";
    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(!strcmp(writer_data_out.ddsPublicationData.topic_name, ""));
  }

  { // Should encode writer type name
    DiscoveredWriterData writer_data = Factory::writer_data("", "Messages");

    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_TYPE_NAME));
    Parameter param = get(param_list, PID_TYPE_NAME);
    EXPECT_TRUE(!strncmp(param.string_data(), "Messages", 8));
  }

  { // Should decode writer type name
    DiscoveredWriterData writer_data = Factory::writer_data("", "Messages");
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(!strcmp(writer_data.ddsPublicationData.type_name,
                        writer_data_out.ddsPublicationData.type_name));
  }

  { // Should set writer topic type to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.type_name = "TEST TYPE";
    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(!strcmp(writer_data_out.ddsPublicationData.type_name, ""));
  }

  { // Should encode writer durability qos policy
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL,
                                                            TRANSIENT_LOCAL_DURABILITY_QOS);

    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info , map));
    EXPECT_TRUE(is_present(param_list, PID_DURABILITY));
    Parameter param = get(param_list, PID_DURABILITY);
    EXPECT_TRUE(param.durability().kind == TRANSIENT_LOCAL_DURABILITY_QOS);
  }

  { // Should decode writer durability
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL,
                                                            TRANSIENT_LOCAL_DURABILITY_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.durability.kind ==
                writer_data_out.ddsPublicationData.durability.kind);
  }

  { // Should set writer durability default if not present in param list
    DiscoveredWriterData writer_data_out = Factory::writer_data(
                                                                NULL, NULL, TRANSIENT_LOCAL_DURABILITY_QOS);
    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    DurabilityQosPolicy defaultQos =
      TheServiceParticipant->initial_DurabilityQosPolicy();
    EXPECT_TRUE(defaultQos.kind ==
                writer_data_out.ddsPublicationData.durability.kind);
  }

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  { // Should encode writer durabiltiy service
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL,
                                                            TRANSIENT_LOCAL_DURABILITY_QOS,
                                                            4, 2000,
                                                            KEEP_LAST_HISTORY_QOS, 172,
                                                            389, 102, 20);

    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_DURABILITY_SERVICE));
    Parameter param = get(param_list, PID_DURABILITY_SERVICE);
    DurabilityServiceQosPolicy dsqp = param.durability_service();
    EXPECT_TRUE(dsqp.service_cleanup_delay.sec == 4);
    EXPECT_TRUE(dsqp.service_cleanup_delay.nanosec == 2000);

    EXPECT_TRUE(dsqp.history_kind == KEEP_LAST_HISTORY_QOS);
    EXPECT_TRUE(dsqp.history_depth == 172);
    EXPECT_TRUE(dsqp.max_samples == 389);
    EXPECT_TRUE(dsqp.max_instances == 102);
    EXPECT_TRUE(dsqp.max_samples_per_instance == 20);
  }

  { // Should decode writer durability service
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, TRANSIENT_LOCAL_DURABILITY_QOS,
                                                            4, 2000,
                                                            KEEP_LAST_HISTORY_QOS, 172,
                                                            389, 102, 20);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    DurabilityServiceQosPolicy& ds_in =
      writer_data.ddsPublicationData.durability_service;
    DurabilityServiceQosPolicy& ds_out =
      writer_data_out.ddsPublicationData.durability_service;
    EXPECT_TRUE(ds_in.service_cleanup_delay.sec ==
                ds_out.service_cleanup_delay.sec);
    EXPECT_TRUE(ds_in.service_cleanup_delay.nanosec ==
                ds_out.service_cleanup_delay.nanosec);
    EXPECT_TRUE(ds_in.history_kind == ds_out.history_kind);
    EXPECT_TRUE(ds_in.history_depth == ds_out.history_depth);
    EXPECT_TRUE(ds_in.max_samples == ds_out.max_samples);
    EXPECT_TRUE(ds_in.max_instances == ds_out.max_instances);
    EXPECT_TRUE(ds_in.max_samples_per_instance == ds_out.max_samples_per_instance);
  }

  { // Should set writer durability service default if not present in param list
    DiscoveredWriterData writer_data_out = Factory::writer_data(
                                                                NULL, NULL, PERSISTENT_DURABILITY_QOS,
                                                                4, 2000,
                                                                KEEP_LAST_HISTORY_QOS, 172,
                                                                389, 102, 20);
    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    DurabilityServiceQosPolicy defaultQos =
      TheServiceParticipant->initial_DurabilityServiceQosPolicy();
    DurabilityServiceQosPolicy& ds_out =
      writer_data_out.ddsPublicationData.durability_service;
    EXPECT_TRUE(defaultQos.service_cleanup_delay.sec ==
                ds_out.service_cleanup_delay.sec);
    EXPECT_TRUE(defaultQos.service_cleanup_delay.nanosec ==
                ds_out.service_cleanup_delay.nanosec);
    EXPECT_TRUE(defaultQos.history_kind == ds_out.history_kind);
    EXPECT_TRUE(defaultQos.history_depth == ds_out.history_depth);
    EXPECT_TRUE(defaultQos.max_samples == ds_out.max_samples);
    EXPECT_TRUE(defaultQos.max_instances == ds_out.max_instances);
    EXPECT_TRUE(defaultQos.max_samples_per_instance == ds_out.max_samples_per_instance);
  }
#endif

  { // Should encode writer deadline
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1,
                                                            127, 35000);

    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_DEADLINE));
    Parameter param = get(param_list, PID_DEADLINE);
    EXPECT_TRUE(param.deadline().period.sec == 127);
    EXPECT_TRUE(param.deadline().period.nanosec == 35000);
  }

  { // Should decode writer deadline
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1,
                                                            127, 35000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.deadline.period.sec ==
                writer_data_out.ddsPublicationData.deadline.period.sec);
    EXPECT_TRUE(writer_data.ddsPublicationData.deadline.period.nanosec ==
                writer_data_out.ddsPublicationData.deadline.period.nanosec);
  }

  { // Should set writer deadline default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.deadline.period.sec = 1;
    writer_data_out.ddsPublicationData.deadline.period.nanosec = 7;

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    DeadlineQosPolicy defaultQos =
      TheServiceParticipant->initial_DeadlineQosPolicy();
    EXPECT_TRUE(defaultQos.period.sec ==
                writer_data_out.ddsPublicationData.deadline.period.sec);
    EXPECT_TRUE(defaultQos.period.nanosec ==
                writer_data_out.ddsPublicationData.deadline.period.nanosec);
  }

  { // Should enode writer latency budget
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0,
                                                            5, 25000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_LATENCY_BUDGET));
    Parameter param = get(param_list, PID_LATENCY_BUDGET);
    EXPECT_TRUE(param.latency_budget().duration.sec == 5);
    EXPECT_TRUE(param.latency_budget().duration.nanosec == 25000);
  }

  { // Should decode writer latency budget
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0,
                                                            5, 25000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.latency_budget.duration.sec ==
                writer_data_out.ddsPublicationData.latency_budget.duration.sec);
  }

  { // Should set writer latency budget to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.latency_budget.duration.sec = 1;
    writer_data_out.ddsPublicationData.latency_budget.duration.nanosec = 7;

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    LatencyBudgetQosPolicy defaultQos =
      TheServiceParticipant->initial_LatencyBudgetQosPolicy();
    EXPECT_TRUE(defaultQos.duration.sec ==
                writer_data_out.ddsPublicationData.latency_budget.duration.sec);
    EXPECT_TRUE(defaultQos.duration.nanosec ==
                writer_data_out.ddsPublicationData.latency_budget.duration.nanosec);
  }
  { // Should encode writer liveliness
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, 17, 15000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_LIVELINESS));
    Parameter param = get(param_list, PID_LIVELINESS);
    EXPECT_TRUE(param.liveliness().kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
    EXPECT_TRUE(param.liveliness().lease_duration.sec == 17);
    EXPECT_TRUE(param.liveliness().lease_duration.nanosec == 15000);
  }

  { // Should decode writer liveliness
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, 17, 15000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.liveliness.kind ==
                writer_data_out.ddsPublicationData.liveliness.kind);
    EXPECT_TRUE(writer_data.ddsPublicationData.liveliness.lease_duration.sec ==
                writer_data_out.ddsPublicationData.liveliness.lease_duration.sec);
    EXPECT_TRUE(writer_data.ddsPublicationData.liveliness.lease_duration.nanosec ==
                writer_data_out.ddsPublicationData.liveliness.lease_duration.nanosec);
  }

  { // Should set writer liveliness to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.liveliness.kind =
      MANUAL_BY_TOPIC_LIVELINESS_QOS;
    writer_data_out.ddsPublicationData.liveliness.lease_duration.sec = 1;
    writer_data_out.ddsPublicationData.liveliness.lease_duration.nanosec = 7;

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    LivelinessQosPolicy defaultQos =
      TheServiceParticipant->initial_LivelinessQosPolicy();
    EXPECT_TRUE(defaultQos.kind ==
                writer_data_out.ddsPublicationData.liveliness.kind);
    EXPECT_TRUE(defaultQos.lease_duration.sec ==
                writer_data_out.ddsPublicationData.liveliness.lease_duration.sec);
    EXPECT_TRUE(defaultQos.lease_duration.nanosec ==
                writer_data_out.ddsPublicationData.liveliness.lease_duration.nanosec);
  }
  { // Should encode writer reliability
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            RELIABLE_RELIABILITY_QOS, 8, 100);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_RELIABILITY));
    Parameter param = get(param_list, PID_RELIABILITY);
    EXPECT_TRUE(param.reliability().kind.value == RELIABLE);
    EXPECT_TRUE(param.reliability().max_blocking_time.sec == 8);
    EXPECT_TRUE(param.reliability().max_blocking_time.nanosec == 100);
  }

  { // Should decode writer reliability
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            RELIABLE_RELIABILITY_QOS, 8, 100);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.reliability.kind ==
                writer_data_out.ddsPublicationData.reliability.kind);
    EXPECT_TRUE(writer_data.ddsPublicationData.reliability.max_blocking_time.sec ==
                writer_data_out.ddsPublicationData.reliability.max_blocking_time.sec);
    EXPECT_TRUE(writer_data.ddsPublicationData.reliability.max_blocking_time.nanosec ==
                writer_data_out.ddsPublicationData.reliability.max_blocking_time.nanosec);
  }

  { // Should set writer reliability to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.reliability.kind =
      RELIABLE_RELIABILITY_QOS;
    writer_data_out.ddsPublicationData.reliability.max_blocking_time.sec = 1;
    writer_data_out.ddsPublicationData.reliability.max_blocking_time.nanosec = 7;

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    ReliabilityQosPolicy defaultQos =
      TheServiceParticipant->initial_DataWriterQos().reliability;
    EXPECT_TRUE(defaultQos.kind ==
                writer_data_out.ddsPublicationData.reliability.kind);
    EXPECT_TRUE(defaultQos.max_blocking_time.sec ==
                writer_data_out.ddsPublicationData.reliability.max_blocking_time.sec);
    EXPECT_TRUE(defaultQos.max_blocking_time.nanosec ==
                writer_data_out.ddsPublicationData.reliability.max_blocking_time.nanosec);
  }
  { // Should encode writer lifespan
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0,
                                                            12, 90000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_LIFESPAN));
    Parameter param = get(param_list, PID_LIFESPAN);
    EXPECT_TRUE(param.lifespan().duration.sec == 12);
    EXPECT_TRUE(param.lifespan().duration.nanosec == 90000);
  }
  { // Should decode writer lifespan
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0,
                                                            12, 90000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.lifespan.duration.sec ==
                writer_data_out.ddsPublicationData.lifespan.duration.sec);
    EXPECT_TRUE(writer_data.ddsPublicationData.lifespan.duration.nanosec ==
                writer_data_out.ddsPublicationData.lifespan.duration.nanosec);
  }
  { // Should set writer lifespan to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.lifespan.duration.sec = 1;
    writer_data_out.ddsPublicationData.lifespan.duration.nanosec = 7;

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    LifespanQosPolicy defaultQos =
      TheServiceParticipant->initial_LifespanQosPolicy();
    EXPECT_TRUE(defaultQos.duration.sec ==
                writer_data_out.ddsPublicationData.lifespan.duration.sec);
    EXPECT_TRUE(defaultQos.duration.nanosec ==
                writer_data_out.ddsPublicationData.lifespan.duration.nanosec);
  }

  { // Should encode writer user data
    const char* ud = "USERDATA TEST";
    CORBA::ULong ud_len = (CORBA::ULong)strlen(ud) + 1;
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0,
                                                            ud, ud_len);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_USER_DATA));
    Parameter param = get(param_list, PID_USER_DATA);
    EXPECT_TRUE(param.user_data().value.length() == ud_len);
    for (CORBA::ULong i = 0; i < ud_len; ++i) {
      EXPECT_TRUE(ud[i] == param.user_data().value[i]);
    }
  }
  { // Should decode writer user data
    const char* ud = "USERDATA TEST";
    CORBA::ULong ud_len = (CORBA::ULong)strlen(ud) + 1;
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0,
                                                            ud, ud_len);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.user_data.value ==
                writer_data_out.ddsPublicationData.user_data.value);
  }
  { // Should set writer user data to default if not present in param list
    const char* ud = "USERDATA TEST";
    CORBA::ULong ud_len = (CORBA::ULong)strlen(ud) + 1;
    DiscoveredWriterData writer_data_out = Factory::writer_data(
                                                                NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                                KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                                AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                                BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0,
                                                                ud, ud_len);

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    UserDataQosPolicy defaultQos =
      TheServiceParticipant->initial_UserDataQosPolicy();
    EXPECT_TRUE(defaultQos.value ==
                writer_data_out.ddsPublicationData.user_data.value);
  }

  { // Should encode writer ownership
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            EXCLUSIVE_OWNERSHIP_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_OWNERSHIP));
    Parameter param = get(param_list, PID_OWNERSHIP);
    EXPECT_TRUE(param.ownership().kind == EXCLUSIVE_OWNERSHIP_QOS);
  }

  { // Should decode writer ownership
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            EXCLUSIVE_OWNERSHIP_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.ownership.kind ==
                writer_data_out.ddsPublicationData.ownership.kind);
  }
  { // Should set writer ownership to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.ownership.kind = EXCLUSIVE_OWNERSHIP_QOS;

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    OwnershipQosPolicy defaultQos =
      TheServiceParticipant->initial_OwnershipQosPolicy();
    EXPECT_TRUE(defaultQos.kind ==
                writer_data_out.ddsPublicationData.ownership.kind);
  }

  { // Should encode writer ownership strength
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            29);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    EXPECT_TRUE(is_present(param_list, PID_OWNERSHIP_STRENGTH));
    Parameter param = get(param_list, PID_OWNERSHIP_STRENGTH);
    EXPECT_TRUE(param.ownership_strength().value == 29);
#endif
  }
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  { // Should decode writer ownership strength
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            29);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.ownership_strength.value ==
                writer_data_out.ddsPublicationData.ownership_strength.value);
  }

  { // Should set writer ownership strength to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.ownership_strength.value = 17;

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    OwnershipStrengthQosPolicy defaultQos =
      TheServiceParticipant->initial_OwnershipStrengthQosPolicy();
    EXPECT_TRUE(defaultQos.value ==
                writer_data_out.ddsPublicationData.ownership_strength.value);
  }
#endif
  { // Should encode writer destination order
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_DESTINATION_ORDER));
    Parameter param = get(param_list, PID_DESTINATION_ORDER);
    EXPECT_TRUE(param.destination_order().kind ==
                BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
  }
  { // Should decode writer destination order
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.destination_order.kind ==
                writer_data_out.ddsPublicationData.destination_order.kind);
  }
  { // Should set writer destination order to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.destination_order.kind =
      BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    DestinationOrderQosPolicy defaultQos =
      TheServiceParticipant->initial_DestinationOrderQosPolicy();
    EXPECT_TRUE(defaultQos.kind ==
                writer_data_out.ddsPublicationData.destination_order.kind);
  }

  { // Should encode writer presentation
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            GROUP_PRESENTATION_QOS, true, true);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_PRESENTATION));
    Parameter param = get(param_list, PID_PRESENTATION);
    EXPECT_TRUE(param.presentation().access_scope ==
                GROUP_PRESENTATION_QOS);
    EXPECT_TRUE(param.presentation().coherent_access == true);
    EXPECT_TRUE(param.presentation().ordered_access == true);
  }
  { // Should decode writer presentation
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            GROUP_PRESENTATION_QOS, true, true);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.presentation.access_scope ==
                writer_data_out.ddsPublicationData.presentation.access_scope);
    EXPECT_TRUE(writer_data.ddsPublicationData.presentation.coherent_access ==
                writer_data_out.ddsPublicationData.presentation.coherent_access);
    EXPECT_TRUE(writer_data.ddsPublicationData.presentation.ordered_access ==
                writer_data_out.ddsPublicationData.presentation.ordered_access);
  }
  { // Should set writer presentation to default if not present in param list
    DiscoveredWriterData writer_data_out;
    writer_data_out.ddsPublicationData.presentation.access_scope =
      GROUP_PRESENTATION_QOS;
    writer_data_out.ddsPublicationData.presentation.coherent_access = true;
    writer_data_out.ddsPublicationData.presentation.ordered_access = true;

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    PresentationQosPolicy defaultQos =
      TheServiceParticipant->initial_PresentationQosPolicy();
    EXPECT_TRUE(defaultQos.access_scope ==
                writer_data_out.ddsPublicationData.presentation.access_scope);
    EXPECT_TRUE(defaultQos.coherent_access ==
                writer_data_out.ddsPublicationData.presentation.coherent_access);
    EXPECT_TRUE(defaultQos.ordered_access ==
                writer_data_out.ddsPublicationData.presentation.ordered_access);
  }

  { // Should encode writer partition
    const char* part = "TESTPARTITION";
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false,
                                                            part);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_PARTITION));
    Parameter param = get(param_list, PID_PARTITION);
    EXPECT_TRUE(param.partition().name.length() == 1);
    EXPECT_TRUE(strncmp(param.partition().name[0], part, 12) == 0);
  }
  { // Should decode writer partition
    const char* part = "TESTPARTITION";
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false,
                                                            part);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data_out.ddsPublicationData.partition.name.length() == 1);
    EXPECT_TRUE(!strcmp(writer_data.ddsPublicationData.partition.name[0],
                        writer_data_out.ddsPublicationData.partition.name[0]));
  }

  { // Should set writer partition to default if not present in param list
    const char* part = "TESTPARTITION";
    DiscoveredWriterData writer_data_out = Factory::writer_data(
                                                                NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                                KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                                AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                                BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                                SHARED_OWNERSHIP_QOS, 0,
                                                                BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                                INSTANCE_PRESENTATION_QOS, false, false,
                                                                part);

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(!writer_data_out.ddsPublicationData.partition.name.length());
  }

  { // Should encode writer topic data
    const char* topic_data = "TEST TD";
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL,
                                                            topic_data, 7);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_TOPIC_DATA));
    Parameter param = get(param_list, PID_TOPIC_DATA);
    CORBA::ULong len = param.topic_data().value.length();
    EXPECT_TRUE(len == 7);
    for (CORBA::ULong i = 0; i < len; ++i) {
      EXPECT_TRUE(param.topic_data().value[i] = topic_data[i]);
    }
  }
  { // Should decode writer topic data
    const char* topic_data = "TEST TD";
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL,
                                                            topic_data, 7);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.topic_data.value ==
                writer_data_out.ddsPublicationData.topic_data.value);
  }

  { // Should set writer presentation to default if not present in param list
    const char* topic_data = "TEST TD";
    DiscoveredWriterData writer_data_out = Factory::writer_data(
                                                                NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                                KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                                AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                                BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                                SHARED_OWNERSHIP_QOS, 0,
                                                                BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                                INSTANCE_PRESENTATION_QOS, false, false, NULL,
                                                                topic_data, 7);

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(!writer_data_out.ddsPublicationData.topic_data.value.length());
  }
  { // Should encode writer group data
    const char* group_data = "TEST GD";
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0,
                                                            group_data, 7);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_GROUP_DATA));
    Parameter param = get(param_list, PID_GROUP_DATA);
    CORBA::ULong len = param.group_data().value.length();
    EXPECT_TRUE(len == 7);
    for (CORBA::ULong i = 0; i < len; ++i) {
      EXPECT_TRUE(param.group_data().value[i] = group_data[i]);
    }
  }
  { // Should decode writer group data
    const char* group_data = "TEST GD";
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0,
                                                            group_data, 7);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data.ddsPublicationData.group_data.value ==
                writer_data_out.ddsPublicationData.group_data.value);
  }

  { // Should set writer presentation to default if not present in param list
    const char* group_data = "TEST GD";
    DiscoveredWriterData writer_data_out = Factory::writer_data(
                                                                NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                                KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                                AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                                BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                                SHARED_OWNERSHIP_QOS, 0,
                                                                BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                                INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0,
                                                                group_data, 7);

    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(!writer_data_out.ddsPublicationData.group_data.value.length());
  }
  { // Should encode writer guid
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_ENDPOINT_GUID));
  }
  { // Should decode writer guid
    DiscoveredWriterData writer_data = Factory::default_writer_data();
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(!memcmp(&writer_data.writerProxy.remoteWriterGuid,
                        &writer_data_out.writerProxy.remoteWriterGuid,
                        sizeof(GUID_t)));
  }

  { // Should encode writer unicast locators
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            locators, 2
                                                            );
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_UNICAST_LOCATOR));
    EXPECT_TRUE(!is_present(param_list, PID_MULTICAST_LOCATOR));
    EXPECT_TRUE(!is_present(param_list, PID_OPENDDS_LOCATOR));
  }
  { // Should decode writer unicast locators into allLocators
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            locators, 2);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data_out.writerProxy.allLocators.length() == 1);
    EXPECT_TRUE(!strcmp(writer_data_out.writerProxy.allLocators[0].transport_type, "rtps_udp"));
  }

  { // Should encode writer multicast locators
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   227, 200, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   7734,
                                   237, 9, 8, 21);
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            NULL, 0, locators, 2);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_MULTICAST_LOCATOR));
    EXPECT_TRUE(!is_present(param_list, PID_UNICAST_LOCATOR));
    EXPECT_TRUE(!is_present(param_list, PID_OPENDDS_LOCATOR));
  }
  { // Should decode writer multicast locators
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    DiscoveredWriterData writer_data = Factory::writer_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS, 0,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            NULL, 0, locators, 2);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    DiscoveredWriterData writer_data_out;
    EXPECT_TRUE(from_param_list(param_list, writer_data_out, true, type_info));
    EXPECT_TRUE(writer_data_out.writerProxy.allLocators.length() == 1);
    EXPECT_TRUE(!strcmp(writer_data_out.writerProxy.allLocators[0].transport_type, "rtps_udp"));
  }

  { // Should encode reader data
    DiscoveredReaderData reader_data = Factory::default_reader_data();
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
  }

  { // Should encode reader topic name
    DiscoveredReaderData reader_data =
      Factory::reader_data("TOPIC NAME TEST");
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_TOPIC_NAME));
    Parameter param = get(param_list, PID_TOPIC_NAME);
    EXPECT_TRUE(!strncmp(param.string_data(), "TOPIC NAME TEST", 15));
  }

  { // Should decode reader topic name
    DiscoveredReaderData reader_data =
      Factory::reader_data("TOPIC NAME TEST");
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(!strcmp(reader_data.ddsSubscriptionData.topic_name,
                        reader_data_out.ddsSubscriptionData.topic_name));
  }

  { // Should encode reader type name
    DiscoveredReaderData reader_data = Factory::reader_data("", "Messages");

    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_TYPE_NAME));
    Parameter param = get(param_list, PID_TYPE_NAME);
    EXPECT_TRUE(!strncmp(param.string_data(), "Messages", 8));
  }

  { // Should decode reader type name
    DiscoveredReaderData reader_data = Factory::reader_data("", "Messages");
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(!strcmp(reader_data.ddsSubscriptionData.type_name,
                        reader_data_out.ddsSubscriptionData.type_name));
  }

  { // Should encode reader durability qos policy
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL,
                                                            TRANSIENT_LOCAL_DURABILITY_QOS);

    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_DURABILITY));
    Parameter param = get(param_list, PID_DURABILITY);
    EXPECT_TRUE(param.durability().kind == TRANSIENT_LOCAL_DURABILITY_QOS);
  }

  { // Should decode reader durability
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL,
                                                            TRANSIENT_LOCAL_DURABILITY_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.durability.kind ==
                reader_data_out.ddsSubscriptionData.durability.kind);
  }

  { // Should encode reader deadline
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS,
                                                            127, 35000);

    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_DEADLINE));
    Parameter param = get(param_list, PID_DEADLINE);
    EXPECT_TRUE(param.deadline().period.sec == 127);
    EXPECT_TRUE(param.deadline().period.nanosec == 35000);
  }

  { // Should decode reader deadline
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS,
                                                            127, 35000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.deadline.period.sec ==
                reader_data_out.ddsSubscriptionData.deadline.period.sec);
    EXPECT_TRUE(reader_data.ddsSubscriptionData.deadline.period.nanosec ==
                reader_data_out.ddsSubscriptionData.deadline.period.nanosec);
  }

  { // Should enode reader latency budget
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            5, 25000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_LATENCY_BUDGET));
    Parameter param = get(param_list, PID_LATENCY_BUDGET);
    EXPECT_TRUE(param.deadline().period.sec == 5);
    EXPECT_TRUE(param.deadline().period.nanosec == 25000);
  }

  { // Should decode reader latency budget
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
                                                            5, 25000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.latency_budget.duration.sec ==
                reader_data_out.ddsSubscriptionData.latency_budget.duration.sec);
  }

  { // Should encode reader liveliness
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, 17, 15000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_LIVELINESS));
    Parameter param = get(param_list, PID_LIVELINESS);
    EXPECT_TRUE(param.liveliness().kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
    EXPECT_TRUE(param.liveliness().lease_duration.sec == 17);
    EXPECT_TRUE(param.liveliness().lease_duration.nanosec == 15000);
  }

  { // Should decode reader liveliness
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, 17, 15000);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.liveliness.kind ==
                reader_data_out.ddsSubscriptionData.liveliness.kind);
    EXPECT_TRUE(reader_data.ddsSubscriptionData.liveliness.lease_duration.sec ==
                reader_data_out.ddsSubscriptionData.liveliness.lease_duration.sec);
    EXPECT_TRUE(reader_data.ddsSubscriptionData.liveliness.lease_duration.nanosec ==
                reader_data_out.ddsSubscriptionData.liveliness.lease_duration.nanosec);
  }

  { // Should encode reader reliability
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            RELIABLE_RELIABILITY_QOS, 8, 100);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_RELIABILITY));
    Parameter param = get(param_list, PID_RELIABILITY);
    EXPECT_TRUE(param.reliability().kind.value == RELIABLE);
    EXPECT_TRUE(param.reliability().max_blocking_time.sec == 8);
    EXPECT_TRUE(param.reliability().max_blocking_time.nanosec == 100);
  }

  { // Should decode reader reliability
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            RELIABLE_RELIABILITY_QOS, 8, 100);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.reliability.kind ==
                reader_data_out.ddsSubscriptionData.reliability.kind);
    EXPECT_TRUE(reader_data.ddsSubscriptionData.reliability.max_blocking_time.sec ==
                reader_data_out.ddsSubscriptionData.reliability.max_blocking_time.sec);
    EXPECT_TRUE(reader_data.ddsSubscriptionData.reliability.max_blocking_time.nanosec ==
                reader_data_out.ddsSubscriptionData.reliability.max_blocking_time.nanosec);
  }

  { // Should encode reader user data
    const char* ud = "USERDATA TEST";
    CORBA::ULong ud_len = (CORBA::ULong)strlen(ud) + 1;
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0,
                                                            ud, ud_len);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_USER_DATA));
    Parameter param = get(param_list, PID_USER_DATA);
    EXPECT_TRUE(param.user_data().value.length() == ud_len);
    for (CORBA::ULong i = 0; i < ud_len; ++i) {
      EXPECT_TRUE(ud[i] == param.user_data().value[i]);
    }
  }
  { // Should decode reader user data
    const char* ud = "USERDATA TEST";
    CORBA::ULong ud_len = (CORBA::ULong)strlen(ud) + 1;
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0,
                                                            ud, ud_len);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.user_data.value ==
                reader_data_out.ddsSubscriptionData.user_data.value);
  }

  { // Should encode reader ownership
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            EXCLUSIVE_OWNERSHIP_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_OWNERSHIP));
    Parameter param = get(param_list, PID_OWNERSHIP);
    EXPECT_TRUE(param.ownership().kind == EXCLUSIVE_OWNERSHIP_QOS);
  }

  { // Should decode reader ownership
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            EXCLUSIVE_OWNERSHIP_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.ownership.kind ==
                reader_data_out.ddsSubscriptionData.ownership.kind);
  }

  { // Should encode reader destination order
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_DESTINATION_ORDER));
    Parameter param = get(param_list, PID_DESTINATION_ORDER);
    EXPECT_TRUE(param.destination_order().kind ==
                BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
  }
  { // Should decode reader destination order
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.destination_order.kind ==
                reader_data_out.ddsSubscriptionData.destination_order.kind);
  }

  { // Should encode reader presentation
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            GROUP_PRESENTATION_QOS, true, true);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_PRESENTATION));
    Parameter param = get(param_list, PID_PRESENTATION);
    EXPECT_TRUE(param.presentation().access_scope ==
                GROUP_PRESENTATION_QOS);
    EXPECT_TRUE(param.presentation().coherent_access == true);
    EXPECT_TRUE(param.presentation().ordered_access == true);
  }
  { // Should decode reader presentation
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            GROUP_PRESENTATION_QOS, true, true);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.presentation.access_scope ==
                reader_data_out.ddsSubscriptionData.presentation.access_scope);
    EXPECT_TRUE(reader_data.ddsSubscriptionData.presentation.coherent_access ==
                reader_data_out.ddsSubscriptionData.presentation.coherent_access);
    EXPECT_TRUE(reader_data.ddsSubscriptionData.presentation.ordered_access ==
                reader_data_out.ddsSubscriptionData.presentation.ordered_access);
  }

  { // Should encode reader partition
    const char* part = "TESTPARTITION";
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false,
                                                            part);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_PARTITION));
    Parameter param = get(param_list, PID_PARTITION);
    EXPECT_TRUE(param.partition().name.length() == 1);
    EXPECT_TRUE(strncmp(param.partition().name[0], part, 12) == 0);
  }
  { // Should decode reader partition
    const char* part = "TESTPARTITION";
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false,
                                                            part);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.partition.name.length() == 1);
    EXPECT_TRUE(!strcmp(reader_data.ddsSubscriptionData.partition.name[0],
                        reader_data_out.ddsSubscriptionData.partition.name[0]));
  }

  { // Should encode reader topic data
    const char* topic_data = "TEST TD";
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL,
                                                            topic_data, 7);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_TOPIC_DATA));
    Parameter param = get(param_list, PID_TOPIC_DATA);
    CORBA::ULong len = param.topic_data().value.length();
    EXPECT_TRUE(len == 7);
    for (CORBA::ULong i = 0; i < len; ++i) {
      EXPECT_TRUE(param.topic_data().value[i] = topic_data[i]);
    }
  }
  { // Should decode reader topic data
    const char* topic_data = "TEST TD";
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL,
                                                            topic_data, 7);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.topic_data.value ==
                reader_data_out.ddsSubscriptionData.topic_data.value);
  }

  { // Should encode reader group data
    const char* group_data = "TEST GD";
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0,
                                                            group_data, 7);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_GROUP_DATA));
    Parameter param = get(param_list, PID_GROUP_DATA);
    CORBA::ULong len = param.group_data().value.length();
    EXPECT_TRUE(len == 7);
    for (CORBA::ULong i = 0; i < len; ++i) {
      EXPECT_TRUE(param.group_data().value[i] = group_data[i]);
    }
  }
  { // Should decode reader group data
    const char* group_data = "TEST GD";
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0,
                                                            group_data, 7);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data.ddsSubscriptionData.group_data.value ==
                reader_data_out.ddsSubscriptionData.group_data.value);
  }

  { // Should encode reader guid
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_ENDPOINT_GUID));
  }
  { // Should decode reader guid
    DiscoveredReaderData reader_data;
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(!memcmp(&reader_data.readerProxy.remoteReaderGuid,
                        &reader_data_out.readerProxy.remoteReaderGuid,
                        sizeof(GUID_t)));
  }

  { // Should encode reader unicast locators
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            locators, 2
                                                            );
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_UNICAST_LOCATOR));
    EXPECT_TRUE(!is_present(param_list, PID_MULTICAST_LOCATOR));
    EXPECT_TRUE(!is_present(param_list, PID_OPENDDS_LOCATOR));
  }
  { // Should decode reader unicast locators
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            locators, 2);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data_out.readerProxy.allLocators.length() == 1);
  }

  { // Should encode reader multicast locators
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   227, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   7734,
                                   237, 9, 8, 21);
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            NULL, 0, locators, 2);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_MULTICAST_LOCATOR));
    Parameter param0 = get(param_list, PID_MULTICAST_LOCATOR, 0);
    Locator_t loc0 = param0.locator();
    EXPECT_TRUE(!memcmp(&loc0, &locators[0], sizeof(Locator_t)));
    Parameter param1 = get(param_list, PID_MULTICAST_LOCATOR, 1);
    Locator_t loc1 = param1.locator();
    EXPECT_TRUE(!memcmp(&loc1, &locators[1], sizeof(Locator_t)));
  }
  { // Should decode reader multicast locators
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   227, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   237, 9, 8, 21);
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            NULL, 0, locators, 2);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(reader_data_out.readerProxy.allLocators.length() == 1);
    EXPECT_TRUE(!strcmp(reader_data_out.readerProxy.allLocators[0].transport_type, "rtps_udp"));
  }
  { // Should encode reader content filter property
    const char* cf_topic_name = "CFTopic test";
    const char* rel_topic_name = "CFTopic rel";
    const char* filter_expr = "sequence > 10";
    const char* params[] = { "17", "32" };

    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            NULL, 0, NULL, 0,
                                                            cf_topic_name, rel_topic_name, NULL, filter_expr, params, 2);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(is_present(param_list, PID_CONTENT_FILTER_PROPERTY));
    // Default check
    Parameter param = get(param_list, PID_CONTENT_FILTER_PROPERTY);
    EXPECT_TRUE(!strcmp("DDSSQL",
                        param.content_filter_property().filterClassName));
  }
  { // Should decode reader content filter property
    const char* cf_topic_name = "CFTopic test";
    const char* rel_topic_name = "CFTopic rel";
    const char* filter_name = "Filter test";
    const char* filter_expr = "sequence > 10";
    const char* params[] = { "17", "32" };

    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            NULL, 0, NULL, 0,
                                                            cf_topic_name, rel_topic_name, filter_name, filter_expr, params, 2);
    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    DiscoveredReaderData reader_data_out;
    EXPECT_TRUE(from_param_list(param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(
                !strcmp(reader_data.contentFilterProperty.contentFilteredTopicName,
                        reader_data_out.contentFilterProperty.contentFilteredTopicName));
    EXPECT_TRUE(
                !strcmp(reader_data.contentFilterProperty.relatedTopicName,
                        reader_data_out.contentFilterProperty.relatedTopicName));
    EXPECT_TRUE(
                !strcmp(reader_data.contentFilterProperty.filterClassName,
                        reader_data_out.contentFilterProperty.filterClassName));
    EXPECT_TRUE(
                !strcmp(reader_data.contentFilterProperty.filterExpression,
                        reader_data_out.contentFilterProperty.filterExpression));
    EXPECT_TRUE(reader_data_out.contentFilterProperty.expressionParameters.length() == 2);
    EXPECT_TRUE(!strcmp("17",
                        reader_data_out.contentFilterProperty.expressionParameters[0]));
    EXPECT_TRUE(!strcmp("32",
                        reader_data_out.contentFilterProperty.expressionParameters[1]));
  }
  { // Should encode/decode reader assoicated guid list properly
    DiscoveredReaderData reader_data = Factory::reader_data(
                                                            NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
                                                            AUTOMATIC_LIVELINESS_QOS, 0, 0,
                                                            BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
                                                            SHARED_OWNERSHIP_QOS,
                                                            BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                            INSTANCE_PRESENTATION_QOS, false, false, NULL, NULL, 0, NULL, 0,
                                                            NULL, 0, NULL, 0);
    OpenDDS::DCPS::GUID_t writer0, writer1;
    OpenDDS::DCPS::GuidBuilder gb0(writer0);
    OpenDDS::DCPS::GuidBuilder gb1(writer1);
    gb0.guidPrefix0(17);
    gb0.guidPrefix1(0);
    gb0.guidPrefix2(167);
    gb1.guidPrefix0(17);
    gb1.guidPrefix1(1);
    gb1.guidPrefix2(167);

    reader_data.readerProxy.associatedWriters.length(2);
    reader_data.readerProxy.associatedWriters[0] = writer0;
    reader_data.readerProxy.associatedWriters[1] = writer1;

    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info));
    Parameter guid0 = get(param_list, PID_OPENDDS_ASSOCIATED_WRITER, 0);
    Parameter guid1 = get(param_list, PID_OPENDDS_ASSOCIATED_WRITER, 1);
    EXPECT_TRUE(!memcmp(guid0.guid().guidPrefix,
                        writer0.guidPrefix,
                        sizeof(OpenDDS::DCPS::GuidPrefix_t)));
    EXPECT_TRUE(!memcmp(guid1.guid().guidPrefix,
                        writer1.guidPrefix,
                        sizeof(OpenDDS::DCPS::GuidPrefix_t)));

    DiscoveredReaderData reader_data_out;
    from_param_list(param_list, reader_data_out, true, type_info);
    EXPECT_TRUE(reader_data_out.readerProxy.associatedWriters.length() == 2);
    EXPECT_TRUE(!memcmp(reader_data_out.readerProxy.associatedWriters[0].guidPrefix,
                        writer0.guidPrefix,
                        sizeof(OpenDDS::DCPS::GuidPrefix_t)));
    EXPECT_TRUE(!memcmp(reader_data_out.readerProxy.associatedWriters[1].guidPrefix,
                        writer1.guidPrefix,
                        sizeof(OpenDDS::DCPS::GuidPrefix_t)));
  }

  { // Should set reader defaults
    Locator_t uc_locators[2];
    Locator_t mc_locators[2];
    uc_locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                      1234,
                                      127, 0, 0, 1);
    uc_locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                      7734,
                                      107, 9, 8, 21);
    mc_locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                      1234,
                                      127, 0, 0, 1);
    mc_locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                      7734,
                                      107, 9, 8, 21);
    const char* ud = "USERDATA TEST";
    const char* part = "TESTPARTITION";
    const char* topic_data = "TEST TD";
    const char* group_data = "TEST GD";
    const char* cf_topic_name = "CFTopic test";
    const char* rel_topic_name = "CFTopic rel";
    const char* filter_name = "Filter test";
    const char* filter_expr = "sequence > 10";
    const char* params[] = { "17", "32" };
    CORBA::ULong ud_len = (CORBA::ULong)strlen(ud) + 1;
    DiscoveredReaderData reader_data_out = Factory::reader_data(
                                                                "ABC", "DEF", TRANSIENT_LOCAL_DURABILITY_QOS, 2, 3, 4, 5,
                                                                MANUAL_BY_TOPIC_LIVELINESS_QOS, 6, 7,
                                                                RELIABLE_RELIABILITY_QOS, 8, 9, ud, ud_len,
                                                                EXCLUSIVE_OWNERSHIP_QOS,
                                                                BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS,
                                                                TOPIC_PRESENTATION_QOS, true, true, part, topic_data, 7, group_data, 7,
                                                                uc_locators, 2, mc_locators, 2,
                                                                cf_topic_name, rel_topic_name, filter_name, filter_expr, params, 2);
    ParameterList empty_param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    EXPECT_TRUE(from_param_list(empty_param_list, reader_data_out, true, type_info));
    EXPECT_TRUE(!strcmp(reader_data_out.ddsSubscriptionData.topic_name, ""));
    EXPECT_TRUE(!strcmp(reader_data_out.ddsSubscriptionData.type_name, ""));
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.durability.kind ==
                TheServiceParticipant->initial_DurabilityQosPolicy().kind);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.deadline.period.sec ==
                TheServiceParticipant->initial_DeadlineQosPolicy().period.sec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.deadline.period.nanosec ==
                TheServiceParticipant->initial_DeadlineQosPolicy().period.nanosec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.latency_budget.duration.sec ==
                TheServiceParticipant->initial_LatencyBudgetQosPolicy().duration.sec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.latency_budget.duration.nanosec ==
                TheServiceParticipant->initial_LatencyBudgetQosPolicy().duration.nanosec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.liveliness.kind ==
                TheServiceParticipant->initial_LivelinessQosPolicy().kind);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.liveliness.lease_duration.sec ==
                TheServiceParticipant->initial_LivelinessQosPolicy().lease_duration.sec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.liveliness.lease_duration.nanosec ==
                TheServiceParticipant->initial_LivelinessQosPolicy().lease_duration.nanosec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.reliability.kind ==
                TheServiceParticipant->initial_DataReaderQos().reliability.kind);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.reliability.max_blocking_time.sec ==
                TheServiceParticipant->initial_ReliabilityQosPolicy().max_blocking_time.sec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.reliability.max_blocking_time.nanosec ==
                TheServiceParticipant->initial_ReliabilityQosPolicy().max_blocking_time.nanosec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.ownership.kind ==
                TheServiceParticipant->initial_OwnershipQosPolicy().kind);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.destination_order.kind ==
                TheServiceParticipant->initial_DestinationOrderQosPolicy().kind);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.user_data.value ==
                TheServiceParticipant->initial_UserDataQosPolicy().value);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.time_based_filter.minimum_separation.sec ==
                TheServiceParticipant->initial_TimeBasedFilterQosPolicy().minimum_separation.sec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.time_based_filter.minimum_separation.nanosec ==
                TheServiceParticipant->initial_TimeBasedFilterQosPolicy().minimum_separation.nanosec);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.presentation.access_scope ==
                TheServiceParticipant->initial_PresentationQosPolicy().access_scope);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.presentation.coherent_access ==
                TheServiceParticipant->initial_PresentationQosPolicy().coherent_access);
    EXPECT_TRUE(reader_data_out.ddsSubscriptionData.presentation.ordered_access ==
                TheServiceParticipant->initial_PresentationQosPolicy().ordered_access);
    EXPECT_TRUE(!reader_data_out.ddsSubscriptionData.partition.name.length());
    EXPECT_TRUE(!reader_data_out.ddsSubscriptionData.topic_data.value.length());
    EXPECT_TRUE(!reader_data_out.ddsSubscriptionData.group_data.value.length());
    EXPECT_TRUE(!strlen(reader_data_out.contentFilterProperty.contentFilteredTopicName));
    EXPECT_TRUE(!strlen(reader_data_out.contentFilterProperty.relatedTopicName));
    EXPECT_TRUE(!strlen(reader_data_out.contentFilterProperty.filterClassName));
    EXPECT_TRUE(!strlen(reader_data_out.contentFilterProperty.filterExpression));
    EXPECT_TRUE(!reader_data_out.contentFilterProperty.expressionParameters.length());
  }
  { // Should not encode participant default data
    OpenDDS::RTPS::SPDPdiscoveredParticipantData participant_data =
      Factory::default_participant_data();
    ParameterList param_list;
    bool status = to_param_list(participant_data, param_list);
    EXPECT_TRUE(status == true);
    EXPECT_TRUE(!is_present(param_list, PID_USER_DATA));
    EXPECT_TRUE(!is_present(param_list, PID_EXPECTS_INLINE_QOS));
    EXPECT_TRUE(!is_present(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }
  { // Should not encode writer default data
    DiscoveredWriterData writer_data = Factory::default_writer_data();

    ParameterList param_list;
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(writer_data, param_list, true, type_info, map));
    EXPECT_TRUE(!is_present(param_list, PID_DURABILITY));
    EXPECT_TRUE(!is_present(param_list, PID_DURABILITY_SERVICE));
    EXPECT_TRUE(!is_present(param_list, PID_DEADLINE));
    EXPECT_TRUE(!is_present(param_list, PID_LATENCY_BUDGET));
    EXPECT_TRUE(!is_present(param_list, PID_LIVELINESS));
    // reliability info is always written:
    //    EXPECT_TRUE(!is_present(param_list, PID_RELIABILITY));
    EXPECT_TRUE(!is_present(param_list, PID_LIFESPAN));
    EXPECT_TRUE(!is_present(param_list, PID_USER_DATA));
    EXPECT_TRUE(!is_present(param_list, PID_OWNERSHIP));
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    EXPECT_TRUE(!is_present(param_list, PID_OWNERSHIP_STRENGTH));
#endif
    EXPECT_TRUE(!is_present(param_list, PID_DESTINATION_ORDER));
    EXPECT_TRUE(!is_present(param_list, PID_PRESENTATION));
    EXPECT_TRUE(!is_present(param_list, PID_PARTITION));
    EXPECT_TRUE(!is_present(param_list, PID_TOPIC_DATA));
    EXPECT_TRUE(!is_present(param_list, PID_GROUP_DATA));
  }
  { // Should not encode reader default data
    DiscoveredReaderData reader_data = Factory::default_reader_data();
    ParameterList param_list;
    EXPECT_TRUE(param_list.length() == 0);
    OpenDDS::XTypes::TypeInformation type_info;
    type_info.minimal.typeid_with_size.typeobject_serialized_size = 0;
    type_info.minimal.dependent_typeid_count = 0;
    type_info.complete.dependent_typeid_count = 0;
    bool map = false;
    EXPECT_TRUE(to_param_list(reader_data, param_list, true, type_info, map));
    EXPECT_TRUE(!is_present(param_list, PID_DURABILITY));
    EXPECT_TRUE(!is_present(param_list, PID_DEADLINE));
    EXPECT_TRUE(!is_present(param_list, PID_LATENCY_BUDGET));
    EXPECT_TRUE(!is_present(param_list, PID_LIVELINESS));
    // reliability info is always written:
    //    EXPECT_TRUE(!is_present(param_list, PID_RELIABILITY));
    EXPECT_TRUE(!is_present(param_list, PID_OWNERSHIP));
    EXPECT_TRUE(!is_present(param_list, PID_DESTINATION_ORDER));
    EXPECT_TRUE(!is_present(param_list, PID_USER_DATA));
    EXPECT_TRUE(!is_present(param_list, PID_TIME_BASED_FILTER));
    EXPECT_TRUE(!is_present(param_list, PID_PRESENTATION));
    EXPECT_TRUE(!is_present(param_list, PID_PARTITION));
    EXPECT_TRUE(!is_present(param_list, PID_TOPIC_DATA));
    EXPECT_TRUE(!is_present(param_list, PID_GROUP_DATA));
    EXPECT_TRUE(!is_present(param_list, PID_CONTENT_FILTER_PROPERTY));
  }
}
