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


#include "Writer.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/PublisherImpl.h"

#include "../TypeNoKeyBounded/Pt128TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt512TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt2048TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt8192TypeSupportImpl.h"

#include "dds/DCPS/transport/framework/EntryExit.h"

#include "ace/Arg_Shifter.h"




#include "common.h"

ACE_Recursive_Thread_Mutex done_lock_;
ACE_Condition<ACE_Recursive_Thread_Mutex> done_condition_(done_lock_);

/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  ACE_Arg_Shifter arg_shifter (argc, argv);
  
  arg_shifter.ignore_arg (); // ignore the command - argv[0]
  while (arg_shifter.is_anything_left ()) 
  {
    // options:
    // -p  <num data writers>
    // -r  <num data readers>
    // -n  <num packets>
    // -d  <data size>
    // -a  <transport address>
    // -t  <max blocking timeout in miliseconds>
    // -msi <max samples per instance>
    // -mxs <max samples>
    // -mxi <max instances>
    // -z  <verbose transport debug>

    const char *currentArg = 0;
    
    if ((currentArg = arg_shifter.get_the_parameter("-p")) != 0) 
    {
      num_datawriters = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    if ((currentArg = arg_shifter.get_the_parameter("-r")) != 0) 
    {
      num_datareaders = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-d")) != 0) 
    {
      DATA_SIZE = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-n")) != 0) 
    {
      NUM_SAMPLES = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-a")) != 0) 
    {
      writer_address_str = currentArg;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-msi")) != 0) 
    {
      MAX_SAMPLES_PER_INSTANCE = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-mxs")) != 0) 
    {
      MAX_SAMPLES = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-mxi")) != 0) 
    {
      MAX_INSTANCES = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-t")) != 0) 
    {
      max_mili_sec_blocking = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-z") == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else 
    {
      ACE_ERROR((LM_ERROR,"(%P|%t) unexpected parameter %s\n", arg_shifter.get_current()));
      return 3;
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}


int main (int argc, char *argv[])
{

  int status = 0;

  ACE_TRY_NEW_ENV
    {
      ACE_DEBUG((LM_INFO," %P|%t %T publisher main\n"));

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
      ACE_TRY_CHECK;

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      status = parse_args (argc, argv);
      if (status) 
        return status;

      ::DDS::DomainParticipant_ptr dp = 
        dpf->create_participant(TEST_DOMAIN, 
                                PARTICIPANT_QOS_DEFAULT, 
                                ::DDS::DomainParticipantListener::_nil() 
                                ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (CORBA::is_nil (dp))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT(" %P|%t ERROR: create_participant failed.\n")));
        return 1 ;
      }

      // Register the type supports
      switch (1 << DATA_SIZE)
      {
      case 128:
        {
          ::Mine::Pt128TypeSupportImpl* pt128ts_servant = new ::Mine::Pt128TypeSupportImpl();
          ::Mine::Pt128TypeSupport_var pt128ts = 
            TAO::DCPS::servant_to_reference< ::Mine::Pt128TypeSupport,
                                            ::Mine::Pt128TypeSupportImpl, 
                                            ::Mine::Pt128TypeSupport_ptr >(pt128ts_servant);
          ACE_TRY_CHECK;

          if (::DDS::RETCODE_OK != pt128ts->register_type(dp, TEST_TYPE))
            {
              ACE_ERROR ((LM_ERROR, 
                          ACE_TEXT (" %P|%t ERROR: Failed to register the Pt128TypeSupport."))); 
              return 1;
            }
          ACE_TRY_CHECK;
        }
        break;
      case 512:
        {
          ::Mine::Pt512TypeSupportImpl* pt512ts_servant = new ::Mine::Pt512TypeSupportImpl();
          ::Mine::Pt512TypeSupport_var pt512ts = 
            TAO::DCPS::servant_to_reference< ::Mine::Pt512TypeSupport,
                                            ::Mine::Pt512TypeSupportImpl, 
                                            ::Mine::Pt512TypeSupport_ptr >(pt512ts_servant);
          ACE_TRY_CHECK;

          if (::DDS::RETCODE_OK != pt512ts->register_type(dp, TEST_TYPE))
            {
              ACE_ERROR ((LM_ERROR, 
                          ACE_TEXT (" %P|%t ERROR: Failed to register the Pt512TypeSupport."))); 
              return 1;
            }
          ACE_TRY_CHECK;
        }
        break;
      case 2048:
        {
          ::Mine::Pt2048TypeSupportImpl* pt2048ts_servant = new ::Mine::Pt2048TypeSupportImpl();
          ::Mine::Pt2048TypeSupport_var pt2048ts = 
            TAO::DCPS::servant_to_reference< ::Mine::Pt2048TypeSupport,
                                            ::Mine::Pt2048TypeSupportImpl, 
                                            ::Mine::Pt2048TypeSupport_ptr >(pt2048ts_servant);
          ACE_TRY_CHECK;

          if (::DDS::RETCODE_OK != pt2048ts->register_type(dp, TEST_TYPE))
            {
              ACE_ERROR ((LM_ERROR, 
                          ACE_TEXT (" %P|%t ERROR: Failed to register the Pt2048TypeSupport."))); 
              return 1;
            }
          ACE_TRY_CHECK;
        }
        break;
      case 8192:
        {
          ::Mine::Pt8192TypeSupportImpl* pt8192ts_servant = new ::Mine::Pt8192TypeSupportImpl();
          ::Mine::Pt8192TypeSupport_var pt8192ts = 
            TAO::DCPS::servant_to_reference< ::Mine::Pt8192TypeSupport,
                                            ::Mine::Pt8192TypeSupportImpl, 
                                            ::Mine::Pt8192TypeSupport_ptr >(pt8192ts_servant);
          ACE_TRY_CHECK;

          if (::DDS::RETCODE_OK != pt8192ts->register_type(dp, TEST_TYPE))
            {
              ACE_ERROR ((LM_ERROR, 
                          ACE_TEXT (" %P|%t ERROR: Failed to register the Pt8192TypeSupport."))); 
              return 1;
            }
          ACE_TRY_CHECK;
        }
      };



      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);
      
      topic_qos.resource_limits.max_samples_per_instance =
            MAX_SAMPLES_PER_INSTANCE;
      topic_qos.resource_limits.max_instances = MAX_INSTANCES;
      topic_qos.resource_limits.max_samples = MAX_SAMPLES;

      topic_qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      topic_qos.reliability.max_blocking_time.sec = max_mili_sec_blocking / 1000;
      topic_qos.reliability.max_blocking_time.nanosec = 
                                   (max_mili_sec_blocking % 1000) * 1000*1000;
      topic_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;

      ::DDS::Topic_ptr topic = 
        dp->create_topic (TEST_TOPIC, 
                          TEST_TYPE, 
                          topic_qos, 
                          ::DDS::TopicListener::_nil()
                          ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (CORBA::is_nil (topic))
      {
        return 1 ;
      }

      // Create the publisher
      ::DDS::Publisher_ptr pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil()
                             ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (CORBA::is_nil (pub))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT(" %P|%t ERROR: create_publisher failed.\n")),
                          1);
      }

      // Initialize the transport
      if (0 != ::init_writer_tranport() )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT(" %P|%t ERROR: init_transport failed!\n")),
                           1);
      }

      // Attach the publisher to the transport.
      ::TAO::DCPS::PublisherImpl* pub_impl 
        = reference_to_servant< ::TAO::DCPS::PublisherImpl,
                                ::DDS::Publisher_ptr>
                              (pub ACE_ENV_SINGLE_ARG_PARAMETER);
        ACE_TRY_CHECK;

      if (0 == pub_impl)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT(" %P|%t ERROR: Failed to obtain servant ::TAO::DCPS::PublisherImpl\n")),
                          1);
      }

      TAO::DCPS::AttachStatus attach_status =
        pub_impl->attach_transport(writer_transport_impl.in());

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

          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT(" %P|%t ERROR: Failed to attach to the transport. ")
                            ACE_TEXT("AttachStatus == %s\n"),
                            status_str.c_str()),
                            1);
        }

      // Create the datawriters
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      pub->copy_from_topic_qos (dw_qos, topic_qos);

      ::DDS::DataWriter_ptr * dws = new ::DDS::DataWriter_ptr[num_datawriters];

      // Create one or multiple datawriters belonging to the same 
      // publisher.
      for (int k = 0; k < num_datawriters; k ++)
      {
        dws[k] = pub->create_datawriter(topic,
                                        dw_qos,
                                        ::DDS::DataWriterListener::_nil()
                                        ACE_ENV_ARG_PARAMETER);
        ACE_TRY_CHECK;

        if (CORBA::is_nil (dws[k]))
        {
          ACE_ERROR ((LM_ERROR,
                     ACE_TEXT(" %P|%t ERROR: create_datawriter failed.\n")));
          return 1 ;
        }
      }


      Writer** writers = new Writer* [num_datawriters] ;

      for (int p = 0; p < num_datawriters; p ++)
      {
        writers[p] = new Writer(dws[p], 
                                NUM_SAMPLES,
                                DATA_SIZE,
                                num_datareaders,
                                p); 
        writers[p]->start ();
      }


      bool writers_finished = false;

      while ( !writers_finished )
        {
          ACE_Guard<ACE_Recursive_Thread_Mutex> just_me(done_lock_);
          // wait for a writer to signal so we done spin
          // waiting to see if the publisher is done.
          //ACE_Time_Value timeout(5,0);
          done_condition_.wait();
          writers_finished = true;
          for (int m = 0; m < num_datawriters; m ++)
            {
              writers_finished = writers_finished && writers[m]->is_finished();
            }
        }

      // Clean up publisher objects
      pub->delete_contained_entities() ;

      delete [] dws;
      delete [] writers;

      dp->delete_publisher(pub ACE_ENV_ARG_PARAMETER);

      dp->delete_topic(topic ACE_ENV_ARG_PARAMETER);
      dpf->delete_participant(dp ACE_ENV_ARG_PARAMETER);

      TheTransportFactory->release();
      TheServiceParticipant->shutdown (); 

    }
  ACE_CATCHANY
    {
      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                           "Exception caught in main.cpp:");
      return 1;
    }
  ACE_ENDTRY;

  return status;
}
