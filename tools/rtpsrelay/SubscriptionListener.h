#ifndef RTPSRELAY_SUBSCRIPTION_LISTENER_H_
#define RTPSRELAY_SUBSCRIPTION_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"

#include "dds/DCPS/DomainParticipantImpl.h"

namespace RtpsRelay {

class SubscriptionListener : public ListenerBase {
public:
  SubscriptionListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                       ReaderEntryDataWriter_ptr writer,
                       const AssociationTable& association_table);
private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;
  void write_sample(const DDS::SubscriptionBuiltinTopicData& data,
                    const DDS::SampleInfo& info);
  void write_dispose(const DDS::SampleInfo& info);

  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  ReaderEntryDataWriter_ptr writer_;
  const AssociationTable& association_table_;
};

}

#endif // RTPSRELAY_SUBSCRIPTION_LISTENER_H_
