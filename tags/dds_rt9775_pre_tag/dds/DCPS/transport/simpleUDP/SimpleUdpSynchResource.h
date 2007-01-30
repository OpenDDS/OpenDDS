// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPSYNCHRESOURCE_H
#define TAO_DCPS_SIMPLEUDPSYNCHRESOURCE_H

#include  "SimpleUdpSocket_rch.h"
#include  "SimpleUdpSocket.h"
#include  "SimpleUdpTransport_rch.h"
#include  "SimpleUdpTransport.h"

#include  "dds/DCPS/transport/framework/ThreadSynchResource.h"
#include  "ace/Handle_Set.h"
#include  "ace/Time_Value.h"


namespace TAO
{
  namespace DCPS
  {

    class SimpleUdp_Export SimpleUdpSynchResource : public ThreadSynchResource
    {
      public:

        SimpleUdpSynchResource(SimpleUdpSocket*  socket,
                               SimpleUdpTransport * transport,
                               const int& max_output_pause_period_ms);
        virtual ~SimpleUdpSynchResource();

        virtual void notify_lost_on_backpressure_timeout ();

      private:

        SimpleUdpSocket_rch socket_;

        SimpleUdpTransport_rch transport_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#endif  /* TAO_DCPS_SIMPLEUDPSYNCHRESOURCE_H */
