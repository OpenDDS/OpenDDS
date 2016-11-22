/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "FailoverListener.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

FailoverListener::FailoverListener(Discovery::RepoKey key)
  : key_(key)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::FailoverListener( key==%C)\n"),
               key.c_str()));
  }
}

FailoverListener::~FailoverListener()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::~FailoverListener\n")));
  }
}

void
FailoverListener::on_data_available(
  DDS::DataReader_ptr /* reader */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_data_available\n")));
  }
}

void
FailoverListener::on_requested_deadline_missed(
  DDS::DataReader_ptr /* reader */,
  const DDS::RequestedDeadlineMissedStatus & /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ")
               ACE_TEXT("Federatorer::on_requested_deadline_missed\n")));
  }
}

void
FailoverListener::on_requested_incompatible_qos(
  DDS::DataReader_ptr /* reader */,
  const DDS::RequestedIncompatibleQosStatus & /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::")
               ACE_TEXT("on_requested_incompatible_qos\n")));
  }
}

void
FailoverListener::on_liveliness_changed(
  DDS::DataReader_ptr /* reader */,
  const DDS::LivelinessChangedStatus & /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_liveliness_changed\n")));
  }
}

void
FailoverListener::on_subscription_matched(
  DDS::DataReader_ptr /* reader */,
  const DDS::SubscriptionMatchedStatus & /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_subscription_matched\n")));
  }
}

void
FailoverListener::on_sample_rejected(
  DDS::DataReader_ptr /* reader */,
  const DDS::SampleRejectedStatus& /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_sample_rejected\n")));
  }
}

void
FailoverListener::on_sample_lost(
  DDS::DataReader_ptr /* reader */,
  const DDS::SampleLostStatus& /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_sample_lost\n")));
  }
}

void
FailoverListener::on_subscription_disconnected(
  DDS::DataReader_ptr /* reader */,
  const OpenDDS::DCPS::SubscriptionDisconnectedStatus& /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_subscription_disconnected\n")));
  }
  TheServiceParticipant->repository_lost(this->key_);
}

void
FailoverListener::on_subscription_reconnected(
  DDS::DataReader_ptr /* reader */,
  const OpenDDS::DCPS::SubscriptionReconnectedStatus& /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_subscription_reconnected\n")));
  }
}

void
FailoverListener::on_subscription_lost(
  DDS::DataReader_ptr /* reader */,
  const OpenDDS::DCPS::SubscriptionLostStatus& /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_subscription_lost: ")
               ACE_TEXT("initiating failover sequencing.\n")));
  }

  TheServiceParticipant->repository_lost(this->key_);
}

void
FailoverListener::on_connection_deleted(
  DDS::DataReader_ptr /* reader */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_connection_deleted\n")));
  }
}

void
FailoverListener::on_budget_exceeded(
  DDS::DataReader_ptr /* reader */,
  const OpenDDS::DCPS::BudgetExceededStatus& /* status */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) FailoverListener::on_budget_exceeded\n")));
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
