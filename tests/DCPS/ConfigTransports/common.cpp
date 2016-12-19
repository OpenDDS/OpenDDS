// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.cpp
 *
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

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include "dds/DCPS/transport/udp/Udp.h"
#include "dds/DCPS/transport/multicast/Multicast.h"
#endif
#include "dds/DCPS/transport/framework/TransportClient.h"

ACE_Thread_Mutex shutdown_lock;
bool shutdown_flag = false;

bool
assert_subscription_matched(const Options& opts, const DDS::DataReaderListener_var& drl)
{
  // Assert if pub/sub made a match ...

  DataReaderListenerImpl* drl_servant =
          dynamic_cast<DataReaderListenerImpl*> (drl.in());

  // there is an error if we matched when not compatible (or vice-versa)
  if (opts.compatible != drl_servant->subscription_matched() && opts.reliability_kind == DDS::RELIABLE_RELIABILITY_QOS)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) Expected publication_matched to be %C, but it was %C [")
                        ACE_TEXT(" durability_kind=%C, liveliness_kind=%C, liveliness_duration=%C, ")
                        ACE_TEXT("reliability_kind=%C]\n"),
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
  if (opts.compatible != dwl_servant->publication_matched() && opts.reliability_kind == DDS::RELIABLE_RELIABILITY_QOS)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) Expected publication_matched to be %C, but it was %C [")
                        ACE_TEXT(" durability_kind=%C, liveliness_kind=%C, liveliness_duration=%C, ")
                        ACE_TEXT("reliability_kind=%C]\n"),
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
assert_supported(const Options& opts, const DDS::Entity_ptr e)
{
  TEST_ASSERT(e != 0);
  OpenDDS::DCPS::TransportClient* tc = dynamic_cast<OpenDDS::DCPS::TransportClient*> (e);
  TEST_ASSERT(tc != 0);

  const std::vector<OPENDDS_STRING>& transporti = opts.protocol_str;

  // Assert effective transport protocols
  size_t left = transporti.size();
  for (std::vector < OPENDDS_STRING>::const_iterator proto = transporti.begin();
          proto < transporti.end(); proto++)
    {
//      ACE_DEBUG((LM_INFO,
//                 ACE_TEXT("(%P|%t) Entity '%C' supports protocol '%C'?\n"),
//                 opts.entity_str.c_str(),
//                 proto->c_str()));

      if (::DDS_TEST::supports(tc, *proto)) left--;
    }

  return (0 == left);
}

bool
assert_negotiated(const Options& opts, const DDS::Entity_ptr e)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, shutdown_lock, false);
  if (shutdown_flag) {
    return true;
  }

  TEST_ASSERT(e != 0);
  OpenDDS::DCPS::TransportClient* tc = dynamic_cast<OpenDDS::DCPS::TransportClient*> (e);
  TEST_ASSERT(tc != 0);

  // Assert negotiated transport protocols
  if (0 == opts.negotiated_str.size())
    {
      return true;
    }

  for (std::vector < OPENDDS_STRING>::const_iterator proto = opts.negotiated_str.begin();
          proto < opts.negotiated_str.end(); proto++)
    {
//      ACE_DEBUG((LM_INFO,
//                 ACE_TEXT("(%P|%t) Entity '%C' negotiated protocol '%C'?\n"),
//                 opts.entity_str.c_str(),
//                 proto->c_str()));

      if (::DDS_TEST::negotiated(tc, *proto))
        return true;
    }

  return false;
}

bool
wait_subscription_matched_status(const Options& /*opts*/, const DDS::DataReader_ptr r)
{
  //  To check the match status ?
  DDS::SubscriptionMatchedStatus matches = {0, 0, 0, 0, 0};
  TEST_ASSERT((r->get_subscription_matched_status(matches) == ::DDS::RETCODE_OK));

  // Block until Subscriber is available
  DDS::StatusCondition_var condition = r->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS
                                  | DDS::SUBSCRIPTION_MATCHED_STATUS
                                  //                                  | DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS
                                  //                                  | DDS::OFFERED_INCOMPATIBLE_QOS_STATUS
                                  );

  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);

//  int duration = opts.test_duration;
  DDS::Duration_t timeout = {
    DDS::DURATION_INFINITE_SEC,
    DDS::DURATION_INFINITE_NSEC
//    (duration < 0) ? DDS::DURATION_INFINITE_SEC : duration,
//    (duration < 0) ? DDS::DURATION_INFINITE_NSEC : 0
  };

  DDS::ConditionSeq conditions;

  int status = ws->wait(conditions, timeout);
  ws->detach_condition(condition);

  if (status != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t)")
                        ACE_TEXT(" ERROR: wait failed: %p\n")), false);
    }

  return true;
}
bool
wait_publication_matched_status(const Options& opts, const DDS::DataWriter_ptr w)
{
  //  To check the match status ?
  DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};
  TEST_ASSERT((w->get_publication_matched_status(matches) == ::DDS::RETCODE_OK));

  // Block until Subscriber is available
  DDS::StatusCondition_var condition = w->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS
                                  | DDS::SUBSCRIPTION_MATCHED_STATUS
                                  //                                  | DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS
                                  //                                  | DDS::OFFERED_INCOMPATIBLE_QOS_STATUS
                                  );

  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);

  int duration = opts.test_duration;
  DDS::Duration_t timeout = {
    (duration < 0) ? DDS::DURATION_INFINITE_SEC : duration,
    (duration < 0) ? DDS::DURATION_INFINITE_NSEC : 0
  };

  DDS::ConditionSeq conditions;

  int status = ws->wait(conditions, timeout);
  ws->detach_condition(condition);

  if (status != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|$t)")
                        ACE_TEXT(" ERROR: wait failed at %N:%l\n")), false);
    }

  return true;
}


