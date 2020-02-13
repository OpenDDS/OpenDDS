#ifndef RTPSRELAY_PUBLICATION_LISTENER_H_
#define RTPSRELAY_PUBLICATION_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class PublicationListener : public ListenerBase {
public:
  PublicationListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                      WriterEntryDataWriter_ptr writer);
private:
  void on_data_available(DDS::DataReader_ptr reader) override;
  void write_sample(const DDS::PublicationBuiltinTopicData& data,
                    const DDS::SampleInfo& info);
  void unregister_instance(const DDS::SampleInfo& info);

  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  WriterEntryDataWriter_ptr writer_;
};

}

#endif // RTPSRELAY_PUBLICATION_LISTENER_H_
