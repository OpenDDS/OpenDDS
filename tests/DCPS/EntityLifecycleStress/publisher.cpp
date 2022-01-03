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

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  sigset_t mask, prev;
  ACE_OS::sigemptyset(&mask);
  ACE_OS::sigaddset(&mask, SIGPIPE);
  ACE_OS::sigprocmask(SIG_BLOCK, &mask, &prev);

  bool large_samples = false;
  for (int i = 0; i < argc; ++i) {
    if (std::string(ACE_TEXT_ALWAYS_CHAR(argv[i])) == "-l") {
      large_samples = true;
    }
  }

  try
  {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    DDS::DomainParticipant_var participant =
      dpf->create_participant(31,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant.in())) {
      cerr << "create_participant failed." << endl;
      return 1;
    }

    MessageTypeSupportImpl::_var_type servant = new MessageTypeSupportImpl();

    if (DDS::RETCODE_OK != servant->register_type(participant.in(), "")) {
      cerr << "register_type failed." << endl;
      exit(1);
    }

    CORBA::String_var type_name = servant->get_type_name();

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name.in(),
                                topic_qos,
                                DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      cerr << "create_topic failed." << endl;
      exit(1);
    }

    DDS::PublisherQos pub_qos;
    participant->get_default_publisher_qos(pub_qos);

    DDS::Publisher_var pub =
      participant->create_publisher(pub_qos, DDS::PublisherListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(pub.in())) {
      cerr << "create_publisher failed." << endl;
      exit(1);
    }

    DDS::DataWriter_var dw =
      pub->create_datawriter(topic.in(),
                              DATAWRITER_QOS_DEFAULT,
                              DDS::DataWriterListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dw.in())) {
      cerr << "create_datawriter failed." << endl;
      exit(1);
    }

    Messenger::MessageDataWriter_var message_dw =
      Messenger::MessageDataWriter::_narrow(dw.in());

    if (CORBA::is_nil(message_dw.in())) {
      cerr << "Messenger::MessageDataWriter::_narrow() failed." << endl;
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = 16;

    ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    {
      stringstream ss;
      ss << ACE_OS::getpid() << flush;

      message.from       = ss.str().c_str();
      message.subject    = "Review";
      message.text       = large_samples ? std::string(4000, 'Z').c_str() : "Wash. Rinse. Repeat.";
      message.count      = 0;
    }

    const ACE_Time_Value delay(large_samples ? 2 : 0, large_samples ? 0 : 10000);

    const size_t count = large_samples ? 4u : 750u;
    for (size_t i = 0; i < count; ++i) {
      ACE_OS::sleep(delay);
      ++message.count;
      DDS::ReturnCode_t error =  message_dw->write(message, handle);

      if (error != DDS::RETCODE_OK) {
        cerr << "Messenger::MessageDataWriter::write() failed." << endl;
        exit(1);
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
    dpf->delete_participant(participant.in());
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
