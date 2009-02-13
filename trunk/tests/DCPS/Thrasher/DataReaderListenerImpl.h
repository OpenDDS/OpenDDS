/*
 * $Id$
 */

#ifndef DCPS_THRASHER_DATAREADERLISTENERIMPL_H
#define DCPS_THRASHER_DATAREADERLISTENERIMPL_H

#include <dds/DdsDcpsSubscriptionS.h>

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  DataReaderListenerImpl(size_t& received_samples);

  virtual ~DataReaderListenerImpl();

  virtual void on_data_available(
      DDS::DataReader_ptr reader)
    throw (CORBA::SystemException);

  virtual void on_requested_deadline_missed(
      DDS::DataReader_ptr reader,
      const DDS::RequestedDeadlineMissedStatus& status)
    throw (CORBA::SystemException);

  virtual void on_requested_incompatible_qos(
      DDS::DataReader_ptr reader,
      const DDS::RequestedIncompatibleQosStatus& status)
    throw (CORBA::SystemException);

  virtual void on_liveliness_changed(
      DDS::DataReader_ptr reader,
      const DDS::LivelinessChangedStatus& status)
    throw (CORBA::SystemException);

  virtual void on_subscription_match(
      DDS::DataReader_ptr reader,
      const DDS::SubscriptionMatchStatus& status)
    throw (CORBA::SystemException);

  virtual void on_sample_rejected(
      DDS::DataReader_ptr reader,
      const DDS::SampleRejectedStatus& status)
    throw (CORBA::SystemException);

  virtual void on_sample_lost(
      DDS::DataReader_ptr reader,
      const DDS::SampleLostStatus& status)
    throw (CORBA::SystemException);

private:
  size_t& received_samples_;
};

#endif /* DCPS_THRASHER_DATAREADERLISTENERIMPL_H */
