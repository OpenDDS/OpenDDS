// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEMCASTSYNCHRESOURCE_H
#define TAO_DCPS_SIMPLEMCASTSYNCHRESOURCE_H

#include  "SimpleMcastSocket_rch.h"
#include  "SimpleMcastSocket.h"
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

        SimpleMcastSynchResource(SimpleMcastSocket*  socket);
        virtual ~SimpleMcastSynchResource();

      private:

        SimpleMcastSocket_rch socket_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#endif  /* TAO_DCPS_SIMPLEMCASTSYNCHRESOURCE_H */
