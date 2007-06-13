// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_DUMMYTCPFACTORY_H
#define TAO_DCPS_DUMMYTCPFACTORY_H

#include "DummyTcp_export.h"

#include "dds/DCPS/transport/framework/TransportImplFactory.h"

class DummyTcpTransport;


namespace TAO
{

  namespace DCPS
  {

    class DummyTcp_Export DummyTcpFactory : public TransportImplFactory
    {
      public:

        DummyTcpFactory();
        virtual ~DummyTcpFactory();

        virtual int requires_reactor() const;


      protected:

        virtual TransportImpl* create();
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "DummyTcpFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DDS_DUMMYTCPFACTORY_H */
