
#include "gtest/gtest.h"
#include "dds/DCPS/RTPS/ParameterListConverter.h"
#include <sstream>
#include <memory>
#include <cstring>

#include "../Utils.h"

using namespace OpenDDS::RTPS;
using namespace OpenDDS::Security;
using namespace DDS::Security;
using namespace std;

namespace {

  Token token(string classid = "Test-Class-Id",
                   size_t proplen = 1,
                   size_t bproplen = 1,
                   bool propagate = true)
  {
    Token t;

    t.class_id = classid.c_str();

    t.properties.length(proplen);
    for (size_t i = 0; i < proplen; ++i) {
        stringstream name, value;
        name << "Property " << i;
        value << "PropertyValue " << i;

        t.properties[i].name = name.str().c_str();
        t.properties[i].value = value.str().c_str();
        t.properties[i].propagate = propagate;
    }

    t.binary_properties.length(bproplen);
    for (size_t i = 0; i < bproplen; ++i) {
        stringstream name, value;
        name << "BinaryProperty " << i;
        value << "BinaryPropertyValue " << i;

        t.binary_properties[i].name = name.str().c_str();

        size_t vlen = value.str().length();
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

TEST(ToParamListTest, From_SPDPdiscoveredParticipantData_SecurityWrapper_IdentityStatusToken)
{
  SPDPdiscoveredParticipantData_SecurityWrapper w1, w2;

  w1.data = Factory::spdp_participant("Test-Test-Test", 10);
  w1.identity_status_token = token();

  ParameterList p;
  ASSERT_EQ(0, ParameterListConverter::to_param_list(w1, p));
  ASSERT_EQ(0, ParameterListConverter::from_param_list(p, w2));

  ASSERT_EQ(0, strcmp("Property 0", w2.identity_status_token.properties[0].name));
  ASSERT_EQ(0, strcmp("PropertyValue 0", w2.identity_status_token.properties[0].value));
  ASSERT_EQ(0, strcmp("BinaryProperty 0", w2.identity_status_token.binary_properties[0].name));
  ASSERT_EQ(0, memcmp("BinaryPropertyValue 0",  w2.identity_status_token.binary_properties[0].value.get_buffer(),  strlen("BinaryPropertyValue 0")));

}


