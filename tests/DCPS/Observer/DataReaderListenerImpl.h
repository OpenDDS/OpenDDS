#pragma once
#include "MessengerC.h"
#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>
#include <set>

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl(const std::string& reader);
  virtual ~DataReaderListenerImpl();
  virtual void on_data_available(DDS::DataReader_ptr dr);
  virtual void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&);
  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&);
  virtual void on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&);
  virtual void on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&);
  virtual void on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&);
  virtual void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&);
private:
  void read(Messenger::MessageDataReader_var mdr);
  void take(Messenger::MessageDataReader_var mdr);
  void read_instance(Messenger::MessageDataReader_var mdr, DDS::InstanceHandle_t i);
  void take_instance(Messenger::MessageDataReader_var mdr, DDS::InstanceHandle_t i);
  void read_next_sample(Messenger::MessageDataReader_var mdr);
  void take_next_sample(Messenger::MessageDataReader_var mdr);
  const std::string reader_;
  long received_;
};
