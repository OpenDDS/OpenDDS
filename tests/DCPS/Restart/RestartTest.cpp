#define ACE_DOESNT_DEFINE_MAIN

#include "MessengerTypeSupportImpl.h"

#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpLoader.h"
#include "dds/DCPS/RTPS/RtpsDiscovery.h"

#include "ace/Init_ACE.h"

#include <cstdlib>
#include <sstream>
#include <string>

struct Application {
  Application()
  {
    ACE::init();
    ++instances_;
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

    DDS::DomainParticipantFactory_var dpf = TheServiceParticipant->get_domain_participant_factory();
    DDS::DomainParticipantFactoryQos factory_qos;
    dpf->get_qos(factory_qos);
    factory_qos.entity_factory.autoenable_created_entities = false;
    dpf->set_qos(factory_qos);
    participant_ = dpf->create_participant(0, PARTICIPANT_QOS_DEFAULT, 0, 0);
    TheTransportRegistry->bind_config(cfg, participant_);
    participant_->enable();

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
  }

  ~Application()
  {
    {
      DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
      participant_->delete_contained_entities();
      dpf->delete_participant(participant_);
      participant_ = 0;
    }

    OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->get_config(transport_config_name_);
    TheTransportRegistry->remove_config(cfg);
    TheTransportRegistry->remove_inst(cfg->instances_[0]);

    if (--instances_ == 0) {
      TheServiceParticipant->shutdown();
    }
    ACE::fini();
  }

  static int instances_;

  std::string transport_config_name_;
  DDS::DomainParticipant_var participant_;
};

int Application::instances_ = 0;

int main()
{
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

  return EXIT_SUCCESS;
}
