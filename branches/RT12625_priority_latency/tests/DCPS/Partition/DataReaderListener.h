// -*- C++ -*-
//
// $Id$

#ifndef DATA_READER_LISTENER_IMPL
#define DATA_READER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionS.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace Test
{
  class DataReaderListener
    : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
  {
  public:

    //Constructor
    DataReaderListener (long expected_matches);

    virtual void on_requested_deadline_missed (
        DDS::DataReader_ptr reader,
        const DDS::RequestedDeadlineMissedStatus & status)
      throw (CORBA::SystemException);

    virtual void on_requested_incompatible_qos (
        DDS::DataReader_ptr reader,
        const DDS::RequestedIncompatibleQosStatus & status)
      throw (CORBA::SystemException);

    virtual void on_liveliness_changed (
        DDS::DataReader_ptr reader,
        const DDS::LivelinessChangedStatus & status)
      throw (CORBA::SystemException);

    virtual void on_subscription_match (
        DDS::DataReader_ptr reader,
        const DDS::SubscriptionMatchStatus & status)
      throw (CORBA::SystemException);

    virtual void on_sample_rejected(
        DDS::DataReader_ptr reader,
        const DDS::SampleRejectedStatus& status)
      throw (CORBA::SystemException);

    virtual void on_data_available(DDS::DataReader_ptr reader)
      throw (CORBA::SystemException);

    virtual void on_sample_lost(DDS::DataReader_ptr reader,
                                const DDS::SampleLostStatus& status)
      throw (CORBA::SystemException);

    virtual void on_subscription_disconnected (
        DDS::DataReader_ptr reader,
        const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus & status)
      throw (CORBA::SystemException);

    virtual void on_subscription_reconnected (
        DDS::DataReader_ptr reader,
        const ::OpenDDS::DCPS::SubscriptionReconnectedStatus & status)
      throw (CORBA::SystemException);

    virtual void on_subscription_lost (
        DDS::DataReader_ptr reader,
        const ::OpenDDS::DCPS::SubscriptionLostStatus & status)
      throw (CORBA::SystemException);

    virtual void on_connection_deleted (DDS::DataReader_ptr)
      throw (CORBA::SystemException);

  protected:

    // Destructor
    virtual ~DataReaderListener (void);

  private:

    void display_partitions (DDS::DataReader_ptr reader) const;

  private:

    /// The number of expected subscription matches.
    long const expected_matches_;

    /// The actual number of subscription matches.
    ACE_Atomic_Op<ACE_Thread_Mutex, long> subscription_matches_;

  };
}

#endif /* DATA_READER_LISTENER_IMPL  */
