// -*- C++ -*-
//
#include "DataReaderListenerImpl.h"
#include "../common/SampleInfo.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/Service_Participant.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

DataReaderListenerImpl::DataReaderListenerImpl()
  : deadline_missed_(0)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr reader, const DDS::LivelinessChangedStatus& status)
{
  ACE_UNUSED_ARG(reader);
  // if we have gone from active to inactive, then we missed a deadline
  if((status.alive_count_change < 0) && (status.not_alive_count_change > 0)) {
    ++deadline_missed_;
  }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_liveliness_changed: ")
    ACE_TEXT("active=%d, inactive=%d, activeDelta=%d, inactiveDelta=%d deadline_missed=%d\n"),
    status.alive_count, status.not_alive_count, status.alive_count_change,
    status.not_alive_count_change, deadline_missed_));
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  Xyz::FooDataReader_var foo_dr = Xyz::FooDataReader::_narrow(reader);
  if (!foo_dr){
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Xyz::FooDataReader::_narrow failed.\n")));
  }
  const int num_ops_per_thread = 100;
  Xyz::FooSeq foo(num_ops_per_thread);
  DDS::SampleInfoSeq si(num_ops_per_thread);
  DDS::ReturnCode_t status = foo_dr->read(foo, si, num_ops_per_thread,
    DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (status != DDS::RETCODE_OK) {
    if (status == DDS::RETCODE_NO_DATA) {
      ACE_OS::fprintf(stderr, "read returned DDS::RETCODE_NO_DATA\n");
    } else {
      ACE_OS::fprintf(stderr, "read - Error: %d\n", status);
    }
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
}

void DataReaderListenerImpl::on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_matched \n")));
}

void DataReaderListenerImpl::on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected \n")));
}
