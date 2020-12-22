// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================

#ifdef ACE_LYNXOS_MAJOR
#define VMOS_DEV
#endif

#include "DataReaderListener.h"
#include "TestException.h"
#include "common.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"

#include "ace/Arg_Shifter.h"

#include <cstdio>
#include <iostream>

::DDS::DomainParticipantFactory_var dpf;
::DDS::DomainParticipant_var participant[2];
::DDS::Topic_var topic[2];
::DDS::Subscriber_var subscriber[2];
OpenDDS::DCPS::TransportImpl_rch reader_impl[2];
::DDS::DataReaderListener_var listener[2];
::DDS::DataReader_var datareader[2];

/// parse the command line arguments
int parse_args(int argc, ACE_TCHAR *argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left()) {
    // options:
    //  -m num_instances_per_writer defaults to 1
    //  -i num_samples_per_instance defaults to 1
    //  -v verbose transport debug

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
    } else {
      arg_shifter.ignore_arg();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

void init_dcps_objects(int i)
{
  participant[i] = dpf->create_participant(domain_id,
                                           PARTICIPANT_QOS_DEFAULT,
                                           ::DDS::DomainParticipantListener::_nil(),
                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(participant[i].in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_participant failed.\n")));
    throw TestException();
  }

  ::Xyz::FooTypeSupportImpl::_var_type fts_servant = new ::Xyz::FooTypeSupportImpl();
  ::Xyz::FooTypeSupportImpl::_var_type another_fts_servant = new ::Xyz::FooTypeSupportImpl();

  if (::DDS::RETCODE_OK != fts_servant->register_type(participant[i].in(), type_name)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to register the FooNoTypeTypeSupport.")));
    throw TestException();
  }

  // Test if different TypeSupport instances of the same TypeSupport type can register
  // with the same type name within the same domain participant.
  if (::DDS::RETCODE_OK != another_fts_servant->register_type(participant[i].in(), type_name)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to register the FooNoTypeTypeSupport.")));
    throw TestException();
  }

  ::DDS::TopicQos topic_qos;
  participant[i]->get_default_topic_qos(topic_qos);

  topic[i] = participant[i]->create_topic(topic_name[i],
                                          type_name,
                                          topic_qos,
                                          ::DDS::TopicListener::_nil(),
                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(topic[i].in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to create_topic.")));
    throw TestException();
  }

  subscriber[i] = participant[i]->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                    ::DDS::SubscriberListener::_nil(),
                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(subscriber[i].in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to create_subscriber.")));
    throw TestException();
  }

  // Create the Datareaders
  ::DDS::DataReaderQos dr_qos;
  subscriber[i]->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  ::DDS::TopicDescription_var description
    = participant[i]->lookup_topicdescription(topic_name[i]);
  // create the datareader.
  datareader[i] = subscriber[i]->create_datareader(description.in(),
                                                   dr_qos,
                                                   listener[i].in(),
                                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(datareader[i].in())) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) create_datareader failed.\n")));
    throw TestException();
  }
}

void init_listener()
{
  for (int i = 0; i < 2; ++i) {
    listener[i] = new DataReaderListenerImpl();

    if (CORBA::is_nil(listener[i].in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) listener is nil.")));
      throw TestException();
    }
  }
}

void shutdown()
{
  dpf = 0;
  participant[0] = 0;
  participant[1] = 0;
  topic[0] = 0;
  topic[1] = 0;
  subscriber[0] = 0;
  subscriber[1] = 0;
  reader_impl[0].reset();
  reader_impl[1].reset();
  listener[0] = 0;
  listener[1] = 0;
  datareader[0] = 0;
  datareader[1] = 0;

  TheServiceParticipant->shutdown();
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;

  try {
    dpf = TheParticipantFactoryWithArgs(argc, argv);
    ACE_DEBUG((LM_INFO,"(%P|%t) %T subscriber main\n"));

    // let the Service_Participant (in above line) strip out -DCPSxxx parameters
    // and then get application specific parameters.
    parse_args(argc, argv);

    init_listener();
    init_dcps_objects(0);
    init_dcps_objects(1);

    Allocator* allocator;
    void* ptr;
    ACE_NEW_RETURN(allocator, Allocator(mmap_file), -1);
    //    std::cout << "Create a new Allocator object!" << std::endl;
    //    std::cout << "Based address on subscriber: " << allocator->base_addr() << std::endl;
    while(allocator->find("state", ptr) != 0) {}
    SharedData* state = reinterpret_cast<SharedData*>(ptr);
    //    std::cout << "Found the SharedData object at address " << ptr << std::endl;

    // Indicate that the subscriber is ready
    state->sub_ready = true;
    //    std::cout << "Subscriber is ready!" << std::endl;

    // Wait for the publisher to be ready
    do {
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);
    } while (state->pub_ready == false);

    int expected = num_datawriters * num_instances_per_writer * num_samples_per_instance;

    //    std::cout << "Start reading data..." << std::endl;
    int timeout_writes = 0;

    int prev_reads = 0;
    while (num_reads < expected) {
      if (prev_reads < num_reads.value()) {
        prev_reads = num_reads.value();
        std::cout << "Read " << prev_reads << " samples!" << std::endl;
      }
      // Get the number of the timed out writes from publisher so we
      // can re-calculate the number of expected messages. Otherwise,
      // the blocking timeout test will never exit from this loop.
      if (state->timeout_writes_ready == true && timeout_writes > 0) {
        expected -= timeout_writes;
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) timed out writes %d, we expect %d\n"),
                   timeout_writes, expected));
      }
      ACE_OS::sleep(1);
    }

    // Indicate that the subscriber is done
    state->sub_finished = true;

    //    std::cout << "Finish reading " << num_reads.value() << " samples. Waiting for publisher to finish..." << std::endl;

    // Wait for the publisher to finish
    while (state->pub_finished == false) {
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);
    }

  } catch (const TestException&) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TestException caught in main(). ")));
    status = 1;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("Exception caught in main():");
    status = 1;
  }

  try {
    for (int i = 0; i < 2; ++i) {
      if (!CORBA::is_nil(participant[i].in())) {
        participant[i]->delete_contained_entities();
      }
      if (!CORBA::is_nil(dpf.in())) {
        dpf->delete_participant(participant[i].in());
      }
    }
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("Exception caught in cleanup.");
    status = 1;
  }

  shutdown();
  return status;
}
