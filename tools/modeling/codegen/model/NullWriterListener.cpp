
#include "NullWriterListener.h"
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::Model::NullWriterListener::NullWriterListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::NullWriterListener()\n")));
  }
}

OpenDDS::Model::NullWriterListener::~NullWriterListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::~NullWriterListener()\n")));
  }
}

void
OpenDDS::Model::NullWriterListener::on_offered_deadline_missed(
  DDS::DataWriter_ptr /* writer */,
  const DDS::OfferedDeadlineMissedStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::on_offered_deadline_missed()\n")));
  }
}

void
OpenDDS::Model::NullWriterListener::on_offered_incompatible_qos(
  DDS::DataWriter_ptr /* writer */,
  const DDS::OfferedIncompatibleQosStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::on_offered_incompatible_qos()\n")));
  }
}

void
OpenDDS::Model::NullWriterListener::on_liveliness_lost(
  DDS::DataWriter_ptr /* writer */,
  const DDS::LivelinessLostStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::on_liveliness_lost()\n")));
  }
}

void
OpenDDS::Model::NullWriterListener::on_publication_matched(
  DDS::DataWriter_ptr /* writer */,
  const DDS::PublicationMatchedStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::on_publication_matched()\n")));
  }
}

void
OpenDDS::Model::NullWriterListener::on_publication_disconnected(
  DDS::DataWriter_ptr /* writer */,
  const OpenDDS::DCPS::PublicationDisconnectedStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::on_publication_disconnected()\n")));
  }
}

void
OpenDDS::Model::NullWriterListener::on_publication_reconnected(
  DDS::DataWriter_ptr /* writer */,
  const OpenDDS::DCPS::PublicationReconnectedStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::on_publication_reconnected()\n")));
  }
}

void
OpenDDS::Model::NullWriterListener::on_publication_lost(
  DDS::DataWriter_ptr /* writer */,
  const OpenDDS::DCPS::PublicationLostStatus& /* status */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::on_publication_lost()\n")));
  }
}

void
OpenDDS::Model::NullWriterListener::on_connection_deleted(
  DDS::DataWriter_ptr /* writer */
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullWriterListener::on_connection_deleted()\n")));
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
