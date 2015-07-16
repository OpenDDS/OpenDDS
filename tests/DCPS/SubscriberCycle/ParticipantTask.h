/*
 */

#ifndef SUBSCRIBER_CYCLE_PARTICIPANTTASK_H
#define SUBSCRIBER_CYCLE_PARTICIPANTTASK_H

#include <cstdlib>

#include <ace/Task.h>

#define DEFAULT_FLAGS (THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED)

class ParticipantTask : public ACE_Task_Base
{
public:
  ParticipantTask(const std::size_t & samples_per_thread,
                  int delay_between_pubs_msec,
                  const DDS::Duration_t & deadline);

  ~ParticipantTask();

  int svc();

private:
  const std::size_t& samples_per_thread_;
  int delay_between_pubs_msec_;
  const DDS::Duration_t & deadline_;
};

#endif
