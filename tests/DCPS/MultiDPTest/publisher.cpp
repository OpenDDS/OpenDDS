// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "common.h"
#include "Writer.h"
#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/PublisherImpl.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/tcp/TcpInst.h"

#include "ace/Arg_Shifter.h"

::DDS::DomainParticipantFactory_var dpf;
::DDS::DomainParticipant_var participant;
::DDS::Topic_var topic[2];
::DDS::Publisher_var publisher;
OpenDDS::DCPS::TransportImpl_rch writer_impl;
::DDS::DataWriter_var datawriter[2];
Writer* writers[2];
int writer_address_given = 0;
ACE_TString writer_address_str = ACE_TEXT("localhost:0");

/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -i num_samples_per_instance    defaults to 1
    //  -w num_datawriters          defaults to 1
    //  -m num_instances_per_writer defaults to 1
    //  -p pub transport address    defaults to localhost:23456
    //  -z length of float sequence in data type   defaults to 10
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
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-p"))) != 0)
    {
      writer_address_str = currentArg;
      writer_address_given = 1;
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

void init ()
{
  participant
    = dpf->create_participant(domain_id,
                              PARTICIPANT_QOS_DEFAULT,
                              ::DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (participant.in ()))
    {
      ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) create_participant failed.\n")));
      throw TestException ();
    }

  ::Xyz::FooTypeSupportImpl* fts_servant
    = new ::Xyz::FooTypeSupportImpl();

  if (::DDS::RETCODE_OK != fts_servant->register_type(participant.in (), type_name))
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("(%P|%t) register_type failed.\n")));
      throw TestException ();
    }

  ::DDS::TopicQos topic_qos;
  participant->get_default_topic_qos(topic_qos);

  topic[0]
    = participant->create_topic (topic_name[0],
                                  type_name,
                                  topic_qos,
                                  ::DDS::TopicListener::_nil(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (topic[0].in ()))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) create_topic failed.\n")));
      throw TestException ();
    }

  topic[1]
    = participant->create_topic (topic_name[1],
                                  type_name,
                                  topic_qos,
                                  ::DDS::TopicListener::_nil(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (topic[1].in ()))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) create_topic failed.\n")));
      throw TestException ();
    }

  writer_impl
    = TheTransportFactory->create_transport_impl (PUB_TRAFFIC_TCP,
                                                  ACE_TEXT("tcp"),
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);

  OpenDDS::DCPS::TransportInst_rch writer_config
    = TheTransportFactory->create_configuration (PUB_TRAFFIC_TCP, ACE_TEXT("tcp"));

  OpenDDS::DCPS::TcpInst* writer_tcp_config
    = static_cast <OpenDDS::DCPS::TcpInst*> (writer_config.in ());

  if (writer_address_given)
    {
      ACE_INET_Addr writer_address (writer_address_str.c_str ());
      writer_tcp_config->local_address_ = writer_address;
      writer_tcp_config->local_address_str_ = writer_address_str.c_str ();
    }
    // else use default address - OS assigned.

  if (writer_impl->configure(writer_config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) init_writer_tranport: pub TCP")
                ACE_TEXT(" Failed to configure the transport.\n")));
      throw TestException ();
    }

  // Create the default publisher
  publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                ::DDS::PublisherListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (publisher.in ()))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) create_publisher failed.\n")));
      throw TestException ();
    }

  // Attach the publisher to the transport.
  OpenDDS::DCPS::PublisherImpl* pub_impl
    = dynamic_cast<OpenDDS::DCPS::PublisherImpl*>
    (publisher.in ());

  if (0 == pub_impl)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) Failed to obtain publisher servant \n")));
      throw TestException ();
    }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) attach to tcp \n")));
  OpenDDS::DCPS::AttachStatus attach_status
    = pub_impl->attach_transport(writer_impl.in());

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

  // Create the datawriters
  ::DDS::DataWriterQos dw_qos;
  publisher->get_default_datawriter_qos (dw_qos);

  // Make it KEEP_ALL history so we can verify the received
  // data without dropping.
  dw_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
  dw_qos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
  dw_qos.reliability.max_blocking_time.sec = 0;
  dw_qos.reliability.max_blocking_time.nanosec = 0;

  for (int i = 0; i < 2; ++i)
    {
      datawriter[i] = publisher->create_datawriter(topic[i].in (),
                                          dw_qos,
                                          ::DDS::DataWriterListener::_nil(),
                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (datawriter[i].in ()))
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
          throw TestException ();
        }

      writers[i] = new Writer (datawriter[i].in (), i);
    }
}

void shutdown ()
{
  TheTransportFactory->release();
  dpf = ::DDS::DomainParticipantFactory::_nil ();
  participant = ::DDS::DomainParticipant::_nil ();
  topic[0] = ::DDS::Topic::_nil ();
  topic[1] = ::DDS::Topic::_nil ();
  publisher = ::DDS::Publisher::_nil ();
  writer_impl = 0;
  datawriter[0] = ::DDS::DataWriter::_nil ();
  datawriter[1] = ::DDS::DataWriter::_nil ();

  TheServiceParticipant->shutdown ();
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;

  try
    {
      ACE_DEBUG((LM_INFO,"(%P|%t) %T publisher main\n"));

      dpf = TheParticipantFactoryWithArgs(argc, argv);

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      init ();

      // Indicate that the publisher is ready
      FILE* writers_ready = ACE_OS::fopen (pub_ready_filename.c_str (), ACE_TEXT("w"));
      if (writers_ready == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to create publisher ready file\n")));
        }

      // Wait for the subscriber to be ready.
      FILE* readers_ready = 0;
      do
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
          readers_ready = ACE_OS::fopen (sub_ready_filename.c_str (), ACE_TEXT("r"));
        } while (0 == readers_ready);

      ACE_OS::fclose(writers_ready);
      ACE_OS::fclose(readers_ready);

      // ensure the associations are fully established before writing.
      ACE_OS::sleep(3);

      {  // Extra scope for VC6
        for (int i = 0; i < num_datawriters; i ++)
          {
            writers[i]->start ();
          }
      }

      int timeout_writes = 0;
      bool writers_finished = false;

      while ( !writers_finished )
        {
          writers_finished = true;
          for (int i = 0; i < num_datawriters; i ++)
            {
              writers_finished = writers_finished && writers[i]->is_finished();
            }
        }

      {  // Extra scope for VC6
        for (int i = 0; i < num_datawriters; i ++)
          {
            timeout_writes += writers[i]->get_timeout_writes();
          }
      }
      // Indicate that the publisher is done
      FILE* writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_TEXT("w"));
      if (writers_completed == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Unable to i publisher completed file\n")));
        }
      else
        {
          ACE_OS::fprintf (writers_completed, "%d\n", timeout_writes);
        }

      // Wait for the subscriber to finish.
      FILE* readers_completed = 0;
      do
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
          readers_completed = ACE_OS::fopen (sub_finished_filename.c_str (), ACE_TEXT("r"));
        } while (0 == readers_completed);

      ACE_OS::fclose(writers_completed);
      ACE_OS::fclose(readers_completed);

      {  // Extra scope for VC6
        for (int i = 0; i < num_datawriters; i ++)
          {
            writers[i]->end ();
            delete writers[i];
          }
      }
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
      if (! CORBA::is_nil (participant.in ()))
        {
          participant->delete_contained_entities();
        }

      if (! CORBA::is_nil (dpf.in ()))
        {
          dpf->delete_participant(participant.in ());
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
