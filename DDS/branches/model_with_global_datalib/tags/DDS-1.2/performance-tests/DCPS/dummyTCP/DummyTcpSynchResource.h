// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_DUMMYTCPSYNCHRESOURCE_H
#define OPENDDS_DCPS_DUMMYTCPSYNCHRESOURCE_H

#include "DummyTcpConnection_rch.h"
#include "DummyTcpConnection.h"
#include "dds/DCPS/transport/framework/ThreadSynchResource.h"
#include "ace/Handle_Set.h"
#include "ace/Time_Value.h"


namespace OpenDDS
{
  namespace DCPS
  {

    class DummyTcpSynchResource : public ThreadSynchResource
    {
      public:

        DummyTcpSynchResource(DummyTcpConnection*  connection,
                               const int& max_output_pause_period_ms);
        virtual ~DummyTcpSynchResource();

        virtual void notify_lost_on_backpressure_timeout ();

      private:

        DummyTcpConnection_rch connection_;
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#endif  /* OPENDDS_DCPS_DUMMYTCPSYNCHRESOURCE_H */
