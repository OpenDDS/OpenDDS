// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#include "common.h"
#include "DDSTEST.h"
#include "DataWriterListenerImpl.h"
#include "Options.h"
#include "Factory.h"

#include "../common/TestSupport.h"

#include "../FooType4/FooDefTypeSupportImpl.h"

#include "dds/DCPS/WaitSet.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/transport/tcp/Tcp.h"
# include "dds/DCPS/transport/udp/Udp.h"
# include "dds/DCPS/transport/multicast/Multicast.h"
#endif





bool
assert_subscription_matched(const Options& opts, const DDS::DataReaderListener_var& drl)
{
  // Assert if pub/sub made a match ...

  DataReaderListenerImpl* drl_servant =
          dynamic_cast<DataReaderListenerImpl*> (drl.in());

  // there is an error if we matched when not compatible (or vice-versa)
  if (opts.compatible != drl_servant->subscription_matched())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) Expected publication_matched to be %C, but it was %C [")
                        ACE_TEXT(" durability_kind=%s, liveliness_kind=%s, liveliness_duration=%s, ")
                        ACE_TEXT("reliability_kind=%s]\n"),
                        (opts.compatible) ? "true" : "false",
                        (drl_servant->subscription_matched()) ? "true" : "false",
                        opts.durability_kind_str.c_str(),
                        opts.liveliness_kind_str.c_str(),
                        opts.LEASE_DURATION_STR.c_str(),
                        opts.reliability_kind_str.c_str()),
                       false);
    }

  return true;
}

bool
assert_publication_matched(const Options& opts, const DDS::DataWriterListener_var& dwl)
{
  // Assert if pub/sub made a match ...
  DataWriterListenerImpl* dwl_servant =
          dynamic_cast<DataWriterListenerImpl*> (dwl.in());

  // check to see if the publisher worked
  if (opts.compatible != dwl_servant->publication_matched())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) Expected publication_matched to be %C, but it was %C [")
                        ACE_TEXT(" durability_kind=%s, liveliness_kind=%s, liveliness_duration=%s, ")
                        ACE_TEXT("reliability_kind=%s]\n"),
                        (opts.compatible) ? "true" : "false",
                        (dwl_servant->publication_matched()) ? "true" : "false",
                        opts.durability_kind_str.c_str(),
                        opts.liveliness_kind_str.c_str(),
                        opts.LEASE_DURATION_STR.c_str(),
                        opts.reliability_kind_str.c_str()),
                       false);
    }

  return true;
}

bool
assert_supports_all(const Options& f, const DDS::Entity_ptr e)
{
  const OpenDDS::DCPS::TransportClient* tc = dynamic_cast<const OpenDDS::DCPS::TransportClient*> (e);
  TEST_ASSERT(tc != 0);

  return assert_supports_all(f, tc, f.protocol_str);
}

bool
assert_supports_all(const Options& opts, const OpenDDS::DCPS::TransportClient* tc, const std::vector<std::string>& transporti)
{
  TEST_ASSERT(tc != 0);

  // Assert effective transport protocols
  size_t left = transporti.size();
  for (std::vector < std::string>::const_iterator proto = transporti.begin();
          proto < transporti.end(); proto++)
    {
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Entity '%C' supports protocol '%C'?\n"),
                 opts.entity_str.c_str(),
                 proto->c_str()));

      if (::DDS_TEST::supports(tc, *proto)) left--;
    }

  return (0 == left);
}

bool
assert_negotiated(const Options& opts, const DDS::Entity_ptr e)
{
  TEST_ASSERT(e != 0);
  OpenDDS::DCPS::TransportClient* tc = dynamic_cast<OpenDDS::DCPS::TransportClient*> (e);
  TEST_ASSERT(tc != 0);

  const std::vector<std::string>& transporti = opts.negotiated_str;

  // Assert negotiated transport protocols
  size_t left = transporti.size();
  for (std::vector < std::string>::const_iterator proto = transporti.begin();
          proto < transporti.end(); proto++)
    {
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Entity '%C' negotiated protocol '%C'?\n"),
                 opts.entity_str.c_str(),
                 proto->c_str()));

      if (::DDS_TEST::negotiated(tc, *proto)) left--;
    }

  return (0 == left);
}

bool
wait_publication_matched_status(const Options& opts, const DDS::Entity_ptr /*reader_*/)
{

  int duration = opts.test_duration;

  ACE_OS::sleep(duration);
  return true;


  // To check the match status?
  //          DDS::SubscriptionMatchedStatus matches = {0, 0, 0, 0, 0};
  //          TEST_ASSERT((r.reader_->get_subscription_matched_status(matches) == ::DDS::RETCODE_OK));
  //          TEST_ASSERT(matches.current_count > 0);
  //
  //          DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};
  //          TEST_ASSERT((w.writer_->get_publication_matched_status(matches) == ::DDS::RETCODE_OK));
  //          TEST_ASSERT(matches.current_count > 0);


  //  // Block until Subscriber is available
  //  DDS::StatusCondition_var condition = writer_->get_statuscondition();
  //  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS
  //                                  | DDS::SUBSCRIPTION_MATCHED_STATUS
  //                                  //                                  | DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS
  //                                  //                                  | DDS::OFFERED_INCOMPATIBLE_QOS_STATUS
  //                                  );
  //
  //  DDS::WaitSet_var ws = new DDS::WaitSet;
  //  ws->attach_condition(condition);
  //
  //  DDS::Duration_t timeout = {
  //    (duration < 0) ? DDS::DURATION_INFINITE_SEC : duration,
  //    DDS::DURATION_INFINITE_NSEC
  //  };
  //
  //  DDS::ConditionSeq conditions;
  //
  //  int status = ws->wait(conditions, timeout);
  //  ws->detach_condition(condition);
  //
  //  if (status != DDS::RETCODE_OK)
  //    {
  //      ACE_ERROR_RETURN((LM_ERROR,
  //                        ACE_TEXT("(%P|$t)")
  //                        ACE_TEXT(" ERROR: wait failed at %N:%l\n")), false);
  //    }
  //
  //  return true;
}


