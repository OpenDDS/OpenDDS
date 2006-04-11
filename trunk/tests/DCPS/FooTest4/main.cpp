// -*- C++ -*-
// ============================================================================
/**
 *  @file   main.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "Reader.h"
#include "Writer.h"
#include "../common/TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportConfiguration.h"
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"

#include "ace/Arg_Shifter.h"


const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = (const char*) "foo";
const char* MY_TYPE     = (const char*) "foo";
const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITY_SEC);

int use_take = 0;
int num_reads_per_thread = 1;
int multiple_instances = 0;
int num_datareaders = 1;
int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 10 ;

enum TransportId {
  ALL_TRAFFIC
};

/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);
  
  while (arg_shifter.is_anything_left ()) 
  {
    // options:
    //  -t use_take?1:0           defaults to 0 
    //  -i num_reads_per_thread   defaults to 1 
    //  -r num_datareaders         defaults to 1 
    //  -m multiple_instances?1:0  defaults to 0
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth           defaults to 1

    const char *currentArg = 0;
    
    if ((currentArg = arg_shifter.get_the_parameter("-t")) != 0) 
    {
      use_take = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-m")) != 0) 
    {
      multiple_instances = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-i")) != 0) 
    {
      num_reads_per_thread = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-r")) != 0) 
    {
      num_datareaders = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-n")) != 0) 
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-d")) != 0) 
    {
      history_depth = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else 
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}


int main (int argc, char *argv[])
{
  ACE_TRY_NEW_ENV
    {
      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
      ACE_TRY_CHECK;

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);


      ::Mine::FooTypeSupportImpl* fts_servant = new ::Mine::FooTypeSupportImpl();
      PortableServer::ServantBase_var safe_servant = fts_servant;

      ::Mine::FooTypeSupport_var fts = 
        TAO::DCPS::servant_to_reference< ::Mine::FooTypeSupport,
                                         ::Mine::FooTypeSupportImpl, 
                                         ::Mine::FooTypeSupport_ptr >(fts_servant);
      ACE_TRY_CHECK;

      ::DDS::DomainParticipant_var dp = 
        dpf->create_participant(MY_DOMAIN, 
                                PARTICIPANT_QOS_DEFAULT, 
                                ::DDS::DomainParticipantListener::_nil() 
                                ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (CORBA::is_nil (dp.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) create_participant failed.\n")));
        return 1 ;
      }

      if (::DDS::RETCODE_OK != fts->register_type(dp.in (), MY_TYPE))
        {
          ACE_ERROR ((LM_ERROR, 
            ACE_TEXT ("Failed to register the FooTypeSupport."))); 
          return 1;
        }

      ACE_TRY_CHECK;

      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);
      
      topic_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      topic_qos.history.depth = history_depth;

      ::DDS::Topic_var topic = 
        dp->create_topic (MY_TOPIC, 
                          MY_TYPE, 
                          topic_qos, 
                          ::DDS::TopicListener::_nil()
                          ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (CORBA::is_nil (topic.in ()))
      {
        return 1 ;
      }
      
      ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                             ::DDS::SubscriberListener::_nil()
                             ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (CORBA::is_nil (sub.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) create_subscriber failed.\n")));
        return 1 ;
      }
      
      // Attach the subscriber to the transport.
      ::TAO::DCPS::SubscriberImpl* sub_impl 
        = ::TAO::DCPS::reference_to_servant< ::TAO::DCPS::SubscriberImpl,
                                             ::DDS::Subscriber_ptr>
                              (sub.in () ACE_ENV_SINGLE_ARG_PARAMETER);
        ACE_TRY_CHECK;

      if (0 == sub_impl)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) Failed to obtain servant ::TAO::DCPS::SubscriberImpl\n")),
                            1);
        }

      TAO::DCPS::TransportImpl_rch transport_impl 
        = TheTransportFactory->create_transport_impl (ALL_TRAFFIC, "SimpleTcp", TAO::DCPS::DONT_AUTO_CONFIG);

      TAO::DCPS::TransportConfiguration_rch config 
        = TheTransportFactory->create_configuration (ALL_TRAFFIC, "SimpleTcp");

      if (transport_impl->configure(config.in ()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    "(%P|%t) Failed to configure the transport impl\n"));
          return 1 ;
        }

      TAO::DCPS::AttachStatus status 
        = sub_impl->attach_transport(transport_impl.in()); 

      if (status != TAO::DCPS::ATTACH_OK)
      {
        // We failed to attach to the transport for some reason.
        std::string status_str;

        switch (status)
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

        ACE_ERROR((LM_ERROR,
                    "(%P|%t) Failed to attach to the transport. "
                    "AttachStatus == %s\n", status_str.c_str()));
        return 1;
      }

      ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

//---------------------------------------------------------------------
//    Test Subscriber unit tests
//
      ::DDS::SubscriberQos sub_qos_got;
      sub->get_qos (sub_qos_got ACE_ENV_ARG_PARAMETER);

      ::DDS::SubscriberQos default_sub_qos;
      dp->get_default_subscriber_qos (default_sub_qos 
                                      ACE_ENV_ARG_PARAMETER);

      ACE_CHECK;

      //The SunOS compiler had problem resolving operator in a namespace. 
      //To resolve the compilation errors, the operator is called explicitly.
      if (!(::TAO::DCPS::operator== (sub_qos_got, default_sub_qos)))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) Subscriber get_default_qos failed.\n")));
        return 1 ;
      }

      ::DDS::SubscriberQos new_sub_qos = sub_qos_got;
      // This qos is not supported, so it's invalid qos.
      new_sub_qos.presentation.access_scope = ::DDS::GROUP_PRESENTATION_QOS;

      ret = sub->set_qos (new_sub_qos ACE_ENV_ARG_PARAMETER);
      if (ret != ::DDS::RETCODE_INCONSISTENT_POLICY)
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) Subscriber set_qos failed.\n")));
        return 1 ;
      }
    
      ::DDS::DomainParticipant_var participant 
        = sub->get_participant (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_CHECK;

      if (dp.in () != participant.in ())
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) Subscriber get_participant failed.\n")));
        return 1 ;
      }

      ::DDS::DataReaderQos default_dr_qos; 
      sub->get_default_datareader_qos (default_dr_qos ACE_ENV_ARG_PARAMETER);
      ACE_CHECK;

      ::DDS::DataReaderQos new_default_dr_qos = default_dr_qos; 
      new_default_dr_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
                  
      if (sub->set_default_datareader_qos(new_default_dr_qos) !=
          ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) Subscriber set_default_datareader_qos failed.\n")));
        return 1 ;
      }

      ::DDS::TopicDescription_var description =
        dp->lookup_topicdescription(MY_TOPIC ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (CORBA::is_nil (description.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n")));
        return 1 ;
      }

      // Create datareeder to test copy_from_topic_qos.
      ::DDS::DataReader_var datareader =
          sub->create_datareader(description.in (),
                                 DATAREADER_QOS_USE_TOPIC_QOS,
                                 ::DDS::DataReaderListener::_nil()
                                 ACE_ENV_ARG_PARAMETER) ;
      ACE_CHECK;
    
      if (CORBA::is_nil (datareader.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) Subscriber create_datareader failed.\n")));
        return 1 ;
      }
   
      ::DDS::DataReaderQos dr_qos_use_topic_qos;
      datareader->get_qos (dr_qos_use_topic_qos ACE_ENV_ARG_PARAMETER);
      ACE_CHECK;

      ::DDS::DataReaderQos copied_from_topic;
      sub->get_default_datareader_qos(copied_from_topic) ;
      ret = sub->copy_from_topic_qos (copied_from_topic, topic_qos);
      ACE_CHECK;
      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) Subscriber copy_from_topic_qos failed.\n")));
        return 1 ;
      }
  
      //The SunOS compiler had problem resolving operator in a namespace. 
      //To resolve the compilation errors, the operator is called explicitly.
      if (!(::TAO::DCPS::operator== (dr_qos_use_topic_qos, copied_from_topic)))
      {
        ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) Subscriber copy_from_topic_qos failed.\n")));
        return 1 ;
      }
  
      ::DDS::TopicDescription_var topic_description_got 
         = datareader->get_topicdescription (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_CHECK;

      if (topic_description_got.in () != description.in ())
      { 
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) datareader get_topicdescription failed.\n")));
        return 1 ;
      }

      ::DDS::Subscriber_var sub_got 
          = datareader->get_subscriber (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_CHECK;

      if (sub_got.in () != sub.in ())
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) datareader get_subscriber failed.\n")));
        return 1 ;
      }
   
      ::DDS::DataReaderQos dr_qos_got;
      datareader->get_qos (dr_qos_got ACE_ENV_ARG_PARAMETER);
      ACE_CHECK;

      ::DDS::DataReaderQos new_dr_qos = dr_qos_got;
     
      new_dr_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
      new_dr_qos.resource_limits.max_samples_per_instance = 2;
      new_dr_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;

      ret = datareader->set_qos (new_dr_qos ACE_ENV_ARG_PARAMETER);
      ACE_CHECK;

      if (ret != ::DDS::RETCODE_INCONSISTENT_POLICY)
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) datareader set_qos failed.\n")));
        return 1 ;
      }
  
      // Delete the datareader.
      sub->delete_datareader (datareader.in () ACE_ENV_ARG_PARAMETER);

//---------------------------------------------------------------------

      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      dr_qos.history.depth = history_depth  ;
      dr_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      ::DDS::DataReader_var * drs = new ::DDS::DataReader_var[num_datareaders];

      // Create one datareader or multiple datareaders belonging to the same 
      // subscriber.
      for (int i = 0; i < num_datareaders; i ++)
      {
        drs[i] = sub->create_datareader(description.in (),
                                        dr_qos,
                                        ::DDS::DataReaderListener::_nil()
                                        ACE_ENV_ARG_PARAMETER);
        ACE_TRY_CHECK;

        if (CORBA::is_nil (drs[i].in ()))
        {
          ACE_ERROR ((LM_ERROR,
                     ACE_TEXT("(%P|%t) create_datareader failed.\n")));
          return 1 ;
        }
      }

      Reader** readers = new Reader* [num_datareaders];
      Writer** writers = new Writer* [num_datareaders] ;
      
//---------------------------------------------------------------------
//
// read/take_next_sample
//

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      // Do the "writes"
      for (int i = 0; i < num_datareaders; i ++)
      {
        writers[i] = new Writer(drs[i].in (), 
                                num_reads_per_thread, 
                                multiple_instances,
                                i + 1); 
        writers[i]->start ();
      }
  }     
  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      // now - do the reads
      for (int i = 0; i < num_datareaders; i ++)
      {
        readers[i] = new Reader(drs[i].in (), 
                                use_take, 
                                num_reads_per_thread, 
                                multiple_instances,
                                i); 
        readers[i]->start ();
      }
  }

      sub->delete_contained_entities() ;
      
  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      for (int i = 0; i < num_datareaders; i ++)
      {
        drs[i] = sub->create_datareader(description.in (),
                                        dr_qos,
                                        ::DDS::DataReaderListener::_nil()
                                        ACE_ENV_ARG_PARAMETER);
        ACE_TRY_CHECK;

        if (CORBA::is_nil (drs[i].in ()))
        {
          ACE_ERROR ((LM_ERROR,
                     ACE_TEXT("(%P|%t) create_datareader failed.\n")));
          return 1 ;
        }
      }
  }
       
  {
      for (int i = 0; i < num_datareaders; i ++)
      {
        delete writers[i];
      }

  }

  {
      for (int i = 0; i < num_datareaders; i ++)
      {
        delete readers[i];
      }

  }
//---------------------------------------------------------------------
//
// read/take_instance
//

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      // write again
      for (int i = 0; i < num_datareaders; i ++)
      {
        writers[i] = new Writer(drs[i].in (), 
                                num_reads_per_thread, 
                                multiple_instances,
                                i + 1); 
        writers[i]->start ();
      }
  }
     
  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      // now - do the reads
      for (int i = 0; i < num_datareaders; i ++)
      {
        readers[i] = new Reader(drs[i].in (), 
                                use_take, 
                                num_reads_per_thread, 
                                multiple_instances,
                                i); 
        readers[i]->start1 ();
      }
  }

      sub->delete_contained_entities() ;
      
  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      for (int i = 0; i < num_datareaders; i ++)
      {
        drs[i] = sub->create_datareader(description.in (),
                                        dr_qos,
                                        ::DDS::DataReaderListener::_nil()
                                        ACE_ENV_ARG_PARAMETER);
        ACE_TRY_CHECK;

        if (CORBA::is_nil (drs[i].in ()))
        {
          ACE_ERROR ((LM_ERROR,
                     ACE_TEXT("(%P|%t) create_datareader failed.\n")));
          return 1 ;
        }
      }
  }
      
  {
      for (int i = 0; i < num_datareaders; i ++)
      {
        delete writers[i];
      }

  }

  {
      for (int i = 0; i < num_datareaders; i ++)
      {
        delete readers[i];
      }

  }
//---------------------------------------------------------------------
//
// loan (via read)/return_loan
//

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      // write again
      for (int i = 0; i < num_datareaders; i ++)
      {
        writers[i] = new Writer(drs[i].in (), 
                                num_reads_per_thread, 
                                0,
                                i + 1); 
        writers[i]->start ();
      }
  }
     
  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      // now - do the reads
      for (int i = 0; i < num_datareaders; i ++)
      {
        readers[i] = new Reader(drs[i].in (), 
                                0, 
                                num_reads_per_thread, 
                                0,
                                i); 
        readers[i]->start2 ();
      }
  }

      delete [] drs;
  
  {
      for (int i = 0; i < num_datareaders; i ++)
      {
        delete writers[i];
      }

  }
   
      delete [] writers;

  {
      for (int i = 0; i < num_datareaders; i ++)
      {
        delete readers[i];
      }

  }

      delete [] readers;

      sub->delete_contained_entities() ;

      dp->delete_subscriber(sub.in () ACE_ENV_ARG_PARAMETER);

      dp->delete_topic(topic.in () ACE_ENV_ARG_PARAMETER);
      dpf->delete_participant(dp.in () ACE_ENV_ARG_PARAMETER);

      TheServiceParticipant->shutdown (); 

    }
  ACE_CATCH (TestException,ex)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
      return 1;
    }
  ACE_CATCHANY
    {
      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                           "Exception caught in main.cpp:");
      return 1;
    }
  ACE_ENDTRY;

  return 0;
}
