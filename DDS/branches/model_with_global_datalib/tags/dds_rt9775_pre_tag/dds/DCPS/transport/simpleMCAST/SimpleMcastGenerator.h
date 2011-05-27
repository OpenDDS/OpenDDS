// -*- C++ -*-
//
// $Id$
#ifndef SIMPLEMCAST_GENERATOR_H
#define SIMPLEMCAST_GENERATOR_H

#include  "SimpleMcast_export.h"
#include  "dds/DCPS/transport/framework/TransportGenerator.h"
#include  "ace/Synch.h"

namespace TAO
{

  namespace DCPS
  {

    class SimpleMcast_Export SimpleMcastGenerator : public TransportGenerator
    {
      public:

        /// Default ctor.
        SimpleMcastGenerator();

        /// Dtor
        virtual ~SimpleMcastGenerator();

        /// Provide a new SimpleMcastFactory instance.
        virtual TransportImplFactory* new_factory();

        /// Provide a new SimpleMcastConfiguration instance.
        virtual TransportConfiguration* new_configuration();
    };

  }

}

#endif
