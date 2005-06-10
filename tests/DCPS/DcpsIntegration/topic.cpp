#include  "dds/DdsDcpsInfoC.h"
#include  "dds/DCPS/Service_Participant.h"
#include  "DomainParticipantListener.h"
#include  "dds/DCPS/Marked_Default_Qos.h"
#include  "dds/DCPS/Qos_Helper.h"
#include  "TopicListener.h"

#include "tests/DCPS/FooType/FooTypeSupportImpl.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpTransport.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"


#include "ace/Arg_Shifter.h"

// const data declarations
const long  TEST_DOMAIN_NUMBER   = 411;
const char* TEST_TOPIC_NAME    = "foo-name";
const char* TEST_TYPE_NAME     = "foo-type";


int
parse_args (int argc, char *argv[])
{

  ACE_Arg_Shifter arg_shifter (argc, argv);
  
  while (arg_shifter.is_anything_left ()) 
    {
      if (arg_shifter.cur_arg_strncasecmp("-?") == 0) 
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

  // Indicates sucessful parsing of the command line
  return 0;
}



int
main (int argc, char *argv[])
{
  if (parse_args (argc, argv) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR, 
                        ACE_TEXT("(%P|%t) Failed to parse the arguments!\n")),
                        1);
    }

  ACE_TRY_NEW_ENV
    {

      ::DDS::DomainParticipantFactory_var dpFactory = 
        TheParticipantFactoryWithArgs(argc, argv ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( CORBA::is_nil (dpFactory.in()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Nil DomainParticipantFactory returned!\n")),
                            2);
        }


      TAO_DCPS_DomainParticipantListener_i* dpListenerImpl = new TAO_DCPS_DomainParticipantListener_i;

      ::DDS::DomainParticipantListener_var dpListener =
        ::TAO::DCPS::servant_to_reference < ::DDS::DomainParticipantListener,
                                            TAO_DCPS_DomainParticipantListener_i,
                                            ::DDS::DomainParticipantListener_ptr> (dpListenerImpl ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
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
                                      dpListener.in()
                                      ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( CORBA::is_nil (participant.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Nil DomainParticipant returned in DomainParticipant test!\n")),
                            3);
        }

      // Intialize the type support
      FooTypeSupportImpl* fts_servant = new FooTypeSupportImpl();
      FooTypeSupport_var fts = 
        TAO::DCPS::servant_to_reference<FooTypeSupport,FooTypeSupportImpl,FooTypeSupport_ptr>(fts_servant);
      ACE_TRY_CHECK;

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
                                                          timeout
                                                          ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( ! CORBA::is_nil (nilTopic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Non nil Topic returned in find for non existant topic!\n")),
                            4);
        }



      ::DDS::TopicQos topicQOSInitialDefault;
      participant->get_default_topic_qos(topicQOSInitialDefault ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;


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
        participant->set_default_topic_qos (topicQosChanged ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (::DDS::RETCODE_INCONSISTENT_POLICY != setQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Was able to set invalid default Topic QOS!\n")),
                            4);
        }

      ::DDS::TopicQos topicQOSNewDefault;
      participant->get_default_topic_qos (topicQOSNewDefault ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
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
                                                             ::DDS::TopicListener::_nil()
                                                             ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( CORBA::is_nil (testTopic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Nil Topic returned from initial create_topic!\n")),
                            5);
        }


      timeout.sec = 0;
      timeout.nanosec = 1500;
      ::DDS::Topic_var foundTopic = participant->find_topic("foo-name",
                                                            timeout
                                                            ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( CORBA::is_nil (foundTopic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Nil Topic returned in find for existing topic!\n")),
                            5);
        }


      ::DDS::ReturnCode_t deleteReturnCode = participant->delete_topic(testTopic.in () ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (::DDS::RETCODE_OK != deleteReturnCode)
      {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Failed to delete existing topic!\n")),
                            5);
      }



      ::DDS::Topic_var foundTopic2 = participant->find_topic("foo-name",
                                                             timeout
                                                             ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( CORBA::is_nil (foundTopic2.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Nil Topic returned in find for existing topic!\n")),
                            5);
        }

      deleteReturnCode = participant->delete_topic(foundTopic.in () ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (::DDS::RETCODE_OK != deleteReturnCode)
      {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Failed to delete existing topic foundTopic!\n")),
                            5);
      }


      deleteReturnCode = participant->delete_topic(foundTopic2.in () ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (::DDS::RETCODE_OK != deleteReturnCode)
      {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Failed to delete existing topic foundTopic2!\n")),
                            5);
      }


      nilTopic = participant->find_topic("foo-name",
                                         timeout
                                         ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( ! CORBA::is_nil (nilTopic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Non nil Topic returned in find for deleted foo-name!\n")),
                            5);
        }


      ::DDS::Topic_var topic = participant->create_topic("foo-name",
                                                         TEST_TYPE_NAME,
                                                         TOPIC_QOS_DEFAULT,
                                                         ::DDS::TopicListener::_nil()
                                                         ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( CORBA::is_nil (topic.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Nil Topic returned from seond create_topic call!\n")),
                            5);
        }


      ::DDS::InconsistentTopicStatus inconsistentStatus =
          topic->get_inconsistent_topic_status(ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;

      ::DDS::DomainParticipant_var topicParticipant;
      topicParticipant = topic->get_participant(ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( CORBA::is_nil (topicParticipant.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Nil DomainParticipant returned by Topic's get_participant!\n")),
                            6);
        }

      // A check for same participant could be added by getting the servant.


      char* topicTypeName = topic->get_type_name(ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( ACE_OS::strcmp(topicTypeName, TEST_TYPE_NAME) != 0)
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Different DomainParticipant returned by Topic's get_participant!\n")),
                            6);
        }


      ::DDS::TopicListener_var initialListener = topic->get_listener (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( ! CORBA::is_nil (initialListener.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Non nil TopicListener returned when expected nil!\n")),
                            7);
        }


      TAO_DCPS_TopicListener_i* topicListenerImpl = new TAO_DCPS_TopicListener_i;

      ::DDS::TopicListener_var topicListener =
        ::TAO::DCPS::servant_to_reference < ::DDS::TopicListener,
                                            TAO_DCPS_TopicListener_i,
                                            ::DDS::TopicListener_ptr> (topicListenerImpl ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if ( CORBA::is_nil (topicListener.in()) )
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Nil TopicListener referenced returned!\n")),
                            7);
        }

      ::DDS::StatusKindMask mask = ::DDS::PUBLICATION_MATCH_STATUS | ::DDS::SUBSCRIPTION_MATCH_STATUS;
      topic->set_listener(topicListener.in(), mask ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;

      ::DDS::TopicListener_var addedListener = topic->get_listener (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_TRY_CHECK;
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
      topic->get_qos(topicQOSInitial ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (topicQOSInitialDefault.reliability.kind
        != topicQOSInitial.reliability.kind)
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Incorrect TopicListener QOS was returned!\n")),
                            8);
        }





      setQosReturnCode =
        topic->set_qos (topicQosChanged ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (::DDS::RETCODE_INCONSISTENT_POLICY != setQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Was able to set invalid Topic QOS!\n")),
                            8);
        }

      ::DDS::TopicQos topicQOSNew;
      topic->get_qos (topicQOSNew ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (topicQOSNew.reliability.kind
        != topicQOSInitialDefault.reliability.kind)
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Incorrect Topic QOS was returned after attempting to set invalid!\n")),
                            8);
        }



      deleteReturnCode = participant->delete_topic(topic.in () ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (::DDS::RETCODE_OK != deleteReturnCode)
      {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Failed to delete existing topic topic!\n")),
                            9);
      }


      ::DDS::ReturnCode_t deleteParticipantReturnCode =
        dpFactory->delete_participant(participant.in () ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (::DDS::RETCODE_OK != deleteParticipantReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR, 
                            ACE_TEXT("(%P|%t) Valid Participant was not deleted!\n")),
                            9);
        }

      TheServiceParticipant->shutdown();

    }
  ACE_CATCHANY
    {
      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                           "Exception caught in client.cpp:");
      return 1;
    }
  ACE_ENDTRY;

  return 0;
}
