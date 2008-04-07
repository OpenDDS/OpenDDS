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

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Thread_Mutex.h"

#include <string>
#include <map>
#include <list>


namespace OpenDDS
{
  namespace DCPS
  {
    /**
     * @class TransientDataDurabilityCache
     *
     * @brief Underlying data cache used by OpenDDS @c TRANSIENT @c
     *        DURABILITY implementation.
     *
     * This class implements a transient cache that outlives @c
     * DataWriters but not the process within which the @c DataWriters
     * reside.
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
       * @brief Map data type for all samples and their corresponding
       *        source timestamp.
       */
      class sample_data_type
      {
      public:

        sample_data_type ();
        sample_data_type (DataSample * s, ::DDS::Time_t t);
        sample_data_type (sample_data_type const & rhs);

        ~sample_data_type ();

        sample_data_type & operator= (sample_data_type const & rhs);

      private:

        /// Non-throwing swap operation.
        /**
         * Non-throwing swap operation used in assignment operator
         * implementation.
         */
        void swap (sample_data_type & rhs);

      public:

        DataSample * sample;
        ::DDS::Time_t source_timestamp;
      };

      /**
       * @struct characteristic_data_type
       *
       * @brief Map data type for all topic instance characteristics.
       */
      struct characteristic_data_type
      {
        characteristic_data_type (::DDS::Duration_t scd,
                                  ::CORBA::Long d)
          : service_cleanup_delay (scd)
          , depth (d)
        {
        }

        ::DDS::Duration_t service_cleanup_delay;
        CORBA::Long depth;
      };


      typedef std::list<sample_data_type> sample_list_type;
      typedef std::map<key_type,
                       sample_list_type> sample_map_type;

      typedef std::map<key_type,
                       characteristic_data_type> characteristic_map_type;

      /// Constructor.
      TransientDataDurabilityCache ();

      /// Destructor.
      virtual ~TransientDataDurabilityCache ();

      virtual bool enqueue (char const * topic_name,
                            char const * type_name,
                            DataSample * sample,
                            ::DDS::Time_t const & source_timestamp);

      virtual bool set_instance_characteristics (
        char const * topic_name,
        char const * type_name,
        ::DDS::Duration_t const & service_cleanup_delay,
        CORBA::Long depth);

      virtual bool send_durable_data (
        char const * topic_name,
        char const * type_name,
        WriteDataContainer * data_container,
        DataWriterImpl * data_writer,
        ::DDS::LifespanQosPolicy const & lifespan);

    private:

      /// Map of all data samples.
      sample_map_type samples_;

      /// Topic instance characteristics map.
      characteristic_map_type characteristics_;

      /// Lock for synchronized access to the underlying list.
      ACE_SYNCH_MUTEX lock_;

    };

  } // DCPS
} // OpenDDS

#endif  /* OPENDDS_TRANSIENT_DATA_DURABILITY_CACHE_H */
