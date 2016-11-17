// -*- C++ -*-
//

#include "NullListener.h"
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::Model::NullListener::NullListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullListener::NullListener()\n")));
  }
}

OpenDDS::Model::NullListener::~NullListener (void)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullListener::~NullListener()\n")));
  }
}

void
OpenDDS::Model::NullListener::on_requested_deadline_missed (
    DDS::DataReader_ptr /* reader */,
    const DDS::RequestedDeadlineMissedStatus& /* status */)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ")
               ACE_TEXT("NullListener::on_requested_deadline_missed()\n")));
  }
}

void
OpenDDS::Model::NullListener::on_requested_incompatible_qos (
  DDS::DataReader_ptr /* reader */,
  const DDS::RequestedIncompatibleQosStatus& /* status */)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullListener::")
               ACE_TEXT("on_requested_incompatible_qos()\n")));
  }
}

void
OpenDDS::Model::NullListener::on_liveliness_changed (
    DDS::DataReader_ptr /* reader */,
    const DDS::LivelinessChangedStatus& /* status */)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullListener::on_liveliness_changed()\n")));
  }
}

void
OpenDDS::Model::NullListener::on_subscription_match (
    DDS::DataReader_ptr /* reader */,
    const DDS::SubscriptionMatchedStatus& /* status */)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullListener::on_subscription_match()\n")));
  }
}

void
OpenDDS::Model::NullListener::on_sample_rejected(
    DDS::DataReader_ptr /* reader */,
   const DDS::SampleRejectedStatus& /* status */)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullListener::on_sample_rejected()\n")));
  }
}

void
OpenDDS::Model::NullListener::on_data_available(::DDS::DataReader_ptr /* reader */)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullListener::on_data_available()\n")));
  }
}

void
OpenDDS::Model::NullListener::on_sample_lost(::DDS::DataReader_ptr /* reader */,
                                  const DDS::SampleLostStatus& /* status */)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullListener::on_sample_lost()\n")));
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
