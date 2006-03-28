// -*- C++ -*-
//
// $Id$
#ifndef SIMPLEUDP_GENERATOR_H
#define SIMPLEUDP_GENERATOR_H

#include  "SimpleUdp_export.h"
#include  "dds/DCPS/transport/framework/TransportGenerator.h"
#include  "ace/Synch.h"

namespace TAO
{

  namespace DCPS
  {

    class SimpleUdp_Export SimpleUdpGenerator : public TransportGenerator
    {
      public:

        /// Default ctor.
        SimpleUdpGenerator();

        /// Dtor
        virtual ~SimpleUdpGenerator();

        /// Provide a new SimpleUdpFactory instance.
        virtual TransportImplFactory* new_factory();

        /// Provide a new SimpleUdpConfiguration instance.
        virtual TransportConfiguration* new_configuration();
    };

  }

}

#endif
