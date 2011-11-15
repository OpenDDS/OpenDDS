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
#include <iostream>

using namespace OpenDDS::RTPS;

namespace {
  ParameterListConverter plc;
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
      size_t user_data_len = 0,
      char major_protocol_version = 0,
      char minor_protocol_version = 0,
      char* vendor_id = NULL,
      GUID_t* guid = NULL,
      bool expects_inline_qos = false,
      unsigned long builtin_endpoints = 0,
      Locator_t* mtu_locs = NULL,
      size_t num_mtu_locs = 0,
      Locator_t* mtm_locs = NULL,
      size_t num_mtm_locs = 0,
      Locator_t* du_locs = NULL,
      size_t num_du_locs = 0,
      Locator_t* dm_locs = NULL,
      size_t num_dm_locs = 0,
      long liveliness_count = 0,
      long lease_dur_seconds = 0,
      unsigned long lease_dur_fraction = 0
    )
    {
      SPDPdiscoveredParticipantData result;
      if (user_data_len && user_data) {
        result.ddsParticipantData.user_data.value.length(user_data_len);
        for (size_t i = 0; i < user_data_len; ++i) {
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
        for (size_t i = 0; i < num_mtu_locs; ++i) {
          result.participantProxy.metatrafficUnicastLocatorList[i] = 
              mtu_locs[i];
        }
      }

      if (num_mtm_locs && mtm_locs) {
        result.participantProxy.metatrafficMulticastLocatorList.length(num_mtm_locs);
        for (size_t i = 0; i < num_mtm_locs; ++i) {
          result.participantProxy.metatrafficMulticastLocatorList[i] = 
              mtm_locs[i];
        }
      }

      if (num_du_locs && du_locs) {
        result.participantProxy.defaultUnicastLocatorList.length(num_du_locs);
        for (size_t i = 0; i < num_du_locs; ++i) {
          result.participantProxy.defaultUnicastLocatorList[i] = du_locs[i];
        }
      }

      if (num_dm_locs && dm_locs) {
        result.participantProxy.defaultMulticastLocatorList.length(num_dm_locs);
        for (size_t i = 0; i < num_dm_locs; ++i) {
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
  }
}

bool is_present(const ParameterList& param_list, const ParameterId_t pid) {
  size_t length = param_list.length();
  for (size_t i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      return true;
    }
  }
  return false;
}

bool is_missing(const ParameterList& param_list, const ParameterId_t pid) {
  size_t length = param_list.length();
  for (size_t i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      return false;
    }
  }
  return true;
}

