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


#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUdpConfiguration.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "ace/Arg_Shifter.h"

#include <string>

const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";
std::string reader_address_str; // = "localhost:16701";
std::string writer_address_str; // = "localhost:29803";
int reader_address_given = 0;
int writer_address_given = 0;

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITY_SEC);

int use_take = 0;
int multiple_instances = 0;
int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 10 ;
bool support_client_side_BIT = false;

// default to using TCP
int sub_using_udp = 0;
int pub_using_udp = 0;
OpenDDS::DCPS::TransportImpl_rch reader_transport_impl;
OpenDDS::DCPS::TransportImpl_rch writer_transport_impl;

enum TransportTypeId
{
  SIMPLE_TCP,
  SIMPLE_UDP
};

enum TransportInstanceId
{
  SUB_TRAFFIC,
  PUB_TRAFFIC
};



int init_tranport ()
{
  int status = 0;

  if (sub_using_udp)
    {
      reader_transport_impl
        = TheTransportFactory->create_transport_impl (SUB_TRAFFIC,
                                                      "SimpleUdp",
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch reader_config
        = TheTransportFactory->create_configuration (SUB_TRAFFIC, "SimpleUdp");

      OpenDDS::DCPS::SimpleUdpConfiguration* reader_udp_config
        = static_cast <OpenDDS::DCPS::SimpleUdpConfiguration*> (reader_config.in ());

      if (!reader_address_given)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: sub UDP")
                    ACE_TEXT(" Must specify an address for UDP.\n")));
          return 11;
        }


      ACE_INET_Addr reader_address (reader_address_str.c_str ());
      reader_udp_config->local_address_ = reader_address;

      if (reader_transport_impl->configure(reader_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: sub UDP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }
  else
    {
      reader_transport_impl
        = TheTransportFactory->create_transport_impl (SUB_TRAFFIC,
                                                      "SimpleTcp",
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch reader_config
        = TheTransportFactory->create_configuration (SUB_TRAFFIC, "SimpleTcp");

      OpenDDS::DCPS::SimpleTcpConfiguration* reader_tcp_config
        = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (reader_config.in ());

      if (reader_address_given)
        {
          ACE_INET_Addr reader_address (reader_address_str.c_str ());
          reader_tcp_config->local_address_ = reader_address;
        }
        // else use default address - OS assigned.

      if (reader_transport_impl->configure(reader_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: sub TCP ")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  if (pub_using_udp)
    {
      writer_transport_impl
         = TheTransportFactory->create_transport_impl (PUB_TRAFFIC,
                                                       "SimpleUdp",
                                                       OpenDDS::DCPS::DONT_AUTO_CONFIG);

      OpenDDS::DCPS::TransportConfiguration_rch writer_config
        = TheTransportFactory->create_configuration (PUB_TRAFFIC, "SimpleUdp");

      OpenDDS::DCPS::SimpleUdpConfiguration* writer_udp_config
        = static_cast <OpenDDS::DCPS::SimpleUdpConfiguration*> (writer_config.in ());

      if (!writer_address_given)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: pub UDP")
                    ACE_TEXT(" Must specify an address for UDP.\n")));
          return 12;
        }

      ACE_INET_Addr writer_address (writer_address_str.c_str ());
      writer_udp_config->local_address_ = writer_address;

      if (writer_transport_impl->configure(writer_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: sub UDP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }
  else
    {
      writer_transport_impl
        = TheTransportFactory->create_transport_impl (PUB_TRAFFIC,
                                                      "SimpleTcp",
                                                      OpenDDS::DCPS::DONT_AUTO_CONFIG);
      OpenDDS::DCPS::TransportConfiguration_rch writer_config
        = TheTransportFactory->create_configuration (PUB_TRAFFIC, "SimpleTcp");

      OpenDDS::DCPS::SimpleTcpConfiguration* writer_tcp_config
        = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (writer_config.in ());

      if (writer_address_given)
        {
          ACE_INET_Addr writer_address (writer_address_str.c_str());
          writer_tcp_config->local_address_ = writer_address;
        }
        // else use default address - OS assigned.

      if (writer_transport_impl->configure(writer_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: sub TCP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }
    }

  return status;
}


int wait_for_data (::DDS::Subscriber_ptr sub,
                   int timeout_sec)
{
  const int factor = 10;
  ACE_Time_Value small(0,1000000/factor);
  int timeout_loops = timeout_sec * factor;

  ::DDS::DataReaderSeq_var discard = new ::DDS::DataReaderSeq(10);
  while (timeout_loops-- > 0)
    {
      sub->get_datareaders (
                    discard.out (),
                    ::DDS::NOT_READ_SAMPLE_STATE,
                    ::DDS::ANY_VIEW_STATE,
                    ::DDS::ANY_INSTANCE_STATE );
      if (discard->length () > 0)
        return 1;

      ACE_OS::sleep (small);
    }
  return 0;
}

/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  reader_address_str = ACE_LOCALHOST;
  reader_address_str += ":16701";
  writer_address_str = ACE_LOCALHOST;
  writer_address_str += ":29803";

  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -t use_take?1:0             defaults to 0
    //  -m multiple_instances?1:0   defaults to 0
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -s sub transport address    defaults to localhost:16701
    //  -p pub transport address    defaults to localhost:29803
    //  -z                          verbose transport debug
    //  -b                          enable client side Built-In topic support
    //  -us                         Subscriber using UDP transport
    //  -up                         Publisher using UDP transport

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
    else if ((currentArg = arg_shifter.get_the_parameter("-s")) != 0)
    {
      reader_address_str = currentArg;
      reader_address_given = 1;
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-p")) != 0)
    {
      writer_address_str = currentArg;
      writer_address_given = 1;
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-z") == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-b") == 0)
    {
      support_client_side_BIT = true;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-us") == 0)
    {
      ACE_DEBUG((LM_DEBUG, "Subscriber Using UDP transport.\n"));
      sub_using_udp = 1;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-up") == 0)
    {
      ACE_DEBUG((LM_DEBUG, "Publisher Using UDP transport.\n"));
      pub_using_udp = 1;
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


int main (int argc, char *argv[])
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

      ::Xyz::FooTypeSupportImpl* fts_servant = new ::Xyz::FooTypeSupportImpl;

      ::Xyz::FooTypeSupport_var fts =
        OpenDDS::DCPS::servant_to_reference (fts_servant);

      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil());
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
                          ::DDS::TopicListener::_nil());
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
                             ::DDS::SubscriberListener::_nil());
      if (CORBA::is_nil (sub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) create_subscriber failed.\n")),
                           1);
      }

      // Create the publisher
      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil());
      if (CORBA::is_nil (pub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) create_publisher failed.\n")),
                          1);
      }

      // Initialize the transport
      if (0 != ::init_tranport() )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) init_transport failed!\n")),
                           1);
      }

      // Attach the subscriber to the transport.
      OpenDDS::DCPS::SubscriberImpl* sub_impl
        = OpenDDS::DCPS::reference_to_servant<OpenDDS::DCPS::SubscriberImpl> (sub.in ());

      if (0 == sub_impl)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) Failed to obtain servant ::OpenDDS::DCPS::SubscriberImpl\n")),
                          1);
      }

      sub_impl->attach_transport(reader_transport_impl.in());


      // Attach the publisher to the transport.
      OpenDDS::DCPS::PublisherImpl* pub_impl
        = OpenDDS::DCPS::reference_to_servant<OpenDDS::DCPS::PublisherImpl> (pub.in ());

      if (0 == pub_impl)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) Failed to obtain servant ::OpenDDS::DCPS::PublisherImpl\n")),
                          1);
      }

      pub_impl->attach_transport(writer_transport_impl.in());

      // Create the datawriter
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.history.depth = history_depth  ;
      dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                        dw_qos,
                                        ::DDS::DataWriterListener::_nil());

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
                                 ::DDS::DataReaderListener::_nil());

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

      ::Xyz::FooDataWriterImpl* fast_dw
        = OpenDDS::DCPS::reference_to_servant<Xyz::FooDataWriterImpl>(foo_dw.in());

      ::Xyz::FooDataReader_var foo_dr
        = ::Xyz::FooDataReader::_narrow(dr.in ());
      if (CORBA::is_nil (foo_dr.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) ::Xyz::FooDataReader::_narrow failed.\n")));
        return 1; // failure
      }

      ::Xyz::FooDataReaderImpl* fast_dr
        = OpenDDS::DCPS::reference_to_servant<Xyz::FooDataReaderImpl>(foo_dr.in());


      // wait for association establishement before writing.
      ACE_OS::sleep(5); //REMOVE if not needed


      // =============== do the test ====


      ::DDS::OfferedIncompatibleQosStatus * incomp =
          foo_dw->get_offered_incompatible_qos_status ();

      int incompatible_transport_found = 0;
      for (CORBA::ULong ii =0; ii < incomp->policies.length (); ii++)
        {
          if (incomp->policies[ii].policy_id
                        == ::DDS::TRANSPORTTYPE_QOS_POLICY_ID)
            incompatible_transport_found = 1;
        }

      ::DDS::SubscriptionMatchStatus matched =
        foo_dr->get_subscription_match_status ();

      ::DDS::InstanceHandle_t handle;

      if (pub_using_udp != sub_using_udp)
        {
          if (!incompatible_transport_found)
            ACE_ERROR_RETURN((LM_ERROR,
              "TEST ERROR: Expected offered_incompatible_qos"
              " with TRANSPORTTYPE_QOS_POLICY_ID"
              " but did not get it. %d incompatible_qos values.\n",
              incomp->policies.length ()),
              7);
          else
            {
              ACE_DEBUG((LM_DEBUG, "Got expected offered_incompatible_qos"
                " with TRANSPORTTYPE_QOS_POLICY_ID"
                " existing with success.\n"));
              goto cleanup;
            }
        }
      else
        {
          if (incompatible_transport_found)
            ACE_ERROR_RETURN((LM_ERROR,
              "TEST ERROR: Did not expect offered_incompatible_qos"
              " with TRANSPORTTYPE_QOS_POLICY_ID"
              " but got it. %d incompatible_qos values.\n",
              incomp->policies.length ()),
              8);

          if (matched.total_count != 1)
            ACE_ERROR_RETURN((LM_ERROR,
              "TEST ERROR: expected subscription_match"
              " with count 1 but got %d\n",
              matched.total_count),
              9);
        }

      ::Xyz::Foo foo;
      foo.key = 10101;
      foo.x = -1;
      foo.y = -1;

      handle
          = fast_dw->_cxx_register (foo);

      foo.x = 7;

      fast_dw->write(foo,
                     handle);

      ::Xyz::Foo sample;
      ::DDS::SampleInfo info ;

      // wait for new data for upto 10 seconds
      if (!wait_for_data(sub.in (), 5))
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")),
                           1);

      DDS::ReturnCode_t status  ;
      status = fast_dr->read_next_sample(sample, info) ;

      if (status == ::DDS::RETCODE_OK)
      {
        ACE_DEBUG((LM_DEBUG,
            "%s foo.x = %f foo.y = %f, foo.key = %d\n",
            "read", sample.x, sample.y, sample.key));

        ACE_DEBUG((LM_DEBUG,
            "%s SampleInfo.sample_rank = %d\n",
            "read", info.sample_rank));
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

cleanup:
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

      TheTransportFactory->release();
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

  // Note: The TransportImpl reference SHOULD be deleted before exit from
  //       main if the concrete transport libraries are loaded dynamically.
  //       Otherwise cleanup after main() will encount access vilation.
  reader_transport_impl = 0;
  writer_transport_impl = 0;
  return test_failed;
}
