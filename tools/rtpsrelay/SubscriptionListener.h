#ifndef RTPSRELAY_SUBSCRIPTION_LISTENER_H_
#define RTPSRELAY_SUBSCRIPTION_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"
#include "DomainStatisticsReporter.h"

#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/PeriodicTask.h>

namespace RtpsRelay {

class SubscriptionListener : public ListenerBase {
public:
  SubscriptionListener(const Config& config,
                       OpenDDS::DCPS::DomainParticipantImpl* participant,
                       ReaderEntryDataWriter_var writer,
                       DomainStatisticsReporter& stats_reporter);
  void enable();
  void disable();

private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;
  void write_sample(const DDS::SubscriptionBuiltinTopicData& data,
                    const DDS::SampleInfo& info);
  void unregister_instance(const DDS::SampleInfo& info);
  void unregister();

  class Unregister : public OpenDDS::DCPS::RcObject {
  public:
    Unregister(SubscriptionListener& listener);
    void enable();
    void disable();

  private:
    void execute(const OpenDDS::DCPS::MonotonicTimePoint& now);
    typedef OpenDDS::DCPS::PmfPeriodicTask<Unregister> PeriodicTask;
    SubscriptionListener& listener_;
    PeriodicTask unregister_task_;
  };

  const Config& config_;
  OpenDDS::DCPS::DomainParticipantImpl* participant_;
  ReaderEntryDataWriter_var writer_;
  DomainStatisticsReporter& stats_reporter_;
  OpenDDS::DCPS::RcHandle<Unregister> unregister_;
  ACE_Thread_Mutex mutex_;
  std::list<OpenDDS::DCPS::GUID_t> unregister_queue_;
};

}

#endif // RTPSRELAY_SUBSCRIPTION_LISTENER_H_
