// -*- C++ -*-
//
// $Id$
#ifndef SIMPLETCP_GENERATOR_H
#define SIMPLETCP_GENERATOR_H

#include "SimpleTcp_export.h"

#include "dds/DCPS/transport/framework/TransportGenerator.h"
#include "ace/Synch.h"

namespace OpenDDS
{

  namespace DCPS
  {

    class SimpleTcp_Export SimpleTcpGenerator : public TransportGenerator
    {
      public:

        /// Default ctor.
        SimpleTcpGenerator();

        /// Dtor
        virtual ~SimpleTcpGenerator();

        /// Provide a new SimpleTcpFactory instance.
        virtual TransportImplFactory* new_factory();

        /// Provide a new SimpleTcpConfiguration instance.
        virtual TransportConfiguration* new_configuration(const TransportIdType id);
        
        /// Provide a list of default transport id.
        virtual void default_transport_ids (TransportIdList & ids);
    };

  }

}

#endif
