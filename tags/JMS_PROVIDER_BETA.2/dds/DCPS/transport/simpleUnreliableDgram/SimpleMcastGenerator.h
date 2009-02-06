// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_SIMPLEMCAST_GENERATOR_H
#define OPENDDS_DCPS_SIMPLEMCAST_GENERATOR_H

#include "SimpleUnreliableDgram_export.h"
#include "dds/DCPS/transport/framework/TransportGenerator.h"
#include "ace/Synch.h"

namespace OpenDDS
{

  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleMcastGenerator : public TransportGenerator
    {
      public:

        /// Default ctor.
        SimpleMcastGenerator();

        /// Dtor
        virtual ~SimpleMcastGenerator();

        /// Provide a new SimpleMcastFactory instance.
        virtual TransportImplFactory* new_factory();

        /// Provide a new SimpleMcastConfiguration instance.
        virtual TransportConfiguration* new_configuration(const TransportIdType id);

        /// Provide a list of default transport id.
        virtual void default_transport_ids (TransportIdList & ids);
    };

  }

}

#endif
