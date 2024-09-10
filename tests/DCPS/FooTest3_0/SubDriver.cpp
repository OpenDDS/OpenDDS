#include "SubDriver.h"
#include "TestException.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/RestoreOutputStreamState.h"
#include "DataReaderListener.h"
#include "DataReaderQCListener.h"
#include "tests/DCPS/common/TestSupport.h"
#include "tests/Utils/ExceptionStreams.h"
#include "tests/Utils/StatusMatching.h"

#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>
#include <ace/OS_NS_unistd.h>

#include <string>
#include <sstream>
#include <iostream>

const long  MY_DOMAIN   = 111;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";

using namespace ::OpenDDS::DCPS;

SubDriver::SubDriver()
  : num_writes_ (0),
    num_disposed_ (0),
    shutdown_pub_ (1),
    add_new_subscription_ (0),
    shutdown_delay_secs_ (10),
    listener_(0),
    qc_usage_ (false)
{
}


SubDriver::~SubDriver()
{
}


void
SubDriver::run(int& argc, ACE_TCHAR* argv[])
{
  DistributedConditionSet_rch dcs =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  init(dcs, argc, argv);
  run(dcs);
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
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0)
        {
          num_disposed_ = ACE_OS::atoi (current_arg);
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-q"))) != 0)
        {
          qc_usage_ = ACE_OS::atoi (current_arg);
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
SubDriver::init(DistributedConditionSet_rch dcs, int& argc, ACE_TCHAR* argv[])
{
  dcs->wait_for("sub", "pub", "ready");
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

  {
    OpenDDS::DCPS::RestoreOutputStreamState ross(std::cout);
    std::cout << std::hex << "0x" << subscriber_->get_instance_handle() << std::endl;
  }

  // Create datareader to test copy_from_topic_qos.
#ifndef OPENDDS_NO_QUERY_CONDITION
  DataReaderQCListenerImpl* qc_listener = 0;
  if (qc_usage_)
  {
    qc_listener = new DataReaderQCListenerImpl(dcs, num_writes_);
    listener_ = qc_listener;
  }
  else
#endif
  {
    listener_ = new DataReaderListenerImpl(dcs, num_writes_);
  }
  ::DDS::DataReaderListener_var drl = listener_;

  datareader_
    = subscriber_->create_datareader(topic_.in (),
                                     DATAREADER_QOS_USE_TOPIC_QOS,
                                     drl.in(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (! CORBA::is_nil (datareader_.in ()));
  TEST_CHECK (participant_->contains_entity(datareader_->get_instance_handle()));

#ifndef OPENDDS_NO_QUERY_CONDITION
  if (qc_usage_)
  {
    DDS::StringSeq params(1);
    params.length (1);
    params[0] = "101010";

    DDS::QueryCondition_var qc =
      datareader_->create_querycondition (DDS::NOT_READ_SAMPLE_STATE,
                                          DDS::ANY_VIEW_STATE,
                                          DDS::ANY_INSTANCE_STATE,
                                          "a_long_value = %0",
                                          params);
    TEST_CHECK (! CORBA::is_nil (qc.in ()));

    qc_listener->set_qc (qc.in ());
  }
#endif

  Utils::wait_match(datareader_, 1);
  dcs->post("sub", "ready");
  // And we are done with the init().
}

void
SubDriver::run(DistributedConditionSet_rch dcs)
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t) SubDriver::run, ")
              ACE_TEXT(" Wait for publisher.\n")));

  dcs->wait_for("sub", "sub", "done");

  TEST_CHECK (participant_->contains_entity(datareader_->get_instance_handle()));

  TEST_CHECK (this->listener_->samples_disposed() == num_disposed_);

  ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  participant_->delete_contained_entities();
  dpf->delete_participant(participant_.in ());

  TheServiceParticipant->shutdown();
}
