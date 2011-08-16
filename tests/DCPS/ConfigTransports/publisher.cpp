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


#include <sys/syslog.h>

#include "common.h"
#include "Options.h"
#include "Factory.h"
#include "Pusher.h"
#include "DataWriterListenerImpl.h"

#include "dds/DCPS/Service_Participant.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif

#include "../common/TestSupport.h"
#include "../FooType4/FooDefTypeSupportImpl.h"

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      OpenDDS::DCPS::TypeSupport_var typsup = new Xyz::FooTypeSupportImpl;

      Options configopt(argc, argv);
      Factory fc(configopt, typsup);

      Options plainopt;
      Factory fp(plainopt, typsup);

      DDS::DataWriterListener_var drl1(new DataWriterListenerImpl);
      DDS::DataWriterListener_var drl2(new DataWriterListenerImpl);

      if (configopt.collocation_str == "none")
        {
          Pusher w(fc, dpf, fc.participant(dpf), drl1);

          TEST_ASSERT(wait_publication_matched_status(configopt, w.writer_));
          TEST_ASSERT(assert_supports_all(configopt, w.writer_));

          ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) End of writer ...\n")));
        }
      else if (configopt.collocation_str == "process")
        {
          DDS::DomainParticipant_var participant1(fc.participant(dpf));
          Pusher w1(fc, dpf, participant1, drl1);
          TEST_ASSERT(wait_publication_matched_status(configopt, w1.writer_));

          DDS::DomainParticipant_var participant2(fp.participant(dpf));
          Pusher w2(fp, dpf, participant2, drl2);
          TEST_ASSERT(wait_publication_matched_status(configopt, w2.writer_));

          TEST_ASSERT(assert_supports_all(configopt, w1.writer_));
          if (configopt.entity_str == "none")
            {
              TEST_ASSERT(assert_supports_all(configopt, w2.writer_));
            }
          else
            {
              TEST_ASSERT(!assert_supports_all(configopt, w2.writer_));
              TEST_ASSERT(assert_supports_all(plainopt, w2.writer_));
            }
        }
      else if (configopt.collocation_str == "participant")
        {
          DDS::DomainParticipant_var participant1(fc.participant(dpf));
          DDS::DomainParticipant_var participant2(participant1);

          DDS::Publisher_var publisher1(fc.publisher(participant1));
          Pusher w1(fc, dpf, participant1, publisher1, drl1);

          DDS::Publisher_var publisher2(fp.publisher(participant2));
          Pusher w2(fp, dpf, participant2, publisher2, drl2);

          TEST_ASSERT(assert_supports_all(configopt, w1.writer_));
          TEST_ASSERT(assert_supports_all(configopt, w2.writer_));
        }
      else if (configopt.collocation_str == "pubsub")
        {
          DDS::DomainParticipant_var participant1(fc.participant(dpf));
          DDS::DomainParticipant_var participant2(participant1);
          DDS::Publisher_var publisher1(fc.publisher(participant1));
          DDS::Publisher_var publisher2(publisher1);

          Pusher w1(fc, dpf, participant1, publisher1, drl1);
          Pusher w2(fp, dpf, participant2, publisher2, drl2);

          TEST_ASSERT(assert_supports_all(configopt, w1.writer_));
          TEST_ASSERT(assert_supports_all(configopt, w2.writer_));
        }

      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Shutting it all down ...\n")));
      TheServiceParticipant->shutdown();

      if (configopt.collocation_str == "none")
        {
          TEST_ASSERT(assert_publication_matched(configopt, drl1));
        }
      else if (configopt.collocation_str == "process")
        {
          TEST_ASSERT(assert_publication_matched(configopt, drl1)
                      && assert_publication_matched(configopt, drl2));
        }
      else if (configopt.collocation_str == "participant")
        {
          TEST_ASSERT(assert_publication_matched(configopt, drl1)
                      && assert_publication_matched(configopt, drl2));
        }
      else if (configopt.collocation_str == "pubsub")
        {
          TEST_ASSERT(assert_publication_matched(configopt, drl1)
                      && assert_publication_matched(configopt, drl2));
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
