#include "DataWriterListenerImpl.h"

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Service_Participant.h>

DataWriterListenerImpl::DataWriterListenerImpl(const OpenDDS::DCPS::String& name)
  : num_liveliness_lost_callbacks_(0)
  , name_(name)
{}

DataWriterListenerImpl::~DataWriterListenerImpl()
{}

void DataWriterListenerImpl::on_liveliness_lost(::DDS::DataWriter_ptr,
                                                const ::DDS::LivelinessLostStatus& status)
{
  ++num_liveliness_lost_callbacks_;
  ACE_DEBUG((LM_INFO,
             ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_liveliness_lost %C %d\n"),
             name_.c_str(), (int) num_liveliness_lost_callbacks_.load()));
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
