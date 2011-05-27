// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "DataReaderListener.h"
#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "ace/Arg_Shifter.h"

#include "common.h"


::DDS::DomainParticipantFactory_var dpf;
::DDS::DomainParticipant_var participant[2];
::DDS::Topic_var topic[2];
::DDS::Subscriber_var subscriber[2];
OpenDDS::DCPS::TransportImpl_rch reader_impl[2];
::DDS::DataReaderListener_var listener[2];
::DDS::DataReader_var datareader[2];
OpenDDS::DCPS::TransportIdType transport_id[2] = {SUB_TRAFFIC_TCP_1, SUB_TRAFFIC_TCP_2};
ACE_TString reader_address_str[2];

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
    //  -s subscriber transport address    defaults to localhost:23456
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

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-s"))) != 0)
    {
      //Maximum two addresses.
      static size_t i = 0;
      reader_address_str[i] = currentArg;
      i++;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-m"))) != 0)
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
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates sucessful parsing of the command line
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

  reader_impl[i]
    = TheTransportFactory->create_transport_impl (transport_id[i],
                                                  ACE_TEXT("SimpleTcp"),
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);

  OpenDDS::DCPS::TransportConfiguration_rch reader_config
    = TheTransportFactory->create_configuration (transport_id[i], ACE_TEXT("SimpleTcp"));

  OpenDDS::DCPS::SimpleTcpConfiguration* reader_tcp_config
    = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (reader_config.in ());

  if (reader_address_str[i] != ACE_TEXT(""))
  {
    ACE_INET_Addr reader_address (reader_address_str[i].c_str());
    reader_tcp_config->local_address_ = reader_address;
    reader_tcp_config->local_address_str_ = reader_address_str[i].c_str();
  }
  // else use default address - OS assigned.

  if (reader_impl[i]->configure(reader_config.in()) != 0)
  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) init_reader_tranport: subscriber TCP ")
      ACE_TEXT(" Failed to configure the transport.\n")));
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

  // Attach the subscriber to the transport.
  OpenDDS::DCPS::SubscriberImpl* sub_impl
    = dynamic_cast<OpenDDS::DCPS::SubscriberImpl*>(subscriber[i].in ());

  if (0 == sub_impl)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) Failed to obtain subscriber servant\n")));
      throw TestException ();
    }

  OpenDDS::DCPS::AttachStatus attach_status;

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) attach to tcp \n")));
  attach_status = sub_impl->attach_transport(reader_impl[i].in());

  if (attach_status != OpenDDS::DCPS::ATTACH_OK)
    {
      // We failed to attach to the transport for some reason.
      ACE_TString status_str;

      switch (attach_status)
        {
          case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
            status_str = ACE_TEXT("ATTACH_BAD_TRANSPORT");
            break;
          case OpenDDS::DCPS::ATTACH_ERROR:
            status_str = ACE_TEXT("ATTACH_ERROR");
            break;
          case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
            status_str = ACE_TEXT("ATTACH_INCOMPATIBLE_QOS");
            break;
          default:
            status_str = ACE_TEXT("Unknown Status");
            break;
        }

      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) Failed to attach to the transport. ")
                  ACE_TEXT("AttachStatus == %s\n"),
                  status_str.c_str()));
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
  TheTransportFactory->release();

  dpf = 0;
  participant[0] = 0;
  participant[1] = 0;
  topic[0] = 0;
  topic[1] = 0;
  subscriber[0] = 0;
  subscriber[1] = 0;
  reader_impl[0] = 0;
  reader_impl[1] = 0;
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
      ACE_DEBUG((LM_INFO,"(%P|%t) %T subscriber main\n"));

      dpf = TheParticipantFactoryWithArgs(argc, argv);

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
                  fscanf (writers_completed, "%d\n", &timeout_writes);
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
