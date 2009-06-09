// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_INSTANCESTATE_H
#define TAO_DDS_DCPS_INSTANCESTATE_H

#include "dcps_export.h"
#include "ace/Time_Value.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/GuidUtils.h"
#include <set>


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS 
{
  namespace DCPS 
  {
    class DataReaderImpl;
    class ReceivedDataElement;

    /**
     * @class InstanceState
     *
     * @brief manage the states of a received data instance.
     *
     * Provide a mechanism to manage the view state and instance
     * state values for an instance contained within a DataReader.
     * The instance_state and view_state are managed by this class.
     * Accessors are provided to query the current value of each of
     * these states.
     */
    class OpenDDS_Dcps_Export InstanceState
    {
    public:
      /// Constructor.
      InstanceState(DataReaderImpl* reader, DDS::InstanceHandle_t handle);

      /// Destructor 
      virtual ~InstanceState();

      /// Populate the SampleInfo structure
      void sample_info(::DDS::SampleInfo& si,
                       const ReceivedDataElement* de);

      /// Access instance state.
      DDS::InstanceStateKind instance_state() const;

      /// Access view state.
      DDS::ViewStateKind view_state() const;

      /// Access disposed generation count
      size_t disposed_generation_count() const;

      /// Access no writers generation count
      size_t no_writers_generation_count() const;
      
      /// DISPOSE message received for this instance.
      void dispose_was_received(const PublicationId& writer_id);

      /// Data sample received for this instance.
      void data_was_received(const PublicationId& writer_id);

      /// LIVELINESS message received for this DataWriter.
      void lively(const PublicationId& writer_id);

      /// A read or take operation has been performed on this instance.
      void accessed();

      bool most_recent_generation(ReceivedDataElement* item) const;

      /// DataReader has become empty.
      void empty(bool value);
      
      /// Remove the instance if it's instance has no samples
      /// and no writers.
      void release_if_empty();

      /// tell this instance when a DataWriter transitions to NOT_ALIVE
      void writer_became_dead(const PublicationId& writer_id,
                              int num_alive_writers,
                              const ACE_Time_Value& when);

      void unregister_was_received(const PublicationId& writer_id);

      DataReaderImpl* data_reader() const;

    private:


      /**
       * Current instance state.
       *
       * Can have values defined as:
       *
       *   DDS::ALIVE_INSTANCE_STATE
       *   DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE
       *   DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE
       *
       * and can be checked with the masks:
       *
       *   DDS::ANY_INSTANCE_STATE
       *   DDS::NOT_ALIVE_INSTANCE_STATE
       */
      DDS::InstanceStateKind instance_state_;

      /**
       * Current instance view state.
       *
       * Can have values defined as:
       *
       *   DDS::NEW_VIEW_STATE
       *   DDS::NOT_NEW_VIEW_STATE
       *
       * and can be checked with the mask:
       *
       *   DDS::ANY_VIEW_STATE
       */
      DDS::ViewStateKind view_state_;

      /// Number of times the instance state changes  
      /// from NOT_ALIVE_DISPOSED to ALIVE.
      size_t disposed_generation_count_;

      /// Number of times the instance state changes 
      /// from NOT_ALIVE_NO_WRITERS to ALIVE.
      size_t no_writers_generation_count_;

      /**
       * Keep track of the existence of a live writer since we need to
       * discern this difference while in the DISPOSED state.
       */
      bool no_writers_;

      /**
       * Keep track of whether the DataReader is empty or not.
       */
      bool empty_;

      /**
       * Reference to our containing reader.  This is used to call back
       * and notify the reader that liveliness has been lost on this
       * instance.  It is also queried to determine if the DataReader is
       * empty -- that it contains no more sample data.
       */
      DataReaderImpl* reader_;
      ::DDS::InstanceHandle_t handle_;

      typedef std::set <PublicationId, GUID_tKeyLessThan> Writers;

      Writers writers_;
    };

  } // namespace DCPS

} // namespace OpenDDS

#if defined (__ACE_INLINE__)
# include "InstanceState.inl"
#endif  /* __ACE_INLINE__ */

#endif /* TAO_DDS_DCPS_INSTANCESTATE_H */

