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
#include "tests/DCPS/FooType5/FooTypeSupportImpl.h"
#include "tests/DCPS/FooType5/FooNoKeyTypeSupportImpl.h"
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
TAO::DCPS::TransportImpl_rch reader_impl[2];
::DDS::DataReaderListener_var listener[2];
::DDS::DataReader_var datareader[2];
TransportIdType transport_id[2] = {SUB_TRAFFIC_TCP_1, SUB_TRAFFIC_TCP_2};
ACE_TString reader_address_str[2];

/// parse the command line arguments
int parse_args (int argc, char *argv[])
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

    const char *currentArg = 0;
    
    if ((currentArg = arg_shifter.get_the_parameter("-s")) != 0) 
    {
      //Maximum two addresses.
      static size_t i = 0;
      reader_address_str[i] = currentArg;
      i++;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-m")) != 0) 
    {
      num_instances_per_writer = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-i")) != 0) 
    {
      num_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-v") == 0)
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
                            ::DDS::DomainParticipantListener::_nil() 
                            ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;
  if (CORBA::is_nil (participant[i].in ()))
    {
      ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) create_participant failed.\n")));
      ACE_THROW (TestException ()); 
    }

  ::Mine::FooTypeSupportImpl* fts_servant 
    = new ::Mine::FooTypeSupportImpl();
  PortableServer::ServantBase_var safe_servant = fts_servant;

  ::Mine::FooTypeSupportImpl* another_fts_servant 
    = new ::Mine::FooTypeSupportImpl();
  PortableServer::ServantBase_var another_safe_servant = another_fts_servant;

  if (::DDS::RETCODE_OK != fts_servant->register_type(participant[i].in (), type_name))
  {
    ACE_ERROR ((LM_ERROR, 
      ACE_TEXT ("Failed to register the FooNoTypeTypeSupport."))); 
    ACE_THROW (TestException ());      
  }

  // Test if different TypeSupport instances of the same TypeSupport type can register
  // with the same type name within the same domain participant.
  if (::DDS::RETCODE_OK != another_fts_servant->register_type(participant[i].in (), type_name))
  {
    ACE_ERROR ((LM_ERROR, 
      ACE_TEXT ("Failed to register the FooNoTypeTypeSupport."))); 
    ACE_THROW (TestException ());      
  }

  ::DDS::TopicQos topic_qos;
  participant[i]->get_default_topic_qos(topic_qos);
  
  topic[i] 
    = participant[i]->create_topic(topic_name[i], 
                                type_name, 
                                topic_qos, 
                                ::DDS::TopicListener::_nil()
                                ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;
  if (CORBA::is_nil (topic[i].in ()))
    {
      ACE_ERROR ((LM_ERROR, 
        ACE_TEXT ("Failed to create_topic."))); 
      ACE_THROW (TestException ());      
    }

  reader_impl[i] 
    = TheTransportFactory->create_transport_impl (transport_id[i],
                                                  "SimpleTcp", 
                                                  TAO::DCPS::DONT_AUTO_CONFIG);

  TAO::DCPS::TransportConfiguration_rch reader_config 
    = TheTransportFactory->create_configuration (transport_id[i], "SimpleTcp");

  TAO::DCPS::SimpleTcpConfiguration* reader_tcp_config 
    = static_cast <TAO::DCPS::SimpleTcpConfiguration*> (reader_config.in ());

  if (reader_address_str[i] != "")
  {
    ACE_INET_Addr reader_address (reader_address_str[i].c_str());
    reader_tcp_config->local_address_ = reader_address;
  }
  // else use default address - OS assigned.

  if (reader_impl[i]->configure(reader_config.in()) != 0)
  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) init_reader_tranport: subscriber TCP ")
      ACE_TEXT(" Failed to configure the transport.\n")));
    ACE_THROW (TestException ()); 
  }

  subscriber[i] = participant[i]->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                          ::DDS::SubscriberListener::_nil()
                          ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;
  if (CORBA::is_nil (subscriber[i].in ()))
    {
      ACE_ERROR ((LM_ERROR, 
        ACE_TEXT ("Failed to create_subscriber."))); 
      ACE_THROW (TestException ()); 
    }

  // Attach the subscriber to the transport.
  ::TAO::DCPS::SubscriberImpl* sub_impl 
    = reference_to_servant< ::TAO::DCPS::SubscriberImpl,
                            ::DDS::Subscriber_ptr>
                          (subscriber[i].in () ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_TRY_CHECK;

  if (0 == sub_impl)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) Failed to obtain subscriber servant\n")));
      ACE_THROW (TestException ()); 
    }

  TAO::DCPS::AttachStatus attach_status;

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) attach to tcp \n")));
  attach_status = sub_impl->attach_transport(reader_impl[i].in());

  if (attach_status != TAO::DCPS::ATTACH_OK)
    {
      // We failed to attach to the transport for some reason.
      std::string status_str;

      switch (attach_status)
        {
          case TAO::DCPS::ATTACH_BAD_TRANSPORT:
            status_str = "ATTACH_BAD_TRANSPORT";
            break;
          case TAO::DCPS::ATTACH_ERROR:
            status_str = "ATTACH_ERROR";
            break;
          case TAO::DCPS::ATTACH_INCOMPATIBLE_QOS:
            status_str = "ATTACH_INCOMPATIBLE_QOS";
            break;
          default:
            status_str = "Unknown Status";
            break;
        }

      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) Failed to attach to the transport. ")
                  ACE_TEXT("AttachStatus == %s\n"),
                  status_str.c_str()));
      ACE_THROW (TestException ()); 
    }
    
        // Create the Datareaders
    ::DDS::DataReaderQos dr_qos;
    subscriber[i]->get_default_datareader_qos (dr_qos);

    ::DDS::TopicDescription_var description
      = participant[i]->lookup_topicdescription(topic_name[i] 
                                                ACE_ENV_ARG_PARAMETER);
    ACE_TRY_CHECK;
    // create the datareader.
    datareader[i] = subscriber[i]->create_datareader(description.in (),
                                              dr_qos,
                                              listener[i].in ()
                                              ACE_ENV_ARG_PARAMETER);
    ACE_TRY_CHECK;

    if (CORBA::is_nil (datareader[i].in ()))
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) create_datareader failed.\n")));
        ACE_THROW (TestException ()); 
      }  
}


