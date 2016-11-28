/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTREACTORTASK_H
#define OPENDDS_DCPS_TRANSPORTREACTORTASK_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "ace/Task.h"
#include "ace/Barrier.h"
#include "ace/Synch_Traits.h"
#include "ace/Condition_T.h"
#include "ace/Condition_Thread_Mutex.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Proactor;
class ACE_Reactor;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportReactorTask : public virtual ACE_Task_Base,
public virtual RcObject<ACE_SYNCH_MUTEX> {
public:

  TransportReactorTask(bool useAsyncSend);
  virtual ~TransportReactorTask();

  virtual int open(void*);
  virtual int svc();
  virtual int close(u_long flags = 0);

  void stop();

  ACE_Reactor* get_reactor();
  const ACE_Reactor* get_reactor() const;

  ACE_thread_t get_reactor_owner() const;

  ACE_Proactor* get_proactor();
  const ACE_Proactor* get_proactor() const;

  void wait_for_startup() { barrier_.wait(); }

  bool is_shut_down() const { return state_ == STATE_NOT_RUNNING; }

  OPENDDS_POOL_ALLOCATION_FWD

private:

  typedef ACE_SYNCH_MUTEX         LockType;
  typedef ACE_Guard<LockType>     GuardType;
  typedef ACE_Condition<LockType> ConditionType;

  enum State { STATE_NOT_RUNNING, STATE_OPENING, STATE_RUNNING };

  ACE_Barrier   barrier_;
  LockType      lock_;
  State         state_;
  ConditionType condition_;
  ACE_Reactor*  reactor_;
  ACE_thread_t  reactor_owner_;
  ACE_Proactor* proactor_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportReactorTask.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTREACTORTASK_H */
