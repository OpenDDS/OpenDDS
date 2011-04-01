// -*- C++ -*-
//
// $Id$
#ifndef DUMMYTCP_GENERATOR_H
#define DUMMYTCP_GENERATOR_H

#include "DummyTcp_export.h"

#include "dds/DCPS/transport/framework/TransportGenerator.h"
#include "ace/Synch.h"

namespace OpenDDS
{

  namespace DCPS
  {

    class DummyTcp_Export DummyTcpGenerator : public TransportGenerator
    {
      public:

        /// Default ctor.
        DummyTcpGenerator();

        /// Dtor
        virtual ~DummyTcpGenerator();

        /// Provide a new DummyTcpFactory instance.
        virtual TransportImplFactory* new_factory();

        /// Provide a new DummyTcpConfiguration instance.
        virtual TransportConfiguration* new_configuration(const TransportIdType id);

        /// Provide a list of default transport id.
        virtual void default_transport_ids (TransportIdList & ids);
    };

  }

}

#endif
