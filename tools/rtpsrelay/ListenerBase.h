#ifndef RTPSRELAY_LISTENER_BASE_H_
#define RTPSRELAY_LISTENER_BASE_H_

#include <dds/DdsDcpsSubscriptionC.h>

namespace RtpsRelay {

class ListenerBase : public DDS::DataReaderListener {
public:

private:
  void on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/,
                                    const DDS::RequestedDeadlineMissedStatus & /*status*/) override {}
  void on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/,
                                     const DDS::RequestedIncompatibleQosStatus & /*status*/) override {}
  void on_sample_rejected(DDS::DataReader_ptr /*reader*/,
                          const DDS::SampleRejectedStatus & /*status*/) override {}
  void on_liveliness_changed(DDS::DataReader_ptr /*reader*/,
                             const DDS::LivelinessChangedStatus & /*status*/) override {}
  void on_data_available(DDS::DataReader_ptr /*reader*/) override {}
  void on_subscription_matched(DDS::DataReader_ptr /*reader*/,
                               const DDS::SubscriptionMatchedStatus & /*status*/) override {}
  void on_sample_lost(DDS::DataReader_ptr /*reader*/,
                      const DDS::SampleLostStatus & /*status*/) override {}
};

}

#endif // RTPSRELAY_LISTENER_BASE_H_
