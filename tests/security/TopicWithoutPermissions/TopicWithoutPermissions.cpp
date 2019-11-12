#include "StockQuoterTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include <dds/DCPS/security/framework/Properties.h>
#include <dds/DCPS/security/framework/SecurityRegistry.h>

int QUOTER_DOMAIN_ID = 0;

const char* QUOTER_QUOTE_TYPE = "Quote Type";
const char* QUOTER_QUOTE_TOPIC = "Stock Quotes";

const std::string file = "file:";

const std::string identity_ca_file = file + "../certs/identity/identity_ca_cert.pem";
const std::string permissions_ca_file = file + "../certs/permissions/permissions_ca_cert.pem";
const std::string identity_certificate_file = file + "../certs/identity/test_participant_01_cert.pem";
const std::string identity_key_file = file + "../certs/identity/identity_ca_private_key.pem";
const std::string governance_file = file + "../attributes/governance/governance_PU_PA_ED_EL_EOR_signed.p7s";
const std::string permissions_file = file + "../attributes/permissions/permissions_test_participant_01_read_signed.p7s";

///
// append
///
void append(DDS::PropertySeq& props, const char* name, const std::string& value, bool propagate = false)
{
  const DDS::Property_t prop = { name, value.c_str(), propagate };
  const unsigned int len = props.length();
  props.length(len + 1);
  try {
  props[len] = prop;
  }
  catch (const CORBA::BAD_PARAM&) {
  ACE_ERROR((LM_ERROR, "Exception caught when appending parameter\n"));
  }
}

///
// cleanup
///
void cleanup(DDS::DomainParticipant_var participant, DDS::DomainParticipantFactory_var dpf) {
  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();
}

///
// main
///
int main(int argc, char* argv[]) {
  // Initialize, and create a DomainParticipant
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  if (!TheServiceParticipant->get_security()) {
    // Security documents provided but security is not enabled;
    ACE_OS::exit(1);
  }

  DDS::DomainParticipantQos participant_qos;
  dpf->get_default_participant_qos(participant_qos);

  DDS::PropertySeq& properties = participant_qos.property.value;

  append(properties, DDS::Security::Properties::AuthIdentityCA, identity_ca_file);
  append(properties, DDS::Security::Properties::AccessPermissionsCA, permissions_ca_file);
  append(properties, DDS::Security::Properties::AuthIdentityCertificate, identity_certificate_file);
  append(properties, DDS::Security::Properties::AuthPrivateKey, identity_key_file);
  append(properties, DDS::Security::Properties::AccessGovernance, governance_file);
  append(properties, DDS::Security::Properties::AccessPermissions, permissions_file);

  // Create the DomainParticipant
  DDS::DomainParticipant_var participant = dpf->create_participant(QUOTER_DOMAIN_ID,
                                                                   participant_qos,
                                                                   NULL,
                                                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (0 == participant) {
    ACE_OS::exit(1);
  }

  // Create and register the Quote Type support object
  StockQuoter::QuoteTypeSupport_var quote_ts = new StockQuoter::QuoteTypeSupportImpl;
  if (DDS::RETCODE_OK != quote_ts->register_type(participant,
                                                 QUOTER_QUOTE_TYPE)) {
    cleanup(participant, dpf);
    ACE_OS::exit(1);
  }

  // Create a topic for the Stock Quotes, which should not be allowed
  DDS::Topic_var quote_topic = participant->create_topic(QUOTER_QUOTE_TOPIC,
                                                         QUOTER_QUOTE_TYPE,
                                                         TOPIC_QOS_DEFAULT,
                                                         NULL,
                                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (0 == quote_topic) {
    cleanup(participant, dpf);
    return 0;
  }

  cleanup(participant, dpf);

  return 1;
}
