// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLETCPACCEPTOR_H
#define TAO_DCPS_SIMPLETCPACCEPTOR_H

#include  "SimpleTcpTransport_rch.h"
#include  "SimpleTcpConnection.h"
#include  "ace/Acceptor.h"
#include  "ace/SOCK_Acceptor.h"


namespace TAO
{
  namespace DCPS
  {

    class SimpleTcpAcceptor : public ACE_Acceptor<SimpleTcpConnection,
                                                  ACE_SOCK_ACCEPTOR>
    {
      public:

        SimpleTcpAcceptor(SimpleTcpTransport* transport_impl);
        virtual ~SimpleTcpAcceptor();

        // Returns a reference that the caller becomes responsible for.
        SimpleTcpTransport* transport();

        // This causes the Acceptor to drop its refernce to the
        // SimpleTcpTransport object.
        void transport_shutdown();


      private:

        SimpleTcpTransport_rch transport_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleTcpAcceptor.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLETCPACCEPTOR_H */
