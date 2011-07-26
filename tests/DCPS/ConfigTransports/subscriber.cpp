// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#include "common.h"
#include "Reader.h"
#include "DataReaderListener.h"

#include "dds/DCPS/Service_Participant.h"
#include "tao/Object.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif

#include "../common/TestSupport.h"
#include "tests/DCPS/CompatibilityTest/common.h"

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
    {
      DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      Options configopt(argc, argv);
      Factory fconfig(configopt);

      Options plainopt;
      Factory fplain(plainopt);

      DDS::DataReaderListener_var drl1(new DataReaderListenerImpl);
      DDS::DataReaderListener_var drl2(new DataReaderListenerImpl);

      if (configopt.collocation_str == "none")
        {
          DDS::DomainParticipant_var participant(fconfig.participant(dpf.in()));
          Reader r(fconfig, dpf.in(), participant.in(), drl1.in());
          TEST_ASSERT(wait_publication_matched_status(configopt, r.reader_.in()));
          TEST_ASSERT(assert_supports_all(configopt, r.reader_.in()));
        }
      else if (configopt.collocation_str == "process")
        {
          DDS::DomainParticipant_var participant1(fconfig.participant(dpf.in()));
          Reader r1(fconfig, dpf.in(), participant1.in(), drl1.in());
          TEST_ASSERT(wait_publication_matched_status(configopt, r1.reader_.in()));

          DDS::DomainParticipant_var participant2(fplain.participant(dpf.in()));
          Reader r2(fplain, dpf.in(), participant2.in(), drl2.in());
          TEST_ASSERT(wait_publication_matched_status(configopt, r2.reader_.in()));

          TEST_ASSERT (participant1.in() != participant2.in());


          // Wait for things to settle ?!
          ACE_OS::sleep(configopt.test_duration);

          TEST_ASSERT(assert_supports_all(configopt, r1.reader_.in()));
          if (configopt.entity_str == "none")
            {
              TEST_ASSERT(assert_supports_all(configopt, r2.reader_.in()));
            }
          else
            {
              TEST_ASSERT(!assert_supports_all(configopt, r2.reader_.in()));
            }
        }
      else if (configopt.collocation_str == "participant")
        {
          DDS::DomainParticipant_var participant1(fconfig.participant(dpf.in()));
          DDS::DomainParticipant_var participant2(participant1);

          Reader r1(fconfig, dpf.in(), participant1.in(), drl1.in());

          Reader r2(fplain, dpf.in(), participant2.in(), drl2.in());

          // Wait for things to settle ?!
          ACE_OS::sleep(configopt.test_duration);

          TEST_ASSERT(assert_supports_all(configopt, r1.reader_.in()));
          TEST_ASSERT(assert_supports_all(configopt, r2.reader_.in()));

        }

      else if (configopt.collocation_str == "pubsub")
        {
          DDS::DomainParticipant_var participant1(fconfig.participant(dpf.in()));
          DDS::DomainParticipant_var participant2(participant1);

          DDS::Subscriber_var subscriber1(fconfig.subscriber(participant1.in()));
          Reader r1(fconfig, dpf.in(), participant1.in(), subscriber1.in(), drl1.in());

          DDS::Subscriber_var subscriber2(fplain.subscriber(participant2.in()));
          Reader r2(fplain, dpf.in(), participant2.in(), subscriber2.in(), drl1.in());

          // Wait for things to settle ?!
          ACE_OS::sleep(configopt.test_duration);

          TEST_ASSERT(assert_supports_all(configopt, r1.reader_.in()));
          TEST_ASSERT(assert_supports_all(configopt, r2.reader_.in()));

        }

      TheServiceParticipant->shutdown();

      if (configopt.collocation_str == "none")
        {
          TEST_ASSERT(assert_subscription_matched(configopt, drl1.in()));
        }
      else if (configopt.collocation_str == "process")
        {
          TEST_ASSERT(assert_subscription_matched(configopt, drl1.in())
                      && assert_subscription_matched(configopt, drl2.in()));
        }
      else if (configopt.collocation_str == "participant")
        {
          TEST_ASSERT(assert_subscription_matched(configopt, drl1.in())
                      && assert_subscription_matched(configopt, drl2.in()));
        }
      else if (configopt.collocation_str == "pubsub")
        {
          TEST_ASSERT(assert_subscription_matched(configopt, drl1.in())
                      && assert_subscription_matched(configopt, drl2.in()));
        }
    }
  catch (const CORBA::Exception &ex)
    {
      ex._tao_print_exception("Exception caught in main.cpp:");
      return 1;
    }

  ACE_ERROR_RETURN((LM_INFO,
                    ACE_TEXT("(%P|%t) done.\n")), 0);
}
