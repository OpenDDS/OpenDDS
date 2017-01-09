// -*- C++ -*-
// ============================================================================
/**
 *  @file   main.cpp
 *
 *
 *
 */
// ============================================================================


#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/WaitSet.h"

#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_unistd.h"

#include <string>

const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITE_SEC);

int use_take = 0;
int multiple_instances = 0;
int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 10 ;
bool support_client_side_BIT = false;

// default to using TCP
int sub_using_udp = 0;
int pub_using_udp = 0;
int sub_using_rtps = 0;
int pub_using_rtps = 0;
int sub_using_shmem = 0;
int pub_using_shmem = 0;


int wait_for_data (::DDS::Subscriber_ptr sub,
                   int timeout_sec)
{
  const int factor = 10;
  ACE_Time_Value small_time(0,1000000/factor);
  int timeout_loops = timeout_sec * factor;

  ::DDS::DataReaderSeq_var discard = new ::DDS::DataReaderSeq(10);
  while (timeout_loops-- > 0)
    {
      sub->get_datareaders (
                    discard.inout (),
                    ::DDS::NOT_READ_SAMPLE_STATE,
                    ::DDS::ANY_VIEW_STATE,
                    ::DDS::ANY_INSTANCE_STATE );
      if (discard->length () > 0)
        return 1;

      ACE_OS::sleep (small_time);
    }
  return 0;
}

