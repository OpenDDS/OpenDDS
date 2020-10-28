#ifndef RTPSRELAY_PARTICIPANT_LISTENER_H_
#define RTPSRELAY_PARTICIPANT_LISTENER_H_

#include "ListenerBase.h"
#include "DomainStatisticsReporter.h"

#include <dds/DCPS/DomainParticipantImpl.h>

namespace RtpsRelay {

class ParticipantListener : public ListenerBase {
public:
  ParticipantListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                      DomainStatisticsReporter& stats_reporter,
                      ParticipantEntryDataWriter_var participant_writer);
private:
  void on_data_available(DDS::DataReader_ptr reader) override;
  void write_sample(const DDS::ParticipantBuiltinTopicData& data,
                    const DDS::SampleInfo& info);
  void unregister_instance(const DDS::SampleInfo& info);

  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  DomainStatisticsReporter& stats_reporter_;
  ParticipantEntryDataWriter_var writer_;
};

}

#endif // RTPSRELAY_PARTICIPANT_LISTENER_H_
