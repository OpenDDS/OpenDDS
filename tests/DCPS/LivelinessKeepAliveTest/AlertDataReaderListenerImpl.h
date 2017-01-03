// -*- C++ -*-
//
#ifndef ALERT_DATAREADER_LISTENER_IMPL
#define ALERT_DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>
#include "SatelliteC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class AlertDataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:

  ::DDS::LivelinessChangedStatus expected_status_;
  ::DDS::LivelinessChangedStatus last_status_;

  AlertDataReaderListenerImpl ();
  virtual ~AlertDataReaderListenerImpl ();

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

  int liveliness_changed_count() const { return liveliness_changed_count_; }
  bool error_occurred() const { return error_occurred_; }

  bool verify_last_liveliness_status()
  {
    return last_status_.alive_count == 0 && last_status_.not_alive_count == 0;
  }

private:
  int liveliness_changed_count_;
  bool error_occurred_;

};

#endif /* ALERT_DATAREADER_LISTENER_IMPL  */
