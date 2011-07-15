// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "DataWriterListenerImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"

#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportInst.h"
#include "dds/DCPS/transport/framework/TransportInst_rch.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif

#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"
#include "tao/ORB_Core.h"

#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

#include "common.h"
#include "../common/TestSupport.h"
#include "../common/TestException.h"

#include "DDSTEST.h"

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs (argc, argv);
      TEST_CHECK (dpf.in () != 0);

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      ::parse_args (argc, argv);

      DDS::DomainParticipant_var dp = dpf->create_participant (MY_DOMAIN,
                                                               PARTICIPANT_QOS_DEFAULT,
                                                               DDS::DomainParticipantListener::_nil (),
                                                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      TEST_CHECK (!CORBA::is_nil (dp.in ()));

      Xyz::FooTypeSupport_var fts (new Xyz::FooTypeSupportImpl);
      TEST_CHECK (DDS::RETCODE_OK == fts->register_type (dp.in (), MY_TYPE));

      DDS::TopicQos topic_qos;
      dp->get_default_topic_qos (topic_qos);

      DDS::Topic_var topic =
              dp->create_topic (MY_TOPIC,
                                MY_TYPE,
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil (),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      TEST_CHECK (!CORBA::is_nil (topic.in ()))

      DDS::TopicDescription_var description =
              dp->lookup_topicdescription (MY_TOPIC);
      TEST_CHECK (!CORBA::is_nil (description.in ()));

      // Create the publisher
      DDS::Publisher_var pub = dp->create_publisher (PUBLISHER_QOS_DEFAULT,
                                                     DDS::PublisherListener::_nil (),
                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      TEST_CHECK (!CORBA::is_nil (pub.in ()))

      // Initialize the transport configuration for the appropriate Entity (except TreansportClients)
      TEST_CHECK (!configuration_str.empty ());
      if (configuration_str == "none")
        {
          TEST_CHECK (!entity_str.empty ());
        }
      else
        {
          TEST_CHECK (!configuration_str.empty ());

          if (entity_str == "pubsub")
            {
              OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str, pub.in ());
            }
          else if (entity_str == "participant")
            {
              OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str, dp.in ());
            }
          else
            {
              TEST_CHECK (entity_str == "none");
            }
        }

      // Create the data writer
      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.durability.kind = durability_kind;
      dw_qos.liveliness.kind = liveliness_kind;
      dw_qos.liveliness.lease_duration = LEASE_DURATION;
      dw_qos.reliability.kind = reliability_kind;

      DDS::DataWriterListener_var dwl (new DataWriterListenerImpl);
      DDS::DataWriter_var dw = pub->create_datawriter (topic.in (),
                                                       dw_qos,
                                                       dwl.in (),
                                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      TEST_CHECK (!CORBA::is_nil (dw.in ()));

      // Initialize the transport configuration for the appropriate entity
      TEST_CHECK (!configuration_str.empty ());
      if (configuration_str == "none")
        {
          TEST_CHECK (!entity_str.empty ());
        }
      else
        {
          if (entity_str == "rw")
            {
              OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str,
                                                                          dw.in ());
            }
        }

      // Wait for things to settle ?!
      ACE_OS::sleep (test_duration);

      // Assert effective configuration properties
      ACE_ERROR ((LM_INFO,
                  ACE_TEXT ("(%P|%t) Validating if the entity '%C' effective protocol %C '%C'\n"),
                  entity_str.c_str (),
                  (compatible ? "is" : "is not"),
                  protocol_str.c_str ()));

      bool supported = (::DDS_TEST::supports (dw.in (), protocol_str) != 0);
      TEST_CHECK (compatible == supported);

      // Clean up publisher objects
      pub->delete_contained_entities ();

      dp->delete_publisher (pub.in ());

      dp->delete_topic (topic.in ());
      dpf->delete_participant (dp.in ());

      TheServiceParticipant->shutdown ();

      // Assert if pub/sub made a match ...
      DataWriterListenerImpl* dwl_servant =
              dynamic_cast<DataWriterListenerImpl*> (dwl.in ());

      // check to see if the publisher worked
      if (dwl_servant->publication_matched () != compatible)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT ("(%P|%t) Expected publication_matched to be %C, but it wasn't.")
                             ACE_TEXT ("durability_kind=%s, liveliness_kind=%s, liveliness_duration=%s, ")
                             ACE_TEXT ("reliability_kind=%s\n"),
                             (compatible) ? "true" : "false",
                             durability_kind_str.c_str (),
                             liveliness_kind_str.c_str (),
                             LEASE_DURATION_STR.c_str (),
                             reliability_kind_str.c_str ()),
                            1);
        }
    }
  catch (const TestException&)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT ("(%P|%t) TestException caught in main.cpp. ")), 1);
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main.cpp:");
      return 1;
    }

  ACE_ERROR_RETURN ((LM_INFO,
                     ACE_TEXT ("(%P|%t) done.\n")), 0);
}
