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

      struct ConnectionInfo  {
        ACE_INET_Addr remote_addr;
        SimpleTcpConnection_rch connection;
      };

      /// Constructor.
      SimpleTcpReconnectTask(SimpleTcpTransport* transport_impl);

      /// Virtual Destructor.
      virtual ~SimpleTcpReconnectTask();

      /// Put a connection info object on to the request queue.
      /// Returns 0 if successful, -1 otherwise (it has been "rejected").
      int add(const ACE_INET_Addr&    remote_addr,
              SimpleTcpConnection_rch connection);

      /// Activate the worker threads
      virtual int open(void* ptr = 0);

      /// The "mainline" executed by the worker thread.
      virtual int svc();

      /// Shutdown the worker thread.
      virtual int close(u_long flag = 0);

    private:

      typedef ACE_SYNCH_MUTEX         LockType;
      typedef ACE_Guard<LockType>     GuardType;
      typedef ACE_Condition<LockType> ConditionType;

      typedef ACE_Unbounded_Queue<ConnectionInfo*> ConnectionInfoQueue;

      /// Lock to protect the "state" (all of the data members) of this object.
      LockType lock_;

      /// Queue that holds the ConnectionInfo objects.
      ConnectionInfoQueue queue_;

      /// Condition used to signal the worker threads that they may be able to
      /// find a connection in the queue_ that needs to be checked.
      /// This condition will be signal()'ed each time a new connection is
      /// added to the queue_, and also when this task is shutdown.
      ConditionType work_available_;

      /// This condition will be signal()'ed when the thread started or
      /// this task is shutdown.
      ConditionType active_workers_;

      /// Flag used to initiate a shutdown request to all worker threads.
      bool shutdown_initiated_;

      /// Flag used to avoid multiple open() calls.
      bool opened_;

      /// The number of currently active worker thread that used to sychronize
      /// the work thread and the caller thread.
      Thread_Counter num_threads_;

      /// Reference to the SimpleTcpTransport needed for the refresh_link() call.
      SimpleTcpTransport_rch transport_;
    };
  }
}


#endif /* TAO_DCPS_SIMPLETCPRECONNECTTASK_H */
