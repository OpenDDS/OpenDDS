// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "InstanceState.h"
#include "DataReaderImpl.h"
#include "SubscriptionInstance.h"
#include "ReceivedDataElementList.h"
#include "Qos_Helper.h"

#if !defined (__ACE_INLINE__)
# include "InstanceState.inl"
#endif /* ! __ACE_INLINE__ */


TAO::DCPS::InstanceState::InstanceState (DataReaderImpl* reader,
                              ::DDS::InstanceHandle_t handle)
   : instance_state_( 0)
   , view_state_( 0)
   , disposed_generation_count_( 0)
   , no_writers_generation_count_( 0)
   , no_writers_( false)
   , empty_( true)
   , reader_( reader)
   , handle_(handle)
{
  ::DDS::DataReaderQos qos ;
  this->reader_->get_qos(qos) ;

}

// cannot ACE_INLINE because of #include loop

void
TAO::DCPS::InstanceState::dispose_was_received()
{
  //
  // Manage the instance state on disposal here.
  //
  if( this->instance_state_ & DDS::ALIVE_INSTANCE_STATE)
    {
      this->instance_state_ = DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE ;
      //spec says: if "no samples in the DataReader && no "live" writers"
      //      then destroy the instance.
      // TAO::DCPS::InstanceState::empty(true) will be called if "no samples"
    }
}

void 
TAO::DCPS::InstanceState::writer_became_dead (PublicationId         writer_id,
                                   int                   num_alive_writers,
                                   const ACE_Time_Value& when)
{
  //TBD keep track of which writer has written to this instance
  // and only set to NOT_ALIVE if no other writers are writing to
  // this instance.
  // the CURRENT implementation just assumes that all writers are
  // writing to all instances.
  ACE_UNUSED_ARG(writer_id);
  ACE_UNUSED_ARG(when);


  if(num_alive_writers == 0 && this->instance_state_ & DDS::ALIVE_INSTANCE_STATE)
    {
      this->no_writers_ = true;
      this->instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE ;
     // spec says if "no samples in the DataReader" then the
     //      instance is removed.
     // TAO::DCPS::InstanceState::empty(true) will be called if "no samples"
    }
}
