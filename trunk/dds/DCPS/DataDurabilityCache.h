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

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <memory>

class ACE_Message_Block;

namespace DDS
{
  struct DurabilityServiceQosPolicy;
  struct LifespanQosPolicy;
}

namespace OpenDDS
{
  namespace DCPS
  {
    class WriteDataContainer;
    class DataWriterImpl;
    class DataSampleList;

    /**
     * @class DataDurabilityCache
     *
     * @brief
     *
     * 
     */
    class DataDurabilityCache
    {
    public:

      /// Destructor.
      virtual ~DataDurabilityCache ();

      /// Insert the samples corresponding to the given topic instance
      /// (uniquely identify by its topic name and type name within a
      /// given domain) into the data durability cache.
      virtual bool insert (char const * topic_name,
                           char const * type_name,
                           DataSampleList & samples,
                           ::DDS::DurabilityServiceQosPolicy const & qos) = 0;

      // Prepare data corresponding to given topic name and type name
      // for transmission over the transport.
      virtual bool send_data (
        char const * topic_name,
        char const * type_name,
        WriteDataContainer * data_container,
        DataWriterImpl * data_writer,
        ::DDS::LifespanQosPolicy const & lifespan) = 0;

    };

  } // DCPS
} // OpenDDS


#endif  /* OPENDDS_DATA_DURABILITY_CACHE_H */
