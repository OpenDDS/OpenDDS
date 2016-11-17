// -*- C++ -*-
//
#include "NullSubscriberListener.h"
#include <dds/DCPS/debug.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::Model::NullSubscriberListener::NullSubscriberListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::NullSubscriberListener()\n")));
  }
}

OpenDDS::Model::NullSubscriberListener::~NullSubscriberListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::~NullSubscriberListener()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_data_on_readers(
  DDS::Subscriber_ptr /* subs */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_data_on_readers()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_data_available(
  DDS::DataReader_ptr
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_data_available()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus&
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_requested_deadline_missed()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus&
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_requested_incompatible_qos()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus&
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_liveliness_changed()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus&
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_subscription_matched()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_sample_rejected()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_sample_lost()\n")));
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
