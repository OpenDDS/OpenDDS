// -*- C++ -*-
//
#include "DataReaderListener.h"

DataReaderListenerImpl::DataReaderListenerImpl(int num_ops_per_thread)
  : num_samples_(0), num_ops_per_thread_(num_ops_per_thread)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::")
             ACE_TEXT("DataReaderListenerImpl\n")));
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::")
             ACE_TEXT("~DataReaderListenerImpl\n")));
}

void DataReaderListenerImpl::on_requested_deadline_missed(
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus& status)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus& status)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus& status)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_liveliness_changed\n")));
}

void DataReaderListenerImpl::on_subscription_matched(
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus& status)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_matched\n")));
}

void DataReaderListenerImpl::on_sample_rejected(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected\n")));
}

void DataReaderListenerImpl::on_data_available(::DDS::DataReader_ptr reader)
{
  read(reader);
}

void DataReaderListenerImpl::on_sample_lost(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost\n")));
}

void DataReaderListenerImpl::on_subscription_disconnected(
      ::DDS::DataReader_ptr reader,
      const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus& status)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_disconnected\n")));
  }

void DataReaderListenerImpl::on_subscription_reconnected(
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionReconnectedStatus& status)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_reconnected\n")));
}

void DataReaderListenerImpl::on_subscription_lost(
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionLostStatus& status)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_lost\n")));
}

void DataReaderListenerImpl::on_budget_exceeded(
    ::DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::BudgetExceededStatus&)
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) received on_budget_exceeded\n"));
}

void DataReaderListenerImpl::on_connection_deleted(::DDS::DataReader_ptr)
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) received on_connection_deleted\n"));
}
