// -*- C++ -*-
//
// $Id$
#include "ThreadSynchResource.h"
#include "EntryExit.h"


/// Note that we allow the ThreadSynchResource pointer to be NULL to
/// support the NullSynch case.
ACE_INLINE
OpenDDS::DCPS::ThreadSynch::ThreadSynch(ThreadSynchResource* resource)
  : worker_(0),
    resource_(resource)
{
  DBG_ENTRY_LVL("ThreadSynch","ThreadSynch",5);
}



ACE_INLINE int
OpenDDS::DCPS::ThreadSynch::register_worker(ThreadSynchWorker* worker)
{
  DBG_ENTRY_LVL("ThreadSynch","register_worker",5);
  this->worker_ = worker;
  return this->register_worker_i();
}


ACE_INLINE void
OpenDDS::DCPS::ThreadSynch::unregister_worker()
{
  DBG_ENTRY_LVL("ThreadSynch","unregister_worker",5);
  this->unregister_worker_i();
  this->worker_ = 0;
  delete this->resource_;
  this->resource_ = 0;
}


ACE_INLINE OpenDDS::DCPS::ThreadSynchWorker::WorkOutcome
OpenDDS::DCPS::ThreadSynch::perform_work()
{
  DBG_ENTRY_LVL("ThreadSynch","perform_work",5);

  if (this->worker_ == 0)
    {
      return ThreadSynchWorker::WORK_OUTCOME_NO_MORE_TO_DO;
    }

  return this->worker_->perform_work();
}


ACE_INLINE int
OpenDDS::DCPS::ThreadSynch::wait_on_clogged_resource()
{
  DBG_ENTRY_LVL("ThreadSynch","wait_on_clogged_resource",5);

  int result = -1;

  if (this->resource_)
    {
      result = this->resource_->wait_to_unclog();
    }
  else
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: ThreadSynch cannot wait on a NULL clogged resource.\n"));
//MJM: Do the %P|%t thing here, and identify where we are at.  This
//MJM: could become a critical message.
    }

  return result;
}
