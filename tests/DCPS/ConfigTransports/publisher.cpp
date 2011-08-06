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
#include "Writer.h"
#include "DataWriterListenerImpl.h"

#include "dds/DCPS/Service_Participant.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif

#include "../common/TestSupport.h"

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      Options configopt(argc, argv);
      Factory fc(configopt);

      Options plainopt;
      Factory fp(plainopt);

      DDS::DataWriterListener_var drl1(new DataWriterListenerImpl);
      DDS::DataWriterListener_var drl2(new DataWriterListenerImpl);

      if (configopt.collocation_str == "none")
        {
          Writer w(fc, dpf.in(), fc.participant(dpf.in()), drl1.in());

          TEST_ASSERT(wait_publication_matched_status(configopt, w.writer_.in()));
          TEST_ASSERT(assert_supports_all(configopt, w.writer_.in()));
        }
      else if (configopt.collocation_str == "process")
        {
          DDS::DomainParticipant_var participant1(fc.participant(dpf.in()));
          Writer w1(fc, dpf.in(), participant1.in(), drl1.in());
          TEST_ASSERT(wait_publication_matched_status(configopt, w1.writer_.in()));

          DDS::DomainParticipant_var participant2(fp.participant(dpf.in()));
          Writer w2(fp, dpf.in(), participant2.in(), drl2.in());
          TEST_ASSERT(wait_publication_matched_status(configopt, w2.writer_.in()));

          TEST_ASSERT(assert_supports_all(configopt, w1.writer_.in()));
          if (configopt.entity_str == "none")
            {
              TEST_ASSERT(assert_supports_all(configopt, w2.writer_.in()));
            }
          else
            {
              TEST_ASSERT(!assert_supports_all(configopt, w2.writer_.in()));
              TEST_ASSERT(assert_supports_all(plainopt, w2.writer_.in()));
            }
        }
      else if (configopt.collocation_str == "participant")
        {
          DDS::DomainParticipant_var participant1(fc.participant(dpf.in()));
          DDS::DomainParticipant_var participant2(participant1);

          DDS::Publisher_var publisher1(fc.publisher(participant1.in()));
          Writer w1(fc, dpf.in(), participant1.in(), publisher1.in(), drl1.in());

          DDS::Publisher_var publisher2(fp.publisher(participant2.in()));
          Writer w2(fp, dpf.in(), participant2.in(), publisher2.in(), drl2.in());

          TEST_ASSERT(assert_supports_all(configopt, w1.writer_.in()));
          TEST_ASSERT(assert_supports_all(configopt, w2.writer_.in()));
        }
      else if (configopt.collocation_str == "pubsub")
        {
          DDS::DomainParticipant_var participant1(fc.participant(dpf.in()));
          DDS::DomainParticipant_var participant2(participant1);
          DDS::Publisher_var publisher1(fc.publisher(participant1.in()));
          DDS::Publisher_var publisher2(publisher1);

          Writer w1(fc, dpf.in(), participant1.in(), publisher1.in(), drl1.in());
          Writer w2(fp, dpf.in(), participant2.in(), publisher2.in(), drl2.in());

          TEST_ASSERT(assert_supports_all(configopt, w1.writer_.in()));
          TEST_ASSERT(assert_supports_all(configopt, w2.writer_.in()));
        }

      TheServiceParticipant->shutdown();

      if (configopt.collocation_str == "none")
        {
          TEST_ASSERT(assert_publication_matched(configopt, drl1.in()));
        }
      else if (configopt.collocation_str == "process")
        {
          TEST_ASSERT(assert_publication_matched(configopt, drl1.in())
                      && assert_publication_matched(configopt, drl2.in()));
        }
      else if (configopt.collocation_str == "participant")
        {
          TEST_ASSERT(assert_publication_matched(configopt, drl1.in())
                      && assert_publication_matched(configopt, drl2.in()));
        }
      else if (configopt.collocation_str == "pubsub")
        {
          TEST_ASSERT(assert_publication_matched(configopt, drl1.in())
                      && assert_publication_matched(configopt, drl2.in()));
        }
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception("Exception caught in main.cpp:");
      return 1;
    }

  ACE_ERROR_RETURN((LM_INFO,
                    ACE_TEXT("(%P|%t) done.\n")), 0);
}
