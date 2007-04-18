// -*- C++ -*-
//
// $Id$

#include "common.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/TopicImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionS.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "tests/DCPS/common/TestSupport.h"

#include "tao/ORB_Core.h"
#include "ace/Get_Opt.h"
#include "ace/High_Res_Timer.h"
#include "ace/Arg_Shifter.h"

#include "ace/Reactor.h"

using namespace ::DDS;
using namespace ::TAO::DCPS;

int ignore_before_association = 0;



#if !defined (DDS_HAS_MINIMUM_BIT)

void
parse_args (int argc,
            ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    const char *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter("-i")) != 0)
    {
      ignore_kind = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-z") == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)


#if !defined (DDS_HAS_MINIMUM_BIT)
int init (int argc, ACE_TCHAR *argv[])
{
  try
    {
      participant_factory = TheParticipantFactoryWithArgs(argc, argv);

      // Initialize the transport
      if (0 != init_transport() )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: init_transport failed!\n")),
          1);
      }

      participant
        = participant_factory->create_participant(TEST_DOMAIN,
                                                  PARTICIPANT_QOS_DEFAULT,
                                                  ::DDS::DomainParticipantListener::_nil ());
      
      //SHH create a separate particpant for the subscriber and publisher

      // Wait a while to give the transport thread time
      // to read the built-in Topic data from the DCPSInfo
      // TBD - find some way to avoid this.
    ACE_OS::sleep (2);

      ::Mine::FooTypeSupportImpl* ts_servant = new ::Mine::FooTypeSupportImpl();
      PortableServer::ServantBase_var safe_servant = ts_servant;

      ::Mine::FooTypeSupport_var ts =
        TAO::DCPS::servant_to_reference (ts_servant);

      if (::DDS::RETCODE_OK != ts->register_type(participant.in (), TEST_TOPIC_TYPE))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("ERROR: Failed to register the FooTypeSupport.")));
        return 1;
      }

      participant_servant
        = ::TAO::DCPS::reference_to_servant < ::TAO::DCPS::DomainParticipantImpl,
                                              ::DDS::DomainParticipant_ptr>
          (participant.in ());

      topic = participant->create_topic (TEST_TOPIC,
                                         TEST_TOPIC_TYPE,
                                         TOPIC_QOS_DEFAULT,
                                         ::DDS::TopicListener::_nil ());

      topic_servant
        = ::TAO::DCPS::reference_to_servant < ::TAO::DCPS::TopicImpl,
                                              ::DDS::Topic_ptr>
          (topic.in ());

      subscriber
        = participant->create_subscriber (SUBSCRIBER_QOS_DEFAULT,
                                         ::DDS::SubscriberListener::_nil ());

      subscriber_servant
        = ::TAO::DCPS::reference_to_servant < ::TAO::DCPS::SubscriberImpl,
                                              ::DDS::Subscriber_ptr>
          (subscriber.in ());

      // Attach the subscriber to transport
      if (0 != attach_subscriber_transport() )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: attach_subscriber_transport failed!\n")),
          -1);
      }

      //SHH make the subscriber participant do this lookup
      ::DDS::TopicDescription_var topicdescription
        = participant->lookup_topicdescription(TEST_TOPIC_TYPE);

      publisher
        = participant->create_publisher (PUBLISHER_QOS_DEFAULT,
                                         ::DDS::PublisherListener::_nil ());

      publisher_servant
        = ::TAO::DCPS::reference_to_servant < ::TAO::DCPS::PublisherImpl,
                                              ::DDS::Publisher_ptr>
          (publisher.in ());

      // Attach the publisher to transport
      if (0 != attach_publisher_transport() )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
             ACE_TEXT("(%P|%t) ERROR: attach_publisher_transport failed!\n")),
             -1);
      }

      // try ignore before the association
      if (ignore_before_association)
      {
        ACE_DEBUG((LM_DEBUG, "Ignoring before the assocaition\n"));
        ignore ();
      }

      datareader
        = subscriber->create_datareader (topicdescription.in (),
                                         DATAREADER_QOS_DEFAULT,
                                         ::DDS::DataReaderListener::_nil ());

      datareader_servant
        = ::TAO::DCPS::reference_to_servant < ::TAO::DCPS::DataReaderImpl,
                                              ::DDS::DataReader_ptr>
          (datareader.in ());

      datawriter
        = publisher->create_datawriter (topic.in (),
                                        DATAWRITER_QOS_DEFAULT,
                                        ::DDS::DataWriterListener::_nil ());

      datawriter_servant
        = ::TAO::DCPS::reference_to_servant < ::TAO::DCPS::DataWriterImpl,
                                              ::DDS::DataWriter_ptr>
          (datawriter.in ());
  }
  catch (...)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Exception caught in init ()\n")),
        1);
    }

  return 0;
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)


