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

using namespace OpenDDS::RTPS;

namespace {
  ParameterListConverter plc;
  namespace Factory {
    SPDPdiscoveredParticipantData spdp_participant(
      const void* user_data = NULL,
      size_t user_data_len = 0,
      char major_protocol_version = 0,
      char minor_protocol_version = 0,
      char* vendor_id = NULL,
      GUID_t* guid = NULL
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

      return result;
    }
  }
}

bool is_present(const ParameterList& param_list, const ParameterId_t pid) {
  int length = param_list.length();
  for (int i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      return true;
    }
  }
  return false;
}
bool is_missing(const ParameterList& param_list, const ParameterId_t pid) {
  int length = param_list.length();
  for (int i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      return false;
    }
  }
  return true;
}

Parameter get(const ParameterList& param_list, const ParameterId_t pid) {
  int length = param_list.length();
  for (int i = 0; i < length; ++i) {
    if (pid == param_list[i]._d()) {
      return param_list[i];
    }
  }
  TEST_ASSERT(false); // Not found
}
int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  { // Should encode participant data with no locators to param list properly
    SPDPdiscoveredParticipantData participant_data;
    ParameterList param_list;
    int status = plc.to_param_list(participant_data, param_list);
    TEST_ASSERT(status == 0);
    TEST_ASSERT(is_present(param_list, PID_USER_DATA));
    // Built-in topic id missing
    TEST_ASSERT(is_present(param_list, PID_PROTOCOL_VERSION));
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_GUID));
    TEST_ASSERT(is_present(param_list, PID_VENDORID));
    TEST_ASSERT(is_present(param_list, PID_EXPECTS_INLINE_QOS));
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_BUILTIN_ENDPOINTS));
    TEST_ASSERT(is_missing(param_list, PID_METATRAFFIC_UNICAST_LOCATOR));
    TEST_ASSERT(is_missing(param_list, PID_METATRAFFIC_MULTICAST_LOCATOR));
    TEST_ASSERT(is_missing(param_list, PID_DEFAULT_UNICAST_LOCATOR));
    TEST_ASSERT(is_missing(param_list, PID_DEFAULT_MULTICAST_LOCATOR));
    TEST_ASSERT(is_present(param_list, PID_PARTICIPANT_LEASE_DURATION));
  }

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
