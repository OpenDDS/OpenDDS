// -*- C++ -*-
//
// $Id$
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include "ace/Synch.h"
#include "dds/DdsDcpsSubscriptionExtS.h"
#include "TestStats.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


//Class DataReaderListenerImpl
class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:

  //Constructor
  DataReaderListenerImpl (int num_publishers,
                          int num_samples,
                          int data_size,
                          int read_interval,
                          bool use_zero_copy_reads);

  //Destructor
  virtual ~DataReaderListenerImpl (void);

  virtual void on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

 virtual void on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_subscription_matched (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_sample_rejected(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_data_available(
    ::DDS::DataReader_ptr reader
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_sample_lost(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));


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
  bool use_zero_copy_reads_;

  int num_publishers_;
  int num_samples_;
  int data_size_;
  int num_floats_per_sample_;

  TestStats stats_;

};

#endif /* DATAREADER_LISTENER_IMPL  */
