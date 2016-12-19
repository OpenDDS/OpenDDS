/*
 */

#ifndef DCPS_THRASHER_DATAREADERLISTENERIMPL_H
#define DCPS_THRASHER_DATAREADERLISTENERIMPL_H

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>

#include "ProgressIndicator.h"

#include <cstdlib>

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  DataReaderListenerImpl(std::size_t& received_samples,
                         const ProgressIndicator& progress);

  virtual ~DataReaderListenerImpl();

  virtual void on_data_available(
      DDS::DataReader_ptr reader);

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

  virtual void on_sample_lost(
      DDS::DataReader_ptr reader,
      const DDS::SampleLostStatus& status);

private:

  std::size_t& received_samples_;

  ProgressIndicator progress_;
};

#endif /* DCPS_THRASHER_DATAREADERLISTENERIMPL_H */
