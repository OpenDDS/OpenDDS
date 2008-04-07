// $Id$

#include "TransientDataDurabilityCache.h"
#include "DataSampleList.h"
#include "WriteDataContainer.h"
#include "DataWriterImpl.h"
#include "Qos_Helper.h"

#include "ace/Message_Block.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_sys_time.h"

#include <algorithm>

// --------------------------------------------------

OpenDDS::DCPS
::TransientDataDurabilityCache::sample_data_type::sample_data_type ()
  : sample (0)
  , source_timestamp ()
{
}

OpenDDS::DCPS
::TransientDataDurabilityCache::sample_data_type::sample_data_type (
  DataSample * s,
  ::DDS::Time_t t)
  : sample (ACE_Message_Block::duplicate (s))
  , source_timestamp (t)
{
}


OpenDDS::DCPS
::TransientDataDurabilityCache::sample_data_type::sample_data_type (
  sample_data_type const & rhs)
  : sample (ACE_Message_Block::duplicate (rhs.sample))
  , source_timestamp (rhs.source_timestamp)
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
  this->swap (tmp);
  return *this;
}

void
OpenDDS::DCPS::TransientDataDurabilityCache::sample_data_type::swap (
  sample_data_type & rhs)
{
  std::swap (this->sample, rhs.sample);
  std::swap (this->source_timestamp, rhs.source_timestamp);
}

// --------------------------------------------------

OpenDDS::DCPS::TransientDataDurabilityCache::TransientDataDurabilityCache ()
  : samples_ ()
  , characteristics_ ()
{
}

OpenDDS::DCPS::TransientDataDurabilityCache::~TransientDataDurabilityCache ()
{
}

bool
OpenDDS::DCPS::TransientDataDurabilityCache::enqueue (
  char const * topic_name,
  char const * type_name,
  DataSample * sample,
  ::DDS::Time_t const & source_timestamp)
{
  key_type const key (topic_name, type_name);

  ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, guard, this->lock_, false);

  sample_list_type & sample_list = this->samples_[key];

  typedef characteristic_map_type::iterator iterator;

  iterator const c (this->characteristics_.find (key));

  if (c == this->characteristics_.end ())
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("OpenDDS (%P|%t) TRANSIENT durability ")
                       ACE_TEXT ("cache not initialized correctly for ")
                       ACE_TEXT ("topic \"%s\" and type \"%s\"\n"),
                       topic_name,
                       type_name),
                      false);
  }

  // The size() accessor could be an O(n) operation in some STL
  // implementations.  We can explicitly keep track of the count in
  // the characteristics map if it proves to be slow in some cases.
  if (sample_list.size ()
      == static_cast<characteristic_map_type::size_type> ((*c).second.depth))
  {
    sample_list.pop_front ();
  }

  sample_list.push_back (sample_data_type (sample, source_timestamp));

  return true;
}

bool
OpenDDS::DCPS::TransientDataDurabilityCache::set_instance_characteristics (
  char const * topic_name,
  char const * type_name,
  ::DDS::Duration_t const & service_cleanup_delay,
  CORBA::Long depth)
{
  
  if (depth <= 0)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("OpenDDS (%P|%t) ERROR: Invalid depth for ")
                       ACE_TEXT("TRANSIENT data cache characteristics for ")
                       ACE_TEXT("topic \"%s\" / type \"%s\"\n")),
                      false);
  }

  typedef characteristic_map_type::iterator iterator;

  std::pair<iterator, bool> result;
  {
    ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, guard, this->lock_, false);

    result =
      this->characteristics_.insert (
        std::make_pair (key_type (topic_name, type_name),
                        characteristic_data_type (service_cleanup_delay,
                                                  depth)));
  }

  if (result.second)
  {
    // @todo For now we ignore the service_cleanup_delay
    //       characteristic.
  }
  else
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("OpenDDS (%P|%t) ERROR: Attempt to reset ")
                ACE_TEXT("TRANSIENT data cache characteristics for ")
                ACE_TEXT("topic \"%s\" / type \"%s\"\n")));
  }

  return result.second;
}

bool
OpenDDS::DCPS::TransientDataDurabilityCache::send_durable_data (
  char const * topic_name,
  char const * type_name,
  WriteDataContainer * data_container,
  DataWriterImpl * data_writer,
  ::DDS::LifespanQosPolicy const & lifespan)
{
  static ::DDS::Duration_t const INFINITE_EXPIRATION =
    {
      ::DDS::DURATION_INFINITY_SEC,
      ::DDS::DURATION_INFINITY_NSEC
    };

  ::DDS::Duration_t const duration = lifespan.duration;

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
    ::DDS::Time_t const & source_timestamp = (*i).source_timestamp;

    // Determine if the cached sample has exceeded its lifespan.
    ::DDS::Duration_t const expiration =
        {
          // Not foolproof on the boundaries but better than nothing.
          //
          // @todo Do we really need to check for an infinite source
          //       timestamp?  Is that a legal timestamp?
          ((duration.sec == ::DDS::DURATION_INFINITY_SEC
            || source_timestamp.sec == ::DDS::DURATION_INFINITY_SEC)
           ? ::DDS::DURATION_INFINITY_SEC
           : duration.sec + source_timestamp.sec),

          ((duration.nanosec == ::DDS::DURATION_INFINITY_NSEC
            || source_timestamp.nanosec == ::DDS::DURATION_INFINITY_NSEC)
           ? ::DDS::DURATION_INFINITY_NSEC
           : duration.nanosec + source_timestamp.nanosec)
        };

    if (!(expiration == INFINITE_EXPIRATION)
        && ::OpenDDS::DCPS::duration_to_time_value (expiration) >=
        ACE_OS::gettimeofday ())
    {
      sample_list.erase (i);
      continue;
    }

    // Data is valid.  Append it to the write data container's resend queue.
    DataSampleListElement* element = 0;
    ret = data_container->obtain_buffer (element,
                                         handle,
                                         data_writer);

    if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("OpenDDS (%P|%t) ERROR: ")
                         ACE_TEXT("TransientDataDurabilityCache::")
                         ACE_TEXT("send_durable_data, ")
                         ACE_TEXT("obtain_buffer returned %d.\n"),
                         ret),
                        false);
    }

    DataSample * const data = (*i).sample;
    ret = data_writer->create_sample_data_message (data,
                                                   handle,
                                                   element->sample_,
                                                   source_timestamp);

    if (ret != ::DDS::RETCODE_OK)
    {
      return false;
    }

    // This approach may cause data that was sent to be sent again.

    ret = data_container->enqueue(element, handle);

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
