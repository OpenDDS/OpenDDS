#include  "dds/DdsDcpsInfoC.h"
#include  "dds/DCPS/Service_Participant.h"
#include  "DomainParticipantListener.h"
#include  "dds/DCPS/Marked_Default_Qos.h"
#include  "dds/DCPS/Qos_Helper.h"
#include  "dds/DCPS/transport/framework/TheTransportFactory.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#endif

#include "ace/Arg_Shifter.h"

// const data declarations
const long  TEST_DOMAIN_NUMBER   = 411;


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

      dpFactory_noargs = TheParticipantFactory;
      if ( CORBA::is_nil (dpFactory_noargs.in()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantFactory returned when no args given!\n")),
                            2);
        }

      if (dpFactory.in() != dpFactory_noargs.in() )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Different DomainParticipantFactories returned!\n")),
                            2);
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

      ::DDS::DomainParticipantQos dPQosInitialDefault;
      dpFactory->get_default_participant_qos (dPQosInitialDefault);

      // change the Entity Factory's auto enable value
      ::DDS::DomainParticipantQos dPQosChangedEntityFactory;
      dPQosChangedEntityFactory.entity_factory.autoenable_created_entities =
        ! dPQosInitialDefault.entity_factory.autoenable_created_entities;

      ::DDS::ReturnCode_t setFactoryReturnCode =
        dpFactory->set_default_participant_qos(dPQosChangedEntityFactory);
      if (::DDS::RETCODE_INCONSISTENT_POLICY != setFactoryReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was able to set invalid default Participant QOS!\n")),
                            3);
        }

      ::DDS::DomainParticipantQos dPQosNewDefault;
      dpFactory->get_default_participant_qos (dPQosNewDefault);

      if (dPQosNewDefault.entity_factory.autoenable_created_entities
        != dPQosInitialDefault.entity_factory.autoenable_created_entities)
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
                                      dpListener.in());
      if ( ! CORBA::is_nil (testParticipant.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Invalid QOS returned a DomainParticipant!\n")),
                            4);
        }

      ::DDS::ReturnCode_t deleteParticipantReturnCode =
      dpFactory->delete_participant(testParticipant.in ());
      if (::DDS::RETCODE_BAD_PARAMETER != deleteParticipantReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Invalid Participant was deleted!\n")),
                            4);
        }

      testParticipant =
        dpFactory->create_participant(TEST_DOMAIN_NUMBER,
                                      PARTICIPANT_QOS_DEFAULT,
                                      dpListener.in());
      if ( CORBA::is_nil (testParticipant.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipant returned!\n")),
                            5);
        }


      deleteParticipantReturnCode =
        dpFactory->delete_participant(testParticipant.in ());
      if (::DDS::RETCODE_OK != deleteParticipantReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Valid Participant was not deleted!\n")),
                            5);
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

      ::DDS::DomainParticipantFactory_var firstInstance =
        dpFactory->get_instance();
      if ( CORBA::is_nil (firstInstance.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantFactory returned on first call!\n")),
                            6);
        }

      ::DDS::DomainParticipantFactory_var secondInstance =
        dpFactory->get_instance();
      if ( CORBA::is_nil (secondInstance.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipantFactory returned on second call!\n")),
                            6);
        }

      if (firstInstance.in() != secondInstance.in())
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipantFactory instances don't match!\n")),
                            6);
        }



      // --
      // Check the DomainParticipant
      // --

      ::DDS::DomainParticipant_var participant;
      participant =
        dpFactory->create_participant(TEST_DOMAIN_NUMBER,
                                      PARTICIPANT_QOS_DEFAULT,
                                      dpListener.in());
      if ( CORBA::is_nil (participant.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Nil DomainParticipant returned in DomainParticipant test!\n")),
                            7);
        }

      ::DDS::DomainParticipantQos participantInitialQOS;
      participant->get_qos(participantInitialQOS);
      if (participantInitialQOS.entity_factory.autoenable_created_entities
        != dPQosInitialDefault.entity_factory.autoenable_created_entities)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Participant has incorrect QOS!\n")),
                            7);
        }

      ::DDS::ReturnCode_t setQosReturnCode =
        participant->set_qos(dPQosChangedEntityFactory);
      if (::DDS::RETCODE_INCONSISTENT_POLICY != setQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was able to set invalid Participant QOS!\n")),
                            7);
        }

      ::DDS::DomainParticipantListener_var participantInitialListener;
      participantInitialListener = participant->get_listener();
      if ( CORBA::is_nil (participantInitialListener.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil listener when expected a valid!\n")),
                            7);
        }

      ::DDS::ReturnCode_t setListenerReturnCode =
        participant->set_listener(dpListener.in(),
                                  ::DDS::INCONSISTENT_TOPIC_STATUS | ::DDS::SUBSCRIPTION_MATCH_STATUS);
      if (::DDS::RETCODE_OK != setListenerReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was unable to set valid Participant listener!\n")),
                            7);
        }

      ::DDS::DomainParticipantListener_var participantListener;
      participantListener = participant->get_listener();
      if (CORBA::is_nil (participantListener.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil listener!\n")),
                            7);
        }

      if (participantListener.in() != dpListener.ptr())
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant listener returned is not the listener set!\n")),
                            7);
        }




      if (participant->get_domain_id() != TEST_DOMAIN_NUMBER)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned the incorrect domain id!\n")),
                            7);
        }




      ::DDS::PublisherQos pubInitialQos;
      participant->get_default_publisher_qos(pubInitialQos);


      ::DDS::PublisherQos pubQosChangedEntityFactory;
      pubQosChangedEntityFactory.entity_factory.autoenable_created_entities =
        ! pubInitialQos.entity_factory.autoenable_created_entities;


      ::DDS::ReturnCode_t pubChangeQosReturnCode =
      participant->set_default_publisher_qos(pubQosChangedEntityFactory);
      if (::DDS::RETCODE_INCONSISTENT_POLICY != pubChangeQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was able to set invalid default Publisher QOS!\n")),
                            8);
        }

      ::DDS::PublisherQos pubQosNewDefault;
      participant->get_default_publisher_qos (pubQosNewDefault);

      if (pubInitialQos.entity_factory.autoenable_created_entities
        != pubQosNewDefault.entity_factory.autoenable_created_entities)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Incorrect default Publisher QOS was returned in the get!\n")),
                            8);
        }


      ::DDS::Publisher_var publisher = ::DDS::Publisher::_nil();
      publisher = participant->create_publisher (pubInitialQos,
                                                 ::DDS::PublisherListener::_nil());
      if (CORBA::is_nil (publisher.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil publisher!\n")),
                            8);
        }

      ::DDS::ReturnCode_t deletePubReturnCode =
      participant->delete_publisher(publisher.in ());
      if (::DDS::RETCODE_OK != deletePubReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Publisher was NOT deleted!\n")),
                            8);
        }



      ::DDS::SubscriberQos subInitialQos;
      participant->get_default_subscriber_qos(subInitialQos);


      ::DDS::SubscriberQos subQosChangedEntityFactory;
      subQosChangedEntityFactory.entity_factory.autoenable_created_entities =
        ! subInitialQos.entity_factory.autoenable_created_entities;


      ::DDS::ReturnCode_t subChangeQosReturnCode =
      participant->set_default_subscriber_qos(subQosChangedEntityFactory);
      if (::DDS::RETCODE_INCONSISTENT_POLICY != subChangeQosReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Was able to set invalid default Subscriber QOS!\n")),
                            9);
        }

      ::DDS::SubscriberQos subQosNewDefault;
      participant->get_default_subscriber_qos (subQosNewDefault);

      if (subInitialQos.entity_factory.autoenable_created_entities
        != subQosNewDefault.entity_factory.autoenable_created_entities)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Incorrect default Subscriber QOS was returned in the get!\n")),
                            9);
        }


      ::DDS::Subscriber_var subscriber = ::DDS::Subscriber::_nil();
      subscriber = participant->create_subscriber (subInitialQos,
                                                 ::DDS::SubscriberListener::_nil());
      if (CORBA::is_nil (subscriber.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil Subscriber!\n")),
                            9);
        }

      ::DDS::ReturnCode_t deleteSubReturnCode =
      participant->delete_subscriber(subscriber.in ());
      if (::DDS::RETCODE_OK != deleteSubReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Subscriber was NOT deleted!\n")),
                            9);
        }



      publisher = participant->create_publisher (pubInitialQos,
                                                 ::DDS::PublisherListener::_nil());
      if (CORBA::is_nil (publisher.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil publisher!\n")),
                            10);
        }
      subscriber = participant->create_subscriber (subInitialQos,
                                                 ::DDS::SubscriberListener::_nil());
      if (CORBA::is_nil (subscriber.in ()) )
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant returned a nil Subscriber!\n")),
                            10);
        }

      ::DDS::ReturnCode_t deleteEntriesReturnCode =
      participant->delete_contained_entities();
      if (::DDS::RETCODE_OK != deleteEntriesReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) DomainParticipant failed to delete_contained_entities!\n")),
                            10);
        }




      deleteParticipantReturnCode =
        dpFactory->delete_participant(participant.in ());
      if (::DDS::RETCODE_OK != deleteParticipantReturnCode)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Valid Participant was not deleted!\n")),
                            7);
        }

     TheTransportFactory->release();
     TheServiceParticipant->shutdown();
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in client.cpp:");
      return 1;
    }

  return 0;
}
