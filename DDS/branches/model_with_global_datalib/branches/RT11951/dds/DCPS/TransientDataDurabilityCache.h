// -*- C++ -*-

//=============================================================================
/**
 *  @file   TransientDataDurabilityCache.h
 *
 *  $Id$
 *
 *  Underlying data cache used by OpenDDS @c TRANSIENT @c DURABILITY
 *  implementation.
 *
 *  @author Ossama Othman <othmano@ociweb.com>
 */
//=============================================================================

#ifndef OPENDDS_TRANSIENT_DATA_DURABILITY_CACHE_H
#define OPENDDS_TRANSIENT_DATA_DURABILITY_CACHE_H

#include "dds/DCPS/DataDurabilityCache.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/DataSampleList.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Thread_Mutex.h"

#include <string>
#include <map>
#include <list>
#include <utility>


namespace OpenDDS
{
  namespace DCPS
  {
    /**
     * @class TransientDataDurabilityCache
     *
     * @brief Underlying data cache used by OpenDDS @c TRANSIENT
     *        @c DURABILITY implementation.
     *
     * This class implements a transient cache that outlives
     * @c DataWriters but not the @c Publisher within which the
     * @c DataWriters reside.
     */
    class TransientDataDurabilityCache
      : public DataDurabilityCache
    {
    public:

      /**
       * @struct key_type
       *
       * @brief Key type for underlying maps.
       *
       * Each instance may be uniquely identified by its topic name
       * and type name.  We use that property to establish a map key
       * type.
       */
      struct key_type
      {
        key_type (char const * topic, char const * type)
          : topic_name (topic)
          , type_name (type)
        {
        }

        key_type (key_type const & rhs)
          : topic_name (rhs.topic_name)
          , type_name (rhs.type_name)
        {
        }

        key_type & operator= (key_type const & rhs)
        {
          topic_name = rhs.topic_name;
          type_name = rhs.type_name;

          return *this;
        }

        bool operator== (key_type const & rhs) const
        {
          return
            topic_name == rhs.topic_name
            && type_name == rhs.type_name;
        }

        bool operator< (key_type const & rhs) const
        {
          return
            topic_name < rhs.topic_name
            && type_name < rhs.type_name;
        }

        std::string topic_name;
        std::string type_name;
      };

      /**
       * @class sample_data_type
       *
       * @brief Sample list data type for all samples.
       */
      class sample_data_type
      {
      public:

        sample_data_type ();
        explicit sample_data_type (DataSample * s);
        sample_data_type (sample_data_type const & rhs);

        ~sample_data_type ();

        sample_data_type & operator= (sample_data_type const & rhs);

      public:

        DataSample * sample;

      };

      typedef std::list<sample_data_type> sample_list_type;
      typedef std::map<key_type,
                       sample_list_type> sample_map_type;
      typedef std::list<long> timer_id_list_type;

      /// Constructor.
      TransientDataDurabilityCache ();

      /// Destructor.
      virtual ~TransientDataDurabilityCache ();

      virtual bool insert (char const * topic_name,
                           char const * type_name,
                           DataSampleList & unsent_data,
                           ::DDS::DurabilityServiceQosPolicy const & qos);

      virtual bool send_data (
        char const * topic_name,
        char const * type_name,
        WriteDataContainer * data_container,
        DataWriterImpl * data_writer,
        ::DDS::LifespanQosPolicy const & lifespan);

    private:

      /// Map of all data samples.
      sample_map_type samples_;

      /// Timer ID list.
      /**
       * Keep track of cleanup timer IDs in case we need to cancel
       * before they expire.
       */
      timer_id_list_type cleanup_timer_ids_;

      /// Lock for synchronized access to the underlying list.
      ACE_SYNCH_MUTEX lock_;

      /// Reactor with which cleanup timers will be registered.
      ACE_Reactor * reactor_;

    };

  } // DCPS
} // OpenDDS

#endif  /* OPENDDS_TRANSIENT_DATA_DURABILITY_CACHE_H */