#if !defined (DDS_HAS_MINIMUM_BIT)
void test_bit_participant ()
{
  ACE_DEBUG((LM_INFO,"sizeof(::DDS::ParticipantBuiltinTopicData) = %d\n",
                     sizeof(::DDS::ParticipantBuiltinTopicData) ));
  ACE_DEBUG((LM_INFO,"sizeof(::DDS::TopicBuiltinTopicData) = %d\n",
                     sizeof(::DDS::TopicBuiltinTopicData) ));
  ACE_DEBUG((LM_INFO,"sizeof(::DDS::PublicationBuiltinTopicData) = %d\n",
                     sizeof(::DDS::PublicationBuiltinTopicData) ));
  ACE_DEBUG((LM_INFO,"sizeof(::DDS::SubscriptionBuiltinTopicData) = %d\n",
                     sizeof(::DDS::SubscriptionBuiltinTopicData) ));

  try
    {
      ::DDS::DataReader_var dr
        = bit_subscriber->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);

      TEST_CHECK (! CORBA::is_nil (dr.in ()));

      ::DDS::ParticipantBuiltinTopicDataDataReader_var part_dr
        = ::DDS::ParticipantBuiltinTopicDataDataReader::_narrow (dr.in ());

      ::DDS::ParticipantBuiltinTopicDataSeq part_data;
      ::DDS::SampleInfoSeq infos;
      ::DDS::ReturnCode_t ret = part_dr->read ( part_data,
                                                infos,
                                                20,
                                                ANY_SAMPLE_STATE,
                                                ANY_VIEW_STATE,
                                                ANY_INSTANCE_STATE) ;
#if 0
      TEST_CHECK (ret == ::DDS::RETCODE_OK);

      CORBA::ULong data_len = part_data.length ();
      CORBA::ULong info_len = infos.length ();

      TEST_CHECK (data_len == 1);
      TEST_CHECK (info_len == 1);

      ::DDS::DomainParticipantQos part_qos;
      participant_servant->get_qos (part_qos);

      TEST_CHECK (part_data[0].key[0] == TEST_DOMAIN);
      TEST_CHECK (part_data[0].key[1] == participant_servant->get_id ());
      TEST_CHECK (part_data[0].key[2] == 0);
#else
      // The paricipant will not know itself from repo since the
      // bit participant topic datareader is created after 
      // DCPS gets add_domain_participant request.
      TEST_CHECK (ret == ::DDS::RETCODE_NO_DATA);
#endif
    }
  catch (...)
    {
      ACE_ERROR ((LM_ERROR, "ERROR: Exception caught in test_bit_participant ()"));
    }
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)


#if !defined (DDS_HAS_MINIMUM_BIT)
void test_bit_topic ()
{
  try
    {
      ::DDS::DataReader_var dr
        = bit_subscriber->lookup_datareader (BUILT_IN_TOPIC_TOPIC);

      TEST_CHECK (! CORBA::is_nil (dr.in ()));

      ::DDS::TopicBuiltinTopicDataDataReader_var topic_dr
        = ::DDS::TopicBuiltinTopicDataDataReader::_narrow (dr.in ());

      ::DDS::TopicBuiltinTopicDataSeq topic_data;
      ::DDS::SampleInfoSeq infos;
      ::DDS::ReturnCode_t ret = topic_dr->read (topic_data,
                                                infos,
                                                20,
                                                ANY_SAMPLE_STATE,
                                                ANY_VIEW_STATE,
                                                ANY_INSTANCE_STATE) ;
      TEST_CHECK (ret == ::DDS::RETCODE_OK);

      CORBA::ULong data_len = topic_data.length ();
      CORBA::ULong info_len = infos.length ();

      TEST_CHECK (data_len == 1);
      TEST_CHECK (info_len == 1);

      ::DDS::TopicQos topic_qos;

      TEST_CHECK (topic_data[0].key[0] == TEST_DOMAIN);
      TEST_CHECK (topic_data[0].key[1] == participant_servant->get_id ());

      topic_servant->get_qos (topic_qos);

      TEST_CHECK (ACE_OS::strcmp (topic_data[0].name.in (), TEST_TOPIC) == 0);
      TEST_CHECK (ACE_OS::strcmp (topic_data[0].type_name.in (), TEST_TOPIC_TYPE) == 0);

      //The SunOS compiler had problem resolving operator in a namespace.
      //To resolve the compilation errors, the operator is called explicitly.
      TEST_CHECK (topic_data[0].durability == topic_qos.durability);
      TEST_CHECK (topic_data[0].deadline == topic_qos.deadline);
      TEST_CHECK (topic_data[0].latency_budget == topic_qos.latency_budget);
      TEST_CHECK (topic_data[0].liveliness == topic_qos.liveliness);
      TEST_CHECK (topic_data[0].reliability == topic_qos.reliability);
      TEST_CHECK (topic_data[0].transport_priority == topic_qos.transport_priority);
      TEST_CHECK (topic_data[0].lifespan == topic_qos.lifespan);
      TEST_CHECK (topic_data[0].destination_order == topic_qos.destination_order);
      TEST_CHECK (topic_data[0].ownership == topic_qos.ownership);
      TEST_CHECK (topic_data[0].topic_data == topic_qos.topic_data);
    }
  catch (...)
    {
      ACE_ERROR ((LM_ERROR, "ERROR: Exception caught in test_bit_topic ()"));
    }
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)


