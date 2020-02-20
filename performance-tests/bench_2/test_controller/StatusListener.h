#ifndef STATUS_LISTENER_HEADER
#define STATUS_LISTENER_HEADER

#include <mutex>
#include <iostream>
#include <map>

#include <dds/DdsDcpsInfrastructureC.h>

#include "BenchTypeSupportImpl.h"

typedef std::map<Bench::NodeController::NodeId, Bench::NodeController::Status, OpenDDS::DCPS::GUID_tKeyLessThan> Nodes;
std::ostream& operator<<(std::ostream& os, const Bench::NodeController::Status& node);

class StatusListener : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  virtual ~StatusListener()
  {
  }

  void on_requested_deadline_missed(
    DDS::DataReader_ptr /* reader */,
    const DDS::RequestedDeadlineMissedStatus& /* status */)
  {
  }

  void on_requested_incompatible_qos(
    DDS::DataReader_ptr /* reader */,
    const DDS::RequestedIncompatibleQosStatus& /* status */)
  {
  }

  void on_liveliness_changed(
    DDS::DataReader_ptr /* reader */,
    const DDS::LivelinessChangedStatus& /* status */)
  {
  }

  void on_subscription_matched(
    DDS::DataReader_ptr /* reader */,
    const DDS::SubscriptionMatchedStatus& /* status */)
  {
  }

  void on_sample_rejected(
    DDS::DataReader_ptr /* reader */,
    const DDS::SampleRejectedStatus& /* status */)
  {
  }

  void on_data_available(DDS::DataReader_ptr reader);

  void on_sample_lost(
    DDS::DataReader_ptr /* reader */,
    const DDS::SampleLostStatus& /* status */)
  {
  }

  Nodes get_available_nodes();

private:
  Nodes nodes_;
  std::mutex nodes_mutex_;
  bool in_discovery_period_ = true;
};

#endif
