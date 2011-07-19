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


#include "common.h"
#include "DDSTEST.h"

#include "../common/TestSupport.h"

#include "dds/DCPS/Service_Participant.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif

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

      // Assert effective transport protocols
      int long left = protocol_str.size ();
      for (std::vector < std::string>::const_iterator proto = protocol_str.begin ();
              proto < protocol_str.end (); proto++)
        {
          bool issupported = ::DDS_TEST::supports (dw.in (), *proto);
          ACE_ERROR ((LM_INFO,
                      ACE_TEXT ("(%P|%t) Validating that '%C' entity supports protocol '%C': %C\n"),
                      entity_str.c_str (),
                      proto->c_str (),
                      issupported ? "true" : "false"));

          if (issupported) left--;
        }

      // All required protocols must have been found
      TEST_CHECK (left == 0);

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
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main.cpp:");
      return 1;
    }
  catch (...)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT ("(%P|%t) runtime exception caught in main.cpp. ")), 1);
    }

  ACE_ERROR_RETURN ((LM_INFO,
                     ACE_TEXT ("(%P|%t) done.\n")), 0);
}
