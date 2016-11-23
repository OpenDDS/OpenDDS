// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================

#include "MessengerTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>

#include "dds/DCPS/StaticIncludes.h"

#include "ace/streams.h"
#include "ace/OS_NS_unistd.h"
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"

using namespace Messenger;
using namespace std;

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    DDS::DomainParticipant_var participant =
      dpf->create_participant(4,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (participant)) {
      cerr << "create_participant failed." << endl;
      return 1;
    }

    DDS::TypeSupport_var ts = new MessageTypeSupportImpl;

    if (DDS::RETCODE_OK != ts->register_type(participant, "")) {
      cerr << "register_type failed." << endl;
      exit(1);
    }

    CORBA::String_var type_name = ts->get_type_name();

    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List TLT",
                                type_name,
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(topic)) {
      cerr << "create_topic failed." << endl;
      exit(1);
    }

    DDS::Publisher_var pub =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
      DDS::PublisherListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(pub)) {
      cerr << "create_publisher failed." << endl;
      exit(1);
    }

    // Create the datawriter
    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);
    dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    dw_qos.resource_limits.max_samples_per_instance = 1000;
    dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

    DDS::DataWriter_var dw =
      pub->create_datawriter(topic,
                             dw_qos,
                             0,
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dw)) {
      cerr << "create_datawriter failed." << endl;
      exit(1);
    }

    MessageDataWriter_var message_dw = MessageDataWriter::_narrow(dw);

    const ACE_Time_Value writer_delay(0, 100*1000);

    Message message;
    message.subject_id = 99;
    DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from = "Comic Book Guy";
    message.subject = "Review";
    message.text = "Worst. Movie. Ever.";
    message.count = 1;

    while (true) {
      DDS::PublicationMatchedStatus pubmatched;
      if (dw->get_publication_matched_status(pubmatched) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: get_publication_matched_status\n")));
        break;
      } else if (pubmatched.current_count == 0 && pubmatched.total_count > 0) {
        // subscriber has come and gone
        break;
      }

      DDS::ReturnCode_t ret = message_dw->write(message, handle);

      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: %dth write() returned %d.\n"),
          message.count, ret));
      } else {
        std::cout << "wrote message " << message.count << " to " << pubmatched.current_count << " matched" << std::endl;
      }

      ++message.count;

      ACE_OS::sleep(writer_delay);
    }

    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();

  } catch (CORBA::Exception& e) {
    cerr << "PUB: Exception caught in main.cpp:" << endl
         << e << endl;
    exit(1);
  }

  return 0;
}