#if !defined (DDS_HAS_MINIMUM_BIT)
void test_bit_publication ()
{
  try
    {
      ::DDS::DataReader_var dr
        = bit_subscriber->lookup_datareader(BUILT_IN_PUBLICATION_TOPIC);

      TEST_CHECK (! CORBA::is_nil (dr.in ()));

      ::DDS::PublicationBuiltinTopicDataDataReader_var pub_dr
        = ::DDS::PublicationBuiltinTopicDataDataReader::_narrow (dr.in ());

      ::DDS::PublicationBuiltinTopicDataSeq pub_data;
      ::DDS::SampleInfoSeq infos;
      ::DDS::ReturnCode_t ret = pub_dr->read ( pub_data,
                                                infos,
                                                20,
                                                ANY_SAMPLE_STATE,
                                                ANY_VIEW_STATE,
                                                ANY_INSTANCE_STATE) ;
      TEST_CHECK (ret == ::DDS::RETCODE_OK);

      CORBA::ULong data_len = pub_data.length ();
      CORBA::ULong info_len = infos.length ();

      TEST_CHECK (data_len == 1);
      TEST_CHECK (info_len == 1);

      ::DDS::PublicationBuiltinTopicData the_pub_data = pub_data[0];

      ::DDS::DataWriterQos dw_qos;
      datawriter->get_qos (dw_qos);

      TEST_CHECK (the_pub_data.key[0] == TEST_DOMAIN);
      TEST_CHECK (the_pub_data.key[1] == participant_servant->get_id ());
      TEST_CHECK (the_pub_data.key[2] == datawriter_servant->get_publication_id ());

      TEST_CHECK (the_pub_data.participant_key[0] == TEST_DOMAIN);
      TEST_CHECK (the_pub_data.participant_key[1] == participant_servant->get_id ());
      TEST_CHECK (the_pub_data.participant_key[2] == 0);

      TEST_CHECK (ACE_OS::strcmp (the_pub_data.topic_name.in (), TEST_TOPIC) == 0);
      TEST_CHECK (ACE_OS::strcmp (the_pub_data.type_name.in (), TEST_TOPIC_TYPE) == 0);

      //The SunOS compiler had problem resolving operator in a namespace.
      //To resolve the compilation errors, the operator is called explicitly.
      TEST_CHECK (the_pub_data.durability == dw_qos.durability);
      TEST_CHECK (the_pub_data.deadline == dw_qos.deadline);
      TEST_CHECK (the_pub_data.latency_budget == dw_qos.latency_budget);
      TEST_CHECK (the_pub_data.liveliness == dw_qos.liveliness);
      //TEST_CHECK (the_pub_data.lifespan == dw_qos.lifespan);
      TEST_CHECK (the_pub_data.user_data == dw_qos.user_data);
      TEST_CHECK (the_pub_data.ownership_strength == dw_qos.ownership_strength);
      //the_pub_data.presentation
      //the_pub_data.partition
      //the_pub_data.topic_data
      //the_pub_data.group_data
    }
  catch (...)
    {
      ACE_ERROR ((LM_ERROR, "ERROR: Exception caught in test_bit_publication ()"));
    }
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)


