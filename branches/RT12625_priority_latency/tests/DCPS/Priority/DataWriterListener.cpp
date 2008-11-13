// $Id$

#include "DataWriterListener.h"

Test::DataWriterListener::DataWriterListener()
{
}

void
Test::DataWriterListener::on_offered_deadline_missed (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::OfferedDeadlineMissedStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
Test::DataWriterListener::on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::OfferedIncompatibleQosStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
Test::DataWriterListener::on_liveliness_lost (
    ::DDS::DataWriter_ptr /* writer */,
    const ::DDS::LivelinessLostStatus & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}
  
void
Test::DataWriterListener::on_publication_match (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::PublicationMatchStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
Test::DataWriterListener::on_publication_disconnected (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationDisconnectedStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
Test::DataWriterListener::on_publication_reconnected (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationReconnectedStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
Test::DataWriterListener::on_publication_lost (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationLostStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
Test::DataWriterListener::on_connection_deleted (
    ::DDS::DataWriter_ptr /* writer */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

