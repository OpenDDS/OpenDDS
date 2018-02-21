/*
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PARAMETER_LIST_CONVERTER_TEST_HELPERS_H
#define PARAMETER_LIST_CONVERTER_TEST_HELPERS_H

#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/RTPS/RtpsCoreC.h"
#include "dds/DCPS/RTPS/GuidGenerator.h"
#include "dds/DCPS/Service_Participant.h"

using namespace OpenDDS::RTPS;
using namespace DDS;
using namespace OpenDDS::DCPS;

namespace {
  GuidGenerator guid_generator;

  namespace Factory {

#if 0
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
#endif

    SPDPdiscoveredParticipantData spdp_participant(
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
      SPDPdiscoveredParticipantData result;
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

#if 0
    SPDPdiscoveredParticipantData default_participant_data()
    {
      SPDPdiscoveredParticipantData part_data;
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
#endif

  } // Factory namespace
} // anon namespace

#endif

