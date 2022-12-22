// -*- C++ -*-
//
#include "DataReaderListenerImpl.h"

#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/ExceptionStreams.h"

#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/Service_Participant.h>

#include <ace/streams.h>

using namespace Messenger;
using namespace std;

extern int sub_num_liveliness_change_callbacks;

DataReaderListenerImpl::DataReaderListenerImpl(DistributedConditionSet_rch dcs)
  : dcs_(dcs)
  , num_reads_(0)
  , num_liveliness_change_callbacks_(0)
{}

DataReaderListenerImpl::~DataReaderListenerImpl()
{}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  ::Messenger::MessageDataReader_var message_dr = ::Messenger::MessageDataReader::_narrow(reader);

  Messenger::Message message;
  DDS::SampleInfo si;
  DDS::ReturnCode_t status = message_dr->take_next_sample(message, si);

  if (status == DDS::RETCODE_OK) {
    if (si.valid_data == 1) {
      ++num_reads_;
      ACE_DEBUG((LM_INFO,
                 "(%P|%t) "
                 "Message: subject    = %C\n"
                 "         subject_id = %d\n"
                 "         from       = %C\n"
                 "         count      = %d\n"
                 "         text       = %C\n",
                 message.subject.in(),
                 message.subject_id,
                 message.from.in(),
                 message.count,
                 message.text.in()));
    } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
      ACE_DEBUG((LM_INFO, "(%P|%t) instance is disposed\n"));
    } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
      ACE_DEBUG((LM_INFO, "(%P|%t) instance is unregistered\n"));
    } else {
      ACE_ERROR ((LM_ERROR, "(%P|%t) DataReaderListenerImpl::on_data_available:"
                  " received unknown instance state %d\n", si.instance_state));
    }
  } else if (status == DDS::RETCODE_NO_DATA) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: reader received DDS::RETCODE_NO_DATA!\n"));
  } else {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: read Message: Error: status=%d\n", status));
  }
}

void DataReaderListenerImpl::on_liveliness_changed(DDS::DataReader_ptr reader,
                                                   const DDS::LivelinessChangedStatus& status)
{
  ++num_liveliness_change_callbacks_;
  if (num_liveliness_change_callbacks_ == sub_num_liveliness_change_callbacks) {
    dcs_->post(SUBSCRIBER_ACTOR, CALLBACKS_DONE_CONDITION);
  }

  DDS::Subscriber_var subscriber = reader->get_subscriber();
  DDS::DomainParticipant_var participant = subscriber->get_participant();
  OpenDDS::DCPS::DomainParticipantImpl* dpi =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in());

  OpenDDS::DCPS::LogGuid lg(dpi->get_repoid(status.last_publication_handle));
  ACE_DEBUG((LM_INFO,
             "(%P|%t) "
             "DataReaderListenerImpl::on_liveliness_changed #%d\n"
             "  alive_count = %d\n"
             "  not_alive_count = %d\n"
             "  alive_count_change = %d\n"
             "  not_alive_count_change = %d\n"
             "  last_publication_handle = %d (%C)\n",
             num_liveliness_change_callbacks_,
             status.alive_count,
             status.not_alive_count,
             status.alive_count_change,
             status.not_alive_count_change,
             status.last_publication_handle, lg.c_str()));
}

void DataReaderListenerImpl::on_requested_deadline_missed(DDS::DataReader_ptr,
                                                          const DDS::RequestedDeadlineMissedStatus&)
{}

void DataReaderListenerImpl::on_requested_incompatible_qos(DDS::DataReader_ptr,
                                                           const DDS::RequestedIncompatibleQosStatus&)
{}

void DataReaderListenerImpl::on_subscription_matched(DDS::DataReader_ptr,
                                                     const DDS::SubscriptionMatchedStatus&)
{}

void DataReaderListenerImpl::on_sample_rejected(DDS::DataReader_ptr,
                                                const DDS::SampleRejectedStatus&)
{}

void DataReaderListenerImpl::on_sample_lost(DDS::DataReader_ptr,
                                            const DDS::SampleLostStatus&)
{}
