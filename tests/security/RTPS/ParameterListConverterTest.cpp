
#include "gtest/gtest.h"
#include "dds/DCPS/RTPS/ParameterListConverter.h"
#include <sstream>
#include <memory>
#include <cstring>

using namespace OpenDDS::RTPS;
using namespace OpenDDS::Security;
using namespace DDS::Security;
using namespace std;

namespace {

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

}

TEST(ToParamListTest, From_SPDPdiscoveredParticipantData_IdentityStatusToken)
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

