#include "MessengerTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/unique_ptr.h>

#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <tests/Utils/ExceptionStreams.h>
#include <tests/Utils/StatusMatching.h>

#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include <ace/OS_NS_unistd.h>

#include <memory>
#include <sstream>

using namespace Messenger;
using namespace std;

int wait_match(const DDS::DataWriter_var& writer, unsigned int current_readers, unsigned int total_readers)
{
  int ret = -1;
  DDS::StatusCondition_var condition = writer->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  DDS::WaitSet_var ws(new DDS::WaitSet);
  DDS::ReturnCode_t a = ws->attach_condition(condition);
  if (a != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - attach_condition returned %d\n"), a));
    return ret;
  }

  const CORBA::Long n_current_readers = static_cast<CORBA::Long>(current_readers);
  const CORBA::Long n_total_readers = static_cast<CORBA::Long>(total_readers);
  const DDS::Duration_t forever = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
  DDS::PublicationMatchedStatus ms = { 0, 0, 0, 0, 0 };
  DDS::ConditionSeq conditions;
  while (ret != 0) {
    if (writer->get_publication_matched_status(ms) == DDS::RETCODE_OK) {
      if (ms.current_count == n_current_readers && ms.total_count == n_total_readers) {
        ret = 0;
      } else { // wait for a change
        DDS::ReturnCode_t w = ws->wait(conditions, forever);
        if ((w != DDS::RETCODE_OK) && (w != DDS::RETCODE_TIMEOUT)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - wait returned %d\n"), w));
          break;
        }
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - get_publication_matched_status failed!\n")));
      break;
    }
  }

  ws->detach_condition(condition);
  return ret;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  sigset_t mask, prev;
  ACE_OS::sigemptyset(&mask);
  ACE_OS::sigaddset(&mask, SIGPIPE);
  ACE_OS::sigprocmask(SIG_BLOCK, &mask, &prev);

  int total_readers = 1;

  bool large_samples = false;
  bool sub_delayed_exit = false;
  for (int i = 1; i < argc; ++i) {
    if (std::string(ACE_TEXT_ALWAYS_CHAR(argv[i])) == "-l") {
      large_samples = true;
    }
    if (std::string(ACE_TEXT_ALWAYS_CHAR(argv[i])) == "-d") {
      sub_delayed_exit = true;
    }
    if (std::string(ACE_TEXT_ALWAYS_CHAR(argv[i])) == "-r") {
      ++i;
      if (i < argc) {
        total_readers = ACE_OS::atoi(argv[i]);
      }
    }
  }

  try
  {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    DDS::DomainParticipant_var participant =
      dpf->create_participant(31,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              0);
    if (!participant) {
      cerr << "create_participant failed." << endl;
      return 1;
    }

    MessageTypeSupportImpl::_var_type servant = new MessageTypeSupportImpl();

    if (DDS::RETCODE_OK != servant->register_type(participant, "")) {
      cerr << "register_type failed." << endl;
      exit(1);
    }

    CORBA::String_var type_name = servant->get_type_name();

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name,
                                topic_qos,
                                0,
                                0);

    if (!topic) {
      cerr << "create_topic failed." << endl;
      exit(1);
    }

    DDS::PublisherQos pub_qos;
    participant->get_default_publisher_qos(pub_qos);

    DDS::Publisher_var pub =
      participant->create_publisher(pub_qos,
                                    0,
                                    0);
    if (!pub) {
      cerr << "create_publisher failed." << endl;
      exit(1);
    }

    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);

    dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    DDS::DataWriter_var dw =
      pub->create_datawriter(topic,
                             dw_qos,
                             0,
                             0);

    if (!dw) {
      cerr << "create_datawriter failed." << endl;
      exit(1);
    }

    Messenger::MessageDataWriter_var message_dw =
      Messenger::MessageDataWriter::_narrow(dw);

    if (!message_dw) {
      cerr << "Messenger::MessageDataWriter::_narrow() failed." << endl;
      exit(1);
    }

    Messenger::Message message;

    {
      stringstream ss;
      ss << ACE_OS::getpid() << flush;

      message.from       = ss.str().c_str();
      message.subject    = "Review";
      message.text       = large_samples ? std::string(4000, 'Z').c_str() : "Wash. Rinse. Repeat.";
    }

    const ACE_Time_Value delay(large_samples ? 2 : 0, large_samples ? 0 : 10000);

    const size_t count = large_samples ? 4u : 20u;
    for (size_t i = 0; i < count; ++i) {
      message.subject_id = i;
      message.count = i;
      DDS::ReturnCode_t error = message_dw->write(message, DDS::HANDLE_NIL);

      if (error != DDS::RETCODE_OK) {
        cerr << "Messenger::MessageDataWriter::write() failed." << endl;
        exit(1);
      }
    }

    const ::DDS::Duration_t forever = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
    for (int current_readers = 1; current_readers <= total_readers; ++current_readers) {

      {
        stringstream ss;
        ss << "Publisher " << ACE_OS::getpid() << " is waiting for " << current_readers << " readers." << endl;

        cout << ss.str() << flush;
      }
      if (sub_delayed_exit) {
        wait_match(dw, current_readers, current_readers);
      } else {
        wait_match(dw, 1, current_readers);
      }
      {
        stringstream ss;
        ss << "Publisher " << ACE_OS::getpid() << " is done waiting for " << current_readers << " readers." << endl;

        cout << ss.str() << flush;
      }

      {
        stringstream ss;
        ss << "Publisher " << ACE_OS::getpid() << " is waiting for acknowledgments." << endl;

        cout << ss.str() << flush;
      }
      if (dw->wait_for_acknowledgments(forever) != DDS::RETCODE_OK) {
        cerr << "wait_for_acknowledgments() failed." << endl;
        exit(1);
      }
      {
        stringstream ss;
        ss << "Publisher " << ACE_OS::getpid() << " is done waiting for acknowledgments." << endl;

        cout << ss.str() << flush;
      }
    }

    {
      stringstream ss;
      ss << "Publisher " << ACE_OS::getpid() << " is done. Exiting." << endl;

      cout << ss.str() << flush;
    }

    // Semi-arbitrary change to clean-up approach
    if (ACE_OS::getpid() % 3) {
      message_dw = 0;

      pub->delete_datawriter(dw);
      participant->delete_publisher(pub);
    }

    participant->delete_contained_entities();
    dpf->delete_participant(participant);
  }
  catch (const CORBA::Exception& e)
  {
    cerr << "PUB: Exception caught in main.cpp:" << endl
         << e << endl;
    exit(1);
  }
  TheServiceParticipant->shutdown();

  return 0;
}
