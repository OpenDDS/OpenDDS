// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLETCPSYNCHRESOURCE_H
#define TAO_DCPS_SIMPLETCPSYNCHRESOURCE_H

#include  "SimpleTcpConnection_rch.h"
#include  "dds/DCPS/transport/framework/ThreadSynchResource.h"
#include  "ace/Handle_Set.h"


namespace TAO
{
  namespace DCPS
  {

    class SimpleTcpSynchResource : public ThreadSynchResource
    {
      public:

        SimpleTcpSynchResource(SimpleTcpConnection* connection);
        virtual ~SimpleTcpSynchResource();

        virtual void wait_to_unclog();


      private:

        SimpleTcpConnection_rch connection_;
        ACE_HANDLE handle_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleTcpSynchResource.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLETCPDATALINK_H */
