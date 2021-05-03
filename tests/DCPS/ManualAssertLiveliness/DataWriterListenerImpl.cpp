#include "DataWriterListenerImpl.h"

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Service_Participant.h>

DataWriterListenerImpl::DataWriterListenerImpl()
  : num_liveliness_lost_callbacks_(0)
{}

DataWriterListenerImpl::~DataWriterListenerImpl()
{}

void DataWriterListenerImpl::on_liveliness_lost(::DDS::DataWriter_ptr writer,
                                                const ::DDS::LivelinessLostStatus& status)
{
  ++num_liveliness_lost_callbacks_;
  ACE_DEBUG((LM_INFO,
              ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_liveliness_lost %@ %d\n"),
              writer, (int) num_liveliness_lost_callbacks_.value()));
  ACE_DEBUG((LM_INFO,
              ACE_TEXT("(%P|%t)    total_count=%d total_count_change=%d\n"),
              status.total_count, status.total_count_change));
}

void DataWriterListenerImpl::on_offered_deadline_missed(::DDS::DataWriter_ptr,
                                                        const ::DDS::OfferedDeadlineMissedStatus&)
{}

void DataWriterListenerImpl::on_offered_incompatible_qos(::DDS::DataWriter_ptr,
                                                         const ::DDS::OfferedIncompatibleQosStatus&)
{}

void DataWriterListenerImpl::on_publication_matched(::DDS::DataWriter_ptr,
                                                    const ::DDS::PublicationMatchedStatus&)
{}

void DataWriterListenerImpl::on_publication_disconnected(::DDS::DataWriter_ptr,
                                                         const ::OpenDDS::DCPS::PublicationDisconnectedStatus&)
{}

void DataWriterListenerImpl::on_publication_reconnected(::DDS::DataWriter_ptr,
                                                        const ::OpenDDS::DCPS::PublicationReconnectedStatus&)
{}

void DataWriterListenerImpl::on_publication_lost(::DDS::DataWriter_ptr,
                                                 const ::OpenDDS::DCPS::PublicationLostStatus&)
{}
