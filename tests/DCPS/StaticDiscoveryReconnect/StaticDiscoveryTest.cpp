#include "TestMsgTypeSupportImpl.h"
#include "DataReaderListenerImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/WaitSet.h"

#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include "dds/DCPS/GuidConverter.h"

#include "dds/DCPS/StaticIncludes.h"

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_stdlib.h"

const int DOMAIN_ID = 100;

int do_reader(DDS::DomainParticipant_var participant, DDS::Topic_var topic, bool toggle)
{
  // Create Subscriber
  DDS::Subscriber_var subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!subscriber) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: main() -")
                      ACE_TEXT(" create_subscriber failed!\n")), -1);
  }

  // Create DataReader
  DDS::DataReaderQos qos;
  subscriber->get_default_datareader_qos(qos);
  qos.user_data.value.length(3);
  qos.user_data.value[0] = 0;
  qos.user_data.value[1] = 0;
  qos.user_data.value[2] = 0;
  qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  ACE_DEBUG((LM_DEBUG, "Creating reader\n"));
  DDS::DataReader_var reader =
    subscriber->create_datareader(topic,
                                  qos,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!reader) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: main() -")
                      ACE_TEXT(" create_datareader failed!\n")), -1);
  }

  if (toggle) {
    ACE_OS::sleep(11);
    // Go away.
    ACE_DEBUG((LM_DEBUG, "Deleting reader\n"));
    subscriber->delete_datareader(reader);
    ACE_OS::sleep(11);
    // Come back.
    ACE_DEBUG((LM_DEBUG, "Creating reader\n"));
    reader = subscriber->create_datareader(topic,
                                           qos,
                                           0,
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  }

  return 0;
}

int do_writer(DDS::DomainParticipant_var participant, DDS::Topic_var topic, bool toggle)
{
  // Create Publisher
  DDS::Publisher_var publisher =
    participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!publisher) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: main() -")
                      ACE_TEXT(" create_publisher failed!\n")),
                     -1);
  }

  DDS::DataWriterQos qos;
  publisher->get_default_datawriter_qos(qos);
  qos.user_data.value.length(3);
  qos.user_data.value[0] = 0;
  qos.user_data.value[1] = 0;
  qos.user_data.value[2] = 1;
  qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  qos.reliability.max_blocking_time.sec = DDS::DURATION_INFINITE_SEC;

  // Create DataWriter
  DDS::DataWriter_var writer =
    publisher->create_datawriter(topic,
                                 qos,
                                 0,
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!writer) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: main() -")
                      ACE_TEXT(" create_datawriter failed!\n")),
                     -1);
  }

  for (size_t i = 0; i != 30; ++i) {
    DDS::PublicationMatchedStatus matches;
    if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" get_publication_matched_status failed!\n")),
                       -1);
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %d readers\n", matches.current_count));
    ACE_OS::sleep(1);
  }

  return 0;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    enum {
      READER,
      WRITER
    } mode;
    bool toggle = false;

    {
      // New scope.
      ACE_Arg_Shifter shifter (argc, argv);
      while (shifter.is_anything_left ()) {
        const ACE_TCHAR* x = shifter.get_current();
        if (strcmp(x, "-reader") == 0) {
          mode = READER;
        }
        if (strcmp(x, "-writer") == 0) {
          mode = WRITER;
        }
        if (strcmp(x, "-toggle") == 0) {
          toggle = true;
        }
        shifter.consume_arg ();
      }
    }

    // Create DomainParticipant
    DDS::DomainParticipantQos dp_qos;
    dpf->get_default_participant_qos(dp_qos);
    dp_qos.user_data.value.length(6);
    dp_qos.user_data.value[0] = 0;
    dp_qos.user_data.value[1] = 0;
    dp_qos.user_data.value[2] = 0;
    dp_qos.user_data.value[3] = 0;
    dp_qos.user_data.value[4] = 0;
    dp_qos.user_data.value[5] = (mode == READER) ? 0 : 1;

    DDS::DomainParticipant_var participant =
      dpf->create_participant(DOMAIN_ID,
                              dp_qos,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_participant failed!\n")),
                       -1);
    }

    // Register TypeSupport
    TestMsgTypeSupport_var ts =
      new TestMsgTypeSupportImpl;

    if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" register_type failed!\n")),
                       -1);
    }

    // Create Topic
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("TheTopic",
                                type_name,
                                TOPIC_QOS_DEFAULT,
                                0,
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_topic failed!\n")),
                       -1);
    }

    switch (mode) {
    case READER:
      return do_reader(participant, topic, toggle);
      break;
    case WRITER:
      return do_writer(participant, topic, toggle);
      break;
    }

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
