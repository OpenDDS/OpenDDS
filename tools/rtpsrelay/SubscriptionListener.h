#ifndef RTPSRELAY_SUBSCRIPTION_LISTENER_H_
#define RTPSRELAY_SUBSCRIPTION_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"
#include "DomainStatisticsWriter.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class SubscriptionListener : public ListenerBase {
public:
  SubscriptionListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                       ReaderEntryDataWriter_ptr writer,
                       DomainStatisticsWriter& stats_writer);
private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;
  void write_sample(const DDS::SubscriptionBuiltinTopicData& data,
                    const DDS::SampleInfo& info);
  void unregister_instance(const DDS::SampleInfo& info);

  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  ReaderEntryDataWriter_ptr writer_;
  DomainStatisticsWriter& stats_writer_;
};

}

#endif // RTPSRELAY_SUBSCRIPTION_LISTENER_H_
