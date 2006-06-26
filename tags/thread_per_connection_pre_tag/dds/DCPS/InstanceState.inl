// -*- C++ -*-
//
// $Id$

#include "ReceivedDataElementList.h"
#include "ace/OS_NS_sys_time.h"


ACE_INLINE
TAO::DCPS::InstanceState::~InstanceState ()
{
}


ACE_INLINE
void
TAO::DCPS::InstanceState::accessed()
{
//
// Manage the view state due to data access here.
//
  if (this->view_state_ & DDS::ANY_VIEW_STATE)
    {
      this->view_state_ = DDS::NOT_NEW_VIEW_STATE ;
    }
}


ACE_INLINE
void TAO::DCPS::InstanceState::sample_info(::DDS::SampleInfo& si,
                                const ReceivedDataElement* de)
{
  si.sample_state = de->sample_state_ ;
  si.view_state = view_state_ ;
  si.instance_state = instance_state_ ;
  si.disposed_generation_count = disposed_generation_count_ ;
  si.no_writers_generation_count = no_writers_generation_count_ ;
  si.source_timestamp = de->source_timestamp_ ;
  si.instance_handle = handle_ ;
/*
 * These are actually calculated later...
 */
  si.sample_rank = 0 ;

  // these aren't the real value, they're being saved 
  // for a later calculation. the actual value is
  // calculated in DataReaderImpl::sample_info using
  // these values.
  si.generation_rank = de->disposed_generation_count_ +
                       de->no_writers_generation_count_ ;
  si.absolute_generation_rank = de->disposed_generation_count_ +
                       de->no_writers_generation_count_ ;
}


ACE_INLINE
DDS::InstanceStateKind
TAO::DCPS::InstanceState::instance_state() const
{
  return this->instance_state_ ;
}

ACE_INLINE
DDS::ViewStateKind
TAO::DCPS::InstanceState::view_state() const
{
  return this->view_state_ ;
}

ACE_INLINE
size_t TAO::DCPS::InstanceState::disposed_generation_count() const
{
  return disposed_generation_count_ ;
}

ACE_INLINE
size_t TAO::DCPS::InstanceState::no_writers_generation_count() const
{
  return no_writers_generation_count_ ;
}


ACE_INLINE
void
TAO::DCPS::InstanceState::data_was_received()
{
  //
  // Update the view state here, since only sample data received affects
  // this state value.  Then manage the data sample only transistions
  // here.  Let the lively() method manage the other transitions.
  //

  switch( this->view_state_)
    {
      case DDS::NEW_VIEW_STATE: break ; // No action.

      case DDS::NOT_NEW_VIEW_STATE:
        if( this->instance_state_ & DDS::NOT_ALIVE_INSTANCE_STATE)
          {
            this->view_state_ = DDS::NEW_VIEW_STATE ;
          }
        break ;

      default:
        this->view_state_ = DDS::NEW_VIEW_STATE ;
        break ;
    }

  switch( this->instance_state_)
    {
      case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
        this->disposed_generation_count_++ ;
        break ;

      case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
        this->no_writers_generation_count_++ ;
        break ;

      default: break ;
    }
    this->instance_state_ = DDS::ALIVE_INSTANCE_STATE ;

    this->no_writers_ = false;
}

ACE_INLINE
void
TAO::DCPS::InstanceState::lively(PublicationId         writer_id)
{
  ACE_UNUSED_ARG(writer_id);
  //
  // Manage transisitions in the instance state that do not require a
  // data sample, but merely the notion of liveliness.
  //
  if( this->instance_state_ == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
    {
      this->no_writers_generation_count_++ ;
      this->instance_state_ = DDS::ALIVE_INSTANCE_STATE ;
      this->no_writers_ = false;
    }
}

ACE_INLINE
void
TAO::DCPS::InstanceState::empty( bool value)
{
  //
  // Manage the instance state due to the DataReader becoming empty
  // here.
  //
  this->empty_ = value ;

  if( this->empty_
      && (     (this->instance_state_ & DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
           || ((this->instance_state_ & DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
                && this->no_writers_
              )
         )
    )
    {
      // Terminate state in figure 2-11
      this->instance_state_ = 0 ;
      this->disposed_generation_count_   = 0 ;
      this->no_writers_generation_count_ = 0 ;
    }
}

