// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_DUMMYTCPRECONNECTTASK_H
#define OPENDDS_DCPS_DUMMYTCPRECONNECTTASK_H

#include /**/ "ace/pre.h"

#include "dds/DCPS/transport/framework/QueueTaskBase_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace OpenDDS
{

  namespace DCPS
  {
    class DummyTcpConnection;

    enum ReconnectOpType
      {
        DO_RECONNECT
      };


    /**
     * @class DummyTcpReconnectTask
     *
     * @brief Active Object managing a queue of reconnecting request.
     *
     *  This task handles request to reconnect to the remotes to avoid the
     *  the caller threads (thread to send or reactor thread) block on reconnecting.
     *  This reconnect task has lifetime as DummyTcpConnection object. One reconnect 
     *  task just dedicates to a single connection.
     */
    class DummyTcpReconnectTask : public QueueTaskBase <ReconnectOpType>
    {
    public:
   
      /// Constructor.
      DummyTcpReconnectTask(DummyTcpConnection* con);

      /// Virtual Destructor.
      virtual ~DummyTcpReconnectTask();

      /// Handle reconnect requests.
      virtual void execute (ReconnectOpType& op);

    private:

      /// The connection that needs be re-established.
      DummyTcpConnection* connection_;
    };
  }
}

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_DUMMYTCPRECONNECTTASK_H */
