// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransientDataDurabilityCache.h"
#include "Service_Participant.h"
#include "DataSampleList.h"
#include "WriteDataContainer.h"
#include "DataWriterImpl.h"
#include "Qos_Helper.h"
#include "debug.h"

#include "tao/ORB_Core.h"

#include "ace/Reactor.h"
#include "ace/Message_Block.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_sys_time.h"

#include <algorithm>

// --------------------------------------------------
namespace
{
  class Cleanup_Handler : public ACE_Event_Handler
  {
  public:

    typedef
    ::OpenDDS::DCPS::TransientDataDurabilityCache::sample_list_type list_type;

    Cleanup_Handler (list_type & sample_list,
                     list_type::iterator begin,
                     list_type::iterator end)
      : sample_list_ (sample_list)
      , begin_ (begin)
      , end_ (end)
      , tid_ (-1)
      , timer_ids_ (0)
    {
      this->reference_counting_policy ().value (
        ACE_Event_Handler::Reference_Counting_Policy::ENABLED);
    }

    virtual int handle_timeout (ACE_Time_Value const & /* current_time */,
                                void const * /* act */)
    {
      if (OpenDDS::DCPS::DCPS_debug_level >= 4)
      {
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT ("(%P|%t) OpenDDS - Cleaning up ")
                    ACE_TEXT ("data durability cache.\n")));
      }

      // Cleanup all data samples corresponding to the cleanup delay.
      this->sample_list_.erase (this->begin_, this->end_);

      // No longer any need to keep track of the timer ID.
      this->timer_ids_->remove (this->tid_);

      return 0;
    }

    void timer_id (
      long tid,
      OpenDDS::DCPS::TransientDataDurabilityCache::timer_id_list_type * timer_ids)
    {
      this->tid_ = tid;
      this->timer_ids_ = timer_ids;
    }

  protected:

    virtual ~Cleanup_Handler() {}

  private:

    /// List containing samples to be cleaned up when the cleanup timer
    /// expires.
    list_type & sample_list_;

    /// Range of iterators pointing to samples to be cleaned up when
    /// the corresponding cleanup timer expires.
    //@{
    list_type::iterator const begin_;
    list_type::iterator const end_;
    //@}

    /// Timer ID corresponding to this cleanup event handler.
    long tid_;

    /// List of timer IDs.
    /**
     * If the cleanup timer fires successfully, the timer ID must be
     * removed from the timer ID list so that a subsequent attempt to
     * cancel the timer during durability cache destruction does not
     * occur.
     */
    OpenDDS::DCPS::TransientDataDurabilityCache::timer_id_list_type *
    timer_ids_;

  };
}

// --------------------------------------------------

OpenDDS::DCPS
::TransientDataDurabilityCache::sample_data_type::sample_data_type ()
  : sample (0)
{
}

OpenDDS::DCPS
::TransientDataDurabilityCache::sample_data_type::sample_data_type (
  DataSample * s)
  : sample (ACE_Message_Block::duplicate (s))
{
}


OpenDDS::DCPS
::TransientDataDurabilityCache::sample_data_type::sample_data_type (
  sample_data_type const & rhs)
  : sample (ACE_Message_Block::duplicate (rhs.sample))
{
}

OpenDDS::DCPS
::TransientDataDurabilityCache::sample_data_type::~sample_data_type ()
{
  ACE_Message_Block::release (this->sample);
}


OpenDDS::DCPS::TransientDataDurabilityCache::sample_data_type &
OpenDDS::DCPS::TransientDataDurabilityCache::sample_data_type::operator= (
  sample_data_type const & rhs)
{
  // Strongly exception-safe copy assignment.
  sample_data_type tmp (rhs);
  std::swap (this->sample, tmp.sample);
  return *this;
}

// --------------------------------------------------

OpenDDS::DCPS::TransientDataDurabilityCache::TransientDataDurabilityCache ()
  : samples_ ()
  , cleanup_timer_ids_ ()
  , lock_ ()
  , reactor_ (0)
{
  CORBA::ORB_var orb = TheServiceParticipant->get_ORB ();
  this->reactor_ = orb->orb_core ()->reactor ();
}

OpenDDS::DCPS::TransientDataDurabilityCache::~TransientDataDurabilityCache ()
{
  // Cancel timers that haven't expired yet.
  timer_id_list_type::const_iterator const end (
    this->cleanup_timer_ids_.end ());
  for (timer_id_list_type::const_iterator i (
         this->cleanup_timer_ids_.begin ());
       i != end;
       ++i)
  {
    (void) this->reactor_->cancel_timer (*i);
  }
}

