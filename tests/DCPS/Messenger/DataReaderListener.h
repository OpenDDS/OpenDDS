/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/GuardCondition.h>

#include <dds/DdsDcpsSubscriptionC.h>

#include <set>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl();

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

  long num_reads() const
  {
    return num_reads_;
  }

  static bool is_reliable();

  bool is_valid() const;

  void set_expected_reads(size_t expected);

  void set_guard_condition(DDS::GuardCondition_var gc);

private:
  typedef std::set<CORBA::ULong> Counts;

  DDS::DataReader_var reader_;
  size_t expected_reads_;
  long num_reads_;
  Counts counts_;
  bool valid_;
  const bool reliable_;
  DDS::GuardCondition_var gc_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
