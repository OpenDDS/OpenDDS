// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================


#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TransportDebug.h>
#include "dds/DCPS/StaticIncludes.h"

#include <ace/streams.h>
#include <ace/Get_Opt.h>
#include <ace/OS_NS_sys_stat.h>
#include <ace/OS_NS_unistd.h>

using namespace Messenger;

const char* pub_ready_filename    = "publisher_ready.txt";
const char* sub_ready_filename    = "subscriber_ready.txt";
const char* sub_finished_filename = "subscriber_finished.txt";

int sub_reinit_itr = 10; // number of Subscriber iterations
bool verbose = false;

/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("vi:"));
  int c;

  while ((c = get_opts ()) != -1)
    switch (c)
      {
      case 'v':
        verbose = true;
        break;
      case 'i':
        sub_reinit_itr = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage:  %s "
                           "-i <subscriber reinit count> "
                           "-v <verbose>"
                           "\n",
                           argv [0]),
                          -1);
      }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);
      if( parse_args(argc, argv) != 0)
        return 1;

      participant =
        dpf->create_participant(411,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%P|%t) create_participant failed.\n")
                          , -1);
      }

      MessageTypeSupportImpl* mts_servant = new MessageTypeSupportImpl;

      if (DDS::RETCODE_OK != mts_servant->register_type(participant.in (),
                                                        "")) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%P|%t) Failed to register the MessageTypeTypeSupport.\n")
                          , -1);
      }

      CORBA::String_var type_name = mts_servant->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name.in (),
                                  topic_qos,
                                  DDS::TopicListener::_nil(),
                                  ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%P|%t) Failed to create_topic.\n")
                          , -1);
      }

      // Indicate that the subscriber is about to become ready
      FILE* readers_ready = ACE_OS::fopen (sub_ready_filename, ACE_TEXT("w"));
      if (readers_ready == 0) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%P|%t) ERROR: Unable to create subscriber ready file.\n")
                          , -1);
      }
      ACE_OS::fclose(readers_ready);

      // Check if the publisher is up and running
      ACE_stat stats;
      while (ACE_OS::stat (pub_ready_filename, &stats) == -1)
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
        }

      for (int count = 1; count <= sub_reinit_itr; count++)
        {
          if (verbose) {
            ACE_DEBUG ((LM_DEBUG, "(%P|%t) Reinitializing subscriber.\n"));
          }

          // Create the subscriber and attach to the corresponding
          // transport.
          DDS::Subscriber_var sub =
            participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                           DDS::SubscriberListener::_nil(),
                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
          if (CORBA::is_nil (sub.in ())) {
            ACE_ERROR_RETURN ((LM_ERROR,
                               "(%P|%t) Failed to create_subscriber.\n")
                              , -1);
          }

          // Create the Datareaders
          DDS::DataReaderQos dr_qos;
          sub->get_default_datareader_qos (dr_qos);
          DDS::DataReader_var dr = sub->create_datareader(topic.in (),
                                                          dr_qos,
                                                          DDS::DataReaderListener::_nil(),
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
          if (CORBA::is_nil (dr.in ())) {
            ACE_ERROR_RETURN ((LM_ERROR,
                               "(%P|%t) create_datareader failed.\n")
                              , -1);
          }

          // This is where a speed-bump should be.
          while (true)
            {
              ::DDS::InstanceHandleSeq handles;
              dr->get_matched_publications (handles);
              if (handles.length() > 0) {
                break;
              }
              ACE_Time_Value small_time (0,250000);
              ACE_OS::sleep (small_time);
            }

          // Add sleep to let the fully_associted message arrive datawriter
          // before remove_associations is called upon delete_datareader,
          // otherwise the datawriter will encounter bit lookup timeout upon
          // fully associated.
          ACE_Time_Value small_time (0,250000);
                ACE_OS::sleep (small_time);

          if (verbose) {
            ACE_DEBUG ((LM_DEBUG, "(%P|%t) *** Destroying Subscriber\n"));
          }

          // Delete data reader
          sub->delete_datareader(dr.in());

          // Delete subscriber
          participant->delete_subscriber(sub.in());
          dr = DDS::DataReader::_nil ();
          sub = DDS::Subscriber::_nil();
        }

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }
      TheServiceParticipant->shutdown ();

      // Indicate that the subscriber is done
      FILE* readers_completed = ACE_OS::fopen (sub_finished_filename, ACE_TEXT("w"));
      if (readers_completed == 0) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%P|%t) ERROR: Unable to create subscriber completed file.\n")
                          , -1);
      }
      ACE_OS::fclose(readers_completed);

    }
  catch (CORBA::Exception& e) {
    ACE_ERROR_RETURN ((LM_ERROR,
                       "(%P|%t) Exception caught in main (): %C (%C).\n"
                       ,  e._name (), e._rep_id ())
                      , -1);
  }

  return 0;
}
