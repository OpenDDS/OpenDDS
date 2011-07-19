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

DDS::Publisher_ptr
create_configured_publisher (DDS::DomainParticipant_ptr dp)
{

  // Create the publisher
  DDS::Publisher_var pub = dp->create_publisher (PUBLISHER_QOS_DEFAULT,
                                                 DDS::PublisherListener::_nil (),
                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK (!CORBA::is_nil (pub.in ()));

  // If there is a ini file-based configuration name initialize
  // the transport configuration for the corresponding Entity
  TEST_CHECK (!configuration_str.empty ());
  if (configuration_str != "none" && entity_str == "pubsub")
    {
      OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str, pub.in ());
    }

  return pub._retn ();
}

DDS::DataWriter_ptr
create_configured_writer (DDS::Publisher_ptr pub, DDS::Topic_ptr topic, DDS::DataWriterListener_ptr dwl)
{
  // Create the data writer
  DDS::DataWriterQos dw_qos;
  pub->get_default_datawriter_qos (dw_qos);

  dw_qos.durability.kind = durability_kind;
  dw_qos.liveliness.kind = liveliness_kind;
  dw_qos.liveliness.lease_duration = LEASE_DURATION;
  dw_qos.reliability.kind = reliability_kind;

  DDS::DataWriter_var dw = pub->create_datawriter (topic,
                                                   dw_qos,
                                                   dwl,
                                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Initialize the transport configuration for the appropriate entity
  TEST_CHECK (!configuration_str.empty ());

  if (configuration_str != "none" && entity_str == "rw")
    {
      OpenDDS::DCPS::TransportRegistry::instance ()->bind_config (configuration_str,
                                                                  dw.in ());
    }

  return dw._retn ();
}

bool
assert_publication_matched (DDS::DataWriterListener_ptr dwl)
{
  // Assert if pub/sub made a match ...
  DataWriterListenerImpl* dwl_servant =
          dynamic_cast<DataWriterListenerImpl*> (dwl);

  // check to see if the publisher worked
  return !compatible || dwl_servant->publication_matched ();

}

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

      DDS::DomainParticipant_var dp (create_configured_participant (dpf.in ()));
      TEST_CHECK (!CORBA::is_nil (dp.in ()));

      DDS::Topic_var topic (create_configured_topic (dp.in ()));
      TEST_CHECK (!CORBA::is_nil (topic.in ()));

      DDS::Publisher_var pub (create_configured_publisher (dp.in ()));
      TEST_CHECK (!CORBA::is_nil (pub.in ()));

      DDS::DataWriterListener_var dwl (new DataWriterListenerImpl);
      DDS::DataWriter_var dw (create_configured_writer (pub.in (), topic.in (), dwl.in ()));
      TEST_CHECK (!CORBA::is_nil (dw.in ()));

      // Wait for things to settle ?!
      ACE_OS::sleep (test_duration);

      // Assert effective configuration properties
      ACE_ERROR ((LM_INFO,
                  ACE_TEXT ("(%P|%t) Validating if the entity '%C' effective protocol is '%C'\n"),
                  entity_str.c_str (),
                  protocol_str.c_str ()));

      TEST_CHECK (::DDS_TEST::supports (dw.in (), protocol_str) != 0);

      // Clean up publisher objects
      pub->delete_contained_entities ();
      dp->delete_publisher (pub.in ());
      dp->delete_topic (topic.in ());
      dpf->delete_participant (dp.in ());

      TheServiceParticipant->shutdown ();

      if (!assert_publication_matched (dwl.in ()))
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
