// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"

using namespace Messenger;
using namespace std;

DataReaderListenerImpl::DataReaderListenerImpl(DistributedConditionSet_rch dcs,
                                               const OpenDDS::DCPS::String& actor,
                                               long expected)
  : dcs_(dcs)
  , actor_(actor)
  , expected_(expected)
  , ok_(true)
  , read_instances_message_count_()
{
  ACE_DEBUG((LM_INFO, "(%P|%t) DataReaderListnerImpl[%@]\n", this));
}

DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  try {
    MessageDataReader_var message_dr = MessageDataReader::_narrow(reader);
    if (CORBA::is_nil(message_dr)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) read: _narrow failed.\n")));
      exit(1);
    }

    Messenger::Message message;
    DDS::SampleInfo si;
    for (DDS::ReturnCode_t status = message_dr->take_next_sample(message, si);
         status == DDS::RETCODE_OK;
         status = message_dr->take_next_sample(message, si)) {

      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

      if (si.valid_data) {
        if (read_instances_message_count_.count(message.subject_id)) {
          read_instances_message_count_[message.subject_id] += 1;
        }
        else {
          read_instances_message_count_.insert(std::make_pair(message.subject_id, 1));
        }

        ACE_DEBUG((LM_INFO, "(%P|%t) DataReaderListenerImpl[%@]::on_data_available - instance: %d count = %d\n", this,
                   message.subject_id, message.count));
        if (message.count != message.subject_id) {
          ACE_DEBUG((LM_INFO, "(%P|%t) ERROR: durable reader received wrong data (instance: %d msg count = %d and num_reads = %d)\n",
                     message.subject_id, message.count, num_reads()));
          ok_ = false;
        }

        if (num_reads() == expected_ && received_all_expected_messages()) {
          dcs_->post(actor_, "read done");
        }
      }
    }
  } catch (CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Exception caught in read:\n%C\n"), e._info().c_str()));
    exit(1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
}

void DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_liveliness_changed\n")));
}

void DataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  ACE_DEBUG((LM_INFO, "(%P|%t) DataReaderListnerImpl::on_subscription_matched\n"));
}

void DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected\n")));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost\n")));
}

long DataReaderListenerImpl::num_reads() const
{
  long num_reads = 0;
  std::map<int, int>::const_iterator map_iter = read_instances_message_count_.begin();
  for (; map_iter != read_instances_message_count_.end(); ++map_iter) {
    num_reads += map_iter->second;
  }
  return num_reads;
}

bool DataReaderListenerImpl::received_all_expected_messages() const
{
  for (int i = 0; i < 4; ++i) {
    int message_instance = i + 1;
    if (!read_instances_message_count_.count(message_instance))
    {
      return false;
    }
    else {
      std::map<int, int>::const_iterator search = read_instances_message_count_.find(message_instance);
      if (search == read_instances_message_count_.end() || 1 != search->second) {
        return false;
      }
    }
  }
  return true;
}
