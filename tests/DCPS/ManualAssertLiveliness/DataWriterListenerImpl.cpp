#include "DataWriterListenerImpl.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DCPS/Service_Participant.h"

DataWriterListenerImpl::DataWriterListenerImpl ()
: num_liveliness_lost_callbacks_(0)
{
}

DataWriterListenerImpl::~DataWriterListenerImpl ()
{
}

void DataWriterListenerImpl::on_offered_deadline_missed (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::OfferedDeadlineMissedStatus & status
    )
{
  ACE_UNUSED_ARG(writer);
  ACE_UNUSED_ARG(status);
}

void DataWriterListenerImpl::on_offered_incompatible_qos (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::OfferedIncompatibleQosStatus & status
    )
{
  ACE_UNUSED_ARG(writer);
  ACE_UNUSED_ARG(status);

  ACE_ERROR ((LM_ERROR,
         ACE_TEXT("(%P|%t) ERROR: DataWriterListenerImpl::on_offered_incompatible_qos ")
         ACE_TEXT("This should appear when the test is designed to be incompatible.\n")));
}

void DataWriterListenerImpl::on_liveliness_lost (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::LivelinessLostStatus & status
    )
{
  ++num_liveliness_lost_callbacks_;
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_liveliness_lost %@ %d\n"),
              writer, (int) num_liveliness_lost_callbacks_.value()));
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t)    total_count=%d total_count_change=%d \n"),
              status.total_count, status.total_count_change));
}

void DataWriterListenerImpl::on_publication_matched (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::PublicationMatchedStatus & status
    )
{
  ACE_UNUSED_ARG(writer);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_publication_matched \n")));
}

void DataWriterListenerImpl::on_publication_disconnected (
      ::DDS::DataWriter_ptr writer,
      const ::OpenDDS::DCPS::PublicationDisconnectedStatus & status
    )
{
  ACE_UNUSED_ARG(writer);
  ACE_UNUSED_ARG(status);
}

void DataWriterListenerImpl::on_publication_reconnected (
      ::DDS::DataWriter_ptr writer,
      const ::OpenDDS::DCPS::PublicationReconnectedStatus & status
    )
{
  ACE_UNUSED_ARG(writer);
  ACE_UNUSED_ARG(status);
}

void DataWriterListenerImpl::on_publication_lost (
      ::DDS::DataWriter_ptr writer,
      const ::OpenDDS::DCPS::PublicationLostStatus & status
    )
{
  ACE_UNUSED_ARG(writer);
  ACE_UNUSED_ARG(status);
}
