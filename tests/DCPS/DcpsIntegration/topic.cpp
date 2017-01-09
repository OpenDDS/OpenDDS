#include  "dds/DdsDcpsInfoUtilsC.h"
#include  "dds/DCPS/Service_Participant.h"
#include  "DomainParticipantListener.h"
#include  "dds/DCPS/Marked_Default_Qos.h"
#include  "dds/DCPS/Qos_Helper.h"
#include  "TopicListener.h"

#include "tests/DCPS/FooType/FooTypeTypeSupportImpl.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_unistd.h"

// const data declarations
const long  TEST_DOMAIN_NUMBER   = 41;
const char* TEST_TOPIC_NAME    = "foo-name";
const char* TEST_TYPE_NAME     = "foo-type";


int
parse_args (int argc, ACE_TCHAR *argv[])
{

  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
    {
      if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            "usage:  %s "
                            "-k <ior> "
                            "\n",
                            argv [0]),
                            -1);
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
  if (parse_args (argc, argv) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) Failed to parse the arguments!\n")),
                        1);
    }

  try
    {

      ::DDS::DomainParticipantFactory_var dpFactory =
        TheParticipantFactoryWithArgs(argc, argv);
      if ( CORBA::is_nil (dpFactory.in()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantFactory returned!\n")),
                            2);
        }

      ::DDS::DomainParticipantListener_var dpListener (new OPENDDS_DCPS_DomainParticipantListener_i);
      if ( CORBA::is_nil (dpListener.in()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantListener returned!\n")),
                            3);
        }


      ::DDS::DomainParticipant_var participant;
      participant =
        dpFactory->create_participant(TEST_DOMAIN_NUMBER,
                                      PARTICIPANT_QOS_DEFAULT,
                                      dpListener.in(),
                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if ( CORBA::is_nil (participant.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipant returned in DomainParticipant test!\n")),
                            3);
        }

      // Intialize the type support
      FooTypeSupport_var fts (new FooTypeSupportImpl);

      if (::DDS::RETCODE_OK != fts->register_type(participant.in (), TEST_TYPE_NAME))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("Failed to register the FooTypeSupport!\n")),
                           3);
        }


      // --
      // Check the DomainParticipant
      // --

      ::DDS::Duration_t timeout;
      timeout.sec = 0;
      timeout.nanosec = 500;
      ::DDS::Topic_var nilTopic = participant->find_topic("non existant topic",
                                                          timeout);
      if ( ! CORBA::is_nil (nilTopic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Non nil Topic returned in find for non existant topic!\n")),
                            4);
        }



      ::DDS::TopicQos topicQOSInitialDefault;
      participant->get_default_topic_qos(topicQOSInitialDefault);


      // change the Entity Factory's auto enable value
      ::DDS::TopicQos topicQosChanged;
      if (::DDS::RELIABLE_RELIABILITY_QOS == topicQOSInitialDefault.reliability.kind)
        {
          topicQosChanged.reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
        }
      else
      {
          topicQosChanged.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      }

      ::DDS::ReturnCode_t setQosReturnCode =
        participant->set_default_topic_qos (topicQosChanged);
      if (::DDS::RETCODE_INCONSISTENT_POLICY != setQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was able to set invalid default Topic QOS!\n")),
                            4);
        }
      ::DDS::TopicQos topicQOSNewDefault;
      participant->get_default_topic_qos (topicQOSNewDefault);
      if (topicQOSNewDefault.reliability.kind
        != topicQOSInitialDefault.reliability.kind)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Incorrect default Participant QOS was returned in the get!\n")),
                            4);
        }


      ::DDS::Topic_var testTopic = participant->create_topic("foo-name",
                                                             TEST_TYPE_NAME,
                                                             TOPIC_QOS_DEFAULT,
                                                             ::DDS::TopicListener::_nil(),
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if ( CORBA::is_nil (testTopic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil Topic returned from initial create_topic!\n")),
                            5);
        }


      timeout.sec = 0;
      timeout.nanosec = 1500;
      ::DDS::Topic_var foundTopic = participant->find_topic("foo-name",
                                                            timeout);
      if ( CORBA::is_nil (foundTopic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil Topic returned in find for existing topic!\n")),
                            5);
        }


      ::DDS::ReturnCode_t deleteReturnCode = participant->delete_topic(testTopic.in ());
      if (::DDS::RETCODE_OK != deleteReturnCode)
      {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Failed to delete existing topic!\n")),
                            5);
      }



      ::DDS::Topic_var foundTopic2 = participant->find_topic("foo-name",
                                                             timeout);
      if ( CORBA::is_nil (foundTopic2.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil Topic returned in find for existing topic!\n")),
                            5);
        }

      deleteReturnCode = participant->delete_topic(foundTopic.in ());
      if (::DDS::RETCODE_OK != deleteReturnCode)
      {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Failed to delete existing topic foundTopic!\n")),
                            5);
      }


      deleteReturnCode = participant->delete_topic(foundTopic2.in ());
      if (::DDS::RETCODE_OK != deleteReturnCode)
      {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Failed to delete existing topic foundTopic2!\n")),
                            5);
      }


      nilTopic = participant->find_topic("foo-name",
                                         timeout);
      if ( ! CORBA::is_nil (nilTopic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Non nil Topic returned in find for deleted foo-name!\n")),
                            5);
        }


      ::DDS::Topic_var topic = participant->create_topic("foo-name",
                                                         TEST_TYPE_NAME,
                                                         TOPIC_QOS_DEFAULT,
                                                         ::DDS::TopicListener::_nil(),
                                                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if ( CORBA::is_nil (topic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil Topic returned from seond create_topic call!\n")),
                            5);
        }


      ::DDS::InconsistentTopicStatus inconsistentStatus;
      if (topic->get_inconsistent_topic_status(inconsistentStatus) != ::DDS::RETCODE_OK)
      {
        ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Failed to get inconsistent topic status!\n")),
                            6);
      }

      ACE_UNUSED_ARG (inconsistentStatus);

      ::DDS::DomainParticipant_var topicParticipant;
      topicParticipant = topic->get_participant();
      if ( CORBA::is_nil (topicParticipant.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipant returned by Topic's get_participant!\n")),
                            6);
        }

      // A check for same participant could be added by getting the servant.


      CORBA::String_var topicTypeName
        = topic->get_type_name();
      if ( ACE_OS::strcmp(topicTypeName, TEST_TYPE_NAME) != 0)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Different DomainParticipant returned by Topic's get_participant!\n")),
                            6);
        }


      ::DDS::TopicListener_var initialListener = topic->get_listener ();
      if ( ! CORBA::is_nil (initialListener.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Non nil TopicListener returned when expected nil!\n")),
                            7);
        }

      ::DDS::TopicListener_var topicListener (new OPENDDS_DCPS_TopicListener_i);
      if ( CORBA::is_nil (topicListener.in()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil TopicListener referenced returned!\n")),
                            7);
        }

      ::DDS::StatusMask mask = ::DDS::PUBLICATION_MATCHED_STATUS | ::DDS::SUBSCRIPTION_MATCHED_STATUS;
      topic->set_listener(topicListener.in(), mask);

      ::DDS::TopicListener_var addedListener = topic->get_listener ();
      if ( CORBA::is_nil (addedListener.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil TopicListener returned after adding TopicListener!\n")),
                            7);
        }

      if ( initialListener.in() == addedListener.in() )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Different TopicListener returned after adding listener!\n")),
                            7);
        }

      ::DDS::TopicQos topicQOSInitial;
      topic->get_qos(topicQOSInitial);
      if (topicQOSInitialDefault.reliability.kind
        != topicQOSInitial.reliability.kind)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Incorrect TopicListener QOS was returned!\n")),
                            8);
        }





      setQosReturnCode =
        topic->set_qos (topicQosChanged);
      if (::DDS::RETCODE_INCONSISTENT_POLICY != setQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was able to set invalid Topic QOS!\n")),
                            8);
        }

      ::DDS::TopicQos topicQOSNew;
      topic->get_qos (topicQOSNew);
      if (topicQOSNew.reliability.kind
        != topicQOSInitialDefault.reliability.kind)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Incorrect Topic QOS was returned after attempting to set invalid!\n")),
                            8);
        }



      deleteReturnCode = participant->delete_topic(topic.in ());
      if (::DDS::RETCODE_OK != deleteReturnCode)
      {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Failed to delete existing topic topic!\n")),
                            9);
      }


      // Sleep for 2 seconds to wait for all built in datareaders to receive the
      // add_associations from InfoRepo before delete the subscriber. Otherwise
      // the add_associations will not add the remote_id to the transport mapping
      // and the remove_associations will fail to find the remote id.
      ACE_OS::sleep (2);

      ::DDS::ReturnCode_t deleteParticipantReturnCode =
        dpFactory->delete_participant(participant.in ());
      if (::DDS::RETCODE_OK != deleteParticipantReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Valid Participant was not deleted!\n")),
                            9);
        }


      TheServiceParticipant->shutdown();

    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in client.cpp:");
      return 1;
    }

  return 0;
}
