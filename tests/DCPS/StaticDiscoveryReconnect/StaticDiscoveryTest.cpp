#include "TestMsgTypeSupportImpl.h"

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
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_unistd.h"

const int DOMAIN_ID = 100;
const int SLEEP_SHORT = 11;
const int SLEEP_LONG = 30;

int do_reader(DDS::DomainParticipant_var participant, DDS::Topic_var topic, bool toggle)
{
  // Create Subscriber
  DDS::Subscriber_var subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!subscriber) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: do_reader() -")
                      ACE_TEXT(" create_subscriber failed!\n")), -1);
  }

  DDS::DataReaderQos qos;
  subscriber->get_default_datareader_qos(qos);
  qos.user_data.value.length(3);
  qos.user_data.value[0] = 0;
  qos.user_data.value[1] = 0;
  qos.user_data.value[2] = 0;
  qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  if (toggle) {
    ACE_DEBUG((LM_DEBUG, "Creating reader\n"));
    DDS::DataReader_var reader =
      subscriber->create_datareader(topic,
                                    qos,
                                    0,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!reader) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: do_reader() -")
                        ACE_TEXT(" create_datareader failed!\n")), -1);
    }

    ACE_OS::sleep(SLEEP_SHORT);
    // Go away.
    ACE_DEBUG((LM_DEBUG, "Deleting reader\n"));
    subscriber->delete_datareader(reader);
    ACE_OS::sleep(SLEEP_SHORT);
    // Come back.
    ACE_DEBUG((LM_DEBUG, "Creating reader\n"));
    reader = subscriber->create_datareader(topic,
                                           qos,
                                           0,
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    ACE_OS::sleep(SLEEP_SHORT);
    return 0;
  } else {
    struct Listener : public DDS::DataReaderListener {
      size_t found, lost;

      Listener() : found(0), lost(0) { }

      virtual void
      on_requested_deadline_missed (::DDS::DataReader_ptr,
                                    const ::DDS::RequestedDeadlineMissedStatus &) { }

      virtual void
      on_requested_incompatible_qos (::DDS::DataReader_ptr,
                                     const ::DDS::RequestedIncompatibleQosStatus &) { }

      virtual void
      on_sample_rejected (::DDS::DataReader_ptr,
                          const ::DDS::SampleRejectedStatus &) { }

      virtual void
      on_liveliness_changed (::DDS::DataReader_ptr,
                             const ::DDS::LivelinessChangedStatus &) { }

      virtual void
      on_data_available (::DDS::DataReader_ptr) { }

      virtual void
      on_subscription_matched (::DDS::DataReader_ptr,
                               const ::DDS::SubscriptionMatchedStatus & status) {
        if (status.current_count_change > 0) {
          ACE_DEBUG((LM_DEBUG, "Reader found writer\n"));
          ++found;
        }
        if (status.current_count_change < 0) {
          ACE_DEBUG((LM_DEBUG, "Reader lost writer\n"));
          ++lost;
        }
      }

      virtual void
      on_sample_lost (::DDS::DataReader_ptr,
                      const ::DDS::SampleLostStatus &) { }
    } listener;

    // Create DataReader
    DDS::DataReader_var reader =
      subscriber->create_datareader(topic,
                                    qos,
                                    &listener,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!reader) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: do_reader() -")
                        ACE_TEXT(" create_datareader failed!\n")),
                       -1);
    }

    ACE_OS::sleep(SLEEP_LONG);

    if (listener.found == 2 && listener.lost == 1) {
      reader->set_listener(0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      return 0;
    }
    return -1;
  }
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
                      ACE_TEXT("ERROR: %N:%l: do_writer() -")
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

  if (toggle) {
    ACE_DEBUG((LM_DEBUG, "Creating writer\n"));
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic,
                                   qos,
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: do_writer() -")
                        ACE_TEXT(" create_datawriter failed!\n")), -1);
    }

    ACE_OS::sleep(SLEEP_SHORT);
    // Go away.
    ACE_DEBUG((LM_DEBUG, "Deleting writer\n"));
    publisher->delete_datawriter(writer);
    ACE_OS::sleep(SLEEP_SHORT);
    // Come back.
    ACE_DEBUG((LM_DEBUG, "Creating writer\n"));
    writer = publisher->create_datawriter(topic,
                                          qos,
                                          0,
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    ACE_OS::sleep(SLEEP_SHORT);
    return 0;
  } else {
    struct Listener : public DDS::DataWriterListener {
      size_t found, lost;

      Listener() : found(0), lost(0) { }

      virtual void
      on_offered_deadline_missed (::DDS::DataWriter_ptr,
                                  const ::DDS::OfferedDeadlineMissedStatus &) { }

      virtual void
      on_offered_incompatible_qos (::DDS::DataWriter_ptr,
                                   const ::DDS::OfferedIncompatibleQosStatus &) { }

      virtual void
      on_liveliness_lost (::DDS::DataWriter_ptr,
                          const ::DDS::LivelinessLostStatus &) { }

      virtual void
      on_publication_matched (::DDS::DataWriter_ptr,
                              const ::DDS::PublicationMatchedStatus & status) {
        if (status.current_count_change > 0) {
          ACE_DEBUG((LM_DEBUG, "Writer found reader\n"));
          ++found;
        }
        if (status.current_count_change < 0) {
          ACE_DEBUG((LM_DEBUG, "Writer lost reader\n"));
          ++lost;
        }
      }

    } listener;

    // Create DataWriter
    DDS::DataWriter_var writer =
      publisher->create_datawriter(topic,
                                   qos,
                                   &listener,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: do_writer() -")
                        ACE_TEXT(" create_datawriter failed!\n")),
                       -1);
    }

    ACE_OS::sleep(SLEEP_LONG);

    if (listener.found == 2 && listener.lost == 1) {
      writer->set_listener(0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      return 0;
    }
    return -1;
  }
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
        if (ACE_OS::strcmp(x, ACE_TEXT("-reader")) == 0) {
          mode = READER;
        }
        if (ACE_OS::strcmp(x, ACE_TEXT("-writer")) == 0) {
          mode = WRITER;
        }
        if (ACE_OS::strcmp(x, ACE_TEXT("-toggle")) == 0) {
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

    int return_code;
    switch (mode) {
    case READER:
      return_code = do_reader(participant, topic, toggle);
      break;
    case WRITER:
      return_code = do_writer(participant, topic, toggle);
      break;
    }

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    TheServiceParticipant->shutdown();
    return return_code;

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
