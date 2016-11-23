#include "SubDriver.h"
#include "TestException.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "DataReaderListener.h"
#include "tests/DCPS/common/TestSupport.h"
#include "tests/Utils/ExceptionStreams.h"

#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>
#include <ace/OS_NS_unistd.h>

#include <string>
#include <sstream>
#include <iostream>

const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";

using namespace ::OpenDDS::DCPS;

SubDriver::SubDriver()
  : num_writes_ (0),
    shutdown_pub_ (1),
    add_new_subscription_ (0),
    shutdown_delay_secs_ (10),
    sub_ready_filename_(ACE_TEXT("sub_ready.txt")),
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
  // -d <delay before shutdown>
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
    {
      if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
        {
          num_writes_ = ACE_OS::atoi (current_arg);
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-a"))) != 0)
        {
          add_new_subscription_ = ACE_OS::atoi (current_arg);
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-x"))) != 0)
        {
          shutdown_pub_ = ACE_OS::atoi (current_arg);
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))) != 0)
        {
          shutdown_delay_secs_ = ACE_OS::atoi (current_arg);
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-f"))) != 0)
        {
          sub_ready_filename_ = current_arg;
          arg_shifter.consume_arg ();
        }
      else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0)
        {
          ACE_DEBUG((LM_DEBUG,
                 "usage: %s "
                 "-n Expected initial messages\n"
                 , argv[0]));

          arg_shifter.consume_arg();
          throw TestException();
        }
      // Anything else we just skip
      else
        {
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
  if (participant_->contains_entity(participant_->get_instance_handle()))
  {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: participant_ should not contain itself\n")));
  }

  ::Xyz::FooTypeSupport_var fts (new ::Xyz::FooTypeSupportImpl);

  if (::DDS::RETCODE_OK != fts->register_type(participant_.in (), MY_TYPE))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Failed to register the FooTypeSupport.")));
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
  if (!participant_->contains_entity(topic_->get_instance_handle()))
  {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: participant_ should indicated it contains topic_\n")));
  }

  subscriber_ =
    participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                    ::DDS::SubscriberListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (subscriber_.in ()));
  if (!participant_->contains_entity(subscriber_->get_instance_handle()))
  {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: participant_ should indicated it contains subscriber_\n")));
  }

  std::cout << std::hex << "0x" << subscriber_->get_instance_handle() << std::endl;

  // Create datareader to test copy_from_topic_qos.
  listener_ = new DataReaderListenerImpl;
  ::DDS::DataReaderListener_var drl (listener_);

  datareader_
    = subscriber_->create_datareader(topic_.in (),
                                     DATAREADER_QOS_DEFAULT,
                                     drl.in(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (datareader_.in ()));

  // Indicate that the subscriber is ready to accept connection
  FILE* readers_ready = ACE_OS::fopen (sub_ready_filename_.c_str (), ACE_TEXT("w"));
  if (readers_ready == 0)
  {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber ready file\n")));
  }
  ACE_OS::fclose(readers_ready);
  // And we are done with the init().
}

void
SubDriver::run()
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t) SubDriver::run, ")
              ACE_TEXT(" Wait for publisher. \n")));

  while (this->listener_->samples_read() != num_writes_) {
    ACE_OS::sleep(1);
  }

  if (!participant_->contains_entity(datareader_->get_instance_handle()))
  {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: participant_ should indicated it contains datareader_\n")));
  }

  ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  participant_->delete_contained_entities();
  dpf->delete_participant(participant_.in ());

  TheServiceParticipant->shutdown();
}
