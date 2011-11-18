/*
 * $Id$
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"
#include "dds/DCPS/RTPS/ParameterListConverter.h"
#include "../common/TestSupport.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/RTPS/RtpsMessageTypesC.h"
#include "dds/DCPS/RTPS/GuidGenerator.h"
#include <iostream>

using namespace OpenDDS::RTPS;
using namespace DDS;

namespace {
  ParameterListConverter plc;
  GuidGenerator guid_generator;

  namespace Factory {
    Locator_t locator(long kind,
                      unsigned long port,
                      unsigned int addr0, 
                      unsigned int addr1, 
                      unsigned int addr2,
                      unsigned int addr3)
    {
      Locator_t result;
      result.kind = kind;
      result.port = port;
      result.address[ 0] = addr0 & 0x000000FF;
      result.address[ 1] = addr0 & 0x0000FF00;
      result.address[ 2] = addr0 & 0x00FF0000;
      result.address[ 3] = addr0 & 0xFF000000;
      result.address[ 4] = addr1 & 0x000000FF;
      result.address[ 5] = addr1 & 0x0000FF00;
      result.address[ 6] = addr1 & 0x00FF0000;
      result.address[ 7] = addr1 & 0xFF000000;
      result.address[ 8] = addr2 & 0x000000FF;
      result.address[ 9] = addr2 & 0x0000FF00;
      result.address[10] = addr2 & 0x00FF0000;
      result.address[11] = addr2 & 0xFF000000;
      result.address[12] = addr3 & 0x000000FF;
      result.address[13] = addr3 & 0x0000FF00;
      result.address[14] = addr3 & 0x00FF0000;
      result.address[15] = addr3 & 0xFF000000;

      return result;
    }

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
      long lease_dur_seconds = 0,
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

      if (lease_dur_seconds || lease_dur_fraction) {
        result.leaseDuration.seconds = lease_dur_seconds;
        result.leaseDuration.fraction = lease_dur_fraction;
      }

      return result;
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
      result.ddsPublicationData.ownership_strength.value = ownership_strength;
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
      if (num_uc_locs && uc_locs) {
        result.writerProxy.unicastLocatorList.length(num_uc_locs);
        for (CORBA::ULong i = 0; i < num_uc_locs; ++i) {
          result.writerProxy.unicastLocatorList[i] = uc_locs[i];
        }
      }
      if (num_mc_locs && mc_locs) {
        result.writerProxy.multicastLocatorList.length(num_mc_locs);
        for (CORBA::ULong i = 0; i < num_mc_locs; ++i) {
          result.writerProxy.multicastLocatorList[i] = mc_locs[i];
        }
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
        Locator_t* mc_locs = NULL, CORBA::ULong num_mc_locs = 0
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
      if (num_uc_locs && uc_locs) {
        result.readerProxy.unicastLocatorList.length(num_uc_locs);
        for (CORBA::ULong i = 0; i < num_uc_locs; ++i) {
          result.readerProxy.unicastLocatorList[i] = uc_locs[i];
        }
      }
      if (num_mc_locs && mc_locs) {
        result.readerProxy.multicastLocatorList.length(num_mc_locs);
        for (CORBA::ULong i = 0; i < num_mc_locs; ++i) {
          result.readerProxy.multicastLocatorList[i] = mc_locs[i];
        }
      }
      return result;
    }
  }
}

bool is_present(const ParameterList& param_list, const ParameterId_t pid) {
  CORBA::ULong length = param_list.length();
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      return true;
    }
  }
  return false;
}

bool is_missing(const ParameterList& param_list, const ParameterId_t pid) {
  CORBA::ULong length = param_list.length();
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      return false;
    }
  }
  return true;
}

Parameter get(const ParameterList& param_list, 
              const ParameterId_t pid,
              const CORBA::ULong instance_num = 0) {

  const CORBA::ULong length = param_list.length();
  CORBA::ULong count = 0;
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      if (count++ == instance_num) {
        return param_list[i];
      }
    }
  }
  TEST_ASSERT(false); // Not found
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  { // Should encode participant data with 1 locator to param list properly
    SPDPdiscoveredParticipantData participant_data;
    ParameterList param_list;
    participant_data.participantProxy.metatrafficUnicastLocatorList.length(1);
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    TEST_ASSERT(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    TEST_ASSERT(is_missing(param_list, PID_METATRAFFIC_MULTICAST_LOCATOR));
    TEST_ASSERT(is_missing(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    TEST_ASSERT(is_missing(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }

  { // Should encode participant data with 2 locators to param list properly
    SPDPdiscoveredParticipantData participant_data;
    ParameterList param_list;
    participant_data.participantProxy.metatrafficUnicastLocatorList.length(1);
    participant_data.participantProxy.metatrafficMulticastLocatorList.length(1);
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    TEST_ASSERT(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_METATRAFFIC_MULTICAST_LOCATOR));
    TEST_ASSERT(is_missing(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    TEST_ASSERT(is_missing(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }

  { // Should encode participant data with 3 locators to param list properly
    SPDPdiscoveredParticipantData participant_data;
    ParameterList param_list;
    participant_data.participantProxy.metatrafficUnicastLocatorList.length(1);
    participant_data.participantProxy.metatrafficMulticastLocatorList.length(1);
    participant_data.participantProxy.defaultUnicastLocatorList.length(1);
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    TEST_ASSERT(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_METATRAFFIC_MULTICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    TEST_ASSERT(is_missing(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }

  { // Should encode participant data with 4 locators to param list properly
    SPDPdiscoveredParticipantData participant_data;
    ParameterList param_list;
    participant_data.participantProxy.metatrafficUnicastLocatorList.length(1);
    participant_data.participantProxy.metatrafficMulticastLocatorList.length(1);
    participant_data.participantProxy.defaultUnicastLocatorList.length(1);
    participant_data.participantProxy.defaultMulticastLocatorList.length(1);
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    TEST_ASSERT(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_METATRAFFIC_MULTICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }

  { // Should encode participant user data properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant("hello user", 10);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_USER_DATA));
    Parameter param = get(param_list, PID_USER_DATA);
    UserDataQosPolicy ud_qos = param.user_data();
    TEST_ASSERT(ud_qos.value.length() == 10);
    TEST_ASSERT(ud_qos.value[0] == 'h');
    TEST_ASSERT(ud_qos.value[1] == 'e');
    TEST_ASSERT(ud_qos.value[2] == 'l');
    TEST_ASSERT(ud_qos.value[3] == 'l');
    TEST_ASSERT(ud_qos.value[4] == 'o');
    TEST_ASSERT(ud_qos.value[5] == ' ');
    TEST_ASSERT(ud_qos.value[6] == 'u');
    TEST_ASSERT(ud_qos.value[7] == 's');
    TEST_ASSERT(ud_qos.value[8] == 'e');
    TEST_ASSERT(ud_qos.value[9] == 'r');
  }

  { // Should decode participant user data properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant("hello user", 10);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[0] ==
                part_data_out.ddsParticipantData.user_data.value[0]);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[1] ==
                part_data_out.ddsParticipantData.user_data.value[1]);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[2] ==
                part_data_out.ddsParticipantData.user_data.value[2]);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[3] ==
                part_data_out.ddsParticipantData.user_data.value[3]);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[4] ==
                part_data_out.ddsParticipantData.user_data.value[4]);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[5] ==
                part_data_out.ddsParticipantData.user_data.value[5]);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[6] ==
                part_data_out.ddsParticipantData.user_data.value[6]);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[7] ==
                part_data_out.ddsParticipantData.user_data.value[7]);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[8] ==
                part_data_out.ddsParticipantData.user_data.value[8]);
    TEST_ASSERT(participant_data.ddsParticipantData.user_data.value[9] ==
                part_data_out.ddsParticipantData.user_data.value[9]);
  }

  { // Should encode participant protocol version properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 3, 8);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PROTOCOL_VERSION));
    Parameter param = get(param_list, PID_PROTOCOL_VERSION);
    OpenDDS::RTPS::ProtocolVersion_t pv = param.version();
    TEST_ASSERT(pv.major == 3);
    TEST_ASSERT(pv.minor == 8);
  }

  { // Should decode participant protocol version properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 9, 1);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(participant_data.participantProxy.protocolVersion.major ==
                part_data_out.participantProxy.protocolVersion.major);
    TEST_ASSERT(participant_data.participantProxy.protocolVersion.minor ==
                part_data_out.participantProxy.protocolVersion.minor);
  }

  { // Should encode participant vendor id properly
    char vendor_id[] = {7, 9};
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, vendor_id);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_VENDORID));
    Parameter param = get(param_list, PID_VENDORID);
    OpenDDS::RTPS::VendorId_t vid = param.vendor();
    TEST_ASSERT(vid.vendorId[0] == 7);
    TEST_ASSERT(vid.vendorId[1] == 9);
  }

  { // Should decode participant vendor id properly
    char vendor_id[] = {7, 9};
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, vendor_id);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(!memcmp(participant_data.participantProxy.vendorId.vendorId,
                        part_data_out.participantProxy.vendorId.vendorId,
                        sizeof(OctetArray2)));
  }

  { // Should encode participant guid prefix properly
    GUID_t guid_in;
    memcpy(guid_in.guidPrefix, "GUID-ABC                 ", 12);
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, &guid_in);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_GUID));
    Parameter param = get(param_list, PID_PARTICIPANT_GUID);
    GUID_t guid_out = param.guid();
    TEST_ASSERT(memcmp(guid_out.guidPrefix, "GUID-ABC    ", 12) == 0);
  }

  { // Should decode participant guid prefix properly
    GUID_t guid_in;
    memcpy(guid_in.guidPrefix, "GUID-ABC                 ", 12);
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, &guid_in);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(memcmp(participant_data.participantProxy.guidPrefix,
                       part_data_out.participantProxy.guidPrefix,
                       sizeof(GuidPrefix_t)) == 0);
  }

  { // Should encode participant expects inline qos properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, true);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_EXPECTS_INLINE_QOS));
    Parameter param = get(param_list, PID_EXPECTS_INLINE_QOS);
    TEST_ASSERT(param.expects_inline_qos() == true);
  }

  { // Should decode participant expects inline qos properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, true);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(part_data_out.participantProxy.expectsInlineQos == true);
    // Change to false
    participant_data.participantProxy.expectsInlineQos = false;
    param_list.length(0);
    plc.to_param_list(participant_data, param_list);
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(part_data_out.participantProxy.expectsInlineQos == false);
  }

  { // Should encode participant builtin endpoints properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 72393L);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    Parameter param = get(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS);
    TEST_ASSERT(param.participant_builtin_endpoints() == 72393L);
  }

  { // Should decode participant builtin endpoints properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 72393L);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(
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
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0, 
                                  locators, 2);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    Parameter param = get(param_list, PID_METATRAFFIC_UNICAST_LOCATOR, 0);
    locator_out = param.locator();
    TEST_ASSERT(locator_out.kind == locators[0].kind);
    TEST_ASSERT(locator_out.port == locators[0].port);
    TEST_ASSERT(memcmp(locator_out.address, locators[0].address, 16) == 0);

    param = get(param_list, PID_METATRAFFIC_UNICAST_LOCATOR, 1);
    locator_out = param.locator();
    TEST_ASSERT(locator_out.kind == locators[1].kind);
    TEST_ASSERT(locator_out.port == locators[1].port);
    TEST_ASSERT(memcmp(locator_out.address, locators[1].address, 16) == 0);
  }

  { // Should decode participant meta unicast locators properly
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0, 
                                  locators, 2);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficUnicastLocatorList[0];
      TEST_ASSERT(locators[0].kind == locator.kind);
      TEST_ASSERT(locators[0].port == locator.port);
      TEST_ASSERT(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficUnicastLocatorList[1];
      TEST_ASSERT(locators[1].kind == locator.kind);
      TEST_ASSERT(locators[1].port == locator.port);
      TEST_ASSERT(memcmp(locators[1].address, locator.address, 16) == 0);
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
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, locators, 2);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficMulticastLocatorList[0];
      TEST_ASSERT(locators[0].kind == locator.kind);
      TEST_ASSERT(locators[0].port == locator.port);
      TEST_ASSERT(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficMulticastLocatorList[1];
      TEST_ASSERT(locators[1].kind == locator.kind);
      TEST_ASSERT(locators[1].port == locator.port);
      TEST_ASSERT(memcmp(locators[1].address, locator.address, 16) == 0);
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
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, locators, 2);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficMulticastLocatorList[0];
      TEST_ASSERT(locators[0].kind == locator.kind);
      TEST_ASSERT(locators[0].port == locator.port);
      TEST_ASSERT(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.metatrafficMulticastLocatorList[1];
      TEST_ASSERT(locators[1].kind == locator.kind);
      TEST_ASSERT(locators[1].port == locator.port);
      TEST_ASSERT(memcmp(locators[1].address, locator.address, 16) == 0);
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
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, NULL, 0, locators, 2);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    Parameter param = get(param_list, PID_DEFAULT_UNICAST_LOCATOR, 0);
    locator_out = param.locator();
    TEST_ASSERT(locator_out.kind == locators[0].kind);
    TEST_ASSERT(locator_out.port == locators[0].port);
    TEST_ASSERT(memcmp(locator_out.address, locators[0].address, 16) == 0);

    param = get(param_list, PID_DEFAULT_UNICAST_LOCATOR, 1);
    locator_out = param.locator();
    TEST_ASSERT(locator_out.kind == locators[1].kind);
    TEST_ASSERT(locator_out.port == locators[1].port);
    TEST_ASSERT(memcmp(locator_out.address, locators[1].address, 16) == 0);
  }

  { // Should decode participant default unicast locators properly
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, NULL, 0, locators, 2);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    {
      Locator_t& locator = part_data_out.participantProxy.defaultUnicastLocatorList[0];
      TEST_ASSERT(locators[0].kind == locator.kind);
      TEST_ASSERT(locators[0].port == locator.port);
      TEST_ASSERT(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.defaultUnicastLocatorList[1];
      TEST_ASSERT(locators[1].kind == locator.kind);
      TEST_ASSERT(locators[1].port == locator.port);
      TEST_ASSERT(memcmp(locators[1].address, locator.address, 16) == 0);
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
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, NULL, 0, NULL, 0,
                                  locators, 2);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    Parameter param = get(param_list, PID_DEFAULT_MULTICAST_LOCATOR, 0);
    locator_out = param.locator();
    TEST_ASSERT(locator_out.kind == locators[0].kind);
    TEST_ASSERT(locator_out.port == locators[0].port);
    TEST_ASSERT(memcmp(locator_out.address, locators[0].address, 16) == 0);

    param = get(param_list, PID_DEFAULT_MULTICAST_LOCATOR, 1);
    locator_out = param.locator();
    TEST_ASSERT(locator_out.kind == locators[1].kind);
    TEST_ASSERT(locator_out.port == locators[1].port);
    TEST_ASSERT(memcmp(locator_out.address, locators[1].address, 16) == 0);
  }

  { // Should decode participant default multicast locators properly
    Locator_t locators[2];
    locators[0] = Factory::locator(LOCATOR_KIND_UDPv4,
                                   1234,
                                   127, 0, 0, 1);
    locators[1] = Factory::locator(LOCATOR_KIND_UDPv6,
                                   7734,
                                   107, 9, 8, 21);
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, NULL, 0, NULL, 0,
                                  locators, 2);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    {
      Locator_t& locator = part_data_out.participantProxy.defaultMulticastLocatorList[0];
      TEST_ASSERT(locators[0].kind == locator.kind);
      TEST_ASSERT(locators[0].port == locator.port);
      TEST_ASSERT(memcmp(locators[0].address, locator.address, 16) == 0);
    }
    {
      Locator_t& locator = part_data_out.participantProxy.defaultMulticastLocatorList[1];
      TEST_ASSERT(locators[1].kind == locator.kind);
      TEST_ASSERT(locators[1].port == locator.port);
      TEST_ASSERT(memcmp(locators[1].address, locator.address, 16) == 0);
    }
  }

  { // Should encode participant liveliness count properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                  7);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT));
    Parameter param = get(param_list, PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT);
    TEST_ASSERT(param.count().value == 7);
  }

  { // Should decode participant liveliness count properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                  6);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(
        part_data_out.participantProxy.manualLivelinessCount.value == 6);
  }

  { // Should encode participant lease duration properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, NULL, 0, NULL, 0, NULL, 0, 7,
                                  12, 300);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_LEASE_DURATION));
    Parameter param = get(param_list, PID_PARTICIPANT_LEASE_DURATION);
    TEST_ASSERT(param.duration().seconds == 12);
    TEST_ASSERT(param.duration().fraction == 300);
  }

  { // Should decode participant lease duration properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 0,
                                  NULL, 0, NULL, 0, NULL, 0, NULL, 0, 7,
                                  12, 300);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    SPDPdiscoveredParticipantData part_data_out;
    status = plc.from_param_list(param_list, part_data_out);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(part_data_out.leaseDuration.seconds == 
                participant_data.leaseDuration.seconds);
    TEST_ASSERT(part_data_out.leaseDuration.fraction == 
                participant_data.leaseDuration.fraction);
  }

  { // Should set participant user data qos to default if not present in param list
    SPDPdiscoveredParticipantData part_data_out;
    ParameterList empty_param_list;
    plc.from_param_list(empty_param_list, part_data_out);
    CORBA::ULong length = part_data_out.ddsParticipantData.user_data.value.length();
    TEST_ASSERT(length == 0);
  }

  { // Should set participant user data qos to default if not present in param list
    SPDPdiscoveredParticipantData part_data_out;
    part_data_out.ddsParticipantData.user_data.value.length(4);
    ParameterList empty_param_list;
    int status = plc.from_param_list(empty_param_list, part_data_out);
    TEST_ASSERT(!status);
    CORBA::ULong length = part_data_out.ddsParticipantData.user_data.value.length();
    TEST_ASSERT(length == 0);
  }

  // There is no default protocol version
  // There is no default guid prefix
  // There is no defaul vendor id

  { // Should set participant expects inline qos to default if not present in param list
    SPDPdiscoveredParticipantData part_data_out;
    part_data_out.participantProxy.expectsInlineQos = true;
    ParameterList empty_param_list;
    int status = plc.from_param_list(empty_param_list, part_data_out);
    TEST_ASSERT(!status);
    TEST_ASSERT(part_data_out.participantProxy.expectsInlineQos == false);
  }

  // There is no default available builtin endpoints 
  // There is no default metatraffic unicast locator list
  // There is no default metatraffic multicast locator list
  // There is no default default unicast locator list
  // There is no default default multicast locator list
  // There is no default manual liveliness count
  // The lease duration default is { 100, 0 }

  { // Should set participant lease duration to default if not present in param list
    SPDPdiscoveredParticipantData part_data_out;
    ParameterList empty_param_list;
    int status = plc.from_param_list(empty_param_list, part_data_out);
    TEST_ASSERT(!status);
    TEST_ASSERT(part_data_out.leaseDuration.seconds == 100);
    TEST_ASSERT(part_data_out.leaseDuration.fraction == 0);
  }

  { // Should ignore participant vendor-specific parameters
    SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(0x8001);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    int status = plc.from_param_list(vs_param_list, part_data_out);
    TEST_ASSERT(!status);
  }

  { // Should not fail on participant optional parameters
    SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(0x3FFF);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    int status = plc.from_param_list(vs_param_list, part_data_out);
    TEST_ASSERT(!status);
  }

  { // Should fail on participant required parameters
    SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(0x4001);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    int status = plc.from_param_list(vs_param_list, part_data_out);
    TEST_ASSERT(status != 0);
  }

  { // Should ignore participant SENTINEL
    SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(PID_SENTINEL);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    int status = plc.from_param_list(vs_param_list, part_data_out);
    TEST_ASSERT(!status);
  }

  { // Should ignore participant PAD
    SPDPdiscoveredParticipantData part_data_out;
    ParameterList vs_param_list;
    Parameter vs_param;
    vs_param._d(PID_PAD);
    vs_param_list.length(1);
    vs_param_list[0] = vs_param;
    int status = plc.from_param_list(vs_param_list, part_data_out);
    TEST_ASSERT(!status);
  }

  { // Should encode writer data
    DiscoveredWriterData writer_data; ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
  }

  { // Should encode writer topic name
    DiscoveredWriterData writer_data = 
        Factory::writer_data("TOPIC NAME TEST");
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_TOPIC_NAME));
    Parameter param = get(param_list, PID_TOPIC_NAME);
    TEST_ASSERT(!strncmp(param.string_data(), "TOPIC NAME TEST", 15));
  }

  { // Should decode writer topic name
    DiscoveredWriterData writer_data = 
        Factory::writer_data("TOPIC NAME TEST");
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(!strcmp(writer_data.ddsPublicationData.topic_name,
                        writer_data_out.ddsPublicationData.topic_name));
  }

  { // Should encode writer type name
    DiscoveredWriterData writer_data = Factory::writer_data("", "Messages");

    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_TYPE_NAME));
    Parameter param = get(param_list, PID_TYPE_NAME);
    TEST_ASSERT(!strncmp(param.string_data(), "Messages", 8));
  }

  { // Should decode writer type name
    DiscoveredWriterData writer_data = Factory::writer_data("", "Messages");
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(!strcmp(writer_data.ddsPublicationData.type_name,
                        writer_data_out.ddsPublicationData.type_name));
  }

  { // Should encode writer durability qos policy
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, 
        TRANSIENT_LOCAL_DURABILITY_QOS);

    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_DURABILITY));
    Parameter param = get(param_list, PID_DURABILITY);
    TEST_ASSERT(param.durability().kind == TRANSIENT_LOCAL_DURABILITY_QOS);
  }

  { // Should decode writer durability
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, 
        TRANSIENT_LOCAL_DURABILITY_QOS);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.durability.kind ==
                writer_data_out.ddsPublicationData.durability.kind);
  }

  { // Should encode writer durabiltiy service
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, 
        TRANSIENT_LOCAL_DURABILITY_QOS,
        4, 2000,
        KEEP_LAST_HISTORY_QOS, 172,
        389, 102, 20);

    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_DURABILITY_SERVICE));
    Parameter param = get(param_list, PID_DURABILITY_SERVICE);
    DurabilityServiceQosPolicy dsqp = param.durability_service();
    TEST_ASSERT(dsqp.service_cleanup_delay.sec = 4);
    TEST_ASSERT(dsqp.service_cleanup_delay.nanosec = 2000);

    TEST_ASSERT(dsqp.history_kind == KEEP_LAST_HISTORY_QOS);
    TEST_ASSERT(dsqp.history_depth == 172);
    TEST_ASSERT(dsqp.max_samples == 389);
    TEST_ASSERT(dsqp.max_instances == 102);
    TEST_ASSERT(dsqp.max_samples_per_instance == 20);
  }

  { // Should decode writer durability service
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, TRANSIENT_LOCAL_DURABILITY_QOS,
        4, 2000,
        KEEP_LAST_HISTORY_QOS, 172,
        389, 102, 20);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    DurabilityServiceQosPolicy& ds_in = 
        writer_data.ddsPublicationData.durability_service;
    DurabilityServiceQosPolicy& ds_out = 
        writer_data_out.ddsPublicationData.durability_service;
    TEST_ASSERT(ds_in.service_cleanup_delay.sec ==
                ds_out.service_cleanup_delay.sec);
    TEST_ASSERT(ds_in.service_cleanup_delay.nanosec ==
                ds_out.service_cleanup_delay.nanosec);
    TEST_ASSERT(ds_in.history_kind == ds_out.history_kind);
    TEST_ASSERT(ds_in.history_depth == ds_out.history_depth);
    TEST_ASSERT(ds_in.max_samples == ds_out.max_samples);
    TEST_ASSERT(ds_in.max_instances == ds_out.max_instances);
    TEST_ASSERT(ds_in.max_samples_per_instance == ds_out.max_samples_per_instance);
  }

  { // Should encode writer deadline
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1,
        127, 35000);

    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_DEADLINE));
    Parameter param = get(param_list, PID_DEADLINE);
    TEST_ASSERT(param.deadline().period.sec == 127);
    TEST_ASSERT(param.deadline().period.nanosec == 35000);
  }

  { // Should decode writer deadline
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1,
        127, 35000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.deadline.period.sec ==
                writer_data_out.ddsPublicationData.deadline.period.sec);
    TEST_ASSERT(writer_data.ddsPublicationData.deadline.period.nanosec ==
                writer_data_out.ddsPublicationData.deadline.period.nanosec);
  }

  { // Should enode writer latency budget
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0,
        5, 25000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_LATENCY_BUDGET));
    Parameter param = get(param_list, PID_LATENCY_BUDGET);
    TEST_ASSERT(param.deadline().period.sec == 5);
    TEST_ASSERT(param.deadline().period.nanosec == 25000);
  }

  { // Should decode writer latency budget
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0,
        5, 25000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.latency_budget.duration.sec ==
                writer_data_out.ddsPublicationData.latency_budget.duration.sec);
  }

  { // Should encode writer liveliness
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
        MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, 17, 15000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_LIVELINESS));
    Parameter param = get(param_list, PID_LIVELINESS);
    TEST_ASSERT(param.liveliness().kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
    TEST_ASSERT(param.liveliness().lease_duration.sec == 17);
    TEST_ASSERT(param.liveliness().lease_duration.nanosec == 15000);
  }

  { // Should decode writer liveliness
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
        MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, 17, 15000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.liveliness.kind ==
                writer_data_out.ddsPublicationData.liveliness.kind);
    TEST_ASSERT(writer_data.ddsPublicationData.liveliness.lease_duration.sec ==
                writer_data_out.ddsPublicationData.liveliness.lease_duration.sec);
    TEST_ASSERT(writer_data.ddsPublicationData.liveliness.lease_duration.nanosec ==
                writer_data_out.ddsPublicationData.liveliness.lease_duration.nanosec);
  }

  { // Should encode writer reliability
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        RELIABLE_RELIABILITY_QOS, 8, 100);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_RELIABILITY));
    Parameter param = get(param_list, PID_RELIABILITY);
    TEST_ASSERT(param.reliability().kind == RELIABLE_RELIABILITY_QOS);
    TEST_ASSERT(param.reliability().max_blocking_time.sec == 8);
    TEST_ASSERT(param.reliability().max_blocking_time.nanosec == 100);
  }

  { // Should decode writer reliability
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        RELIABLE_RELIABILITY_QOS, 8, 100);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.reliability.kind ==
                writer_data_out.ddsPublicationData.reliability.kind);
    TEST_ASSERT(writer_data.ddsPublicationData.reliability.max_blocking_time.sec ==
                writer_data_out.ddsPublicationData.reliability.max_blocking_time.sec);
    TEST_ASSERT(writer_data.ddsPublicationData.reliability.max_blocking_time.nanosec ==
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_LIFESPAN));
    Parameter param = get(param_list, PID_LIFESPAN);
    TEST_ASSERT(param.lifespan().duration.sec == 12);
    TEST_ASSERT(param.lifespan().duration.nanosec == 90000);
  }
  { // Should decode writer lifespan
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        BEST_EFFORT_RELIABILITY_QOS, 0, 0,
        12, 90000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.lifespan.duration.sec ==
                writer_data_out.ddsPublicationData.lifespan.duration.sec);
    TEST_ASSERT(writer_data.ddsPublicationData.lifespan.duration.nanosec ==
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_USER_DATA));
    Parameter param = get(param_list, PID_USER_DATA);
    TEST_ASSERT(param.user_data().value.length() == ud_len);
    for (CORBA::ULong i = 0; i < ud_len; ++i) {
      TEST_ASSERT(ud[i] == param.user_data().value[i]);
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.user_data.value ==
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_OWNERSHIP));
    Parameter param = get(param_list, PID_OWNERSHIP);
    TEST_ASSERT(param.ownership().kind == EXCLUSIVE_OWNERSHIP_QOS);
  }

  { // Should decode writer ownership
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
        EXCLUSIVE_OWNERSHIP_QOS);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.ownership.kind ==
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_OWNERSHIP_STRENGTH));
    Parameter param = get(param_list, PID_OWNERSHIP_STRENGTH);
    TEST_ASSERT(param.ownership_strength().value == 29);
  }
  { // Should decode writer ownership strength
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
        SHARED_OWNERSHIP_QOS,
        29);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.ownership_strength.value ==
                writer_data_out.ddsPublicationData.ownership_strength.value);
  }

  { // Should encode writer destination order
    DiscoveredWriterData writer_data = Factory::writer_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        KEEP_LAST_HISTORY_QOS, 1, 1, 1, 1, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        BEST_EFFORT_RELIABILITY_QOS, 0, 0, 0, 0, NULL, 0,
        SHARED_OWNERSHIP_QOS, 0,
        BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_DESTINATION_ORDER));
    Parameter param = get(param_list, PID_DESTINATION_ORDER);
    TEST_ASSERT(param.destination_order().kind == 
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.destination_order.kind ==
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_PRESENTATION));
    Parameter param = get(param_list, PID_PRESENTATION);
    TEST_ASSERT(param.presentation().access_scope == 
           GROUP_PRESENTATION_QOS);
    TEST_ASSERT(param.presentation().coherent_access == true);
    TEST_ASSERT(param.presentation().ordered_access == true);
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.presentation.access_scope ==
                writer_data_out.ddsPublicationData.presentation.access_scope);
    TEST_ASSERT(writer_data.ddsPublicationData.presentation.coherent_access ==
                writer_data_out.ddsPublicationData.presentation.coherent_access);
    TEST_ASSERT(writer_data.ddsPublicationData.presentation.ordered_access ==
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_PARTITION));
    Parameter param = get(param_list, PID_PARTITION);
    TEST_ASSERT(param.partition().name.length() == 1);
    TEST_ASSERT(strncmp(param.partition().name[0], part, 12) == 0);
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data_out.ddsPublicationData.partition.name.length() == 1);
    TEST_ASSERT(!strcmp(writer_data.ddsPublicationData.partition.name[0],
                        writer_data_out.ddsPublicationData.partition.name[0]));
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_TOPIC_DATA));
    Parameter param = get(param_list, PID_TOPIC_DATA);
    CORBA::ULong len = param.topic_data().value.length();
    TEST_ASSERT(len == 7);
    for (CORBA::ULong i = 0; i < len; ++i) {
      TEST_ASSERT(param.topic_data().value[i] = topic_data[i]);
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.topic_data.value ==
                writer_data_out.ddsPublicationData.topic_data.value);
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_GROUP_DATA));
    Parameter param = get(param_list, PID_GROUP_DATA);
    CORBA::ULong len = param.group_data().value.length();
    TEST_ASSERT(len == 7);
    for (CORBA::ULong i = 0; i < len; ++i) {
      TEST_ASSERT(param.group_data().value[i] = group_data[i]);
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data.ddsPublicationData.group_data.value ==
                writer_data_out.ddsPublicationData.group_data.value);
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_GROUP_GUID));
  }
  { // Should decode writer guid
    DiscoveredWriterData writer_data;
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(!memcmp(&writer_data.writerProxy.remoteWriterGuid,
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_UNICAST_LOCATOR));
  }
  { // Should decode writer unicast locators
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data_out.writerProxy.unicastLocatorList.length() == 2);
    TEST_ASSERT(!memcmp(&writer_data_out.writerProxy.unicastLocatorList[0],
                        &locators[0],
                        sizeof(Locator_t)));
    TEST_ASSERT(!memcmp(&writer_data_out.writerProxy.unicastLocatorList[1],
                        &locators[1],
                        sizeof(Locator_t)));
  }

  { // Should encode writer multicast locators
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_MULTICAST_LOCATOR));
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
    TEST_ASSERT(!plc.to_param_list(writer_data, param_list));
    DiscoveredWriterData writer_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, writer_data_out));
    TEST_ASSERT(writer_data_out.writerProxy.multicastLocatorList.length() == 2);
    TEST_ASSERT(!memcmp(&writer_data_out.writerProxy.multicastLocatorList[0],
                        &locators[0],
                        sizeof(Locator_t)));
    TEST_ASSERT(!memcmp(&writer_data_out.writerProxy.multicastLocatorList[1],
                        &locators[1],
                        sizeof(Locator_t)));
  }

  { // Should encode reader data
    DiscoveredReaderData reader_data; 
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
  }

  { // Should encode reader topic name
    DiscoveredReaderData reader_data = 
        Factory::reader_data("TOPIC NAME TEST");
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_TOPIC_NAME));
    Parameter param = get(param_list, PID_TOPIC_NAME);
    TEST_ASSERT(!strncmp(param.string_data(), "TOPIC NAME TEST", 15));
  }

  { // Should decode reader topic name
    DiscoveredReaderData reader_data = 
        Factory::reader_data("TOPIC NAME TEST");
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(!strcmp(reader_data.ddsSubscriptionData.topic_name,
                        reader_data_out.ddsSubscriptionData.topic_name));
  }

  { // Should encode reader type name
    DiscoveredReaderData reader_data = Factory::reader_data("", "Messages");

    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_TYPE_NAME));
    Parameter param = get(param_list, PID_TYPE_NAME);
    TEST_ASSERT(!strncmp(param.string_data(), "Messages", 8));
  }

  { // Should decode reader type name
    DiscoveredReaderData reader_data = Factory::reader_data("", "Messages");
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(!strcmp(reader_data.ddsSubscriptionData.type_name,
                        reader_data_out.ddsSubscriptionData.type_name));
  }

  { // Should encode reader durability qos policy
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, 
        TRANSIENT_LOCAL_DURABILITY_QOS);

    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_DURABILITY));
    Parameter param = get(param_list, PID_DURABILITY);
    TEST_ASSERT(param.durability().kind == TRANSIENT_LOCAL_DURABILITY_QOS);
  }

  { // Should decode reader durability
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, 
        TRANSIENT_LOCAL_DURABILITY_QOS);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.durability.kind ==
                reader_data_out.ddsSubscriptionData.durability.kind);
  }

  { // Should encode reader deadline
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS,
        127, 35000);

    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_DEADLINE));
    Parameter param = get(param_list, PID_DEADLINE);
    TEST_ASSERT(param.deadline().period.sec == 127);
    TEST_ASSERT(param.deadline().period.nanosec == 35000);
  }

  { // Should decode reader deadline
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS,
        127, 35000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.deadline.period.sec ==
                reader_data_out.ddsSubscriptionData.deadline.period.sec);
    TEST_ASSERT(reader_data.ddsSubscriptionData.deadline.period.nanosec ==
                reader_data_out.ddsSubscriptionData.deadline.period.nanosec);
  }

  { // Should enode reader latency budget
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        5, 25000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_LATENCY_BUDGET));
    Parameter param = get(param_list, PID_LATENCY_BUDGET);
    TEST_ASSERT(param.deadline().period.sec == 5);
    TEST_ASSERT(param.deadline().period.nanosec == 25000);
  }

  { // Should decode reader latency budget
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0,
        5, 25000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.latency_budget.duration.sec ==
                reader_data_out.ddsSubscriptionData.latency_budget.duration.sec);
  }

  { // Should encode reader liveliness
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
        MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, 17, 15000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_LIVELINESS));
    Parameter param = get(param_list, PID_LIVELINESS);
    TEST_ASSERT(param.liveliness().kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
    TEST_ASSERT(param.liveliness().lease_duration.sec == 17);
    TEST_ASSERT(param.liveliness().lease_duration.nanosec == 15000);
  }

  { // Should decode reader liveliness
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
        MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, 17, 15000);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.liveliness.kind ==
                reader_data_out.ddsSubscriptionData.liveliness.kind);
    TEST_ASSERT(reader_data.ddsSubscriptionData.liveliness.lease_duration.sec ==
                reader_data_out.ddsSubscriptionData.liveliness.lease_duration.sec);
    TEST_ASSERT(reader_data.ddsSubscriptionData.liveliness.lease_duration.nanosec ==
                reader_data_out.ddsSubscriptionData.liveliness.lease_duration.nanosec);
  }

  { // Should encode reader reliability
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        RELIABLE_RELIABILITY_QOS, 8, 100);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_RELIABILITY));
    Parameter param = get(param_list, PID_RELIABILITY);
    TEST_ASSERT(param.reliability().kind == RELIABLE_RELIABILITY_QOS);
    TEST_ASSERT(param.reliability().max_blocking_time.sec == 8);
    TEST_ASSERT(param.reliability().max_blocking_time.nanosec == 100);
  }

  { // Should decode reader reliability
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        RELIABLE_RELIABILITY_QOS, 8, 100);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.reliability.kind ==
                reader_data_out.ddsSubscriptionData.reliability.kind);
    TEST_ASSERT(reader_data.ddsSubscriptionData.reliability.max_blocking_time.sec ==
                reader_data_out.ddsSubscriptionData.reliability.max_blocking_time.sec);
    TEST_ASSERT(reader_data.ddsSubscriptionData.reliability.max_blocking_time.nanosec ==
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_USER_DATA));
    Parameter param = get(param_list, PID_USER_DATA);
    TEST_ASSERT(param.user_data().value.length() == ud_len);
    for (CORBA::ULong i = 0; i < ud_len; ++i) {
      TEST_ASSERT(ud[i] == param.user_data().value[i]);
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.user_data.value ==
                reader_data_out.ddsSubscriptionData.user_data.value);
  }

  { // Should encode reader ownership
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
        EXCLUSIVE_OWNERSHIP_QOS);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_OWNERSHIP));
    Parameter param = get(param_list, PID_OWNERSHIP);
    TEST_ASSERT(param.ownership().kind == EXCLUSIVE_OWNERSHIP_QOS);
  }

  { // Should decode reader ownership
    DiscoveredReaderData reader_data = Factory::reader_data(
        NULL, NULL, VOLATILE_DURABILITY_QOS, 0, 0, 0, 0,
        AUTOMATIC_LIVELINESS_QOS, 0, 0,
        BEST_EFFORT_RELIABILITY_QOS, 0, 0, NULL, 0,
        EXCLUSIVE_OWNERSHIP_QOS);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.ownership.kind ==
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_DESTINATION_ORDER));
    Parameter param = get(param_list, PID_DESTINATION_ORDER);
    TEST_ASSERT(param.destination_order().kind == 
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.destination_order.kind ==
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_PRESENTATION));
    Parameter param = get(param_list, PID_PRESENTATION);
    TEST_ASSERT(param.presentation().access_scope == 
           GROUP_PRESENTATION_QOS);
    TEST_ASSERT(param.presentation().coherent_access == true);
    TEST_ASSERT(param.presentation().ordered_access == true);
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.presentation.access_scope ==
                reader_data_out.ddsSubscriptionData.presentation.access_scope);
    TEST_ASSERT(reader_data.ddsSubscriptionData.presentation.coherent_access ==
                reader_data_out.ddsSubscriptionData.presentation.coherent_access);
    TEST_ASSERT(reader_data.ddsSubscriptionData.presentation.ordered_access ==
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_PARTITION));
    Parameter param = get(param_list, PID_PARTITION);
    TEST_ASSERT(param.partition().name.length() == 1);
    TEST_ASSERT(strncmp(param.partition().name[0], part, 12) == 0);
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data_out.ddsSubscriptionData.partition.name.length() == 1);
    TEST_ASSERT(!strcmp(reader_data.ddsSubscriptionData.partition.name[0],
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_TOPIC_DATA));
    Parameter param = get(param_list, PID_TOPIC_DATA);
    CORBA::ULong len = param.topic_data().value.length();
    TEST_ASSERT(len == 7);
    for (CORBA::ULong i = 0; i < len; ++i) {
      TEST_ASSERT(param.topic_data().value[i] = topic_data[i]);
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.topic_data.value ==
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_GROUP_DATA));
    Parameter param = get(param_list, PID_GROUP_DATA);
    CORBA::ULong len = param.group_data().value.length();
    TEST_ASSERT(len == 7);
    for (CORBA::ULong i = 0; i < len; ++i) {
      TEST_ASSERT(param.group_data().value[i] = group_data[i]);
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data.ddsSubscriptionData.group_data.value ==
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_GROUP_GUID));
  }
  { // Should decode reader guid
    DiscoveredReaderData reader_data;
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(!memcmp(&reader_data.readerProxy.remoteReaderGuid,
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_UNICAST_LOCATOR));
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
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data_out.readerProxy.unicastLocatorList.length() == 2);
    TEST_ASSERT(!memcmp(&reader_data.readerProxy.unicastLocatorList[0],
                        &locators[0],
                        sizeof(Locator_t)));
    TEST_ASSERT(!memcmp(&reader_data.readerProxy.unicastLocatorList[1],
                        &locators[1],
                        sizeof(Locator_t)));
  }

  { // Should encode reader multicast locators
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
        NULL, 0, locators, 2);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    TEST_ASSERT(is_present(param_list, PID_MULTICAST_LOCATOR));
  }
  { // Should decode reader multicast locators
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
        NULL, 0, locators, 2);
    ParameterList param_list;
    TEST_ASSERT(!plc.to_param_list(reader_data, param_list));
    DiscoveredReaderData reader_data_out;
    TEST_ASSERT(!plc.from_param_list(param_list, reader_data_out));
    TEST_ASSERT(reader_data_out.readerProxy.multicastLocatorList.length() == 2);
    TEST_ASSERT(!memcmp(&reader_data.readerProxy.multicastLocatorList[0],
                        &locators[0],
                        sizeof(Locator_t)));
    TEST_ASSERT(!memcmp(&reader_data.readerProxy.multicastLocatorList[1],
                        &locators[1],
                        sizeof(Locator_t)));
  }
  return 0;
}
