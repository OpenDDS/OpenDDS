
#include <ace/OS_main.h>
#include <ace/Log_Msg.h>
// TODO: Convert to GTEST.
#include "../../../DCPS/common/TestSupport.h"

#include "dds/DCPS/JobQueue.h"

#include "ace/Select_Reactor.h"
#include "ace/Reactor.h"

#include <string.h>

using namespace OpenDDS::DCPS;

namespace {

class TestExecutable : public Executable {
public:
  TestExecutable()
    : executed(false)
  {}

  virtual void execute()
  {
    executed = true;
  }

  bool executed;
};

void test_JobQueueSchedulable_initialize()
{
  ACE_Select_Reactor impl;
  ACE_Reactor reactor(&impl);
  RcHandle<JobQueue> job_queue = make_rch<JobQueue>(&reactor);

  RcHandle<JobQueueSchedulable> schedulable = make_rch<JobQueueSchedulable>();
  RcHandle<TestExecutable> executable = make_rch<TestExecutable>();

  schedulable->set_job_queue(job_queue);
  schedulable->set_executable(static_rchandle_cast<Executable>(executable));

  schedulable->schedule();

  ACE_Time_Value tv(1,0);
  reactor.run_reactor_event_loop(tv);

  TEST_ASSERT(executable->executed);
}

}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  try
  {
    test_JobQueueSchedulable_initialize();
  }
  catch (char const *ex)
  {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) Assertion failed.\n"), ex), -1);
  }
  return 0;
}
