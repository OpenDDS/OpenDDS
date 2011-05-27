// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPFACTORY_H
#define TAO_DCPS_SIMPLEUDPFACTORY_H

#include "SimpleUnreliableDgram_export.h"
#include "dds/DCPS/transport/framework/TransportImplFactory.h"

class SimpleUdpTransport;


namespace TAO
{

  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleUdpFactory : public TransportImplFactory
    {
      public:

        SimpleUdpFactory();
        virtual ~SimpleUdpFactory();

        virtual int requires_reactor() const;


      protected:

        virtual TransportImpl* create();
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleUdpFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DDS_SIMPLEUDPFACTORY_H */
