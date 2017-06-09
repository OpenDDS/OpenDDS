/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionC.h>
#include "MessengerC.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

enum TESTCASE {
  strength,
  liveliness_change,
  miss_deadline,
  update_strength
};

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl(const char* reader_id);

  virtual ~DataReaderListenerImpl();

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

  long num_reads() const {
    return num_reads_;
  }

  bool verify_result ();

private:

  bool verify (const Messenger::Message& msg);

  DDS::DataReader_var  reader_;
  long                 num_reads_;
  const char*          reader_id_;

  bool  verify_result_;
  bool  result_verify_complete_;
  long  current_strength_[2]; // instance 0, 1

  // For deadline missed and liveliness changed test cases.
  ACE_Time_Value start_missing_;
  ACE_Time_Value end_missing_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
