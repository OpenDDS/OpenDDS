
#include "DataWriterListenerImpl.h"
#include "dds/DdsDcpsPublicationC.h"

DataWriterListenerImpl::DataWriterListenerImpl ()
  : publication_matched_ (false)
{
}

DataWriterListenerImpl::~DataWriterListenerImpl ()
{
}

void
DataWriterListenerImpl::on_offered_deadline_missed (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::OfferedDeadlineMissedStatus const & /* status */)
{
}

void
DataWriterListenerImpl::on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::OfferedIncompatibleQosStatus const & /* status */)
{
}

void
DataWriterListenerImpl::on_liveliness_lost (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::LivelinessLostStatus const & /* status */)
{
}

void
DataWriterListenerImpl::on_publication_matched (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::PublicationMatchedStatus const & /* status */)
{
  this->publication_matched_ = true;

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::")
             ACE_TEXT("on_publication_matched\n")));
}

void
DataWriterListenerImpl::on_publication_disconnected (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationDisconnectedStatus const & /* status */)
{
}

void
DataWriterListenerImpl::on_publication_reconnected (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationReconnectedStatus const & /* status */)
{
}

void
DataWriterListenerImpl::on_publication_lost (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationLostStatus const & /* status */)
{
}

void
DataWriterListenerImpl::on_connection_deleted (
    ::DDS::DataWriter_ptr /* writer */)
{
}
