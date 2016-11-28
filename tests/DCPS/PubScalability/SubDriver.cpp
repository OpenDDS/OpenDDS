#include "SubDriver.h"
#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "tests/DCPS/common/TestSupport.h"
#include "DataReaderListener.h"
#include "tests/Utils/ExceptionStreams.h"

#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>
#include <string>
#include <sstream>
#include <iostream>

const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";

SubDriver::SubDriver()
: num_writes_ (0),
  receive_delay_msec_ (0),
  listener_(0)
{
}


SubDriver::~SubDriver()
{
}


void
SubDriver::run(int& argc, ACE_TCHAR* argv[])
{
  init(argc, argv);
  run();
}


void
SubDriver::parse_args(int& argc, ACE_TCHAR* argv[])
{
  // Command-line arguments:
  //
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
  {
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
    {
      num_writes_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-l"))) != 0)
    {
      receive_delay_msec_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0) {
      ACE_DEBUG((LM_DEBUG,
                 "usage: %s\n",
                 argv[0]));

      arg_shifter.consume_arg();
      throw TestException();
    }
    // Anything else we just skip
    else {
      arg_shifter.ignore_arg();
    }
  }
}


void
SubDriver::init(int& argc, ACE_TCHAR* argv[])
{
  ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  parse_args(argc, argv);

  participant_ =
    dpf->create_participant(MY_DOMAIN,
                            PARTICIPANT_QOS_DEFAULT,
                            ::DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (participant_.in ()));

  ::Xyz::SampleTypeTypeSupport_var fts (new ::Xyz::SampleTypeTypeSupportImpl);

  if (::DDS::RETCODE_OK != fts->register_type(participant_.in (), MY_TYPE))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Failed to register the SampleTypeTypeSupport.")));
    }

  ::DDS::TopicQos default_topic_qos;
  participant_->get_default_topic_qos(default_topic_qos);

  ::DDS::TopicQos new_topic_qos = default_topic_qos;
  new_topic_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;

  participant_->set_default_topic_qos(new_topic_qos);

  topic_ = participant_->create_topic (MY_TOPIC,
                                       MY_TYPE,
                                       TOPIC_QOS_DEFAULT,
                                       ::DDS::TopicListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (topic_.in ()));

  subscriber_ =
    participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                    ::DDS::SubscriberListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (subscriber_.in ()));

  std::cout << std::hex << "0x" << subscriber_->get_instance_handle() << std::endl;

  // Create datareader to test copy_from_topic_qos.
  listener_ = new DataReaderListenerImpl;
  ::DDS::DataReaderListener_var drl (listener_);

  datareader_
    = subscriber_->create_datareader(topic_.in (),
                                     DATAREADER_QOS_USE_TOPIC_QOS,
                                     drl.in(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (datareader_.in ()));

  // And we are done with the init().
}


void
SubDriver::run()
{
  while (this->listener_->samples_read() != num_writes_) {
    ACE_OS::sleep(1);
  }
  ACE_DEBUG((LM_DEBUG, "(%P|%t) Received all %d samples\n", num_writes_));

  // Tear-down the entire Transport Framework.
  ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  participant_->delete_contained_entities();
  dpf->delete_participant(participant_.in ());

  TheServiceParticipant->shutdown();
}
