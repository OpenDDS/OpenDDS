// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDP_GENERATOR_H
#define TAO_DCPS_SIMPLEUDP_GENERATOR_H

#include "SimpleUnreliableDgram_export.h"
#include "dds/DCPS/transport/framework/TransportGenerator.h"
#include "ace/Synch.h"

namespace TAO
{

  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleUdpGenerator : public TransportGenerator
    {
      public:

        /// Default ctor.
        SimpleUdpGenerator();

        /// Dtor
        virtual ~SimpleUdpGenerator();

        /// Provide a new SimpleUdpFactory instance.
        virtual TransportImplFactory* new_factory();

        /// Provide a new SimpleUdpConfiguration instance.
        virtual TransportConfiguration* new_configuration(const TransportIdType id);
        
        /// Provide a list of default transport id.
        virtual void default_transport_ids (TransportIdList & ids);
    };

  }

}

#endif
