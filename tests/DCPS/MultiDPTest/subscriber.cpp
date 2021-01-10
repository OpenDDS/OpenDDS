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

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/TopicDescriptionImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/EntryExit.h>
// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include <dds/DCPS/transport/framework/TransportImpl.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>

#include <ace/Arg_Shifter.h>

#include <cstdio>
#include <iostream>

::DDS::DomainParticipantFactory_var dpf;
::DDS::DomainParticipant_var participant[2];
::DDS::Topic_var topic[2];
::DDS::Subscriber_var subscriber[2];
OpenDDS::DCPS::TransportImpl_rch reader_impl[2];
::DDS::DataReaderListener_var listener[2];
::DDS::DataReader_var datareader[2];

bool safety_profile = false;

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
    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-safety-profile")) == 0) {
      safety_profile = true;
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

  if (safety_profile) {
    OpenDDS::DCPS::Discovery_rch disc = TheServiceParticipant->get_discovery(111);
    OpenDDS::RTPS::RtpsDiscovery_rch rd = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(disc);
    if (!rd.is_nil()) {
      char config_name[64], inst_name[64];
      ACE_TCHAR nak_depth[8];
      ACE_OS::snprintf(config_name, 64, "cfg_%d", i);
      ACE_OS::snprintf(inst_name, 64, "rtps_%d", i);
      // The 2 is a safety factor to allow for control messages.
      ACE_OS::snprintf(nak_depth, 8, ACE_TEXT("%lu"),
        static_cast<unsigned long>(2 * num_instances_per_writer * num_samples_per_instance));

      OpenDDS::DCPS::TransportConfig_rch config = TheTransportRegistry->create_config(config_name);
      OpenDDS::DCPS::TransportInst_rch inst = TheTransportRegistry->create_inst(inst_name, "rtps_udp");
      ACE_Configuration_Heap ach;
      ACE_Configuration_Section_Key sect_key;
      ach.open();
      ach.open_section(ach.root_section(), ACE_TEXT("not_root"), 1, sect_key);
      ach.set_string_value(sect_key, ACE_TEXT("use_multicast"), ACE_TEXT("0"));
      ach.set_string_value(sect_key, ACE_TEXT("nak_depth"), nak_depth);
      ach.set_string_value(sect_key, ACE_TEXT("heartbeat_period"), ACE_TEXT("200"));
      ach.set_string_value(sect_key, ACE_TEXT("heartbeat_response_delay"), ACE_TEXT("100"));
      inst->load(ach, sect_key);
      config->instances_.push_back(inst);
      TheTransportRegistry->bind_config(config_name, participant[i]);
    }
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

    while(allocator->find(obj_name, ptr) != 0) {}
    SharedData* state = reinterpret_cast<SharedData*>(ptr);

    // Indicate that the subscriber is ready
    state->sub_ready = true;

    const ACE_Time_Value small_time(0, 250000);
    // Wait for the publisher to be ready
    do {
      ACE_OS::sleep(small_time);
    } while (!state->pub_ready);

    int expected = num_datawriters * num_instances_per_writer * num_samples_per_instance;

    int prev_reads = 0;
    while (num_reads < expected) {
      if (prev_reads < num_reads.value()) {
        prev_reads = num_reads.value();
        std::cout << "Read " << prev_reads << " samples!" << std::endl;
      }
      // Get the number of the timed out writes from publisher so we
      // can re-calculate the number of expected messages. Otherwise,
      // the blocking timeout test will never exit from this loop.
      if (state->timeout_writes_ready) {
        expected -= state->timeout_writes;
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) timed out writes %d, we expect %d\n"),
                   state->timeout_writes, expected));
        // Make sure timeout_writes is accounted only once
        state->timeout_writes_ready = false;
      }
      ACE_OS::sleep(1);
    }

    // Indicate that the subscriber is done
    state->sub_finished = true;

    // Wait for the publisher to finish
    while (!state->pub_finished) {
      ACE_OS::sleep(small_time);
    }

    delete allocator;
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
