// -*- C++ -*-
//
// $Id$



#ifndef OPENDDS_DCPS_SIMPLETCPACCEPTOR_H
#define OPENDDS_DCPS_SIMPLETCPACCEPTOR_H

#include "SimpleTcpTransport_rch.h"
#include "SimpleTcpConnection.h"
#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"


namespace OpenDDS
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

} /* namespace OpenDDS */



#endif  /* OPENDDS_DCPS_SIMPLETCPACCEPTOR_H */
