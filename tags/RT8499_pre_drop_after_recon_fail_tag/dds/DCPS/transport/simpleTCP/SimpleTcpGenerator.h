// -*- C++ -*-
//
// $Id$
#ifndef SIMPLETCP_GENERATOR_H
#define SIMPLETCP_GENERATOR_H

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/transport/framework/TransportGenerator.h"
#include  "ace/Synch.h"

namespace TAO
{

  namespace DCPS
  {

    class TAO_DdsDcps_Export SimpleTcpGenerator : public TransportGenerator
    {
      public:

        /// Default ctor.
        SimpleTcpGenerator();

        /// Dtor
        virtual ~SimpleTcpGenerator();

        /// Provide a new SimpleTcpFactory instance.
        virtual TransportImplFactory* new_factory();

        /// Provide a new SimpleTcpConfiguration instance.
        virtual TransportConfiguration* new_configuration();
    };

  }

}

#endif