#if !defined (DDS_HAS_MINIMUM_BIT)
void test_bit_subscription ()
{
  try
    {
      ::DDS::DataReader_var dr
        = bit_subscriber->lookup_datareader(BUILT_IN_SUBSCRIPTION_TOPIC);

      TEST_CHECK (! CORBA::is_nil (dr.in ()));

      ::DDS::SubscriptionBuiltinTopicDataDataReader_var sub_dr
        = ::DDS::SubscriptionBuiltinTopicDataDataReader::_narrow (dr.in ());

      ::DDS::SubscriptionBuiltinTopicDataSeq sub_data;
      ::DDS::SampleInfoSeq infos;
      ::DDS::ReturnCode_t ret = sub_dr->read ( sub_data,
                                                infos,
                                                20,
                                                ANY_SAMPLE_STATE,
                                                ANY_VIEW_STATE,
                                                ANY_INSTANCE_STATE) ;
      TEST_CHECK (ret == ::DDS::RETCODE_OK);

      CORBA::ULong data_len = sub_data.length ();
      CORBA::ULong info_len = infos.length ();

      TEST_CHECK (data_len == 1);
      TEST_CHECK (info_len == 1);

      ::DDS::SubscriptionBuiltinTopicData the_sub_data = sub_data[0];

      ::DDS::DataReaderQos dr_qos;
      datareader->get_qos (dr_qos);

      TEST_CHECK (the_sub_data.key[0] == TEST_DOMAIN);
      TEST_CHECK (the_sub_data.key[1] == participant_servant->get_id ());
      TEST_CHECK (the_sub_data.key[2] == datareader_servant->get_subscription_id ());

      TEST_CHECK (the_sub_data.participant_key[0] == TEST_DOMAIN);
      TEST_CHECK (the_sub_data.participant_key[1] == participant_servant->get_id ());
      TEST_CHECK (the_sub_data.participant_key[2] == 0);

      TEST_CHECK (ACE_OS::strcmp (the_sub_data.topic_name.in (), TEST_TOPIC) == 0);
      TEST_CHECK (ACE_OS::strcmp (the_sub_data.type_name.in (), TEST_TOPIC_TYPE) == 0);

      //The SunOS compiler had problem resolving operator in a namespace.
      //To resolve the compilation errors, the operator is called explicitly.
      TEST_CHECK (the_sub_data.durability == dr_qos.durability);
      TEST_CHECK (the_sub_data.deadline == dr_qos.deadline);
      TEST_CHECK (the_sub_data.latency_budget == dr_qos.latency_budget);
      TEST_CHECK (the_sub_data.liveliness == dr_qos.liveliness);
      TEST_CHECK (the_sub_data.reliability == dr_qos.reliability);
      TEST_CHECK (the_sub_data.destination_order == dr_qos.destination_order);
      TEST_CHECK (the_sub_data.user_data == dr_qos.user_data);
      TEST_CHECK (the_sub_data.time_based_filter == dr_qos.time_based_filter);
      //the_sub_data.presentation
      //the_sub_data.partition
      //the_sub_data.topic_data
      //the_sub_data.group_data
    }
  catch (...)
    {
      ACE_ERROR ((LM_ERROR, "ERROR: Exception caught in test_bit_subscription ()"));
    }
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)


#if !defined (DDS_HAS_MINIMUM_BIT)
void shutdown ()
{
  if (participant->delete_contained_entities ()
    != ::DDS::RETCODE_OK)
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
       "shutdown: participant delete_contained_entities failed\n"));
  }

  ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;

  if (dpf->delete_participant (participant.in ())
    != ::DDS::RETCODE_OK)
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: shutdown: "
      "participant  delete_participant failed\n"));
  }

  TheTransportFactory->release();

  TheServiceParticipant->shutdown ();
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)


int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  parse_args (argc, argv);

  if (ignore_kind != DONT_IGNORE)
  {
    ACE_RANDR_TYPE seed =
      ACE_static_cast(ACE_RANDR_TYPE, ACE_OS::time (0));
    // GCC complains about seed not being used.
    ACE_UNUSED_ARG(seed);


    ignore_before_association = 1;

    if (ignore_before_association == 1
      && (ignore_kind == IGNORE_PUBLICATION
          || ignore_kind == IGNORE_SUBSCRIPTION))
    {
      // Always ignore the publication and subscription after the add_association
      // because the add_association is called during create_datawriter/datareader
      // and we only can specify the ignored datawriter/datareader after they
      // are created.
      ignore_before_association = 0;
    }
  }

  if (init (argc, argv) == -1)
    {
      return 1;
    }

  // Wait a while to give the transport thread time
  // to read the built-in Topic data from the DCPSInfo
  // TBD - find some way to avoid this.
  ACE_OS::sleep (5);

  if (ignore_kind == DONT_IGNORE)
    {
      bit_subscriber = participant->get_builtin_subscriber () ;
      TEST_CHECK (! CORBA::is_nil (bit_subscriber.in ()));

      test_bit_participant ();
      test_bit_topic ();
      test_bit_publication ();
      test_bit_subscription ();
    }
  else
    {
      if (ignore_kind != DONT_IGNORE && !ignore_before_association)
      {
        ACE_DEBUG((LM_DEBUG, "Ignoring after the association\n"));
        ignore ();
      }
    }

  ACE_OS::sleep (5); //REMOVE when fully established association works

  int failed = write ();
  if (failed == 0)
  {
    // wait for the write be handled by the Transport thread
    ACE_OS::sleep (1);

    failed = read (ignore_kind == DONT_IGNORE);
  }

  shutdown ();

  cleanup_transport ();

  return failed;
#else
  ACE_UNUSED_ARG (argc);
  ACE_UNUSED_ARG (argv);
  ACE_DEBUG((LM_DEBUG, "BIT support is not enabled.\n"));
  return 0;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}
