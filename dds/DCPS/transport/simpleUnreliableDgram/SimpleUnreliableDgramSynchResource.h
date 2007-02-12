// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUNRELIABLEDGRAMSYNCHRESOURCE_H
#define TAO_DCPS_SIMPLEUNRELIABLEDGRAMSYNCHRESOURCE_H

#include "SimpleUnreliableDgram_export.h"
#include "SimpleUnreliableDgramSocket_rch.h"
#include "SimpleUnreliableDgramTransport.h"
#include "SimpleUnreliableDgramTransport_rch.h"
#include "dds/DCPS/transport/framework/ThreadSynchResource.h"
#include "ace/Handle_Set.h"
#include "ace/Time_Value.h"


namespace TAO
{
  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleUnreliableDgramSynchResource 
      : public ThreadSynchResource
    {
      public:

        SimpleUnreliableDgramSynchResource(SimpleUnreliableDgramSocket*  socket,
                                 SimpleUnreliableDgramTransport* transport,
                                 const int& max_output_pause_period_ms);
        virtual ~SimpleUnreliableDgramSynchResource();

        void notify_lost_on_backpressure_timeout ();

      private:

        SimpleUnreliableDgramSocket_rch socket_;
        SimpleUnreliableDgramTransport_rch transport_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#endif  /* TAO_DCPS_SIMPLEUNRELIABLEDGRAMSYNCHRESOURCE_H */
