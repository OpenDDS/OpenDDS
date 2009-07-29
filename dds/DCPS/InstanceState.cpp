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


OpenDDS::DCPS::InstanceState::InstanceState (DataReaderImpl* reader,
                              ::DDS::InstanceHandle_t handle)
   : instance_state_( 0)
   , view_state_( 0)
   , disposed_generation_count_( 0)
   , no_writers_generation_count_( 0)
   , empty_( true)
   , release_pending_(false)
   , reader_( reader)
   , handle_(handle)
{
  ::DDS::DataReaderQos qos ;
  this->reader_->get_qos(qos) ;
}

// cannot ACE_INLINE because of #include loop

void
OpenDDS::DCPS::InstanceState::dispose_was_received(const PublicationId& writer_id)
{
  writers_.erase (writer_id);

  //
  // Manage the instance state on disposal here.
  //
  if( this->instance_state_ & DDS::ALIVE_INSTANCE_STATE)
    {
      this->instance_state_ = DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE ;
      // N.B. dispose instance transitions are always followed by a
      // non-valid sample being queued to the ReceivedDataElementList;
      // marking the release as pending prevents this sample from being lost.
      this->release_pending_ = true;
    }
}

void
OpenDDS::DCPS::InstanceState::unregister_was_received(const PublicationId& writer_id)
{
  writers_.erase (writer_id);

  if(writers_.empty () && this->instance_state_ & DDS::ALIVE_INSTANCE_STATE)
    {
      this->instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE ;
      // N.B. unregister instance transitions are always followed by a
      // non-valid sample being queued to the ReceivedDataElementList;
      // marking the release as pending prevents this sample from being lost.
      this->release_pending_ = true;
    }
}

void 
OpenDDS::DCPS::InstanceState::writer_became_dead (
  const PublicationId&  writer_id,
  int                   /*num_alive_writers*/,
  const ACE_Time_Value& /* when */)
{
  writers_.erase (writer_id);

  if(writers_.empty () && this->instance_state_ & DDS::ALIVE_INSTANCE_STATE)
    {
      this->instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE ;
     // spec says if "no samples in the DataReader" then the
     //      instance is removed.
      this->release_if_empty ();
    }
}

void
OpenDDS::DCPS::InstanceState::cancel_pending()
{
  // Cancel pending release
  this->release_pending_ = false;
}

void
OpenDDS::DCPS::InstanceState::release_if_empty()
{
  if( this->empty_ && this->writers_.empty())
  {
    this->reader_->release_instance (this->handle_);
    this->release_pending_ = false;
  }
  else
  {
    this->release_pending_ = true;
  }
}

