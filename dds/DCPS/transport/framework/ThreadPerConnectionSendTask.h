/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADPERCONNECTIONSENDER_H
#define OPENDDS_DCPS_THREADPERCONNECTIONSENDER_H

#include /**/ "ace/pre.h"

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/PoolAllocationBase.h"
#include "BasicQueue_T.h"
#include "TransportDefs.h"

#include "ace/Condition_T.h"
#include "ace/Synch_Traits.h"
#include "ace/Task.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataLink;
class TransportQueueElement;
class DataSampleElement;
class TransportSendElement;


enum SendStrategyOpType {
  SEND_START,
  SEND,
  SEND_STOP,
  REMOVE_SAMPLE,
  REMOVE_ALL_CONTROL_SAMPLES
};

struct SendRequest : public PoolAllocationBase {
  SendStrategyOpType op_;
  TransportQueueElement* element_;
};

/**
 * @class ThreadPerConnectionSendTask
 *
 * @brief Execute the requests of sending a sample or control message.
 *
 *  This task implements the request execute method which handles each step
 *  of sending a sample or control message.
 */
class OpenDDS_Dcps_Export ThreadPerConnectionSendTask : public ACE_Task_Base {
public:
  ThreadPerConnectionSendTask(DataLink* link);

  virtual ~ThreadPerConnectionSendTask();

  /// Put the request to the request queue.
  /// Returns 0 if successful, -1 otherwise (it has been "rejected" or this
  /// task is shutdown).
  int add_request(SendStrategyOpType op, TransportQueueElement* element = 0);

  /// Activate the worker threads
  virtual int open(void* = 0);

  /// The "mainline" executed by the worker thread.
  virtual int svc();

  /// Called when the thread exits.
  virtual int close(u_long flag = 0);

  /// Remove sample from the thread per connection queue.
  RemoveResult remove_sample(const DataSampleElement* element);

private:

  /// Handle the request.
  virtual void execute(SendRequest& req);

  typedef ACE_SYNCH_MUTEX         LockType;
  typedef ACE_Guard<LockType>     GuardType;
  typedef ACE_Condition<LockType> ConditionType;

  typedef BasicQueue<SendRequest> QueueType;

  /// Lock to protect the "state" (all of the data members) of this object.
  LockType lock_;

  /// The request queue.
  QueueType queue_;

  /// Condition used to signal the worker threads that they may be able to
  /// find a request in the queue_ that needs to be executed.
  /// This condition will be signal()'ed each time a request is
  /// added to the queue_, and also when this task is shutdown.
  ConditionType work_available_;

  /// Flag used to initiate a shutdown request to all worker threads.
  bool shutdown_initiated_;

  /// Flag used to avoid multiple open() calls.
  bool opened_;

  /// The id of the thread created by this task.
  ACE_thread_t thr_id_;

  /// The datalink to send the samples or control messages.
  DataLink* link_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_THREADPERCONNECTIONSENDER_H */
