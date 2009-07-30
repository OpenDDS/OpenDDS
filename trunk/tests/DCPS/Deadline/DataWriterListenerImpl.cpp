#include "DataWriterListenerImpl.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DCPS/Service_Participant.h"

#include <iostream>

using namespace std;


DataWriterListenerImpl::DataWriterListenerImpl ()
{
}

DataWriterListenerImpl::~DataWriterListenerImpl ()
{
}

void
DataWriterListenerImpl::on_offered_deadline_missed (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::OfferedDeadlineMissedStatus const & status)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
  ACE_DEBUG ((LM_DEBUG, "(%P|%t)%T DataWriterListenerImpl::on_offered_deadline_missed \n"));//rm
  ACE_DEBUG ((LM_DEBUG, "(%P|%t)%T total_count=%d total_count_change=%d last_instance_handle=%d \n",
    status.total_count, status.total_count_change, status.last_instance_handle));//rm
  //cerr << "DataWriterListenerImpl::on_offered_deadline_missed" << endl
  //     << "  total_count        = " << status.total_count << endl
  //     << "  total_count_change = " << status.total_count_change << endl;
}

void
DataWriterListenerImpl::on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::OfferedIncompatibleQosStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
DataWriterListenerImpl::on_liveliness_lost (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::LivelinessLostStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}
  
void
DataWriterListenerImpl::on_publication_matched (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::PublicationMatchedStatus const & /* status */)
    ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
DataWriterListenerImpl::on_publication_disconnected (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationDisconnectedStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
DataWriterListenerImpl::on_publication_reconnected (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationReconnectedStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
DataWriterListenerImpl::on_publication_lost (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationLostStatus const & /* status */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}

void
DataWriterListenerImpl::on_connection_deleted (
    ::DDS::DataWriter_ptr /* writer */)
  ACE_THROW_SPEC ((::CORBA::SystemException))
{
}
