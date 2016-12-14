// -*- C++ -*-
//
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include "ace/Synch.h"
#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/LocalObject.h"
#include "TestStats.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject< DDS::DataReaderListener>
{
public:
  DataReaderListenerImpl (int num_publishers,
                          int num_samples,
                          int data_size,
                          int read_interval,
                          bool use_zero_copy);

  virtual ~DataReaderListenerImpl (void);

  virtual void on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status);

 virtual void on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status);

  virtual void on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status);

  virtual void on_subscription_matched (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status);

  virtual void on_sample_rejected(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    ::DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

  bool is_finished ();
private:

  int read_samples (::DDS::DataReader_ptr reader);


  typedef ACE_Recursive_Thread_Mutex LockType;
  typedef ACE_Guard<LockType>        GuardType;

  // Used to protect the counts
  LockType lock_;

  int samples_lost_count_ ;
  int samples_rejected_count_ ;
  int samples_received_count_;
  int total_samples_count_;

  int read_interval_;

  int num_publishers_;
  int num_samples_;
  int data_size_;
  bool use_zero_copy_;

  TestStats stats_;

};

#endif /* DATAREADER_LISTENER_IMPL  */
