#include  "dds/DdsDcpsInfoUtilsC.h"
#include  "dds/DCPS/Service_Participant.h"
#include  "DomainParticipantListener.h"
#include  "dds/DCPS/Marked_Default_Qos.h"
#include  "dds/DCPS/Qos_Helper.h"

#include "dds/DCPS/StaticIncludes.h"

#include "ace/Arg_Shifter.h"

// const data declarations
const long  TEST_DOMAIN_NUMBER   = 411;


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
      // --
      // Check the Service_Participant
      // --

      ::DDS::DomainParticipantFactory_var dpFactory;
      ::DDS::DomainParticipantFactory_var dpFactory_noargs;
      dpFactory = TheParticipantFactoryWithArgs(argc, argv);
      if ( CORBA::is_nil (dpFactory.in()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantFactory returned!\n")),
                            2);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 1 passed.\n")));
        }

      dpFactory_noargs = TheParticipantFactory;
      if ( CORBA::is_nil (dpFactory_noargs.in()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantFactory returned when no args given!\n")),
                            2);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 2 passed.\n")));
        }

      if (dpFactory.in() != dpFactory_noargs.in() )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Different DomainParticipantFactories returned!\n")),
                            2);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 3 passed.\n")));
        }



      // --
      // Check the DomainParticipantFactory
      // --
      ::DDS::DomainParticipantListener_var dpListener (new OPENDDS_DCPS_DomainParticipantListener_i);

      if ( CORBA::is_nil (dpListener.in()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantListener returned!\n")),
                            3);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 4 passed.\n")));
        }

      ::DDS::DomainParticipantQos dPQosInitialDefault;
      dpFactory->get_default_participant_qos (dPQosInitialDefault);

      // change the Entity Factory's auto enable value
      ::DDS::DomainParticipantQos dPQosChangedEntityFactory;
      dPQosChangedEntityFactory.entity_factory.autoenable_created_entities =
        ! dPQosInitialDefault.entity_factory.autoenable_created_entities;

      ::DDS::ReturnCode_t setFactoryReturnCode =
        dpFactory->set_default_participant_qos(dPQosChangedEntityFactory);
      if (::DDS::RETCODE_OK != setFactoryReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Failed to set default Participant QOS!\n")),
                            3);
        }
      else
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 5 passed.\n")));
      }

      ::DDS::DomainParticipantQos dPQosNewDefault;
      dpFactory->get_default_participant_qos (dPQosNewDefault);

      if (dPQosNewDefault.entity_factory.autoenable_created_entities
        != dPQosInitialDefault.entity_factory.autoenable_created_entities)
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 6 passed.\n")));
      }
      else
      {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("(%P|%t) Incorrect default Participant QOS was returned in the get!\n")),
          3);
      }

      ::DDS::DomainParticipant_var testParticipant;

      // Try to create a participant with a bad QOS
      testParticipant =
        dpFactory->create_participant(TEST_DOMAIN_NUMBER,
                                      dPQosChangedEntityFactory,
                                      dpListener.in(),
                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if ( CORBA::is_nil (testParticipant.in ()) )
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) Failed to create DomainParticipant!\n")),
                          4);
      }
      else
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 7 passed.\n")));
      }

      ::DDS::ReturnCode_t deleteParticipantReturnCode =
      dpFactory->delete_participant(testParticipant.in ());

      if (::DDS::RETCODE_OK != deleteParticipantReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) The delete_participant returned %d!\n"),
                            deleteParticipantReturnCode),
                            4);
        }
      else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 8-1 passed.\n")));
        }

      testParticipant = ::DDS::DomainParticipant::_nil ();
      deleteParticipantReturnCode =
        dpFactory->delete_participant(testParticipant.in ());

      if (::DDS::RETCODE_BAD_PARAMETER != deleteParticipantReturnCode)
      {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("(%P|%t) Invalid Participant was deleted!\n")),
          4);
      }
      else
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 8-2 passed.\n")));
      }

      testParticipant =
        dpFactory->create_participant(TEST_DOMAIN_NUMBER,
                                      PARTICIPANT_QOS_DEFAULT,
                                      dpListener.in(),
                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if ( CORBA::is_nil (testParticipant.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipant returned!\n")),
                            5);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 9 passed.\n")));
        }


      deleteParticipantReturnCode =
        dpFactory->delete_participant(testParticipant.in ());
      if (::DDS::RETCODE_OK != deleteParticipantReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Valid Participant was not deleted!\n")),
                            5);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 10 passed.\n")));
        }

      /* Cant do the below because the delete_participant doesnt handle it
      deleteParticipantReturnCode =
        dpFactory->delete_participant(testParticipant.in ());
      if (::DDS::RETCODE_BAD_PARAMETER != deleteParticipantReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Participant was deleted twice!\n")),
                            5);
        }
      */
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 11 skipped.\n")));

      ::DDS::DomainParticipantFactory_var firstInstance =
        dpFactory->get_instance();
      if ( CORBA::is_nil (firstInstance.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantFactory returned on first call!\n")),
                            6);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 12 passed.\n")));
        }

      ::DDS::DomainParticipantFactory_var secondInstance =
        dpFactory->get_instance();
      if ( CORBA::is_nil (secondInstance.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantFactory returned on second call!\n")),
                            6);
        }
      else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 13 passed.\n")));
        }

      if (firstInstance.in() != secondInstance.in())
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipantFactory instances don't match!\n")),
                            6);
        }
      else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 14 passed.\n")));
        }



      // --
      // Check the DomainParticipant
      // --

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
                            7);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 15 passed.\n")));
        }

      ::DDS::DomainParticipantQos participantInitialQOS;
      participant->get_qos(participantInitialQOS);
      // PARTICIPANT_QOS_DEFAULT was got from DomainParticipantFactory which has changed its
      // default participant qos from initial default.
      if (participantInitialQOS.entity_factory.autoenable_created_entities
        == dPQosInitialDefault.entity_factory.autoenable_created_entities)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Participant has incorrect QOS!\n")),
                            7);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 16 passed.\n")));
        }

      ::DDS::ReturnCode_t setQosReturnCode =
        participant->set_qos(dPQosChangedEntityFactory);
      if (::DDS::RETCODE_OK != setQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was not able to set Participant QOS!\n")),
                            7);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 17 passed.\n")));
        }

      ::DDS::DomainParticipantListener_var participantInitialListener;
      participantInitialListener = participant->get_listener();
      if ( CORBA::is_nil (participantInitialListener.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil listener when expected a valid!\n")),
                            7);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 18 passed.\n")));
        }

      ::DDS::ReturnCode_t setListenerReturnCode =
        participant->set_listener(dpListener.in(),
                                  ::DDS::INCONSISTENT_TOPIC_STATUS | ::DDS::SUBSCRIPTION_MATCHED_STATUS);
      if (::DDS::RETCODE_OK != setListenerReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was unable to set valid Participant listener %d!\n"), setListenerReturnCode),
                            7);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 19 passed.\n")));
        }

      ::DDS::DomainParticipantListener_var participantListener;
      participantListener = participant->get_listener();
      if (CORBA::is_nil (participantListener.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil listener!\n")),
                            7);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 20 passed.\n")));
        }

      if (participantListener.in() != dpListener.ptr())
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant listener returned is not the listener set!\n")),
                            7);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 21 passed.\n")));
        }




      if (participant->get_domain_id() != TEST_DOMAIN_NUMBER)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned the incorrect domain id!\n")),
                            7);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 22 passed.\n")));
        }




      ::DDS::PublisherQos pubInitialQos;
      participant->get_default_publisher_qos(pubInitialQos);


      ::DDS::PublisherQos pubQosChangedEntityFactory;
      participant->get_default_publisher_qos(pubQosChangedEntityFactory);

      pubQosChangedEntityFactory.entity_factory.autoenable_created_entities =
        ! pubInitialQos.entity_factory.autoenable_created_entities;


      ::DDS::ReturnCode_t pubChangeQosReturnCode =
      participant->set_default_publisher_qos(pubQosChangedEntityFactory);
      if (::DDS::RETCODE_OK != pubChangeQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was unable to set valid default Publisher QOS %d!\n"), pubChangeQosReturnCode),
                            8);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 23 passed.\n")));
        }

      ::DDS::PublisherQos pubQosNewDefault;
      participant->get_default_publisher_qos (pubQosNewDefault);

      if (pubInitialQos.entity_factory.autoenable_created_entities
        == pubQosNewDefault.entity_factory.autoenable_created_entities)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Incorrect default Publisher QOS was returned in the get!\n")),
                            8);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 24 passed.\n")));
        }


      ::DDS::Publisher_var publisher = ::DDS::Publisher::_nil();
      publisher = participant->create_publisher (pubInitialQos,
                                                 ::DDS::PublisherListener::_nil(),
                                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (publisher.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil publisher!\n")),
                            8);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 25 passed.\n")));
        }

      ::DDS::ReturnCode_t deletePubReturnCode =
      participant->delete_publisher(publisher.in ());
      if (::DDS::RETCODE_OK != deletePubReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Publisher was NOT deleted!\n")),
                            8);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 26 passed.\n")));
        }



      ::DDS::SubscriberQos subInitialQos;
      participant->get_default_subscriber_qos(subInitialQos);


      ::DDS::SubscriberQos subQosChangedEntityFactory;
      participant->get_default_subscriber_qos(subQosChangedEntityFactory);

      subQosChangedEntityFactory.entity_factory.autoenable_created_entities =
        ! subInitialQos.entity_factory.autoenable_created_entities;


      ::DDS::ReturnCode_t subChangeQosReturnCode =
      participant->set_default_subscriber_qos(subQosChangedEntityFactory);
      if (::DDS::RETCODE_OK != subChangeQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was unable to set valid default Subscriber QOS!\n")),
                            9);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 27 passed.\n")));
        }

      ::DDS::SubscriberQos subQosNewDefault;
      participant->get_default_subscriber_qos (subQosNewDefault);

      if (subInitialQos.entity_factory.autoenable_created_entities
        == subQosNewDefault.entity_factory.autoenable_created_entities)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Incorrect default Subscriber QOS was returned in the get!\n")),
                            9);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 28 passed.\n")));
        }


      ::DDS::Subscriber_var subscriber = ::DDS::Subscriber::_nil();
      subscriber = participant->create_subscriber (subInitialQos,
                                                 ::DDS::SubscriberListener::_nil(),
                                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (subscriber.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil Subscriber!\n")),
                            9);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 29 passed.\n")));
        }

      ::DDS::ReturnCode_t deleteSubReturnCode =
      participant->delete_subscriber(subscriber.in ());
      if (::DDS::RETCODE_OK != deleteSubReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Subscriber was NOT deleted!\n")),
                            9);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 30 passed.\n")));
        }



      publisher = participant->create_publisher (pubInitialQos,
                                                 ::DDS::PublisherListener::_nil(),
                                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (publisher.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil publisher!\n")),
                            10);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 31 passed.\n")));
        }
      subscriber = participant->create_subscriber (subInitialQos,
                                                 ::DDS::SubscriberListener::_nil(),
                                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (subscriber.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil Subscriber!\n")),
                            10);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 32 passed.\n")));
        }

      ::DDS::ReturnCode_t deleteEntriesReturnCode =
      participant->delete_contained_entities();
      if (::DDS::RETCODE_OK != deleteEntriesReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant failed to delete_contained_entities!\n")),
                            10);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 33 passed.\n")));
        }




      deleteParticipantReturnCode =
        dpFactory->delete_participant(participant.in ());
      if (::DDS::RETCODE_OK != deleteParticipantReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Valid Participant was not deleted!\n")),
                            7);
        }
        else
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Test 34 passed.\n")));
        }

     TheServiceParticipant->shutdown();
     ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) All tests completed without exceptions.\n")));
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in client.cpp:");
      return 1;
    }

  return 0;
}
