// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEMCASTSYNCHRESOURCE_H
#define TAO_DCPS_SIMPLEMCASTSYNCHRESOURCE_H

#include  "SimpleMcastSocket_rch.h"
#include  "SimpleMcastSocket.h"
#include  "SimpleMcastTransport_rch.h"
#include  "SimpleMcastTransport.h"
#include  "dds/DCPS/transport/framework/ThreadSynchResource.h"
#include  "ace/Handle_Set.h"
#include  "ace/Time_Value.h"


namespace TAO
{
  namespace DCPS
  {

    class SimpleMcast_Export SimpleMcastSynchResource : public ThreadSynchResource
    {
      public:

        SimpleMcastSynchResource(SimpleMcastSocket*  socket,
                                 SimpleMcastTransport* transport,
                                 const int& max_output_pause_period_ms);
        virtual ~SimpleMcastSynchResource();

        void notify_lost_on_backpressure_timeout ();

      private:

        SimpleMcastSocket_rch socket_;
        SimpleMcastTransport_rch transport_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#endif  /* TAO_DCPS_SIMPLEMCASTSYNCHRESOURCE_H */
