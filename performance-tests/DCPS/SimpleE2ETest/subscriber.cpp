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


#include "Reader.h"
#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "../TypeNoKeyBounded/Pt128TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt512TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt2048TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt8192TypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "ace/Arg_Shifter.h"




#include "common.h"


/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  ACE_Arg_Shifter arg_shifter (argc, argv);

  arg_shifter.ignore_arg (); // ignore the command - argv[0]
  while (arg_shifter.is_anything_left ())
  {
    // options:
    // -p  <num data writers>
    // -n  <num samples>
    // -d  <data size>
    // -a  <transport address>
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
    else if ((currentArg = arg_shifter.get_the_parameter("-d")) != 0)
    {
      int shift_bits = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
      DATA_SIZE = 1 << shift_bits;
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-n")) != 0)
    {
      NUM_SAMPLES = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-a")) != 0)
    {
      reader_address_str = currentArg;
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
    else if (arg_shifter.cur_arg_strncasecmp("-z") == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else
    {
      ACE_ERROR((LM_ERROR,"(%P|%t) unexpected parameter \"%s\"\n", arg_shifter.get_current()));
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

  try
    {
      ACE_DEBUG((LM_INFO," %P|%t %T subscriber main\n"));

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      status = parse_args (argc, argv);
      if (status)
        return status;



      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(TEST_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil());
      if (CORBA::is_nil (dp.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT(" %P|%t ERROR: create_participant failed.\n")));
        return 1 ;
      }

      // Register the type supports
      switch (DATA_SIZE)
      {
      case 128:
        {
          ::Xyz::Pt128TypeSupportImpl* pt128ts_servant = new ::Xyz::Pt128TypeSupportImpl();
          OpenDDS::DCPS::LocalObject_var safe_servant = pt128ts_servant;

          ::Xyz::Pt128TypeSupport_var pt128ts =
            OpenDDS::DCPS::servant_to_reference (pt128ts_servant);

          if (::DDS::RETCODE_OK != pt128ts->register_type(dp.in (), TEST_TYPE))
            {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT (" %P|%t ERROR: Failed to register the Pt128TypeSupport.")));
              return 1;
            }
        }
        break;
      case 512:
        {
          ::Xyz::Pt512TypeSupportImpl* pt512ts_servant = new ::Xyz::Pt512TypeSupportImpl();
          OpenDDS::DCPS::LocalObject_var safe_servant = pt512ts_servant;

          ::Xyz::Pt512TypeSupport_var pt512ts =
            OpenDDS::DCPS::servant_to_reference (pt512ts_servant);

          if (::DDS::RETCODE_OK != pt512ts->register_type(dp.in (), TEST_TYPE))
            {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT (" %P|%t ERROR:Failed to register the Pt512TypeSupport.")));
              return 1;
            }
        }
        break;
      case 2048:
        {
          ::Xyz::Pt2048TypeSupportImpl* pt2048ts_servant = new ::Xyz::Pt2048TypeSupportImpl();
          OpenDDS::DCPS::LocalObject_var safe_servant = pt2048ts_servant;

          ::Xyz::Pt2048TypeSupport_var pt2048ts =
            OpenDDS::DCPS::servant_to_reference (pt2048ts_servant);

          if (::DDS::RETCODE_OK != pt2048ts->register_type(dp.in (), TEST_TYPE))
            {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT (" %P|%t ERROR: Failed to register the Pt2048TypeSupport.")));
              return 1;
            }
        }
        break;
      case 8192:
        {
          ::Xyz::Pt8192TypeSupportImpl* pt8192ts_servant = new ::Xyz::Pt8192TypeSupportImpl();
          OpenDDS::DCPS::LocalObject_var safe_servant = pt8192ts_servant;

          ::Xyz::Pt8192TypeSupport_var pt8192ts =
            OpenDDS::DCPS::servant_to_reference (pt8192ts_servant);

          if (::DDS::RETCODE_OK != pt8192ts->register_type(dp.in (), TEST_TYPE))
            {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT (" %P|%t ERROR: Failed to register the Pt8192TypeSupport.")));
              return 1;
            }
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

      ::DDS::Topic_var topic =
        dp->create_topic (TEST_TOPIC,
                          TEST_TYPE,
                          topic_qos,
                          ::DDS::TopicListener::_nil());
      if (CORBA::is_nil (topic.in ()))
      {
        return 1 ;
      }

      ::DDS::TopicDescription_var description =
        dp->lookup_topicdescription(TEST_TOPIC);
      if (CORBA::is_nil (description.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT(" %P|%t ERROR: lookup_topicdescription failed.\n")),
                           1);
      }


      // Create the subscriber
      ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                             ::DDS::SubscriberListener::_nil());
      if (CORBA::is_nil (sub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT(" %P|%t ERROR: create_subscriber failed.\n")),
                           1);
      }

      // Initialize the transport
      if (0 != ::init_reader_tranport() )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT(" %P|%t ERROR: init_transport failed!\n")),
                           1);
      }

      // Attach the subscriber to the transport.
      OpenDDS::DCPS::SubscriberImpl* sub_impl
        = OpenDDS::DCPS::reference_to_servant<OpenDDS::DCPS::SubscriberImpl> (sub.in ());

      if (0 == sub_impl)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT(" %P|%t ERROR: Failed to obtain servant ::OpenDDS::DCPS::SubscriberImpl\n")),
                          1);
      }

      OpenDDS::DCPS::AttachStatus attach_status =
        sub_impl->attach_transport(reader_transport_impl.in());

      if (attach_status != OpenDDS::DCPS::ATTACH_OK)
        {
          // We failed to attach to the transport for some reason.
          const char* status_str = "";

          switch (attach_status)
            {
              case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
                status_str = "ATTACH_BAD_TRANSPORT";
                break;
              case OpenDDS::DCPS::ATTACH_ERROR:
                status_str = "ATTACH_ERROR";
                break;
              case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
                status_str = "ATTACH_INCOMPATIBLE_QOS";
                break;
              default:
                status_str = "Unknown Status";
                break;
            }

          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT(" %P|%t ERROR: Failed to attach to the transport. ")
                            ACE_TEXT("AttachStatus == %s\n"),
                            status_str),
                            1);
        }


      // Create the Datareaders
      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      sub->copy_from_topic_qos (dr_qos, topic_qos);

      ::DDS::DataReader_var  the_dr
               = sub->create_datareader(description.in (),
                                        dr_qos,
                                        ::DDS::DataReaderListener::_nil());

      if (CORBA::is_nil (the_dr.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT(" %P|%t ERROR: create_datareader failed.\n")),
                          1);
      }

      Reader the_reader(sub.in (),
                        the_dr.in (),
                        num_datawriters,
                        NUM_SAMPLES,
                        DATA_SIZE);
      status = the_reader.svc();

      // clean up subscriber objects
      sub->delete_contained_entities() ;

      dp->delete_subscriber(sub.in ());

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      reader_transport_impl = 0;
      writer_transport_impl = 0;

      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();

    }
  catch (const TestException&)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("( %P|%t ERROR: TestException caught in main.cpp. ")));
      return 1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main.cpp:");
      return 1;
    }

  return status;
}
