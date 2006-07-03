// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_THREADPERCONNECTIONSENDER_H
#define TAO_DCPS_THREADPERCONNECTIONSENDER_H

#include /**/ "ace/pre.h"

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/transport/framework/QueueTaskBase_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace TAO
{

  namespace DCPS
  {
    class DataLink;
    class TransportQueueElement;

    enum SendStrategyOpType
    {
      SEND_START,
      SEND,
      SEND_STOP
    };

    struct SendRequest
    {
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
    class TAO_DdsDcps_Export ThreadPerConnectionSendTask : public QueueTaskBase <SendRequest>
    {
    public:
   
      /// Constructor.
      ThreadPerConnectionSendTask(DataLink* link);

      /// Virtual Destructor.
      virtual ~ThreadPerConnectionSendTask();

      /// Handle the request.
      virtual void execute (SendRequest& req);

    private:

      /// The datalink to send the samples or control messages.
      DataLink* link_;
    };
  }
}

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_THREADPERCONNECTIONSENDER_H */
