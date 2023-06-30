// -*- C++ -*-

// ============================================================================
/**
 *  @file   publisher.cpp
 *
 */
// ============================================================================

#include "Common.h"
#include "DataReaderListener.h"
#include "Writer.h"

#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/unique_ptr.h>

#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>

#include <memory>

using namespace Messenger;

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[]) {
  int retval = 0;

  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<InMemoryDistributedConditionSet>();

  MessageTypeSupport_var type_support = new MessageTypeSupportImpl();
  CORBA::String_var type_name = type_support->get_type_name();

  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var pub_participant =
    dpf->create_participant(111,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (DDS::RETCODE_OK != type_support->register_type(pub_participant, "")) {
    cerr << "register_type failed." << endl;
    exit(1);
  }

  DDS::Topic_var pub_topic =
    pub_participant->create_topic("Movie Discussion List",
                                  type_name,
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::Publisher_var pub_publisher =
    pub_participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Configure DataWriter QoS policies.
  const DDS::Duration_t max_block_time = {2, 0};
  const DDS::Duration_t deadline_time = {3, 0};
  DDS::DataWriterQos dw_qos;
  pub_publisher->get_default_datawriter_qos(dw_qos);
  dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
  dw_qos.reliability.max_blocking_time = max_block_time;
  dw_qos.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
  dw_qos.durability_service.history_kind = ::DDS::KEEP_LAST_HISTORY_QOS;
  dw_qos.durability_service.history_depth = 10;
  dw_qos.durability_service.max_samples = 10;
  dw_qos.durability_service.max_samples_per_instance = 10;
  dw_qos.durability_service.max_instances = 2;
  dw_qos.deadline.period = deadline_time;

  // Create a DataWriter.
  DDS::DataWriter_var pub_datawriter =
    pub_publisher->create_datawriter(pub_topic,
                                     dw_qos,
                                     0,
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);


  DDS::DomainParticipant_var sub_participant =
    dpf->create_participant(111,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (DDS::RETCODE_OK != type_support->register_type(sub_participant, "")) {
    cerr << "Failed to register the MessageTypeTypeSupport." << endl;
    exit(1);
  }

  DDS::Topic_var sub_topic =
    sub_participant->create_topic("Movie Discussion List",
                                  type_name,
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Create the subscriber
  DDS::Subscriber_var sub_subscriber =
    sub_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                       DDS::SubscriberListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // activate the listener
  const int expected = 10;
  DDS::DataReaderListener_var listener(new DataReaderListenerImpl(distributed_condition_set, expected));
  DataReaderListenerImpl* const listener_impl =
    dynamic_cast<DataReaderListenerImpl*>(listener.in());

  if (!listener_impl) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l main()")
                      ACE_TEXT(" ERROR: listener_servant is nil (dynamic_cast failed)!\n")), -1);
  }

  // Create the Datareaders
  DDS::DataReaderQos dr_qos;
  sub_subscriber->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
  dr_qos.history.kind  = ::DDS::KEEP_LAST_HISTORY_QOS;
  dr_qos.history.depth = 4;
  dr_qos.deadline.period = deadline_time;

  DDS::DataReader_var sub_datareader = sub_subscriber->create_datareader(sub_topic,
                                                                         dr_qos,
                                                                         listener,
                                                                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Utils::wait_match(sub_datareader, 1);

  // Write samples using multiple threads.
  OpenDDS::DCPS::unique_ptr<Writer> pub_writer(new Writer(pub_datawriter));
  if (!pub_writer->start()) {
    // Error logging performed in above method call.
    exit(1);
  }

  distributed_condition_set->wait_for(SUBSCRIBER, SUBSCRIBER, ON_DATA_AVAILABLE);
  distributed_condition_set->wait_for(SUBSCRIBER, SUBSCRIBER, ON_SAMPLE_LOST);

  cerr << "Got " << listener_impl->num_data_available() << " on_data_available callbacks" << endl;
  cerr << "Got " << listener_impl->num_samples_lost() << " on_sample_lost callbacks" << endl;
  cerr << "Got " << listener_impl->num_samples_rejected() << " on_sample_rejected callbacks" << endl;
  cerr << "Got " << listener_impl->num_budget_exceeded() << " on_budget_exceeded callbacks" << endl;

  if (listener_impl->num_budget_exceeded() > 0) {
    cerr << "ERROR: Got at least one on_budget_exceeded callback" << endl;
    ++retval;
  }

  if (!pub_writer->end()) {
    // Error logging performed in above method call.
    exit(1);
  }

  // Explicitly destroy the DataWriter.
  if (pub_publisher->delete_datawriter(pub_datawriter) == ::DDS::RETCODE_PRECONDITION_NOT_MET) {
    cerr << "Unable to delete DataWriter" << endl;
    exit(1);
  }

  ACE_DEBUG ((LM_INFO,
              ACE_TEXT ("(%P|%t) Deleted DataWriter.\n")));

  pub_participant->delete_contained_entities();
  sub_participant->delete_contained_entities();
  dpf->delete_participant(pub_participant);
  dpf->delete_participant(sub_participant);
  TheServiceParticipant->shutdown();

  return 0;
}
