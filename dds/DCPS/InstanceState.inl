/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReceivedDataElementList.h"
#include "ace/OS_NS_sys_time.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE
OpenDDS::DCPS::DataReaderImpl*
OpenDDS::DCPS::InstanceState::data_reader() const
{
  return reader_;
}

ACE_INLINE
void
OpenDDS::DCPS::InstanceState::accessed()
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard, this->lock_);
  //
  // Manage the view state due to data access here.
  //
  if (this->view_state_ & DDS::ANY_VIEW_STATE) {
    this->view_state_ = DDS::NOT_NEW_VIEW_STATE ;
  }
}

ACE_INLINE
bool
OpenDDS::DCPS::InstanceState::most_recent_generation(ReceivedDataElement* item) const
{
  return item->disposed_generation_count_ == disposed_generation_count_
         && item->no_writers_generation_count_ == no_writers_generation_count_;
}

ACE_INLINE
DDS::InstanceStateKind
OpenDDS::DCPS::InstanceState::instance_state() const
{
  return this->instance_state_ ;
}

ACE_INLINE
DDS::ViewStateKind
OpenDDS::DCPS::InstanceState::view_state() const
{
  return this->view_state_ ;
}

ACE_INLINE
size_t OpenDDS::DCPS::InstanceState::disposed_generation_count() const
{
  return disposed_generation_count_ ;
}

ACE_INLINE
size_t OpenDDS::DCPS::InstanceState::no_writers_generation_count() const
{
  return no_writers_generation_count_ ;
}

ACE_INLINE
void
OpenDDS::DCPS::InstanceState::data_was_received(const PublicationId& writer_id)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard, this->lock_);
  cancel_release();

  //
  // Update the view state here, since only sample data received affects
  // this state value.  Then manage the data sample only transistions
  // here.  Let the lively() method manage the other transitions.
  //
  writers_.insert(writer_id);

  switch (this->view_state_) {
  case DDS::NEW_VIEW_STATE:
    break ; // No action.

  case DDS::NOT_NEW_VIEW_STATE:

    if (this->instance_state_ & DDS::NOT_ALIVE_INSTANCE_STATE) {
      this->view_state_ = DDS::NEW_VIEW_STATE ;
    }

    break ;

  default:
    this->view_state_ = DDS::NEW_VIEW_STATE ;
    break ;
  }

  switch (this->instance_state_) {
  case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    ++this->disposed_generation_count_;
    break ;

  case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
    ++this->no_writers_generation_count_;
    break ;

  default:
    break ;
  }

  this->instance_state_ = DDS::ALIVE_INSTANCE_STATE ;
}

ACE_INLINE
void
OpenDDS::DCPS::InstanceState::lively(const PublicationId& writer_id)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard, this->lock_);

  //
  // Manage transisitions in the instance state that do not require a
  // data sample, but merely the notion of liveliness.
  //
  writers_.insert(writer_id);

  if (this->instance_state_ == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
    cancel_release(); // cancel unregister

    this->no_writers_generation_count_++ ;
    this->instance_state_ = DDS::ALIVE_INSTANCE_STATE ;
  }
}

ACE_INLINE
bool
OpenDDS::DCPS::InstanceState::empty(bool value)
{
  //
  // Manage the instance state due to the DataReader becoming empty
  // here.
  //
  if ((this->empty_ = value) && this->release_pending_) {
    return release_if_empty();
  } else {
    return false;
  }
}


ACE_INLINE
bool
OpenDDS::DCPS::InstanceState::is_last (const PublicationId& pub)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard, this->lock_, false);

  return (this->writers_.size () == 1) && *(this->writers_.begin ()) == pub;
}

ACE_INLINE
bool
OpenDDS::DCPS::InstanceState::no_writer () const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard, this->lock_, true);

  return this->writers_.size () == 0;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