/// parse the command line arguments
int parse_args(int argc, ACE_TCHAR *argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left())
  {
    // options:
    //  -t use_take?1:0             defaults to 0
    //  -m multiple_instances?1:0   defaults to 0
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -z                          verbose transport debug
    //  -b                          enable client side Built-In topic support
    //  -us                         Subscriber using UDP transport
    //  -up                         Publisher using UDP transport
    //  -rs                         Subscriber using RTPS transport
    //  -rp                         Publisher using RTPS transport
    //  -ss                         Subscriber using Shared Memory transport
    //  -sp                         Publisher using Shared Memory transport

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-t"))) != 0)
    {
      use_take = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-m"))) != 0)
    {
      multiple_instances = ACE_OS::atoi (currentArg);
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
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-b")) == 0)
    {
      support_client_side_BIT = true;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-us")) == 0)
    {
      ACE_DEBUG((LM_DEBUG, "Subscriber Using UDP transport.\n"));
      sub_using_udp = 1;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-up")) == 0)
    {
      ACE_DEBUG((LM_DEBUG, "Publisher Using UDP transport.\n"));
      pub_using_udp = 1;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-rs")) == 0)
    {
      ACE_DEBUG((LM_DEBUG, "Subscriber Using RTPS transport.\n"));
      sub_using_rtps = 1;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-rp")) == 0)
    {
      ACE_DEBUG((LM_DEBUG, "Publisher Using RTPS transport.\n"));
      pub_using_rtps = 1;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-ss")) == 0)
    {
      ACE_DEBUG((LM_DEBUG, "Subscriber Using Shared Memory transport.\n"));
      sub_using_shmem = 1;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-sp")) == 0)
    {
      ACE_DEBUG((LM_DEBUG, "Publisher Using Shared Memory transport.\n"));
      pub_using_shmem = 1;
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


int run_test(int argc, ACE_TCHAR *argv[])
{

  int test_failed = 0;

  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;

  ACE_DEBUG((LM_DEBUG,"(%P|%t) FooTest5_0 main\n"));
  try
    {
      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
      if (CORBA::is_nil (dpf.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) creating the DomainParticipantFactory failed.\n")));
        return 1 ;
      }

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      ::Xyz::FooTypeSupport_var fts (new ::Xyz::FooTypeSupportImpl);

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

      // Attach the subscriber to the transport.
      if (sub_using_udp) {
        TheTransportRegistry->bind_config("udp", sub.in());
      } else if (sub_using_rtps) {
        TheTransportRegistry->bind_config("rtps", sub.in());
      } else if (sub_using_shmem) {
        TheTransportRegistry->bind_config("shmem", sub.in());
      } else {
        TheTransportRegistry->bind_config("tcp", sub.in());
      }

      // Attach the publisher to the transport.
      if (pub_using_udp) {
        TheTransportRegistry->bind_config("udp", pub.in());
      } else if (pub_using_rtps) {
        TheTransportRegistry->bind_config("rtps", pub.in());
      } else if (pub_using_shmem) {
        TheTransportRegistry->bind_config("shmem", pub.in());
      } else {
        TheTransportRegistry->bind_config("tcp", pub.in());
      }

      // Create the datawriter
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.history.depth = history_depth  ;
      dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

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

      // Create the Datareader
      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      dr_qos.history.depth = history_depth  ;
      dr_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      ::DDS::DataReader_var dr
        = sub->create_datareader(description.in (),
                                 dr_qos,
                                 ::DDS::DataReaderListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dr.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            ACE_TEXT("(%P|%t) create_datareader failed.\n")),
            1);
        }

      ::Xyz::FooDataWriter_var foo_dw
           = ::Xyz::FooDataWriter::_narrow(dw.in ());
      if (CORBA::is_nil (foo_dw.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) ::Xyz::FooDataWriter::_narrow failed.\n")));
        return 1; // failure
      }

      ::Xyz::FooDataReader_var foo_dr
        = ::Xyz::FooDataReader::_narrow(dr.in ());
      if (CORBA::is_nil (foo_dr.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) ::Xyz::FooDataReader::_narrow failed.\n")));
        return 1; // failure
      }

      // wait for association establishment before writing.
      ACE_OS::sleep(5); //REMOVE if not needed


      // =============== do the test ====


      ::DDS::OfferedIncompatibleQosStatus incomp;
      if (foo_dw->get_offered_incompatible_qos_status (incomp) != ::DDS::RETCODE_OK)
      {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT ("ERROR: failed to get offered incompatible qos status\n")),
          1);
      }

      // TODO: Make this test work (incompatible transports) with the
      //       new transport scheme.
      int incompatible_transport_found = 0;
      for (CORBA::ULong ii =0; ii < incomp.policies.length (); ii++)
        {
          if (incomp.policies[ii].policy_id
                        == ::OpenDDS::TRANSPORTTYPE_QOS_POLICY_ID)
            incompatible_transport_found = 1;
        }

      ::DDS::InstanceHandle_t handle;

      if (pub_using_udp != sub_using_udp)
        {
          if (!incompatible_transport_found)
            ACE_ERROR_RETURN((LM_ERROR,
              "TEST ERROR: Expected offered_incompatible_qos"
              " with TRANSPORTTYPE_QOS_POLICY_ID"
              " but did not get it. %d incompatible_qos values.\n",
              incomp.policies.length ()),
              7);
          else
            {
              ACE_DEBUG((LM_DEBUG, "Got expected offered_incompatible_qos"
                " with TRANSPORTTYPE_QOS_POLICY_ID"
                " existing with success.\n"));
              return 0;
            }
        }
      else
        {
          if (incompatible_transport_found)
            ACE_ERROR_RETURN((LM_ERROR,
              "TEST ERROR: Did not expect offered_incompatible_qos"
              " with TRANSPORTTYPE_QOS_POLICY_ID"
              " but got it. %d incompatible_qos values.\n",
              incomp.policies.length ()),
              8);
        }

      DDS::WaitSet_var ws = new DDS::WaitSet;
      DDS::StatusCondition_var sc = foo_dr->get_statuscondition();
      ws->attach_condition(sc);

      DDS::ConditionSeq active;
      const DDS::Duration_t infinite = { DDS::DURATION_INFINITE_SEC,
                                         DDS::DURATION_INFINITE_NSEC };
      DDS::SubscriptionMatchedStatus matched;
      while (foo_dr->get_subscription_matched_status(matched) ==
             DDS::RETCODE_OK && matched.total_count == 0) {
        ws->wait(active, infinite);
      }
      ws->detach_condition(sc);

      ::Xyz::Foo foo;
      foo.key = 10101;
      foo.x = -1;
      foo.y = -1;

      handle = foo_dw->register_instance(foo);

      foo.x = 7;

      foo_dw->write(foo, handle);

      ::Xyz::Foo sample;
      ::DDS::SampleInfo info ;

      // wait for new data for upto 10 seconds
      if (!wait_for_data(sub.in (), 5))
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")),
                           1);

      DDS::ReturnCode_t status  ;
      status = foo_dr->read_next_sample(sample, info) ;

      if (status == ::DDS::RETCODE_OK)
      {
        ACE_DEBUG((LM_DEBUG,
            "read foo.x = %f foo.y = %f, foo.key = %d\n",
            sample.x, sample.y, sample.key));

        ACE_DEBUG((LM_DEBUG,
            "read SampleInfo.sample_rank = %d\n",
            info.sample_rank));
        if (foo.x != sample.x)
          {
            ACE_ERROR((LM_ERROR,
              "ERROR: unexpected foo value wrote %f got %f\n",
              foo.x, sample.x));
            test_failed = 1;
          }
      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: reader received ::DDS::RETCODE_NO_DATA!\n")
                    ));
        test_failed = 1;
      }
      else
      {
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: unexpected status %d!\n"),
            status ));
        test_failed = 1;
      }



      //======== clean up ============

      // Clean up publisher objects
//      pub->delete_contained_entities() ;

      pub->delete_datawriter(dw.in ());
      dp->delete_publisher(pub.in ());


      //clean up subscriber objects
//      sub->delete_contained_entities() ;

      sub->delete_datareader(dr.in ());
      dp->delete_subscriber(sub.in ());

      // clean up common objects
      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());
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
  return test_failed;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[]) {
  const int test_failed = run_test(argc, argv);
  TheServiceParticipant->shutdown();
  return test_failed;
}
