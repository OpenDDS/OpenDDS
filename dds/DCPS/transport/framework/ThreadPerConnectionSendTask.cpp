/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ThreadPerConnectionSendTask.h"
#include "TransportQueueElement.h"
#include "TransportSendElement.h"
#include "DataLink.h"
#include "ThreadPerConRemoveVisitor.h"
#include "DirectPriorityMapper.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/Service_Participant.h"

#include "ace/Auto_Ptr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ThreadPerConnectionSendTask::ThreadPerConnectionSendTask(DataLink* link)
  : lock_()
  , work_available_(lock_)
  , shutdown_initiated_(false)
  , opened_(false)
  , thr_id_(ACE_OS::NULL_thread)
  , link_(link)
{
  DBG_ENTRY_LVL("ThreadPerConnectionSendTask", "ThreadPerConnectionSendTask", 6);
}

ThreadPerConnectionSendTask::~ThreadPerConnectionSendTask()
{
  DBG_ENTRY_LVL("ThreadPerConnectionSendTask", "~ThreadPerConnectionSendTask", 6);
}

int ThreadPerConnectionSendTask::add_request(SendStrategyOpType op,
                                             TransportQueueElement* element)
{
  DBG_ENTRY("ThreadPerConnectionSendTask", "add");

  ACE_Auto_Ptr<SendRequest> req(new SendRequest);
  req->op_ = op;
  req->element_ = element;

  int result = -1;
  { // guard scope
    GuardType guard(lock_);

    if (shutdown_initiated_) {
      return -1;
    }

    result = queue_.put(req.get());

    if (result == 0 && op == SEND_STOP) {
      work_available_.signal();
    }
  }

  if (result == 0) {
    req.release();
  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ThreadPerConnectionSendTask::add %p\n"),
               ACE_TEXT("put")));
  }

  return result;
}

int ThreadPerConnectionSendTask::open(void*)
{
  DBG_ENTRY("ThreadPerConnectionSendTask", "open");

  GuardType guard(lock_);

  // We can assume that we are in the proper state to handle this open()
  // call as long as we haven't been open()'ed before.
  if (opened_) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ThreadPerConnectionSendTask failed to open.  "
                      "Task has previously been open()'ed.\n"),
                     -1);
  }

  DirectPriorityMapper mapper(link_->transport_priority());
  int priority = mapper.thread_priority();

  long flags  = THR_NEW_LWP | THR_JOINABLE ;//|THR_SCOPE_PROCESS | THR_SCOPE_THREAD;
  int policy = TheServiceParticipant->scheduler();

  if (policy >= 0) {
    flags |= policy;
  } else {
    flags |= THR_INHERIT_SCHED;
  }

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ThreadPerConnectionSendTask::open(): ")
               ACE_TEXT("activating thread with flags 0x%08.8x ")
               ACE_TEXT("and priority %d.\n"),
               flags,
               priority));
  }

  // Activate this task object with one worker thread.
  if (activate(flags, 1, 0, priority) != 0) {
    // Assumes that when activate returns non-zero return code that
    // no threads were activated.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ThreadPerConnectionSendTask failed to activate "
                      "the worker threads.\n"),
                     -1);
  }

  // Now we have past the point where we can say we've been open()'ed before.
  opened_ = true;

  return 0;
}

int ThreadPerConnectionSendTask::svc()
{
  DBG_ENTRY_LVL("ThreadPerConnectionSendTask", "svc", 6);

  thr_id_ = ACE_OS::thr_self();

  // Ignore all signals to avoid
  //     ERROR: <something descriptive> Interrupted system call
  // The main thread will handle signals.
  sigset_t set;
  ACE_OS::sigfillset(&set);
  ACE_OS::thr_sigsetmask(SIG_SETMASK, &set, NULL);

  SendRequest* req;
  OPENDDS_VECTOR(SendRequest*) reqs;

  ACE_Reverse_Lock<LockType> rev_lock(lock_);
  GuardType guard(lock_);

  // Start the "GetWork-And-PerformWork" loop for the current worker thread.
  while (!shutdown_initiated_) {

    if (queue_.size() == 0) {
      work_available_.wait();
    }

    if (shutdown_initiated_) {
      break;
    }

    while (queue_.size() != 0 && (reqs.empty() || reqs.back()->op_ != SEND_STOP)) {
      req = queue_.get();

      if (req == 0) {
        //I'm not sure why this thread got more signals than actual signals
        //when using thread_per_connection and the user application thread
        //send requests without interval. We just need ignore the dequeue
        //failure.
        //ACE_ERROR ((LM_ERROR, "(%P|%t) ERROR: ThreadPerConnectionSendTask::svc  %p\n",
        //  ACE_TEXT("dequeue_head")));
        continue;
      }
      reqs.push_back(req);
    }

    ACE_Guard<ACE_Reverse_Lock<LockType> > rev_guard(rev_lock);

    if (!reqs.empty() && reqs.back()->op_ == SEND_STOP) {
      for (size_t i = 0; i < reqs.size(); ++i) {
        execute(*reqs[i]);
        delete reqs[i];
      }
      reqs.clear();
    }
  }

  return 0;
}

int ThreadPerConnectionSendTask::close(u_long flag)
{
  DBG_ENTRY("ThreadPerConnectionSendTask","close");

  if (flag == 0) {
    return 0;
  }

  {
    GuardType guard(lock_);

    if (shutdown_initiated_) {
      return 0;
    }

    // Set the shutdown flag to true.
    shutdown_initiated_ = true;
    work_available_.broadcast();
  }

  if (opened_ && !ACE_OS::thr_equal(thr_id_, ACE_OS::thr_self())) {
    wait();
  }

  return 0;
}

RemoveResult
ThreadPerConnectionSendTask::remove_sample(const DataSampleElement* element)
{
  DBG_ENTRY("ThreadPerConnectionSendTask", "remove_sample");

  ACE_Message_Block* payload = element->get_sample()->cont();
  ThreadPerConRemoveVisitor visitor(payload);

  GuardType guard(lock_);

  queue_.accept_visitor(visitor);

  return visitor.status();
}

void ThreadPerConnectionSendTask::execute(SendRequest& req)
{
  DBG_ENTRY_LVL("ThreadPerConnectionSendTask", "execute", 6);



  switch (req.op_) {
  case SEND_START:
    link_->send_start_i();
    break;
  case SEND:
    link_->send_i(req.element_);
    break;
  case SEND_STOP:
    //DataLink::send_stop_i expects the RepoId of the message sender, however, in ThreadPerConnectionSendTask
    //the control element will be a null element with only the op_ set.  Thus pass in GUID_UNKNOWN which will
    //allow send_stop to call send_delayed_notifications without a match.  In the case of ThreadPerConnectionSendTask
    //this is allowable because only one thread will be managing the sending thus no deadlock down in send_delayed_notifications()
    link_->send_stop_i(GUID_UNKNOWN);
    break;
  default:
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ThreadPerConnectionSendTask::execute unknown command %d\n",
               req.op_));
    break;
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
