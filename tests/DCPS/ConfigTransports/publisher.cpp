// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================


#include "common.h"
#include "Options.h"
#include "Factory.h"

#include "Pusher.h"
#include "DataWriterListenerImpl.h"

#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "../common/TestSupport.h"
#include "../FooType4/FooDefTypeSupportImpl.h"
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#include <stdexcept>

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
  {
    DDS::DomainParticipant_var participant1;
    DDS::DomainParticipant_var participant2;

    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    {
      OpenDDS::DCPS::TypeSupport_var typsup = new Xyz::FooTypeSupportImpl;

      Options configopt(argc, argv);
      Factory fc(configopt, typsup);

      Options plainopt;
      Factory fp(plainopt, typsup);

      DDS::DataWriterListener_var dwl1(new DataWriterListenerImpl);
      DDS::DataWriterListener_var dwl2(new DataWriterListenerImpl);

      if (configopt.collocation_str == "none")
      {
        participant1 = fc.participant(dpf);
        Pusher w(fc, dpf, participant1, dwl1);
        TEST_ASSERT(assert_supported(configopt, w.writer_));

        w.push(ACE_Time_Value(configopt.test_duration));
        if (!configopt.negotiated_str.empty())
        {
          TEST_ASSERT(assert_negotiated(configopt, w.writer_));
        }
      }
      else if (configopt.collocation_str == "process")
      {
        participant1 = fc.participant(dpf);
        Pusher w1(fc, dpf, participant1, dwl1);
//          TEST_ASSERT(wait_publication_matched_status(configopt, w1.writer_));

        participant2 = fp.participant(dpf);
        Pusher w2(fp, dpf, participant2, dwl2);
//          TEST_ASSERT(wait_publication_matched_status(configopt, w2.writer_));

        TEST_ASSERT(assert_supported(configopt, w1.writer_));
        if (configopt.entity_str == "none")
        {
          TEST_ASSERT(assert_supported(configopt, w2.writer_));
        }
        else
        {
          TEST_ASSERT(!assert_supported(configopt, w2.writer_));
          TEST_ASSERT(assert_supported(plainopt, w2.writer_));
        }

        w1.push(ACE_Time_Value(1));
        if (!configopt.negotiated_str.empty())
        {
          TEST_ASSERT(assert_negotiated(configopt, w1.writer_));
        }
      }
      else if (configopt.collocation_str == "participant")
      {
        participant1 = fc.participant(dpf);
        participant2 = participant1;

        DDS::Publisher_var publisher1(fc.publisher(participant1));
        Pusher w1(fc, dpf, participant1, publisher1, dwl1);

        DDS::Publisher_var publisher2(fp.publisher(participant2));
        Pusher w2(fp, dpf, participant2, publisher2, dwl2);

        TEST_ASSERT(assert_supported(configopt, w1.writer_));
        TEST_ASSERT(assert_supported(configopt, w2.writer_));

        w1.push(ACE_Time_Value(1));
        if (!configopt.negotiated_str.empty())
        {
          TEST_ASSERT(assert_negotiated(configopt, w1.writer_));
        }
      }
      else if (configopt.collocation_str == "pubsub")
      {
        participant1 = fc.participant(dpf);
        participant2 = participant1;
        DDS::Publisher_var publisher1(fc.publisher(participant1));
        DDS::Publisher_var publisher2(publisher1);

        Pusher w1(fc, dpf, participant1, publisher1, dwl1);
        Pusher w2(fp, dpf, participant2, publisher2, dwl2);

        TEST_ASSERT(assert_supported(configopt, w1.writer_));
        TEST_ASSERT(assert_supported(configopt, w2.writer_));

        w1.push(ACE_Time_Value(1));
        if (!configopt.negotiated_str.empty())
        {
          TEST_ASSERT(assert_negotiated(configopt, w1.writer_));
        }
      }

      if (configopt.collocation_str == "none")
      {
        TEST_ASSERT(assert_publication_matched(configopt, dwl1));
      }
      else if (configopt.collocation_str == "process")
      {
        TEST_ASSERT(assert_publication_matched(configopt, dwl1)
                    && assert_publication_matched(configopt, dwl2));
      }
      else if (configopt.collocation_str == "participant")
      {
        TEST_ASSERT(assert_publication_matched(configopt, dwl1)
                    && assert_publication_matched(configopt, dwl2));
      }
      else if (configopt.collocation_str == "pubsub")
      {
        TEST_ASSERT(assert_publication_matched(configopt, dwl1)
                    && assert_publication_matched(configopt, dwl2));
      }
    }

    {
      ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, shutdown_lock, 1);
      shutdown_flag = true;
    }

    // only want to clean up participant2 if it isn't just pointing to
    // participant1
    if (participant1.in() == participant2.in()) {
      participant2 = 0;
    }
    // Clean up
    if (participant1) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) deleting entities1\n")));
      participant1->delete_contained_entities();
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) deleting participant1\n")));
      dpf->delete_participant(participant1);
    }
    if (participant2) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) deleting entities2\n")));
      participant2->delete_contained_entities();
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) deleting participant2\n")));
      dpf->delete_participant(participant2);
    }
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Shutting publisher down ...\n")));
    TheServiceParticipant->shutdown();
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Publisher shutdown complete.\n")));
  }
  catch (char const *ex)
  {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) Assertion failed.\n"), ex), -1);
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception("Exception caught in main.cpp:");
    return 1;
  }
  catch (const std::runtime_error& rte)
  {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) main() exception: %s\n"), rte.what()), -1);
  }
  catch (const OpenDDS::DCPS::Transport::MiscProblem& )
  {
    ACE_ERROR_RETURN((LM_ERROR,
                  ACE_TEXT("(%P|%t) Transport::MiscProblem caught.\n")), -1);
  }

  ACE_ERROR_RETURN((LM_INFO,
                    ACE_TEXT("(%P|%t) done.\n")), 0);
}
