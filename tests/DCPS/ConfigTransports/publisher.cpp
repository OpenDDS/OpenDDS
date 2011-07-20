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

#include "Reader.h"
#include "Writer.h"

#include "../common/TestSupport.h"

#include "dds/DCPS/Service_Participant.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf1 = TheParticipantFactoryWithArgs(argc, argv);
      TEST_ASSERT(dpf1.in() != 0);

      DDS::DomainParticipantFactory_var dpf2 = TheParticipantFactoryWithArgs(argc, argv);
      TEST_ASSERT(dpf2.in() != 0);

      DDS::DataWriterListener_var dwl1(new DataWriterListenerImpl);

      DDS::DataReaderListener_var drl2(new DataReaderListenerImpl);

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      ::parse_args(argc, argv);


      if (collocation_str == "none")
        {
          Writer w(dpf1.in(), dwl1.in());
          TEST_ASSERT(w.verify_transport());
        }
      else if (collocation_str == "process")
        {
          Writer w(dpf1.in(), dwl1.in());
          TEST_ASSERT(w.verify_transport());

          Reader r(dpf2.in(), drl2.in());
          TEST_ASSERT(r.verify_transport());
        }
      else if (collocation_str == "participant")
        {
          DDS::DomainParticipant_var participant(create_configured_participant(dpf1.in()));

          Writer w(dpf1.in(), participant.in(), dwl1.in());
          TEST_ASSERT(w.verify_transport());

          Reader r(dpf2.in(), participant.in(), drl2.in());
          TEST_ASSERT(r.verify_transport());
        }

      TheServiceParticipant->shutdown();

      if (collocation_str == "none")
        {
          assert_publication_matched(dwl1.in());

        }
      else if (collocation_str == "process")
        {
          assert_publication_matched(dwl1.in());
          assert_subscription_matched(drl2.in());
        }
      else if (collocation_str == "participant")
        {
          assert_publication_matched(dwl1.in());
          assert_subscription_matched(drl2.in());
        }
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception("Exception caught in main.cpp:");
      return 1;
    }
  catch (...)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) runtime exception caught in main.cpp.\n")), 1);
    }

  ACE_ERROR_RETURN((LM_INFO,
                    ACE_TEXT("(%P|%t) done.\n")), 0);
}
