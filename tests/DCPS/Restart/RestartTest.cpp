#define ACE_DOESNT_DEFINE_MAIN

#include <MessengerTypeSupportImpl.h>

#include <dds/DCPS/debug.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpLoader.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DCPS/security/BuiltInPlugins.h>
#  include <dds/DCPS/security/framework/Properties.h>
#  ifdef ACE_AS_STATIC_LIBS
#    include <dds/DCPS/security/BuiltInPluginLoader.h>
#  endif
#endif

#include <ace/Init_ACE.h>

#include <cstdlib>
#include <sstream>
#include <string>
#include <stdexcept>

#ifdef OPENDDS_SECURITY
const char auth_ca_file[] = "file:../../security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file[] = "file:../../security/certs/permissions/permissions_ca_cert.pem";
const char id_cert_file[] = "file:../../security/certs/identity/test_participant_02_cert.pem";
const char id_key_file[] = "file:../../security/certs/identity/test_participant_02_private_key.pem";
const char governance_file[] = "file:../Messenger/governance_signed.p7s";
const char permissions_file[] = "file:../Messenger/permissions_2_signed.p7s";
#endif

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

struct Application {
  Application()
  {
    ACE::init();
    ++instances_;
    id_ = ++total_instances_;
    std::cout << "Application " << id_ << " Starting\n";
    OpenDDS::DCPS::RtpsUdpLoader::load();
    OpenDDS::RTPS::RtpsDiscovery::StaticInitializer initialize_rtps;
    TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_RTPS);

    std::stringstream ss;
    ss << "Config" << instances_;
    transport_config_name_ = ss.str();
    ss.str("");
    ss << "Inst" << instances_;
    const std::string transport_instance_name = ss.str();

    OpenDDS::DCPS::TransportInst_rch ti = TheTransportRegistry->create_inst(transport_instance_name, "rtps_udp");
    OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->create_config(transport_config_name_);
    cfg->instances_.push_back(ti);

#if defined OPENDDS_SECURITY && defined ACE_AS_STATIC_LIBS
    OpenDDS::Security::BuiltInPluginLoader().init(0, 0);
#endif

    DDS::DomainParticipantFactory_var dpf = TheServiceParticipant->get_domain_participant_factory();
    if (!dpf) {
      throw std::runtime_error("Failed to get participant factory");
    } else {
      std::cout << "Application " << id_ << " retrieved domain participant factory\n";
    }
    DDS::DomainParticipantFactoryQos factory_qos;
    dpf->get_qos(factory_qos);
    factory_qos.entity_factory.autoenable_created_entities = false;
    dpf->set_qos(factory_qos);

    DDS::DomainParticipantQos participant_qos;
    dpf->get_default_participant_qos(participant_qos);
#ifdef OPENDDS_SECURITY
    DDS::PropertySeq& props = participant_qos.property.value;
    if (TheServiceParticipant->get_security()) {
      using namespace DDS::Security::Properties;
      append(props, AuthIdentityCA, auth_ca_file);
      append(props, AuthIdentityCertificate, id_cert_file);
      append(props, AuthPrivateKey, id_key_file);
      append(props, AccessPermissionsCA, perm_ca_file);
      append(props, AccessGovernance, governance_file);
      append(props, AccessPermissions, permissions_file);
    }
#endif
    participant_ = dpf->create_participant(4, participant_qos, 0, 0);
    if (!participant_) {
      std::cout << "Application " << id_ << " failed to create participant\n";
      throw std::runtime_error("Failed to create participant");
    } else {
      std::cout << "Application " << id_ << " created domain participant\n";
    }
    TheTransportRegistry->bind_config(cfg, participant_);
    if (participant_->enable()) {
      throw std::runtime_error("Failed to enable participant");
    }

    DDS::TypeSupport_var ts = new Messenger::MessageTypeSupportImpl;
    CORBA::String_var tn = ts->get_type_name();
    ts->register_type(participant_, tn);
    DDS::Topic_var topic = participant_->create_topic("Topic", tn, TOPIC_QOS_DEFAULT, 0, 0);

    DDS::Publisher_var pub = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);
    DDS::DataWriter_var dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0, 0);
    DDS::Subscriber_var sub = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, 0);
    DDS::DataReaderQos reader_qos;
    sub->get_default_datareader_qos(reader_qos);
    reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    DDS::DataReader_var dr = sub->create_datareader(topic, reader_qos, 0, 0);

    std::cout << "Application " << id_ << " Started\n";
  }

  ~Application()
  {
    std::cout << "Application " << id_ << " Ending\n";
    {
      DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
      DDS::ReturnCode_t retval = participant_->delete_contained_entities();
      if (retval != DDS::RETCODE_OK) {
        std::cout << "Failed to deleted contained entities\n";
      } else {
        std::cout << "Application " << id_ << " did delete contained_entities\n";
      }
      retval = dpf->delete_participant(participant_);
      if (retval != DDS::RETCODE_OK) {
        std::cout << "Failed to deleted domain participant\n";
      } else {
        std::cout << "Application " << id_ << " did delete domain participant\n";
      }
      participant_ = 0;
    }

    OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->get_config(transport_config_name_);
    TheTransportRegistry->remove_config(cfg);
    TheTransportRegistry->remove_inst(cfg->instances_[0]);

    if (--instances_ == 0) {
      const DDS::ReturnCode_t retval = TheServiceParticipant->shutdown();
      if (retval != DDS::RETCODE_OK) {
        std::cout << "Failed to shutdown the service participant\n";
      } else {
        std::cout << "Application " << id_ << " shutdown service participant\n";
      }
    }
    ACE::fini();
    std::cout << "Application " << id_ << " Ended\n";
  }

  static int instances_;
  static int total_instances_;

  int id_;
  std::string transport_config_name_;
  DDS::DomainParticipant_var participant_;
};

int Application::instances_ = 0;
int Application::total_instances_ = 0;

int main(int argc, char* argv[])
{
  const std::string usage =
    std::string("usage: ") + argv[0] + " [--help|-h] [--secure]\n";
  for (int i = 1; i < argc; ++i) {
    const std::string argument(argv[i]);
    if (argument == "--help" || argument == "-h") {
      std::cout << usage;
      return EXIT_SUCCESS;
#ifdef OPENDDS_SECURITY
    } else if (argument == "--secure") {
      TheServiceParticipant->set_security(true);
#endif
    }
  }

#ifdef OPENDDS_SECURITY
  OpenDDS::DCPS::security_debug.new_entity_error = true;
#endif

  int status = EXIT_SUCCESS;

  try {
    {
      Application a1;
      {
        Application a2;
      }
      {
        Application a3;
      }
    }
    Application a4;
  } catch (const std::exception& error) {
    ACE_ERROR((LM_ERROR, "Caught Standard Exception: %C\n", error.what()));
    status = EXIT_FAILURE;
  } catch (const CORBA::Exception& error) {
    error._tao_print_exception("Caught CORBA Exception:");
    status = EXIT_FAILURE;
  }

  return status;
}
