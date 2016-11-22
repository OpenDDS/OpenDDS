// -*- C++ -*-
//

#include "NullReaderListener.h"
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::Model::NullReaderListener::NullReaderListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullReaderListener::NullReaderListener()\n")));
  }
}

OpenDDS::Model::NullReaderListener::~NullReaderListener (void)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullReaderListener::~NullReaderListener()\n")));
  }
}

void
OpenDDS::Model::NullReaderListener::on_requested_deadline_missed (
  DDS::DataReader_ptr /* reader */,
  const DDS::RequestedDeadlineMissedStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ")
               ACE_TEXT("NullReaderListener::on_requested_deadline_missed()\n")));
  }
}

void
OpenDDS::Model::NullReaderListener::on_requested_incompatible_qos (
  DDS::DataReader_ptr /* reader */,
  const DDS::RequestedIncompatibleQosStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullReaderListener::")
               ACE_TEXT("on_requested_incompatible_qos()\n")));
  }
}

void
OpenDDS::Model::NullReaderListener::on_liveliness_changed (
  DDS::DataReader_ptr /* reader */,
  const DDS::LivelinessChangedStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullReaderListener::on_liveliness_changed()\n")));
  }
}

void
OpenDDS::Model::NullReaderListener::on_subscription_matched (
  DDS::DataReader_ptr /* reader */,
  const DDS::SubscriptionMatchedStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullReaderListener::on_subscription_matched()\n")));
  }
}

void
OpenDDS::Model::NullReaderListener::on_sample_rejected(
  DDS::DataReader_ptr /* reader */,
  const DDS::SampleRejectedStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullReaderListener::on_sample_rejected()\n")));
  }
}

void
OpenDDS::Model::NullReaderListener::on_data_available(
  DDS::DataReader_ptr /* reader */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullReaderListener::on_data_available()\n")));
  }
}

void
OpenDDS::Model::NullReaderListener::on_sample_lost(
  DDS::DataReader_ptr /* reader */,
  const DDS::SampleLostStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullReaderListener::on_sample_lost()\n")));
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