void init_listener()
{
  for (int i = 0; i < 2; ++i)
  {
    DataReaderListenerImpl* listener_servant = new DataReaderListenerImpl();
    PortableServer::ServantBase_var safe_servant = listener_servant;

    PortableServer::POA_var poa 
      = TheServiceParticipant->the_poa ();

    CORBA::Object_var obj
      = poa->servant_to_reference(listener_servant
                                  ACE_ENV_ARG_PARAMETER);
    ACE_TRY_CHECK;

    listener[i]  
      = ::DDS::DataReaderListener::_narrow (obj.in ()
                                            ACE_ENV_ARG_PARAMETER);
    ACE_TRY_CHECK;

    if (CORBA::is_nil (listener[i].in ()))
      {
        ACE_ERROR ((LM_ERROR, ACE_TEXT ("(%P|%t) listener is nil."))); 
        ACE_THROW (TestException ());      
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


int main (int argc, char *argv[])
{
  int status = 0;

  ACE_TRY_NEW_ENV
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
      FILE* readers_ready = ACE_OS::fopen (sub_ready_filename.c_str (), ACE_LIB_TEXT("w"));
      if (readers_ready == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR Unable to create subscriber completed file\n")));
        }

      // Wait for the publisher to be ready
      FILE* writers_ready = 0;
      do
        {
          ACE_Time_Value small(0,250000);
          ACE_OS::sleep (small);
          writers_ready = ACE_OS::fopen (pub_ready_filename.c_str (), ACE_LIB_TEXT("r"));
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
              writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_LIB_TEXT("r"));          
              if (writers_completed != 0)
                { 
                  //writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_LIB_TEXT("r"));          
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
      FILE* readers_completed = ACE_OS::fopen (sub_finished_filename.c_str (), ACE_LIB_TEXT("w"));
      if (readers_completed == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR Unable to create subscriber completed file\n")));
        }

      // Wait for the publisher to finish
      while (writers_completed == 0)
        {
          ACE_Time_Value small(0,250000);
          ACE_OS::sleep (small);
          writers_completed = ACE_OS::fopen (pub_finished_filename.c_str (), ACE_LIB_TEXT("r"));
        } 

      ACE_OS::fclose(readers_completed);
      ACE_OS::fclose(writers_completed);
    }
  ACE_CATCH (TestException,ex)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main (). ")));
      status = 1;
    }
  ACE_CATCHANY
    {
      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                           "Exception caught in main ():");
      status = 1;
    }
  ACE_ENDTRY;

  ACE_TRY_NEW_ENV
    {
      for (int i = 0; i < 2; ++i)
      {
        if (! CORBA::is_nil (participant[i].in ()))
          {
            participant[i]->delete_contained_entities(ACE_ENV_SINGLE_ARG_PARAMETER);
            ACE_TRY_CHECK;
          }
        if (! CORBA::is_nil (dpf.in ()))
          {
            dpf->delete_participant(participant[i].in () ACE_ENV_ARG_PARAMETER);
            ACE_TRY_CHECK;
          }
      }
    }
  ACE_CATCHANY
    {
      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
        "Exception caught in cleanup.");
      status = 1;
    }
  ACE_ENDTRY;

  shutdown ();

  return status;
}
