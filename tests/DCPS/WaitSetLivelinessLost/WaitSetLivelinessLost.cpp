// -*- C++ -*-
// ============================================================================
/**
 *  @file   WaitSetLivelinessLost.cpp
 *
 *
 *
 */
// ============================================================================

#include "../common/TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include <dds/DCPS/GuardCondition.h>
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "common.h"

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;

  try
  {
    ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

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

      TheServiceParticipant->shutdown ();

      return 1;
    }

    fts->register_type(dp.in (), MY_TYPE);

    ::DDS::TopicQos topic_qos;
    dp->get_default_topic_qos(topic_qos);

    ::DDS::Topic_var topic =
      dp->create_topic (MY_TOPIC,
                        MY_TYPE,
                        TOPIC_QOS_DEFAULT,
                        ::DDS::TopicListener::_nil(),
                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (topic.in ()))
    {
      ACE_ERROR ((LM_ERROR,
                 ACE_TEXT("(%P|%t) create_topic failed!\n")));

      dpf->delete_participant(dp.in ());
      TheServiceParticipant->shutdown ();

      return 1;
    }

    // Create the publisher
    ::DDS::Publisher_var pub =
      dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                           ::DDS::PublisherListener::_nil(),
                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (pub.in ()))
    {
      ACE_ERROR ((LM_ERROR,
                 ACE_TEXT("(%P|%t) create_publisher failed.\n")));

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());
      TheServiceParticipant->shutdown ();

      return 1;
    }

    // Create the DataWriter
    ::DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos (dw_qos);

    dw_qos.liveliness.kind = ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    dw_qos.liveliness.lease_duration.sec = 1;
    dw_qos.liveliness.lease_duration.nanosec = 0;

    ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                dw_qos,
                                0,
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil (dw.in()))
    {
      ACE_ERROR ((LM_ERROR,
                 ACE_TEXT("(%P|%t) create_datawriter failed.\n")));

      pub->delete_contained_entities() ;
      dp->delete_publisher(pub.in());

      dp->delete_topic(topic.in());
      dpf->delete_participant(dp.in());
      TheServiceParticipant->shutdown();

      return 1;
    }

    dw->assert_liveliness();

    DDS::WaitSet_var ws = new DDS::WaitSet;
    DDS::StatusCondition_var sc = dw->get_statuscondition();
    sc->set_enabled_statuses(DDS::LIVELINESS_LOST_STATUS);
    ws->attach_condition(sc);

    dw->assert_liveliness();

    const ::DDS::Duration_t three_seconds = {3, 0 };
    {
      DDS::ConditionSeq active;
      if (ws->wait(active, three_seconds) != DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) WaitSet wait failed.\n")));

        status = 1;
      }

      if (active.length() != 1u)
      {
        ACE_ERROR ((LM_ERROR,
            ACE_TEXT("(%P|%t) active condition length failed.\n")));

        status = 1;
      }

      if (active[0] != sc)
      {
        ACE_ERROR ((LM_ERROR,
            ACE_TEXT("(%P|%t) not the expected active condition.\n")));

        status = 1;
      }

      if (!sc->get_trigger_value())
      {
        ACE_ERROR ((LM_ERROR,
            ACE_TEXT("(%P|%t) active condition trigger is false.\n")));

        status = 1;
      }
    }

    ws->detach_condition(sc);

    pub->delete_datawriter(dw.in ());

    pub->delete_contained_entities() ;
    dp->delete_publisher(pub.in ());

    dp->delete_topic(topic.in ());
    dpf->delete_participant(dp.in ());

    TheServiceParticipant->shutdown ();

    ACE_DEBUG((LM_DEBUG, "(%P|%t) %T WaitSetLivelinessLost main done\n"));
  }
  catch (const TestException&)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) TestException caught in main.cpp\n")));
    return 1;
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in main.cpp:");
    return 1;
  }

  return status;
}