bool
OpenDDS::DCPS::TransientDataDurabilityCache::insert (
  char const * topic_name,
  char const * type_name,
  DataSampleList & unsent_data,
  ::DDS::DurabilityServiceQosPolicy const & qos)
{
  if (unsent_data.size_ == 0)
    return true;  // Nothing to cache.

  // Apply DURABILITY_SERVICE QoS HISTORY and RESOURCE_LIMITS related
  // settings prior to data insertion into the cache.
  CORBA::Long const depth =
    get_instance_sample_list_depth (
      qos.history_kind,
      qos.history_depth,
      qos.max_samples_per_instance);

  // Iterator to first DataSampleListElement to be copied.
  DataSampleList::iterator element (unsent_data.begin ());

  if (depth < 0)
    return false; // Should never occur.
  else if (depth == 0)
    return true;  // Nothing else to do.  Discard all data.
  else if (unsent_data.size_ > depth)
  { 
    // Drop "old" samples.  Only keep the "depth" most recent
    // samples, i.e. those found at the tail end of the
    // DataSampleList.
    ssize_t const advance_amount = unsent_data.size_ - depth;
    std::advance (element, advance_amount);      
  }

  // -----------

  // Copy unsent samples to the domain/topic/type-specific cache.

  key_type const key (topic_name, type_name);
  DataSampleList::iterator unsent_end (unsent_data.end ());

  // Keep track of where the data is inserted in the list so that we
  // can easily remove that data later on during cleanup.
  sample_list_type::iterator list_start;
  sample_list_type::iterator list_end;
  sample_list_type * sample_list = 0;
  {
    ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, guard, this->lock_, false);

    sample_list_type & samples = this->samples_[key];
    sample_list = &samples;

    list_start = samples.end ();

    for (DataSampleList::iterator i (element); i != unsent_end; ++i)
      samples.push_back (sample_data_type ((*i).sample_));

    list_end = samples.end ();
  }

  // -----------

  // Schedule cleanup timer.
  Cleanup_Handler * const cleanup =
    new Cleanup_Handler (*sample_list,
                         list_start,
                         list_end);
  ACE_Event_Handler_var safe_cleanup (cleanup);  // Transfer ownership

  ACE_Time_Value const cleanup_delay (
    duration_to_time_value (qos.service_cleanup_delay));

  if (cleanup_delay > ACE_Time_Value::zero)
  {
    long const tid =
      this->reactor_->schedule_timer (cleanup,
                                      0, // ACT
                                      cleanup_delay);

    if (tid == -1)
    {
      ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, guard, this->lock_, false);

      sample_list->erase (list_start, list_end);

      return false;
    }
    else
    {
      {
        ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, guard, this->lock_, false);
        this->cleanup_timer_ids_.push_back (tid);
      }

      cleanup->timer_id (tid,
                         &this->cleanup_timer_ids_);
    }
  }

  return true;
}

bool
OpenDDS::DCPS::TransientDataDurabilityCache::send_data (
  char const * topic_name,
  char const * type_name,
  WriteDataContainer * data_container,
  DataWriterImpl * data_writer,
  ::DDS::LifespanQosPolicy const & /* lifespan */)
{
//   static ::DDS::Duration_t const INFINITE_EXPIRATION =
//     {
//       ::DDS::DURATION_INFINITY_SEC,
//       ::DDS::DURATION_INFINITY_NSEC
//     };

//   ::DDS::Duration_t const duration = lifespan.duration;

  key_type const key (topic_name, type_name);

  ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, guard, this->lock_, false);

  sample_list_type & sample_list = this->samples_[key];

  ::DDS::InstanceHandle_t handle;
  DataSample * initial_sample = sample_list.front ().sample;
  ::DDS::ReturnCode_t ret =
      data_container->register_instance (handle,
                                         initial_sample);
  
  if (ret != ::DDS::RETCODE_OK)
    return ret;

  // Transfer ownership to the data_container.
  (void) ACE_Message_Block::duplicate (initial_sample);

  typedef sample_list_type::iterator iterator;

  // Send all samples.
  iterator const the_end (sample_list.end ());
  for (iterator i (sample_list.begin ()); i != the_end; ++i)
  {
//     ::DDS::Time_t const & source_timestamp = (*i).source_timestamp;

//     // Determine if the cached sample has exceeded its lifespan.
//     ::DDS::Duration_t const expiration =
//         {
//           // Not foolproof on the boundaries but better than nothing.
//           //
//           // @todo Do we really need to check for an infinite source
//           //       timestamp?  Is that a legal timestamp?
//           ((duration.sec == ::DDS::DURATION_INFINITY_SEC
//             || source_timestamp.sec == ::DDS::DURATION_INFINITY_SEC)
//            ? ::DDS::DURATION_INFINITY_SEC
//            : duration.sec + source_timestamp.sec),

//           ((duration.nanosec == ::DDS::DURATION_INFINITY_NSEC
//             || source_timestamp.nanosec == ::DDS::DURATION_INFINITY_NSEC)
//            ? ::DDS::DURATION_INFINITY_NSEC
//            : duration.nanosec + source_timestamp.nanosec)
//         };

//     if (!(expiration == INFINITE_EXPIRATION)
//         && ::OpenDDS::DCPS::duration_to_time_value (expiration) >=
//         ACE_OS::gettimeofday ())
//     {
//       sample_list.erase (i);
//       continue;
//     }

    // Data is valid.  Append it to the write data container's resend queue.
    DataSampleListElement* element = 0;
    ret = data_container->obtain_buffer (element,
                                         handle,
                                         data_writer);

    if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT ("OpenDDS (%P|%t) ERROR: ")
                         ACE_TEXT ("TransientDataDurabilityCache::")
                         ACE_TEXT ("send_durable_data, ")
                         ACE_TEXT ("obtain_buffer returned %d.\n"),
                         ret),
                        false);
    }

    element->sample_ = ACE_Message_Block::duplicate ((*i).sample);

    if (ret != ::DDS::RETCODE_OK)
    {
      return false;
    }

    // This approach may cause data that was sent to be sent again.

    ret = data_container->enqueue (element, handle);

    if (ret != ::DDS::RETCODE_OK)
    {
      return false;
    }

    // Remove the sample from the TRANSIENT durability queue.  We no
    // longer need it.
    sample_list.erase (i);
  }

  // All done.
  data_container->unregister (handle,
                              initial_sample,
                              data_writer,
                              false);  // No dup

  return true;
}

// OpenDDS::DCPS::TransientDataDurabilityCache::get_sample_list (
//   char const * topic_name,
//   char const * type_name)
// {
// }
