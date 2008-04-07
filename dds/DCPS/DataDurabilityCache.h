// -*- C++ -*-

//=============================================================================
/**
 *  @file   DataCache.h
 *
 *  $Id$
 *
 *  Data cache interface used by OpenDDS @c TRANSIENT and
 *  @c PERSISTENT @c DURABILITY implementations.
 *
 *  @author Ossama Othman <othmano@ociweb.com>
 */
//=============================================================================

#ifndef OPENDDS_DATA_DURABILITY_CACHE_H
#define OPENDDS_DATA_DURABILITY_CACHE_H

#include "dds/DdsDcpsInfrastructureC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class ACE_Message_Block;

namespace OpenDDS
{
  namespace DCPS
  {
    typedef ACE_Message_Block DataSample;

    class WriteDataContainer;
    class DataWriterImpl;

    class DataDurabilityCache
    {
    public:

      /// Destructor.
      virtual ~DataDurabilityCache ();

      /// Enqueue the sample corresponding to the given topic instance
      /// (uniquely identify by its topic name and type name) into the
      /// @c TRANSIENT data durability cache.
      virtual bool enqueue (char const * topic_name,
                            char const * type_name,
                            DataSample * sample,
                            ::DDS::Time_t const & source_timestamp) = 0;


      /// Set characteristics for the given topic instance.
      virtual bool set_instance_characteristics (
        char const * topic_name,
        char const * type_name,
        ::DDS::Duration_t const & service_cleanup_delay,
        CORBA::Long depth) = 0;

      // Prepare data corresponding to given topic name and type name
      // for transmission over the transport.
      virtual bool send_durable_data (
        char const * topic_name,
        char const * type_name,
        WriteDataContainer * data_container,
        DataWriterImpl * data_writer,
        ::DDS::LifespanQosPolicy const & lifespan) = 0;

    };

  } // DCPS
} // OpenDDS


#endif  /* OPENDDS_DATA_DURABILITY_CACHE_H */
