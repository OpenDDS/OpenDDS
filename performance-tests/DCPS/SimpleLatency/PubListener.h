// -*- C++ -*-
//
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DdsDcpsPublicationC.h>
#include "ace/High_Res_Timer.h"
#include "DDSPerfTestTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class AckDataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  AckDataReaderListenerImpl (CORBA::Long size);

  void init (DDS::DataReader_ptr dr, DDS::DataWriter_ptr dw, bool use_zero_copy_read);

  virtual ~AckDataReaderListenerImpl (void);

  virtual void on_requested_deadline_missed (
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus & status);

  virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status);

  virtual void on_liveliness_changed (
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status);

  virtual void on_subscription_matched (
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus & status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

  int done() const {
    return this->done_;
  }

private:

  DDS::DataWriter_var writer_;
  DDS::DataReader_var reader_;
  DDSPerfTest::AckMessageDataReader_var dr_servant_;
  DDSPerfTest::PubMessageDataWriter_var dw_servant_;
  DDS::InstanceHandle_t handle_;
  CORBA::Long sample_num_;
  int   done_;
  ACE_High_Res_Timer timer_;
  bool  use_zero_copy_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
