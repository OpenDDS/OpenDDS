#include "common.h"

#include <dds/DCPS/Service_Participant.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DCPS/security/framework/Properties.h>
#endif

#include <sstream>
#include <iomanip>

unsigned expected_data_field_length(unsigned offset, int writer_id, int sample_id)
{
  // Writer ID is 1 or 2
  // Sample ID is 0 to 9
  // Lengths will vary from 10k to 155k
  return offset + unsigned((sample_id * 1.5) + writer_id) * 10 * 1024;
}

unsigned char expected_data_field_element(int writer_id, int sample_id, int j)
{
  return static_cast<unsigned char>(j % 256) + writer_id + sample_id * 3;
}

#ifdef OPENDDS_SECURITY
void append(
  DDS::PropertySeq& props, const char* name, const char* value, bool propagate)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

void set_security_qos(DDS::DomainParticipantQos& participant_qos, unsigned secid)
{
  if (TheServiceParticipant->get_security()) {
    const std::string base_path = "file:../../security/";
    std::stringstream ss;
    ss << "test_participant_" << std::setw(2) << std::setfill('0') << secid;
    const std::string base_name = ss.str();

    DDS::PropertySeq& props = participant_qos.property.value;

    append(props, DDS::Security::Properties::AuthIdentityCA,
      (base_path + "certs/identity/identity_ca_cert.pem").c_str());
    append(props, DDS::Security::Properties::AuthIdentityCertificate,
      (base_path + "certs/identity/" + base_name + "_cert.pem").c_str());
    append(props, DDS::Security::Properties::AuthPrivateKey,
      (base_path + "certs/identity/" + base_name + "_private_key.pem").c_str());
    append(props, DDS::Security::Properties::AccessPermissionsCA,
      (base_path + "certs/permissions/permissions_ca_cert.pem").c_str());
    append(props, DDS::Security::Properties::AccessGovernance,
      "file:governance_signed.p7s");
    append(props, DDS::Security::Properties::AccessPermissions,
      (base_path + "permissions/permissions_" + base_name + "_signed.p7s").c_str());
  }
}
#endif
