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
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "ace/Arg_Shifter.h"

#include "common.h"

#include <cstdio>


::DDS::DomainParticipantFactory_var dpf;
::DDS::DomainParticipant_var participant[2];
::DDS::Topic_var topic[2];
::DDS::Subscriber_var subscriber[2];
OpenDDS::DCPS::TransportImpl_rch reader_impl[2];
::DDS::DataReaderListener_var listener[2];
::DDS::DataReader_var datareader[2];

/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -d history.depth            defaults to 1
    //  -m num_instances_per_writer defaults to 1
    //  -i num_samples_per_instance defaults to 1
    //  -z length of float sequence in data type   defaults to 10
    //  -y read operation interval                 defaults to 0
    //  -k data type has no key flag               defaults to 0 - has key
    //  -f mixed transport test flag               defaults to 0 - single transport test
    //  -o directory of synch files used to coordinate publisher and subscriber
    //                              defaults to current directory.
    //  -v                          verbose transport debug

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-m"))) != 0)
    {
      num_instances_per_writer = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0)
    {
      num_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-v")) == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-o"))) != 0)
    {
      synch_file_dir = currentArg;
      pub_ready_filename = synch_file_dir + pub_ready_filename;
      pub_finished_filename = synch_file_dir + pub_finished_filename;
      sub_ready_filename = synch_file_dir + sub_ready_filename;
      sub_finished_filename = synch_file_dir + sub_finished_filename;

      arg_shifter.consume_arg ();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}


void init_dcps_objects (int i)
{
  participant[i] =
    dpf->create_participant(domain_id,
                            PARTICIPANT_QOS_DEFAULT,
                            ::DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (participant[i].in ()))
    {
      ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) create_participant failed.\n")));
      throw TestException ();
    }

  ::Xyz::FooTypeSupportImpl* fts_servant
    = new ::Xyz::FooTypeSupportImpl();

  ::Xyz::FooTypeSupportImpl* another_fts_servant
    = new ::Xyz::FooTypeSupportImpl();

  if (::DDS::RETCODE_OK != fts_servant->register_type(participant[i].in (), type_name))
  {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT ("Failed to register the FooNoTypeTypeSupport.")));
    throw TestException ();
  }

  // Test if different TypeSupport instances of the same TypeSupport type can register
  // with the same type name within the same domain participant.
  if (::DDS::RETCODE_OK != another_fts_servant->register_type(participant[i].in (), type_name))
  {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT ("Failed to register the FooNoTypeTypeSupport.")));
    throw TestException ();
  }

  ::DDS::TopicQos topic_qos;
  participant[i]->get_default_topic_qos(topic_qos);

  topic[i]
    = participant[i]->create_topic(topic_name[i],
                                type_name,
                                topic_qos,
                                ::DDS::TopicListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (topic[i].in ()))
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("Failed to create_topic.")));
      throw TestException ();
    }

  subscriber[i] = participant[i]->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                          ::DDS::SubscriberListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (subscriber[i].in ()))
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("Failed to create_subscriber.")));
      throw TestException ();
    }

  // Create the Datareaders
  ::DDS::DataReaderQos dr_qos;
  subscriber[i]->get_default_datareader_qos (dr_qos);

  ::DDS::TopicDescription_var description
    = participant[i]->lookup_topicdescription(topic_name[i]);
  // create the datareader.
  datareader[i] = subscriber[i]->create_datareader(description.in (),
                                            dr_qos,
                                            listener[i].in (),
                                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil (datareader[i].in ()))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) create_datareader failed.\n")));
      throw TestException ();
    }
}


void init_listener()
{
  for (int i = 0; i < 2; ++i)
  {
    listener[i] = new DataReaderListenerImpl();

    if (CORBA::is_nil (listener[i].in ()))
      {
        ACE_ERROR ((LM_ERROR, ACE_TEXT ("(%P|%t) listener is nil.")));
        throw TestException ();
      }
  }
}

void shutdown ()
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

  TheServiceParticipant->shutdown ();
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;

  try
    {

      dpf = TheParticipantFactoryWithArgs(argc, argv);
      ACE_DEBUG((LM_INFO,"(%P|%t) %T subscriber main\n"));

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      init_listener ();
      init_dcps_objects (0);
      init_dcps_objects (1);

      // Indicate that the subscriber is ready
      FILE* readers_ready = ACE_OS::fopen (sub_ready_filename.c_str (), ACE_TEXT("w"));
      if (readers_ready == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
        }

      // Wait for the publisher to be ready
      FILE* writers_ready = 0;
      do
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
          writers_ready = ACE_OS::fopen (pub_ready_filename.c_str (), ACE_TEXT("r"));
        } while (0 == writers_ready);

      ACE_OS::fclose(readers_ready);
      ACE_OS::fclose(writers_ready);

      int expected
        = num_datawriters * num_instances_per_writer * num_samples_per_instance;

      FILE* writers_completed = 0;
      int timeout_writes = 0;

      while ( num_reads < expected)
        {
          // Get the number of the timed out writes from publisher so we
          // can re-calculate the number of expected messages. Otherwise,
          // the blocking timeout test will never exit from this loop.
          if (writers_completed == 0)
            {
              writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_TEXT("r"));
              if (writers_completed != 0)
                {
                  //writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_TEXT("r"));
                  std::fscanf (writers_completed, "%d\n", &timeout_writes);
                  expected -= timeout_writes;
                  ACE_DEBUG((LM_DEBUG,
                             ACE_TEXT ("(%P|%t) timed out writes %d, we expect %d\n"),
                             timeout_writes, expected));
                }

            }
          ACE_OS::sleep (1);
        }

      // Indicate that the subscriber is done
      FILE* readers_completed = ACE_OS::fopen (sub_finished_filename.c_str (), ACE_TEXT("w"));
      if (readers_completed == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber completed file\n")));
        }

      // Wait for the publisher to finish
      while (writers_completed == 0)
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
          writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_TEXT("r"));
        }

      ACE_OS::fclose(readers_completed);
      ACE_OS::fclose(writers_completed);
    }
  catch (const TestException&)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main (). ")));
      status = 1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main ():");
      status = 1;
    }

  try
    {
      for (int i = 0; i < 2; ++i)
      {
        if (! CORBA::is_nil (participant[i].in ()))
          {
            participant[i]->delete_contained_entities();
          }
        if (! CORBA::is_nil (dpf.in ()))
          {
            dpf->delete_participant(participant[i].in ());
          }
      }
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in cleanup.");
      status = 1;
    }

  shutdown ();

  return status;
}
