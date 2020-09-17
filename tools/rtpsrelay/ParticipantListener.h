#ifndef RTPSRELAY_PARTICIPANT_LISTENER_H_
#define RTPSRELAY_PARTICIPANT_LISTENER_H_

#include "ListenerBase.h"
#include "DomainStatisticsWriter.h"

namespace RtpsRelay {

class ParticipantListener : public ListenerBase {
public:
  explicit ParticipantListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                               DomainStatisticsWriter& stats_writer,
                               ParticipantEntryDataWriter_ptr participant_writer);
private:
  void on_data_available(DDS::DataReader_ptr reader) override;
  void write_sample(const DDS::ParticipantBuiltinTopicData& data, const DDS::SampleInfo& info);

  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  DomainStatisticsWriter& stats_writer_;
  ParticipantEntryDataWriter_ptr participant_writer_;
};

}

#endif // RTPSRELAY_PARTICIPANT_LISTENER_H_
