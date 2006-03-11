// -*- C++ -*-
//
// $Id$

// WARNING!
// These are outside of the defines to force the compiler to recognize
//  the SimpleTcpTransport class BEFORE the SimpleTcpAcceptor class.
//  This is only a problem on gcc (3.4.4) on solaris x86
#include  "SimpleTcpTransport_rch.h"
#include  "SimpleTcpTransport.h"

#ifndef TAO_DCPS_SIMPLETCPACCEPTOR_H
#define TAO_DCPS_SIMPLETCPACCEPTOR_H

#include  "SimpleTcpConnection.h"
#include  "ace/Acceptor.h"
#include  "ace/SOCK_Acceptor.h"


namespace TAO
{
  namespace DCPS
  {

    class SimpleTcpConfiguration;

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

        SimpleTcpConfiguration* get_configuration();

      private:

        SimpleTcpTransport_rch transport_;
    };

  } /* namespace DCPS */

} /* namespace TAO */



#endif  /* TAO_DCPS_SIMPLETCPACCEPTOR_H */