Parameter get(const ParameterList& param_list, 
              const ParameterId_t pid,
              const size_t instance_num = 0) {

  const size_t length = param_list.length();
  size_t count = 0;
  for (size_t i = 0; i < length; ++i) {
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

  { // Should encode user data properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant("hello user", 10);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_USER_DATA));
    Parameter param = get(param_list, PID_USER_DATA);
    DDS::UserDataQosPolicy ud_qos = param.user_data();
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

  { // Should decode user data properly
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
  }

  { // Should encode protocol version properly
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

  { // Should decode protocol version properly
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

  { // Should encode vendor id properly
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

  { // Should decode vendor id properly
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

  { // Should encode guid prefix properly
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

  { // Should decode guid prefix properly
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

  { // Should encode expects inline qos properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, true);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_EXPECTS_INLINE_QOS));
    Parameter param = get(param_list, PID_EXPECTS_INLINE_QOS);
    TEST_ASSERT(param.expects_inline_qos() == true);
  }

  { // Should decode expects inline qos properly
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

  { // Should encode builtin endpoints properly
    SPDPdiscoveredParticipantData participant_data = 
        Factory::spdp_participant(NULL, 0, 0, 0, NULL, NULL, false, 72393L);
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    Parameter param = get(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS);
    TEST_ASSERT(param.participant_builtin_endpoints() == 72393L);
  }

  { // Should decode builtin endpoints properly
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

  { // Should encode meta unicast locators properly
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

  { // Should decode meta unicast locators properly
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

  { // Should encode meta multicast locators properly
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

  { // Should decode meta multicast locators properly
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

  { // Should encode default unicast locators properly
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

  { // Should decode default unicast locators properly
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

  { // Should encode default multicast locators properly
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

  { // Should decode default multicast locators properly
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

  { // Should encode liveliness count properly
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

  { // Should decode liveliness count properly
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

  { // Should encode lease duration properly
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

  { // Should decode lease duration properly
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
/*
  { // Should decode param list to participant data properly
    int a = 0;
  }

  { // Should fill in default QOS parameters when not specified
    int a = 0;
  }

  { // Should not endode default QOS parameters
    int a = 0;
  }

  { // Should ignore vendor-specific paramaters from other vendors
    int a = 0;
  }
  */
/*
  // testing numerical sequence
  TEST_CHECK(SequenceNumber(SN_MIN) < SequenceNumber(SN_MIN+1));
  TEST_CHECK(!(SequenceNumber(SN_MIN+1) < SequenceNumber(SN_MIN)));
  TEST_CHECK(SequenceNumber(SN_SEAM) < SequenceNumber(SN_SEAM+1));
  TEST_CHECK(!(SequenceNumber(SN_SEAM+1) < SequenceNumber(SN_SEAM)));
  TEST_CHECK(SequenceNumber(SN_MAX-1) < SequenceNumber(SN_MAX));
  TEST_CHECK(!(SequenceNumber(SN_MAX) < SequenceNumber(SN_MAX-1)));

  // testing wide ranges
  TEST_CHECK(SequenceNumber() < SequenceNumber(SN_RANGE/2));
  TEST_CHECK(!(SequenceNumber(SN_RANGE/2) < SequenceNumber()));
  TEST_CHECK(SequenceNumber(SN_RANGE/2+1) < SequenceNumber());
  TEST_CHECK(!(SequenceNumber() < SequenceNumber(SN_RANGE/2+1)));
  TEST_CHECK(SequenceNumber(SN_RANGE/2+1) < SequenceNumber(SN_MAX));
  TEST_CHECK(!(SequenceNumber(SN_MAX) < SequenceNumber(SN_RANGE/2+1)));
  TEST_CHECK(SequenceNumber(SN_MAX) < SequenceNumber(SN_RANGE/2));
  TEST_CHECK(!(SequenceNumber(SN_RANGE/2) < SequenceNumber(SN_MAX)));
  TEST_CHECK(SequenceNumber(SN_MAX) < SequenceNumber());
  TEST_CHECK(!(SequenceNumber() < SequenceNumber(SN_MAX)));

  // testing values and increment operator
  {
    SequenceNumber num(SN_MIN);
    TEST_CHECK(num.getValue() == SN_MIN);
    TEST_CHECK((++num).getValue() == SN_MIN+1);
  }

  {
    SequenceNumber num(SN_SEAM);
    TEST_CHECK(num.getValue() == SN_SEAM);
    TEST_CHECK((++num).getValue() == SN_SEAM+1);
    TEST_CHECK((++num).getValue() == SN_SEAM+2);
  }

  {
    SequenceNumber num(SN_MAX);
    TEST_CHECK(num.getValue() == SN_MAX);
    TEST_CHECK((++num).getValue() == SN_MIN);
    // test post-incrementer
    TEST_CHECK((num++).getValue() == SN_MIN);
    TEST_CHECK(num.getValue() == SN_MIN+1);
  }

  // Test SEQUENCENUMBER_UNKNOWN
  {
    SequenceNumber num = SequenceNumber::SEQUENCENUMBER_UNKNOWN();
    TEST_CHECK(num.getValue() == ACE_INT64(0xffffffff) << 32);
    SequenceNumber min;
    TEST_CHECK(num != min);
    TEST_CHECK(num == SequenceNumber::SEQUENCENUMBER_UNKNOWN());
  }

  // Test previous() member function
  {
    SequenceNumber num(SN_MIN);
    TEST_CHECK(num.previous() == SN_MAX);
  }

  {
    SequenceNumber num(SN_SEAM+1);
    TEST_CHECK(num.previous() == SN_SEAM);
  }

  {
    SequenceNumber num(99);
    TEST_CHECK(num.previous() == 98);
  }

  {
    SequenceNumber num(SN_MAX);
    TEST_CHECK(num.previous() == SN_MAX-1);
  }

*/
  return 0;
}
