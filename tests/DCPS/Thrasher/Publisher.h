#ifndef DCPS_THRASHER_PUBLISHER_H
#define DCPS_THRASHER_PUBLISHER_H

#include <ace/Task.h>

#include <cstdlib>

#define DEFAULT_FLAGS (THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED)

class Publisher : public ACE_Task_Base
{
public:
  Publisher(std::size_t samples_per_thread, bool durable);
  ~Publisher();
  int svc();

private:
  typedef ACE_SYNCH_MUTEX LockType;
  typedef ACE_Guard<LockType> GuardType;

  LockType lock_;
  const std::size_t samples_per_thread_;
  const bool durable_;
  int thread_index_;
};

#endif // DCPS_THRASHER_PUBLISHER_H
