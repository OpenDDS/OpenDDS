#ifndef RTPSRELAY_SUBSCRIPTION_LISTENER_H_
#define RTPSRELAY_SUBSCRIPTION_LISTENER_H_

#include "AssociationTable.h"

#include "dds/DCPS/DomainParticipantImpl.h"

class SubscriptionListener : public DDS::DataReaderListener {
public:
  SubscriptionListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                       RtpsRelay::ReaderEntryDataWriter_ptr writer,
                       const AssociationTable& association_table);
private:
  void on_requested_deadline_missed(::DDS::DataReader_ptr /*reader*/,
                                    const ::DDS::RequestedDeadlineMissedStatus & /*status*/) override;
  void on_requested_incompatible_qos(::DDS::DataReader_ptr /*reader*/,
                                     const ::DDS::RequestedIncompatibleQosStatus & /*status*/) override;
  void on_sample_rejected(::DDS::DataReader_ptr /*reader*/,
                          const ::DDS::SampleRejectedStatus & /*status*/) override;
  void on_liveliness_changed(::DDS::DataReader_ptr /*reader*/,
                             const ::DDS::LivelinessChangedStatus & /*status*/) override;
  void on_data_available(::DDS::DataReader_ptr /*reader*/) override;
  void on_subscription_matched(::DDS::DataReader_ptr /*reader*/,
                               const ::DDS::SubscriptionMatchedStatus & /*status*/) override;
  void on_sample_lost(::DDS::DataReader_ptr /*reader*/,
                      const ::DDS::SampleLostStatus & /*status*/) override;
  void write_sample(const DDS::SubscriptionBuiltinTopicData& data,
                    const DDS::SampleInfo& info);
  void write_dispose(const DDS::SampleInfo& info);

  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  RtpsRelay::ReaderEntryDataWriter_ptr writer_;
  const AssociationTable& association_table_;
};

#endif // RTPSRELAY_SUBSCRIPTION_LISTENER_H_
