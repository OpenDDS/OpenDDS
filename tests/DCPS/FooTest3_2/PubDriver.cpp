#include "PubDriver.h"
#include "Writer.h"
#include "TestException.h"
#include "tests/DCPS/FooType3Unbounded/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType3Unbounded/FooDefTypeSupportImpl.h"
#include "tests/DCPS/FooType3Unbounded/FooDefC.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "tests/DCPS/common/TestSupport.h"

#include <ace/Arg_Shifter.h>
#include <sstream>

const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";

PubDriver::PubDriver()
: participant_ (::DDS::DomainParticipant::_nil ()),
  topic_ (::DDS::Topic::_nil ()),
  publisher_ (::DDS::Publisher::_nil ()),
  datawriters_ (0),
  writers_ (0),
  block_on_write_ (0),
  num_threads_to_write_ (0),
  multiple_instances_ (0),
  num_writes_per_thread_ (1),
  num_datawriters_ (1),
  max_samples_per_instance_(::DDS::LENGTH_UNLIMITED),
  history_depth_ (1),
  has_key_ (1),
  write_delay_msec_ (0),
  check_data_dropped_ (0)
{
}


PubDriver::~PubDriver()
{
  delete [] datawriters_;
  for (int i = 0; i < num_datawriters_; i ++)
  {
    delete writers_[i];
  }

  delete [] writers_;
}


void
PubDriver::run(int& argc, ACE_TCHAR* argv[])
{
  parse_args(argc, argv);
  init(argc, argv);

  run();

  end();
}


void
PubDriver::parse_args(int& argc, ACE_TCHAR* argv[])
{
  // Command-line arguments:
  //
  //  -b <block/non-block waiting>
  //  -t num_threads_to_write    defaults to 1
  //  -i num_writes_per_thread   defaults to 1
  //  -w num_datawriters_        defaults to 1
  //  -b block_on_write?1:0      defaults to 0
  //  -m multiple_instances?1:0  defaults to 0
  //  -n max_samples_per_instance defaults to INFINITE
  //  -d history.depth           defaults to 1
  //  -y has_key_flag            defaults to 1
  //  -r data_dropped            defaults to 0
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
  {
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-b"))) != 0)
    {
      block_on_write_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-t"))) != 0)
    {
      num_threads_to_write_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-m"))) != 0)
    {
      multiple_instances_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0)
    {
      num_writes_per_thread_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-w"))) != 0)
    {
      num_datawriters_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
    {
      max_samples_per_instance_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-DCPS")) != -1)
    {
      // ignore -DCPSxxx options that will be handled by Service_Participant
      arg_shifter.ignore_arg();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))) != 0)
    {
      history_depth_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-y"))) != 0)
    {
      has_key_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-l"))) != 0)
    {
      write_delay_msec_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-r"))) != 0)
    {
      check_data_dropped_ = ACE_OS::atoi (current_arg);
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
PubDriver::init(int& argc, ACE_TCHAR *argv[])
{
  // Create DomainParticipant and then publisher, topic and datawriter.
  ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  datawriters_ = new ::DDS::DataWriter_var[num_datawriters_];
  writers_ = new Writer* [num_datawriters_];

  ::Xyz::FooTypeSupport_var fts (new ::Xyz::FooTypeSupportImpl);

  participant_ =
    dpf->create_participant(MY_DOMAIN,
                            PARTICIPANT_QOS_DEFAULT,
                            ::DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (participant_.in ()));

  if (::DDS::RETCODE_OK != fts->register_type(participant_.in (), MY_TYPE))
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("Failed to register the FooTypeSupport.")));
    }


  ::DDS::TopicQos topic_qos;
  participant_->get_default_topic_qos(topic_qos);

  if (block_on_write_)
  {
    topic_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
    topic_qos.resource_limits.max_samples_per_instance = max_samples_per_instance_;
    topic_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;
  }
  else
  {
    topic_qos.history.depth = history_depth_;
  }

  topic_ = participant_->create_topic (MY_TOPIC,
                                       MY_TYPE,
                                       topic_qos,
                                       ::DDS::TopicListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (topic_.in ()));


  publisher_ =
    participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                          ::DDS::PublisherListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (publisher_.in ()));

  ::DDS::DataWriterQos datawriter_qos;
  publisher_->get_default_datawriter_qos (datawriter_qos);

  if (block_on_write_)
  {
    datawriter_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
    datawriter_qos.resource_limits.max_samples_per_instance = max_samples_per_instance_;
    datawriter_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;
  }
  else
  {
    datawriter_qos.history.depth = history_depth_;
  }

  // Create one datawriter or multiple datawriters belong to the same
  // publisher.
  for (int i = 0; i < num_datawriters_; i ++)
  {
    datawriters_[i]
    = publisher_->create_datawriter(topic_.in (),
                                    datawriter_qos,
                                    ::DDS::DataWriterListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    TEST_CHECK (! CORBA::is_nil (datawriters_[i].in ()));
  }
}

void
PubDriver::end()
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) PubDriver::end \n"));

  // Record samples been written in the Writer's data map.
  // Verify the number of instances and the number of samples
  // written to the datawriter.
  for (int i = 0; i < num_datawriters_; i ++)
  {
    writers_[i]->end ();
    InstanceDataMap& map = writers_[i]->data_map ();
    if (multiple_instances_ == 0 || has_key_ == 0)
    {
      // One instance when data type has a key value and all instances
      // have the same key or has no key value.
      TEST_CHECK (map.num_instances() == 1);
    }
    else
    {
      // multiple instances test - an instance per thread
      TEST_CHECK (map.num_instances() == num_threads_to_write_);
    }
    TEST_CHECK (map.num_samples() == num_threads_to_write_ * num_writes_per_thread_);

    publisher_->delete_datawriter(datawriters_[i].in ());
  }

  ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  participant_->delete_contained_entities();
  dpf->delete_participant(participant_.in());
  TheServiceParticipant->shutdown();
}

void
PubDriver::run()
{
  // Each Writer/DataWriter launch threads to write samples
  // to the same instance or multiple instances.
  // When writing to multiple instances, the instance key
  // identifies instances is the thread id.
  for (int i = 0; i < num_datawriters_; i ++) {
    writers_[i] = new Writer(this,
                             datawriters_[i].in (),
                             num_threads_to_write_,
                             num_writes_per_thread_,
                             multiple_instances_,
                             i,
                             has_key_,
                             write_delay_msec_,
                             check_data_dropped_);
    writers_[i]->start ();
  }

  bool finished = false;
  while (!finished) {
    finished = true;
    for (int i = 0; i < num_datawriters_; i ++) {
      if (!writers_[i]->finished()) {
        finished = false;
        break;
      }
    }
    ACE_OS::sleep(ACE_Time_Value(0,200000));
  }
}
