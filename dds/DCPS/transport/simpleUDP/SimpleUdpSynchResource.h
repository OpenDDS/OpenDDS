// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPSYNCHRESOURCE_H
#define TAO_DCPS_SIMPLEUDPSYNCHRESOURCE_H

#include  "SimpleUdpSocket_rch.h"
#include  "SimpleUdpSocket.h"
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

        SimpleUdpSynchResource(SimpleUdpSocket*  socket);
        virtual ~SimpleUdpSynchResource();

      private:

        SimpleUdpSocket_rch socket_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#endif  /* TAO_DCPS_SIMPLEUDPSYNCHRESOURCE_H */
