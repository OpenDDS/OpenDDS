// -*- C++ -*-
//
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DCPS/Definitions.h"
#include "../common/SampleInfo.h"
#include "dds/DCPS/TypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

template<typename MessageType>
class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
{
public:
  typedef MessageType DSample;
  typedef void (*DSPrinter)(const DSample&, int);

  DataReaderListenerImpl(int num_ops_per_thread, int& num_samples, DSPrinter printer)
  : num_samples_(num_samples), num_ops_per_thread_(num_ops_per_thread)
  , print_sample_(printer)
  {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::")
               ACE_TEXT("DataReaderListenerImpl\n")));
  }

  virtual ~DataReaderListenerImpl()
  {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderListenerImpl::")
               ACE_TEXT("~DataReaderListenerImpl\n")));
  }

  virtual void on_requested_deadline_missed(::DDS::DataReader_ptr,
    const ::DDS::RequestedDeadlineMissedStatus&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
  }

  virtual void on_requested_incompatible_qos(::DDS::DataReader_ptr,
    const ::DDS::RequestedIncompatibleQosStatus&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
  }

  virtual void on_liveliness_changed(::DDS::DataReader_ptr,
    const ::DDS::LivelinessChangedStatus&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_liveliness_changed\n")));
  }

  virtual void on_subscription_matched(::DDS::DataReader_ptr,
    const ::DDS::SubscriptionMatchedStatus&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_matched \n")));
  }

  virtual void on_sample_rejected(::DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected \n")));
  }

  virtual void on_data_available(::DDS::DataReader_ptr r) { read(r); }

  virtual void on_sample_lost(::DDS::DataReader_ptr,
    const DDS::SampleLostStatus&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_disconnected \n")));
  }

  virtual void on_subscription_disconnected(::DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_disconnected \n")));
  }

  virtual void on_subscription_reconnected(::DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionReconnectedStatus&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_reconnected \n")));
  }

  virtual void on_subscription_lost(::DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionLostStatus&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_lost \n")));
  }

  virtual void on_budget_exceeded(::DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::BudgetExceededStatus&)
  {
    ACE_DEBUG ((LM_DEBUG, "(%P|%t) received on_budget_exceeded \n"));
  }

  virtual void on_connection_deleted(::DDS::DataReader_ptr)
  {
    ACE_DEBUG ((LM_DEBUG, "(%P|%t) received on_connection_deleted  \n"));
  }

  void read(::DDS::DataReader_ptr reader);

  int num_samples() const { return num_samples_ ; }

private:
  int& num_samples_;
  const int num_ops_per_thread_;
  DSPrinter print_sample_;
};

template<typename MessageType>
void DataReaderListenerImpl<MessageType>::read(DDS::DataReader_ptr dr)
{
  typedef OpenDDS::DCPS::DDSTraits<MessageType> TraitsType;
  const typename TraitsType::DataReaderType::_var_type foo_dr =
    TraitsType::DataReaderType::_narrow(dr);

  typename TraitsType::MessageSequenceType foo(num_ops_per_thread_);
  DDS::SampleInfoSeq si(num_ops_per_thread_);

  const DDS::ReturnCode_t status =
    foo_dr->read(foo, si, num_ops_per_thread_,
                 ::DDS::NOT_READ_SAMPLE_STATE,
                 ::DDS::ANY_VIEW_STATE,
                 ::DDS::ANY_INSTANCE_STATE);

  if (status == ::DDS::RETCODE_OK)
  {
    for (CORBA::ULong i = 0 ; i < si.length() ; i++)
    {
      if (si[i].valid_data)
      {
        ++num_samples_;
        print_sample_(foo[i], int(i));
      }
      PrintSampleInfo(si[i]);
    }
  }
  else if (status == ::DDS::RETCODE_NO_DATA)
  {
    ACE_OS::printf("read returned ::DDS::RETCODE_NO_DATA\n") ;
  }
  else
  {
    ACE_OS::printf("read - Error: %d\n", status) ;
  }
}

#endif /* DATAREADER_LISTENER_IMPL  */
