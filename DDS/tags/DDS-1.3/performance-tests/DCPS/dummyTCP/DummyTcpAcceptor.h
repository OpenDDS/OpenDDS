// -*- C++ -*-
//
// $Id$



#ifndef OPENDDS_DCPS_DUMMYTCPACCEPTOR_H
#define OPENDDS_DCPS_DUMMYTCPACCEPTOR_H

#include "DummyTcpTransport_rch.h"
#include "DummyTcpConnection.h"
#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"


namespace OpenDDS
{
  namespace DCPS
  {

    class DummyTcpConfiguration;

    class DummyTcpAcceptor : public ACE_Acceptor<DummyTcpConnection,
                                                  ACE_SOCK_ACCEPTOR>
    {
      public:

        DummyTcpAcceptor(DummyTcpTransport* transport_impl);
        virtual ~DummyTcpAcceptor();

        // Returns a reference that the caller becomes responsible for.
        DummyTcpTransport* transport();

        // This causes the Acceptor to drop its refernce to the
        // DummyTcpTransport object.
        void transport_shutdown();

        DummyTcpConfiguration* get_configuration();

      private:

        DummyTcpTransport_rch transport_;
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */



#endif  /* OPENDDS_DCPS_DUMMYTCPACCEPTOR_H */
