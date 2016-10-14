/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include <ace/Global_Macros.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DdsDcpsPublicationC.h>
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/Definitions.h>

#include <string>

typedef void (*callback_t)();

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl(const std::string& id, int expected_samples, callback_t done_callback, bool origin, DDS::DataWriter_var writer, std::string writer_id, int total_readers)
    : id_(id)
    , expected_samples_(expected_samples)
    , received_samples_(0)
    , done_callback_(done_callback)
    , origin_(origin)
    , writer_(writer)
    , writer_id_(writer_id)
    , total_readers_(total_readers)
  {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Starting DataReader %C using writer with id: %C\n", id.c_str(), writer_id.c_str()));
  }

  ~DataReaderListenerImpl();

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

private:
  std::string id_;
  const int expected_samples_;
  int received_samples_;
  callback_t done_callback_;
  bool origin_;
  DDS::DataWriter_var writer_;
  std::string writer_id_;
  int total_readers_;
};

#endif /* DATAREADER_LISTENER_IMPL_H */
