#ifndef RTPSRELAY_SUBSCRIPTION_LISTENER_H_
#define RTPSRELAY_SUBSCRIPTION_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"
#include "DomainStatisticsReporter.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class SubscriptionListener : public ListenerBase {
public:
  SubscriptionListener(const Config& config,
                       OpenDDS::DCPS::DomainParticipantImpl* participant,
                       ReaderEntryDataWriter_var writer,
                       DomainStatisticsReporter& stats_reporter);
private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;
  void write_sample(const DDS::SubscriptionBuiltinTopicData& data,
                    const DDS::SampleInfo& info);
  void unregister_instance(const DDS::SampleInfo& info);

  const Config& config_;
  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  ReaderEntryDataWriter_var writer_;
  DomainStatisticsReporter& stats_reporter_;
};

}

#endif // RTPSRELAY_SUBSCRIPTION_LISTENER_H_
