#include "Writer.h"
#include "DataReaderListenerImpl.h"

#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/StatusMatching.h"
#include "tests/Utils/ExceptionStreams.h"
#include "tests/Utils/DistributedConditionSet.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/unique_ptr.h>

#include <dds/DCPS/transport/framework/TransportRegistry.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include <ace/OS_NS_unistd.h>

#include <memory>

using namespace Messenger;
using namespace std;

const char TOPIC[] = "Movie Discussion List";
const char PARTITION_A[] = "partition A";
const char PARTITION_B[] = "partition B";

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {
  DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<InMemoryDistributedConditionSet>();

  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  // Publisher setup.
  DDS::DomainParticipant_var pub_participant = dpf->create_participant(311,
                                                                       PARTICIPANT_QOS_DEFAULT,
                                                                       DDS::DomainParticipantListener::_nil(),
                                                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->get_config("pub_part");
  TheTransportRegistry->bind_config(cfg, pub_participant);

  MessageTypeSupportImpl::_var_type pub_type_support = new MessageTypeSupportImpl();
  pub_type_support->register_type(pub_participant.in(), "");

  CORBA::String_var pub_type_name = pub_type_support->get_type_name();

  DDS::TopicQos pub_topic_qos;
  pub_participant->get_default_topic_qos(pub_topic_qos);
  DDS::Topic_var pub_topic = pub_participant->create_topic(TOPIC,
                                                           pub_type_name.in(),
                                                           pub_topic_qos,
                                                           DDS::TopicListener::_nil(),
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::PublisherQos pub_qos;
  pub_participant->get_default_publisher_qos(pub_qos);

  pub_qos.partition.name.length(1);
  pub_qos.partition.name[0] = PARTITION_A;

  DDS::Publisher_var pub = pub_participant->create_publisher(pub_qos, DDS::PublisherListener::_nil(),
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);


  DDS::DataWriter_var dw = pub->create_datawriter (pub_topic.in(),
                                                   DATAWRITER_QOS_DEFAULT,
                                                   DDS::DataWriterListener::_nil(),
                                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Subscriber setup.
  DDS::DomainParticipant_var sub_participant = dpf->create_participant(311,
                                                                       PARTICIPANT_QOS_DEFAULT,
                                                                       DDS::DomainParticipantListener::_nil(),
                                                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  cfg = TheTransportRegistry->get_config("sub_part");
  TheTransportRegistry->bind_config(cfg, sub_participant);

  MessageTypeSupportImpl::_var_type sub_type_support = new MessageTypeSupportImpl();

  sub_type_support->register_type(sub_participant.in(), "");

  CORBA::String_var sub_type_name = sub_type_support->get_type_name();

  DDS::TopicQos sub_topic_qos;
  sub_participant->get_default_topic_qos(sub_topic_qos);
  DDS::Topic_var sub_topic = sub_participant->create_topic(TOPIC,
                                                           sub_type_name.in(),
                                                           sub_topic_qos,
                                                           DDS::TopicListener::_nil(),
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::SubscriberQos sub_qos_1;
  sub_participant->get_default_subscriber_qos(sub_qos_1);

  sub_qos_1.partition.name.length(1);
  sub_qos_1.partition.name[0] = PARTITION_A;

  DDS::Subscriber_var sub_1 = sub_participant->create_subscriber(sub_qos_1, DDS::SubscriberListener::_nil(),
                                                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::SubscriberQos sub_qos_2;
  sub_participant->get_default_subscriber_qos(sub_qos_2);

  sub_qos_2.partition.name.length(1);
  sub_qos_2.partition.name[0] = PARTITION_B;

  DDS::Subscriber_var sub_2 = sub_participant->create_subscriber(sub_qos_2, DDS::SubscriberListener::_nil(),
                                                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataReaderQos dr_qos;
  sub_1->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReaderListener_var listener_1(new DataReaderListenerImpl(dcs, SUBSCRIBER_ACTOR_1));

  DDS::DataReader_var dr_1 = sub_1->create_datareader(sub_topic.in(),
                                                      dr_qos,
                                                      listener_1.in(),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataReaderListener_var listener_2(new DataReaderListenerImpl(dcs, SUBSCRIBER_ACTOR_2));

  DDS::DataReader_var dr_2 = sub_2->create_datareader(sub_topic.in(),
                                                      dr_qos,
                                                      listener_2.in(),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DataReaderListenerImpl* listener_impl_1 = dynamic_cast<DataReaderListenerImpl*>(listener_1.in());

  DataReaderListenerImpl* listener_impl_2 = dynamic_cast<DataReaderListenerImpl*>(listener_2.in());

  // Write to PARTITION_A, one reader.
  ::DDS::InstanceHandle_t reader_handle_1 = -1;
  {
    listener_impl_1->reset(EXPECTED_READS_1, 10);
    Writer writer(dw, " 1", 10);

    cout << "Pub waiting for match on " << PARTITION_A << std::endl;
    if (Utils::wait_match(dw, 1, Utils::GTE)) {
      cerr << "Error waiting for match on " << PARTITION_A << std::endl;
      return 1;
    }
    ::DDS::InstanceHandleSeq handles;
    dw->get_matched_subscriptions(handles);
    if (handles.length() == 1) {
      reader_handle_1 = handles[0];
    } else {
      cerr << "ERROR: could not get matched subscription handles" << endl;
      return 1;
    }

    writer.start();
    dcs->wait_for("driver", SUBSCRIBER_ACTOR_1, EXPECTED_READS_1);
    writer.end();
  }

  // Write to PARTITION_B, one reader.
  pub_qos.partition.name[0] = PARTITION_B;
  if (pub->set_qos(pub_qos)!= ::DDS::RETCODE_OK) {
    cerr << "ERROR: failed to set_qos" << endl;
    return 1;
  }

  int ret = -1;
  DDS::PublicationMatchedStatus ms = { 0, 0, 0, 0, 0 };
  while (ret != 0) {
    if (dw->get_publication_matched_status(ms) == DDS::RETCODE_OK) {
      if (ms.total_count == 2 && ms.current_count == 1) {
        ret = 0;
      } else { // wait for a change
        ACE_OS::sleep(1);
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - get_publication_matched_status failed!\n")));
      break;
    }
  }

  ::DDS::InstanceHandle_t reader_handle_2 = -1;
  {
    listener_impl_2->reset(EXPECTED_READS_1, 10);
    Writer writer(dw, " 2", 10);

    cout << "Pub waiting for match on " << PARTITION_B << std::endl;
    if (wait_match(dr_1, 0, Utils::EQ)) {
      cerr << "Error waiting for set_qos to propagate" << std::endl;
      return 1;
    }
    if (wait_match(dw, 1, Utils::GTE)) {
      cerr << "Error waiting for match on " << PARTITION_B << std::endl;
      return 1;
    }
    ::DDS::InstanceHandleSeq handles;
    dw->get_matched_subscriptions(handles);
    if (handles.length() == 1) {
      reader_handle_2 = handles[0];
    } else {
      cerr << "ERROR: could not get matched subscription handles" << endl;
      return 1;
    }

    writer.start();
    dcs->wait_for("driver", SUBSCRIBER_ACTOR_2, EXPECTED_READS_1);
    writer.end();
  }

#ifndef DDS_HAS_MINIMUM_BIT
  if (reader_handle_1 == reader_handle_2) {
    cerr << "ERROR: readers have the same handle" << endl;
    return 1;
  }
#endif

  // Write to PARTITION_B, two readers.
  sub_qos_1.partition.name[0] = PARTITION_B;
  if (sub_1->set_qos(sub_qos_1) != ::DDS::RETCODE_OK) {
    cerr << "ERROR: failed to set_qos" << endl;
    return 1;
  }

  std::set<DDS::InstanceHandle_t> handle_set;
  {
    listener_impl_1->reset(EXPECTED_READS_2, 20);
    listener_impl_2->reset(EXPECTED_READS_2, 20);
    Writer writer(dw, " 3", 10);

    cout << "Pub waiting for additional match on " << PARTITION_B << std::endl;
    if (wait_match(dw, 2, Utils::GTE)) {
      cerr << "Error waiting for additional match on " << PARTITION_B << std::endl;
      return 1;
    }
    ::DDS::InstanceHandleSeq handles;
    dw->get_matched_subscriptions(handles);
    for (CORBA::ULong idx = 0; idx != handles.length(); ++idx) {
      handle_set.insert(handles[idx]);
    }

    writer.start();
    dcs->wait_for("driver", SUBSCRIBER_ACTOR_1, EXPECTED_READS_2);
    dcs->wait_for("driver", SUBSCRIBER_ACTOR_2, EXPECTED_READS_2);
    // NOTE: At this point, the readers have received all of the data.
    writer.end();
  }

  if (handle_set.count(reader_handle_1) != 1) {
    cerr << "ERROR: " << SUBSCRIBER_ACTOR_1 << " did not join " << PARTITION_B << endl;
    return 1;
  }

  if (handle_set.count(reader_handle_2) != 1) {
    cerr << "ERROR: " << SUBSCRIBER_ACTOR_2 << " did not join " << PARTITION_B << endl;
    return 1;
  }

  if (handle_set.size() != 2) {
    cerr << "ERROR: too many readers in " << PARTITION_B << endl;
    return 1;
  }

  pub_participant->delete_contained_entities();
  sub_participant->delete_contained_entities();
  dpf->delete_participant(pub_participant.in());
  dpf->delete_participant(sub_participant.in());

  TheServiceParticipant->shutdown();

  return 0;
}
