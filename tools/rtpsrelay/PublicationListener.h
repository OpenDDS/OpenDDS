#ifndef RTPSRELAY_PUBLICATION_LISTENER_H_
#define RTPSRELAY_PUBLICATION_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"
#include "DomainStatisticsReporter.h"

#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/PeriodicTask.h>

namespace RtpsRelay {

class PublicationListener : public ListenerBase {
public:
  PublicationListener(const Config& config,
                      OpenDDS::DCPS::DomainParticipantImpl* participant,
                      WriterEntryDataWriter_var writer,
                      DomainStatisticsReporter& stats_reporter);
private:
  void on_data_available(DDS::DataReader_ptr reader) override;
  void write_sample(const DDS::PublicationBuiltinTopicData& data,
                    const DDS::SampleInfo& info);
  void unregister_instance(const DDS::SampleInfo& info);
  void unregister();

  class Unregister : public OpenDDS::DCPS::RcObject {
  public:
    Unregister(PublicationListener& listener);
    ~Unregister();

  private:
    void execute(const OpenDDS::DCPS::MonotonicTimePoint& now);
    typedef OpenDDS::DCPS::PmfPeriodicTask<Unregister> PeriodicTask;
    PublicationListener& listener_;
    PeriodicTask unregister_task_;
  };

  const Config& config_;
  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  WriterEntryDataWriter_var writer_;
  DomainStatisticsReporter& stats_reporter_;
  OpenDDS::DCPS::RcHandle<Unregister> unregister_;
  ACE_Thread_Mutex mutex_;
  std::list<OpenDDS::DCPS::GUID_t> unregister_queue_;
};

}

#endif // RTPSRELAY_PUBLICATION_LISTENER_H_
