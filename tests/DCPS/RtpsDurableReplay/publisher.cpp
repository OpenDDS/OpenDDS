// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================

#include "Common.h"
#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/DistributedConditionSet.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace Messenger;
using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

const TimeDuration LEASE_DURATION(5, 0); // 5 seconds
const TimeDuration RESEND_PERIOD(1, 0); // 1 second

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  // Create discovery.
  RcHandle<RtpsDiscovery> discovery = make_rch<RtpsDiscovery>("RtpsDiscovery");
  discovery->config()->lease_duration(LEASE_DURATION);
  discovery->config()->resend_period(RESEND_PERIOD);
  TheServiceParticipant->add_discovery(discovery);
  TheServiceParticipant->set_repo_domain(TEST_DOMAIN, discovery->key());

  // Create transport.
  RcHandle<TransportConfig> config = TheTransportRegistry->create_config("RtpsTransport");
  RcHandle<TransportInst> transport_inst = TheTransportRegistry->create_inst("RtpsTransport", "rtps_udp");
  config->instances_.push_back(transport_inst);

  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  DDS::DomainParticipant_var participant =
    dpf->create_participant(TEST_DOMAIN,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  OpenDDS::DCPS::TransportRegistry::instance()->bind_config("RtpsTransport", participant);

  OpenDDS::DCPS::DomainParticipantImpl* impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in());
  RcHandle<TransportInst> discovery_inst = discovery->sedp_transport_inst(TEST_DOMAIN, impl->get_id());

  MessageTypeSupport_var type_support = new MessageTypeSupportImpl();

  type_support->register_type(participant, "");

  CORBA::String_var type_name = type_support->get_type_name();

  DDS::Topic_var topic =
    participant->create_topic("Movie Discussion List",
                              type_name,
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::Publisher_var publisher =
    participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                       DDS::PublisherListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  ::DDS::DataWriterQos writer_qos;
  publisher->get_default_datawriter_qos(writer_qos);
  writer_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

  DDS::DataWriter_var writer_var =
    publisher->create_datawriter(topic,
                                 writer_qos,
                                 DDS::DataWriterListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Messenger::MessageDataWriter_var writer = MessageDataWriter::_narrow(writer_var);

  Messenger::Message message;
  message.from = "from";
  message.subject = "subject";
  message.text = "text";

  // Write the instances.
  for (size_t idx = 0; idx != INSTANCE_COUNT; ++idx) {
    message.subject_id = static_cast<int>(idx);
    writer->write(message, DDS::HANDLE_NIL);
  }

  // Wait for the subscriber.
  dcs->wait_for(PUBLISHER_ACTOR, SUBSCRIBER_ACTOR, ALL_INSTANCES_RECEIVED_BEFORE);

  // Kill communication.
  discovery_inst->drop_messages_b(1.0);
  discovery_inst->drop_messages(true);
  transport_inst->drop_messages_b(1.0);
  transport_inst->drop_messages(true);

  // Write to engage the backoff.
  for (size_t idx = 0; idx != INSTANCE_COUNT; ++idx) {
    message.subject_id = static_cast<int>(idx);
    writer->write(message, DDS::HANDLE_NIL);
  }

  dcs->wait_for(PUBLISHER_ACTOR, SUBSCRIBER_ACTOR, RESUME);

  discovery_inst->drop_messages(false);
  transport_inst->drop_messages(false);

  dcs->wait_for(PUBLISHER_ACTOR, SUBSCRIBER_ACTOR, ALL_INSTANCES_RECEIVED_AFTER);

  participant->delete_contained_entities();
  dpf->delete_participant(participant);

  TheServiceParticipant->shutdown();

  return 0;
}
