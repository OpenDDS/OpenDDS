// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLETCPRECONNECTTASK_H
#define TAO_DCPS_SIMPLETCPRECONNECTTASK_H

#include /**/ "ace/pre.h"

#include  "SimpleTcpTransport_rch.h"
#include  "SimpleTcpConnection_rch.h"
#include  "dds/DCPS/RcObject_T.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Task.h"
#include "ace/Synch.h"
#include "ace/Unbounded_Queue.h"
#include "ace/INET_Addr.h"


namespace TAO
{

  namespace DCPS
  {
    class SimpleTcpTransport;

    /// Typedef for the number of threads.
    typedef unsigned long Thread_Counter;

    /**
     * @class SimpleTcpReconnectTask
     *
     * @brief Active Object managing a queue of connection info objects.
     *
     *  This task is dedicated to check the incoming connections are re-established
     *  connection from the remote. This would resolve the deadlock problem between the
     *  reactor thread (calling SimpleTcpTransport::passive_connction()) and the orb
     *  thread (calling SimpleTcpTransport::make_passive_connction()). The reactor
     *  thread will enqueue the new connection to this task and let this task dequeue
     *  and check the connection.
     */
    class SimpleTcpReconnectTask : public ACE_Task_Base, public RcObject<ACE_SYNCH_MUTEX>
    {
    public:

      enum COMMAND
      {
        NEW_CONNECTION_CHECK,
        DO_RECONNECT
      };

      struct ConnectionInfo : public RcObject<ACE_SYNCH_MUTEX> {
        COMMAND command;
        SimpleTcpConnection_rch connection;
      };

      typedef RcHandle<ConnectionInfo> ConnectionInfo_rch;

      /// Constructor.
      SimpleTcpReconnectTask(SimpleTcpTransport* transport_impl = 0);

      /// Virtual Destructor.
      virtual ~SimpleTcpReconnectTask();

      /// Put a connection info object on to the request queue.
      /// Returns 0 if successful, -1 otherwise (it has been "rejected").
      int add(COMMAND command, SimpleTcpConnection* connection);

      /// Activate the worker threads
      virtual int open(void* ptr = 0);

      /// The "mainline" executed by the worker thread.
      virtual int svc();

      /// Called when the thread exits.
      virtual int close(u_long flag = 0);

      /// Shutdown the worker thread.
      int shutdown();

    private:

      typedef ACE_SYNCH_MUTEX         LockType;
      typedef ACE_Guard<LockType>     GuardType;
      typedef ACE_Condition<LockType> ConditionType;

      typedef ACE_Unbounded_Queue<ConnectionInfo_rch> ConnectionInfoQueue;

      /// Lock to protect the "state" (all of the data members) of this object.
      LockType lock_;

      /// Queue that holds the ConnectionInfo objects.
      ConnectionInfoQueue queue_;

      /// Condition used to signal the worker threads that they may be able to
      /// find a connection in the queue_ that needs to be checked.
      /// This condition will be signal()'ed each time a new connection is
      /// added to the queue_, and also when this task is shutdown.
      ConditionType work_available_;

      /// Flag used to initiate a shutdown request to all worker threads.
      bool shutdown_initiated_;

      /// Flag used to avoid multiple open() calls.
      bool opened_;

      /// Reference to the SimpleTcpTransport needed for the refresh_link() call.
      SimpleTcpTransport_rch transport_;

      /// The id of the thread created by this task.
      ACE_thread_t thr_id_;
    };
  }
}

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_SIMPLETCPRECONNECTTASK_H */
