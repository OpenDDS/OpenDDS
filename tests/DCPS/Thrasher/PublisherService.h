#ifndef DCPS_THRASHER_PUBLISHERSERVICE_H
#define DCPS_THRASHER_PUBLISHERSERVICE_H

#include "Publisher.h"

#include <ace/Task.h>

class PublisherService : public ACE_Task_Base
{
public:
  PublisherService(const long domain_id, std::size_t samples_per_thread, bool durable);
  ~PublisherService();
  bool start(const int n_threads, const long flags=THR_NEW_LWP|THR_JOINABLE|THR_INHERIT_SCHED);
  bool end();
  int svc();
private:
  Publisher* createPublisher();
  typedef ACE_SYNCH_MUTEX Mutex;
  typedef ACE_Guard<Mutex> Lock;
  const long domain_id_;
  const std::size_t samples_per_thread_;
  const bool durable_;
  Mutex mutex_;
  int thread_index_;
};

#endif // DCPS_THRASHER_PUBLISHERSERVICE_H
