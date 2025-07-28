#ifndef RTPSRELAY_RELAY_CONTROL_HANDLER_H
#define RTPSRELAY_RELAY_CONTROL_HANDLER_H

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/DataReaderImpl.h>
#include <dds/DCPS/Definitions.h>
#include <dds/rtpsrelaylib/RelayC.h>

namespace RtpsRelay {

class RelayConfigControlListener : public OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  // Override all required virtual methods:
  void on_data_available(DDS::DataReader_ptr reader) override;

  void on_requested_deadline_missed(DDS::DataReader_ptr,
                                    const DDS::RequestedDeadlineMissedStatus&) override {}

  void on_requested_incompatible_qos(DDS::DataReader_ptr,
                                     const DDS::RequestedIncompatibleQosStatus&) override {}

  void on_sample_rejected(DDS::DataReader_ptr,
                          const DDS::SampleRejectedStatus&) override {}

  void on_liveliness_changed(DDS::DataReader_ptr,
                             const DDS::LivelinessChangedStatus&) override {}

  void on_subscription_matched(DDS::DataReader_ptr,
                               const DDS::SubscriptionMatchedStatus&) override {}

  void on_sample_lost(DDS::DataReader_ptr,
                      const DDS::SampleLostStatus&) override {}
};

}

#endif
