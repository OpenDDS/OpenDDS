// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_DUMMYTCPCONNECTIONREPLACETASK_H
#define OPENDDS_DCPS_DUMMYTCPCONNECTIONREPLACETASK_H

#include /**/ "ace/pre.h"

#include "DummyTcpConnection_rch.h"
#include "dds/DCPS/transport/framework/QueueTaskBase_T.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace OpenDDS
{

  namespace DCPS
  {
    class DummyTcpTransport;

    /**
     * @class DummyTcpConnectionReplaceTask
     *
     * @brief Active Object managing a queue of connection info objects.
     *
     *  This task is dedicated to check if the incoming connections are re-established
     *  connection from the remote. This would resolve the deadlock problem between the
     *  reactor thread (calling DummyTcpTransport::passive_connction()) and the orb
     *  thread (calling DummyTcpTransport::make_passive_connction()). The reactor
     *  thread will enqueue the new connection to this task and let this task dequeue
     *  and check the connection. This task handles all connections associated with
     *  a TransportImpl object.
     */
    class DummyTcpConnectionReplaceTask : public QueueTaskBase <DummyTcpConnection_rch>
    {
    public:



      /// Constructor.
      DummyTcpConnectionReplaceTask(DummyTcpTransport* trans);

      /// Virtual Destructor.
      virtual ~DummyTcpConnectionReplaceTask();

      /// Handle the request.
      virtual void execute (DummyTcpConnection_rch& con);

    private:

      DummyTcpTransport* trans_;
    };
  }
}

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_DUMMYTCPCONNECTIONREPLACETASK_H */
