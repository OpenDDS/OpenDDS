#ifndef DCPS_THRASHER_PUBLISHER_H
#define DCPS_THRASHER_PUBLISHER_H

#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsInfrastructureC.h>

#include <ace/Task.h>

#include <cstdlib>

class Publisher : public ACE_Task_Base
{
public:
  static const long FLAGS = (THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
  Publisher(const DDS::DomainId_t domainId, std::size_t samples_per_thread, bool durable);
  ~Publisher();
  void start(const int n_threads, const long flags = FLAGS);
  int svc();
private:
  int get_thread_index(std::string& pfx);
  DDS::DomainParticipant_var create_participant(const std::string& pfx, const int thread_index, DDS::DomainParticipantFactory_var& dpf);
  typedef ACE_SYNCH_MUTEX Mutex;
  typedef ACE_Guard<Mutex> Lock;
  const DDS::DomainId_t domainId_;
  const std::size_t samples_per_thread_;
  const bool durable_;
  Mutex mutex_;
  int thread_index_;
};

#endif // DCPS_THRASHER_PUBLISHER_H
