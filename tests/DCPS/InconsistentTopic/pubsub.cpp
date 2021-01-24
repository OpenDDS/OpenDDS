#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_unistd.h"

using namespace Messenger;
using namespace std;

int ACE_TMAIN(int argc, ACE_TCHAR *argv[]) {
  try
    {
      const char* topic = "Movie Discussion List";
      bool test_error = false;

      DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
      DDS::DomainParticipant_var p1 = dpf->create_participant(111,
                                                              PARTICIPANT_QOS_DEFAULT,
                                                              DDS::DomainParticipantListener::_nil(),
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(p1.in())) {
        cerr << "ERROR: create_participant failed." << endl;
        return 1;
      }
      OpenDDS::DCPS::TransportRegistry::instance()->bind_config("config_1", p1);

      DDS::Subscriber_var bit_subscriber1 = p1->get_builtin_subscriber() ;
      DDS::DataReader_var bit_drg1 = bit_subscriber1->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
      DDS::SubscriptionBuiltinTopicDataDataReader_var bit_dr1 = DDS::SubscriptionBuiltinTopicDataDataReader::_narrow(bit_drg1);

      Message1TypeSupport_var mts1 = new Message1TypeSupportImpl();

      if (DDS::RETCODE_OK != mts1->register_type(p1.in(), "")) {
        cerr << "ERROR: register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name1 = mts1->get_type_name();
      DDS::Topic_var topic1 = p1->create_topic(topic,
                          type_name1.in(),
                          TOPIC_QOS_DEFAULT,
                          DDS::TopicListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(topic1.in())) {
        cerr << "ERROR: create_topic 1 failed." << endl;
        exit(1);
      }

      DDS::DomainParticipant_var p2 = dpf->create_participant(111,
                                                              PARTICIPANT_QOS_DEFAULT,
                                                              DDS::DomainParticipantListener::_nil(),
                                                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(p2.in())) {
        cerr << "ERROR: create_participant failed." << endl;
        return 1;
      }

      OpenDDS::DCPS::TransportRegistry::instance()->bind_config("config_2", p2);

      DDS::Subscriber_var bit_subscriber2 = p2->get_builtin_subscriber() ;
      DDS::DataReader_var bit_drg2 = bit_subscriber2->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
      DDS::PublicationBuiltinTopicDataDataReader_var bit_dr2 = DDS::PublicationBuiltinTopicDataDataReader::_narrow(bit_drg2);

      Message2TypeSupport_var mts2 = new Message2TypeSupportImpl();

      if (DDS::RETCODE_OK != mts2->register_type(p2.in(), "")) {
        cerr << "ERROR: register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name2 = mts2->get_type_name();
      DDS::Topic_var topic2 =
        p2->create_topic(topic,
                         type_name2.in(),
                         TOPIC_QOS_DEFAULT,
                         DDS::TopicListener::_nil(),
                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(topic2.in())) {
        cerr << "ERROR: create_topic 2 failed." << endl;
        exit(1);
      }

      // At this point the participants and topics exist but have not
      // been discovered since there are no readers or writers.
      DDS::Publisher_var pub1 =
        p1->create_publisher(PUBLISHER_QOS_DEFAULT,
                              DDS::PublisherListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(pub1.in())) {
        cerr << "ERROR: Failed to create_subscriber." << endl;
        exit(1);
      }

      DDS::DataWriter_var dw1 = pub1->create_datawriter(topic1.in(),
                                                        DATAWRITER_QOS_DEFAULT,
                                                        DDS::DataWriterListener::_nil(),
                                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(dw1.in())) {
        cerr << "ERROR: create_datawriter failed." << endl;
        exit(1);
      }

      // At this point dw1 should not have a publication matched
      DDS::PublicationMatchedStatus pub_status;
      DDS::ReturnCode_t retcode = dw1->get_publication_matched_status(pub_status);
      if (retcode != DDS::RETCODE_OK) {
        cerr << "ERROR: not able to retrieve publication matched status from dw1." << endl;
        exit(1);
      }
      if (pub_status.total_count != 0) {
        cerr << "ERROR: dw1 total_count should be 0 but is " << pub_status.total_count << endl;
        test_error = true;
      }
      if (pub_status.total_count_change != 0) {
        cerr << "ERROR: dw1 total_count_change should be 0 but is " << pub_status.total_count_change << endl;
        test_error = true;
      }

      DDS::Subscriber_var sub2 =
        p2->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                              DDS::SubscriberListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(sub2.in())) {
        cerr << "ERROR: Failed to create_subscriber." << endl;
        exit(1);
      }

      DDS::DataReader_var dr2 = sub2->create_datareader(topic2.in(),
                                                        DATAREADER_QOS_DEFAULT,
                                                        DDS::DataReaderListener::_nil(),
                                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil(dr2.in())) {
        cerr << "ERROR: create_datareader failed." << endl;
        exit(1);
      }

      // Wait for p2 to see p1's datawriter.
      for (;;) {
        DDS::PublicationBuiltinTopicDataSeq pub_data;
        DDS::SampleInfoSeq infos;
        DDS::ReturnCode_t ret = bit_dr2->read(pub_data, infos, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
        if (ret == DDS::RETCODE_OK) {
           break;
        } else {
          cout << "Waiting for participant 2 to discover topic from participant 1 " << std::endl;
          ACE_OS::sleep (1);
        }
      }

      // Wait for p1 to see p2's datareader.
      for (;;) {
        DDS::SubscriptionBuiltinTopicDataSeq sub_data;
        DDS::SampleInfoSeq infos;
        const DDS::ReturnCode_t ret = bit_dr1->read(sub_data, infos, 1, DDS::NOT_READ_SAMPLE_STATE, DDS::NEW_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
        if (ret == DDS::RETCODE_OK) {
          break;
        } else {
          cout << "Waiting for participant 1 to discover topic from participant 2" << std::endl;
          ACE_OS::sleep (1);
        }
      }

      // At this point, p2 should have an inconsistent topic and so will p1
      DDS::InconsistentTopicStatus status;

      retcode = topic1->get_inconsistent_topic_status(status);
      if (retcode != DDS::RETCODE_OK) {
        cerr << "ERROR: not able to retrieve topic status." << endl;
        exit(1);
      }
      if (status.total_count != 1) {
        cerr << "ERROR: participant 1 total_count should be 1 but is " << status.total_count << endl;
        test_error = true;
      }
      if (status.total_count_change != 1) {
        cerr << "ERROR: participant 1 total_count_change should be 1 but is " << status.total_count_change << endl;
        test_error = true;
      }

      retcode = topic2->get_inconsistent_topic_status(status);
      if (retcode != DDS::RETCODE_OK) {
        cerr << "ERROR: not able to retrieve topic status." << endl;
        exit(1);
      }
      if (status.total_count != 1) {
        cerr << "ERROR: participant 2 total_count should be 1 but is " << status.total_count << endl;
        test_error = true;
      }
      if (status.total_count_change != 1) {
        cerr << "ERROR: participant 2 total_count_change should be 1 but is " << status.total_count_change << endl;
        test_error = true;
      }

      // At this moment there should be no subscription matched
      DDS::SubscriptionMatchedStatus sub_status;
      retcode = dr2->get_subscription_matched_status(sub_status);
      if (retcode != DDS::RETCODE_OK) {
        cerr << "ERROR: not able to retrieve subscription matched status from dr2." << endl;
        exit(1);
      }
      if (sub_status.total_count != 0) {
        cerr << "ERROR: dr2 total_count should be 0 but is " << sub_status.total_count << endl;
        test_error = true;
      }
      if (sub_status.total_count_change != 0) {
        cerr << "ERROR: dr2 total_count_change should be 0 but is " << sub_status.total_count_change << endl;
        test_error = true;
      }

      // At this moment there should be no publication matched
      retcode = dw1->get_publication_matched_status(pub_status);
      if (retcode != DDS::RETCODE_OK) {
        cerr << "ERROR: not able to retrieve publication matched status from dw1." << endl;
        exit(1);
      }
      if (pub_status.total_count != 0) {
        cerr << "ERROR: dw1 total_count should be 0 but is " << pub_status.total_count << endl;
        test_error = true;
      }
      if (pub_status.total_count_change != 0) {
        cerr << "ERROR: dw1 total_count_change should be 0 but is " << pub_status.total_count_change << endl;
        test_error = true;
      }

      p1->delete_contained_entities();
      dpf->delete_participant(p1);

      p2->delete_contained_entities();
      dpf->delete_participant(p2);

      TheServiceParticipant->shutdown();

      if (test_error) {
        exit(1);
      }
  }
  catch (CORBA::Exception& e)
    {
       cerr << "ERROR: Exception caught in main.cpp:" << endl
         << e << endl;
      exit(1);
    }

  return 0;
}
