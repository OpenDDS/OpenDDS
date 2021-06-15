#ifndef DCPS_THRASHER_PUBLISHER_H
#define DCPS_THRASHER_PUBLISHER_H

#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsInfrastructureC.h>

#include <ace/Task.h>

#include <cstdlib>

#define DEFAULT_FLAGS (THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED)

class Publisher : public ACE_Task_Base
{
public:
  Publisher(const DDS::DomainId_t domainId, std::size_t samples_per_thread, bool durable);
  ~Publisher();
  int svc();
private:
  int get_thread_index(std::string& pfx);
  void set_rtps_discovery(const std::string& pfx, const int thread_index, const DDS::DomainParticipant_var& dp);
  typedef ACE_SYNCH_MUTEX Mutex;
  typedef ACE_Guard<Mutex> Lock;
  const DDS::DomainId_t domainId_;
  const std::size_t samples_per_thread_;
  const bool durable_;
  Mutex mutex_;
  int thread_index_;
};

#endif // DCPS_THRASHER_PUBLISHER_H
