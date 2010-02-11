/*
 * $Id$
 */

#ifndef DCPS_THRASHER_PARTICIPANTTASK_H
#define DCPS_THRASHER_PARTICIPANTTASK_H

#include <cstdlib>

#include <ace/Task.h>

#define DEFAULT_FLAGS (THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED)

class ParticipantTask : public ACE_Task_Base
{
public:
  explicit ParticipantTask(const std::size_t& samples_per_thread);

  ~ParticipantTask();

  int svc();

private:
  const std::size_t& samples_per_thread_;
};

#endif /* DCPS_THRASHER_PARTICIPANTTASK_H */
