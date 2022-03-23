// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================


#include "common.h"
#include "Writer.h"
#include "TestException.h"

#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"

#include "tests/Utils/WaitForSample.h"

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/EntryExit.h>

#include <ace/Arg_Shifter.h>

enum Subscriber_State { READY, DONE };

::DDS::DomainParticipantFactory_var dpf;
::DDS::DomainParticipant_var participant;
::DDS::Topic_var topic[2];
::DDS::Publisher_var publisher;
OpenDDS::DCPS::TransportImpl_rch writer_impl;
::DDS::DataWriter_var datawriter[2];
Writer* writers[2];

/// parse the command line arguments
int parse_args(int argc, ACE_TCHAR *argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left()) {
    // options:
    //  -i num_samples_per_instance (defaults to 1)
    //  -m num_instances_per_writer (defaults to 1)
    //  -v verbose transport debug
    //  -o directory of synch file used to coordinate publisher and subscriber
    //     defaults to current directory
    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-m"))) != 0) {
      num_instances_per_writer = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0) {
      num_samples_per_instance = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-v")) == 0) {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-o"))) != 0) {
      synch_file_dir = currentArg;
      pub_finished_filename = synch_file_dir + pub_finished_filename;

      arg_shifter.consume_arg ();
    } else {
      arg_shifter.ignore_arg();
    }
  }

  // Indicates successful parsing of the command line
  return 0;
}

void init()
{
  participant = dpf->create_participant(domain_id,
                                        PARTICIPANT_QOS_DEFAULT,
                                        ::DDS::DomainParticipantListener::_nil(),
                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(participant.in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_participant failed.\n")));
    throw TestException();
  }

  ::Xyz::FooTypeSupportImpl::_var_type fts_servant = new ::Xyz::FooTypeSupportImpl();

  if (::DDS::RETCODE_OK != fts_servant->register_type(participant.in(), type_name)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) register_type failed.\n")));
    throw TestException();
  }

  ::DDS::TopicQos topic_qos;
  participant->get_default_topic_qos(topic_qos);

  topic[0] = participant->create_topic(topic_name[0],
                                       type_name,
                                       topic_qos,
                                       ::DDS::TopicListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(topic[0].in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_topic failed.\n")));
    throw TestException();
  }

  topic[1] = participant->create_topic(topic_name[1],
                                       type_name,
                                       topic_qos,
                                       ::DDS::TopicListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(topic[1].in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_topic failed.\n")));
    throw TestException();
  }

  // Create the default publisher
  publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                            ::DDS::PublisherListener::_nil(),
                                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(publisher.in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_publisher failed.\n")));
    throw TestException();
  }

  // Create the datawriters
  ::DDS::DataWriterQos dw_qos;
  publisher->get_default_datawriter_qos(dw_qos);

  // Make it KEEP_ALL history so we can verify the received
  // data without dropping.
  dw_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
  dw_qos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
  dw_qos.reliability.max_blocking_time.sec = 0;
  dw_qos.reliability.max_blocking_time.nanosec = 0;

  for (int i = 0; i < 2; ++i) {
    datawriter[i] = publisher->create_datawriter(topic[i].in(),
                                                 dw_qos,
                                                 ::DDS::DataWriterListener::_nil(),
                                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(datawriter[i].in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
      throw TestException();
    }

    writers[i] = new Writer(datawriter[i].in(), i);
  }
}

void shutdown()
{
  dpf = ::DDS::DomainParticipantFactory::_nil();
  participant = ::DDS::DomainParticipant::_nil();
  topic[0] = ::DDS::Topic::_nil();
  topic[1] = ::DDS::Topic::_nil();
  publisher = ::DDS::Publisher::_nil();
  writer_impl.reset();
  datawriter[0] = ::DDS::DataWriter::_nil();
  datawriter[1] = ::DDS::DataWriter::_nil();

  TheServiceParticipant->shutdown();
}

void wait_for_subscriber(Subscriber_State state)
{
    // ensure the associations are fully established before writing.
    DDS::WaitSet_var ws = new DDS::WaitSet;
    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0 };
    DDS::Duration_t timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    for (int ctr = 0; ctr < 2; ++ctr) {
      DDS::StatusCondition_var condition = datawriter[ctr]->get_statuscondition();
      condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

      ws->attach_condition(condition);

      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: wait failed!\n")));
        ACE_OS::exit(-1);
      }

      if (datawriter[ctr]->get_publication_matched_status(matches) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: get_publication_matched_status failed!\n")));
        ACE_OS::exit(-1);
      } else {
        if (state == READY && matches.current_count == 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: get_publication_matched_status READY but current_count is 0\n")));
          ACE_OS::exit(-1);
        } else if (state == DONE && matches.current_count == 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: get_publication_matched_status DONE but current_count is not 0\n")));
          ACE_OS::exit(-1);
        }
      }

      ws->detach_condition(condition);
    }
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;

  try {
    dpf = TheParticipantFactoryWithArgs(argc, argv);
    ACE_DEBUG((LM_INFO, "(%P|%t) %T publisher main\n"));

    // let the Service_Participant (in above line) strip out -DCPSxxx parameters
    // and then get application specific parameters.
    parse_args(argc, argv);

    init();

    // wait for subscrier ready
    wait_for_subscriber(READY);

    {  // Extra scope for VC6
      for (int i = 0; i < num_datawriters; ++i) {
        writers[i]->start();
      }
    }

    const ACE_Time_Value small_time(0, 250000);

    int timeout_writes = 0;
    bool writers_finished = false;

    while (!writers_finished) {
      writers_finished = true;
      for (int i = 0; i < num_datawriters; ++i) {
        writers_finished = writers_finished && writers[i]->is_finished();
      }
      ACE_OS::sleep(small_time);
    }

    {  // Extra scope for VC6
      for (int i = 0; i < num_datawriters; ++i) {
        timeout_writes += writers[i]->get_timeout_writes();
      }
    }

    // Indicate that the publisher is done
    FILE* writers_completed = ACE_OS::fopen(pub_finished_filename.c_str(), ACE_TEXT("w"));
    if (writers_completed == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Unable to write publisher completed file\n")));
    } else {
      ACE_OS::fprintf(writers_completed, "%d\n", timeout_writes);
    }

    if (writers_completed) ACE_OS::fclose(writers_completed);

    // wait for subscriber finish
    wait_for_subscriber(DONE);

  } catch (const TestException&) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TestException caught in main(). ")));
    status = 1;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("Exception caught in main():");
    status = 1;
  }

  try {
    if (!CORBA::is_nil(participant.in())) {
      participant->delete_contained_entities();
    }

    if (!CORBA::is_nil(dpf.in())) {
      dpf->delete_participant(participant.in());
    }
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("Exception caught in cleanup.");
    status = 1;
  }

  shutdown();
  return status;
}
