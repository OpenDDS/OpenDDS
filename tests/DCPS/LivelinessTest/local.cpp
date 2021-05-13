// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================


#include "Writer.h"
#include "../common/TestException.h"
#include "DataReaderListener.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include "dds/DCPS/transport/udp/Udp.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"
#include "ace/OS_NS_unistd.h"

#include "common.h"


class ReactorCtrl : public ACE_Event_Handler
{
public:
  ReactorCtrl() : cond_(lock_) {}

  int handle_timeout (const ACE_Time_Value &tv,
                      const void *arg)
  {
    ACE_UNUSED_ARG(tv);
    ACE_UNUSED_ARG(arg);

    // it appears that you must have the lock before waiting or signaling on Win32
    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                      guard,
                      this->lock_,
                      -1);

    return cond_.wait();
  }

  void pause()
  {
    ACE_Reactor_Timer_Interface* reactor = TheServiceParticipant->timer();

    if (reactor->schedule_timer(this,
                                0,
                                ACE_Time_Value(0,1)) == -1)
    {
      ACE_ERROR ((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: PauseReactor, ")
                 ACE_TEXT(" %p.\n"), ACE_TEXT("schedule_timer")));
    }
  }

  void resume()
  {
    // it appears that you must have the lock before waiting or signaling on Win32
    ACE_GUARD (ACE_Recursive_Thread_Mutex,
               guard,
               this->lock_);
    cond_.signal();
  }

private:
  ACE_Recursive_Thread_Mutex lock_;
  ACE_Condition<ACE_Recursive_Thread_Mutex> cond_;
} ;


/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -i num_ops_per_thread       defaults to 1
    //  -l num_unlively_periods     defaults to 10
    //  -w num_datawriters          defaults to 1
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -z                          verbose transport debug
    //  -T                          prefix for temporary files

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0)
    {
      num_ops_per_thread = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-l"))) != 0)
    {
      num_unlively_periods = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))) != 0)
    {
      history_depth = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-T"))) == 0)
    {
      temp_file_prefix = currentArg;
      arg_shifter.consume_arg();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{

  int status = 0;

  try
    {
      ACE_DEBUG((LM_INFO,"(%P|%t) %T publisher main\n"));

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      ::Xyz::FooTypeSupport_var fts(new ::Xyz::FooTypeSupportImpl);

      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
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


      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);

      topic_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      topic_qos.history.depth = history_depth;

      ::DDS::Topic_var topic =
        dp->create_topic (MY_TOPIC,
                          MY_TYPE,
                          topic_qos,
                          ::DDS::TopicListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ()))
      {
        return 1 ;
      }

      // Create the publisher
      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) create_publisher failed.\n")),
                          1);
      }

      // Create the datawriters
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.history.depth = history_depth  ;
      dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;
      dw_qos.liveliness.kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC ;
      dw_qos.liveliness.lease_duration.nanosec = 0 ;

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                  dw_qos,
                                  ::DDS::DataWriterListener::_nil(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dw.in ()))
        {
          ACE_ERROR ((LM_ERROR,
                     ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
          return 1 ;
        }

      ::DDS::TopicDescription_var description =
        dp->lookup_topicdescription(MY_TOPIC);
      if (CORBA::is_nil (description.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n")),
                           1);
      }

      // Create the subscriber
      ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                             ::DDS::SubscriberListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) create_subscriber failed.\n")),
                           1);
      }

      // Create the Datareaders
      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      dr_qos.history.depth = history_depth  ;
      dr_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;
      dr_qos.liveliness.kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
      dr_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC ;
      dr_qos.liveliness.lease_duration.nanosec = 0 ;

      ::DDS::DataReaderListener_var drl (new DataReaderListenerImpl);
      DataReaderListenerImpl* drl_servant =
        dynamic_cast<DataReaderListenerImpl*>(drl.in());

      if (!drl_servant) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: drl_servant is nil (dynamic_cast failed)!\n")), -1);
      }

      ::DDS::DataReader_var dr ;

      dr = sub->create_datareader(description.in (),
                                  dr_qos,
                                  drl.in (),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      

      // ensure that the connection and association has been fully established
      ACE_OS::sleep(5);  //TBD remove this kludge when the transport is fixed.

      // stop the Service_Participant reactor so LIVELINESS.kind=AUTOMATIC does not
      // send out an automatic liveliness control message when sleeping in the loop
      // below.

      Writer* writer = new Writer(dw.in (),
                                1,
                                num_ops_per_thread);

      for (int i = 0 ; i < num_unlively_periods ; i++)
        {
          writer->run_test (i);

          // 3 ensures that we will detect when an DataReader detects
          // liveliness lost on an already unliveliy DataReader.
          ACE_OS::sleep (3 * LEASE_DURATION_SEC);
        }
      writer->run_test (num_unlively_periods);

      // We need to wait for liveliness to go away here.
      //

      // We need to wait for liveliness to go away here.
      //
      // clean up subscriber objects

      sub->delete_contained_entities() ;

      dp->delete_subscriber(sub.in ());

      // Clean up publisher objects
      pub->delete_contained_entities() ;

      delete writer;

      dp->delete_publisher(pub.in ());

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      ACE_OS::sleep(5);

      //
      // Determine the test status at this point.
      //
      ACE_OS::fprintf (stderr, "**********\n") ;
      ACE_OS::fprintf (stderr, "drl_servant->liveliness_changed_count() = %d\n",
                     drl_servant->liveliness_changed_count()) ;
      ACE_OS::fprintf (stderr, "drl_servant->no_writers_generation_count() = %d\n",
                     drl_servant->no_writers_generation_count()) ;
      ACE_OS::fprintf (stderr, "********** use_take=%d\n", use_take) ;

      if( drl_servant->liveliness_changed_count() < 2 + 2 * num_unlively_periods) {
        status = 1;
        // Some error condition.
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: subscriber - ")
          ACE_TEXT("test failed first condition.\n")
        ));

      } else if( drl_servant->verify_last_liveliness_status () == false) {
        status = 1;
        // Some other error condition.
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: subscriber - ")
          ACE_TEXT("test failed second condition.\n")
        ));

      } else if( drl_servant->no_writers_generation_count() != num_unlively_periods) {
        status = 1;
        // Yet another error condition.

        // Using take will remove the instance and instance state will be
        // reset for any subsequent samples sent.  Since there are no
        // more samples sent, the information available from the listener
        // retains that from the last read sample rather than the reset
        // value for an (as yet unreceived) next sample.
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: subscriber - ")
          ACE_TEXT("test failed third condition.\n")
        ));
      }

      ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %T publisher is finish - cleanup subscriber\n") ));

      TheServiceParticipant->shutdown ();

    }
  catch (const TestException&)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
      return 1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main.cpp:");
      return 1;
    }

  return status;
}
