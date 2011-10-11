/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "WriteDataContainer.h"
#include "Service_Participant.h"
#include "DataSampleHeader.h"
#include "DataSampleList.h"
#include "DataWriterImpl.h"
#include "DataDurabilityCache.h"
#include "PublicationInstance.h"
#include "Util.h"
#include "Qos_Helper.h"
#include "RepoIdConverter.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"
#include "dds/DCPS/transport/framework/TransportDebug.h"

#include "tao/debug.h"

#include <sstream>

namespace OpenDDS {
namespace DCPS {

/**
 * @todo Refactor this code and DataReaderImpl::data_expired() to
 *       a common function.
 */
bool
resend_data_expired(DataSampleListElement const & element,
                    DDS::LifespanQosPolicy const & lifespan)
{
  if (lifespan.duration.sec != DDS::DURATION_INFINITE_SEC
      || lifespan.duration.nanosec != DDS::DURATION_INFINITE_NSEC) {
    // Finite lifespan.  Check if data has expired.

    DDS::Time_t const tmp = {
      element.header_.source_timestamp_sec_ + lifespan.duration.sec,
      element.header_.source_timestamp_nanosec_ + lifespan.duration.nanosec
    };

    ACE_Time_Value const now(ACE_OS::gettimeofday());
    ACE_Time_Value const expiration_time(time_to_time_value(tmp));

    if (now >= expiration_time) {
      if (DCPS_debug_level >= 8) {
        ACE_Time_Value const diff(now - expiration_time);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("OpenDDS (%P|%t) Data to be sent ")
                   ACE_TEXT("expired by %d seconds, %d microseconds.\n"),
                   diff.sec(),
                   diff.usec()));
      }

      return true;  // Data expired.
    }
  }

  return false;
}

WriteDataContainer::WriteDataContainer(
  DataWriterImpl* writer,
  CORBA::Long    depth,
  bool           should_block ,
  ::DDS::Duration_t max_blocking_time,
  size_t         n_chunks,
  DDS::DomainId_t domain_id,
  char const * topic_name,
  char const * type_name,
  DataDurabilityCache* durability_cache,
  DDS::DurabilityServiceQosPolicy const & durability_service,
  std::auto_ptr<OfferedDeadlineWatchdog>& watchdog)
  : writer_(writer),
    depth_(depth),
    should_block_(should_block),
    max_blocking_time_(max_blocking_time),
    waiting_on_release_(false),
    condition_(lock_),
    empty_condition_(lock_),
    n_chunks_(n_chunks),
    sample_list_element_allocator_(2 * n_chunks_),
    transport_send_element_allocator_(2 * n_chunks_,
                                      sizeof(TransportSendElement)),
    transport_customized_element_allocator_(2 * n_chunks_,
                                            sizeof(TransportCustomizedElement)),
    shutdown_(false),
    domain_id_(domain_id),
    topic_name_(topic_name),
    type_name_(type_name),
    durability_cache_(durability_cache),
    durability_service_(durability_service),
    watchdog_(watchdog)
{

  if (DCPS_debug_level >= 2) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) WriteDataContainer "
               "sample_list_element_allocator %x with %d chunks\n",
               &sample_list_element_allocator_, n_chunks_));
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) WriteDataContainer "
               "transport_send_element_allocator %x with %d chunks\n",
               &transport_send_element_allocator_, n_chunks_));
  }
}

WriteDataContainer::~WriteDataContainer()
{
  if (this->unsent_data_.size_ > 0) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: WriteDataContainer::~WriteDataContainer() - ")
               ACE_TEXT("destroyed with %d samples unsent.\n"),
               this->unsent_data_.size_));
  }

  if (this->sending_data_.size_ > 0) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: WriteDataContainer::~WriteDataContainer() - ")
               ACE_TEXT("destroyed with %d samples sending.\n"),
               this->sending_data_.size_));
  }

  if (this->sent_data_.size_ > 0) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::~WriteDataContainer() - ")
                 ACE_TEXT("destroyed with %d samples sent.\n"),
                 this->sent_data_.size_));
    }
  }

  if (!shutdown_) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("WriteDataContainer::~WriteDataContainer, ")
               ACE_TEXT("The container has not been cleaned.\n")));
  }
}

// This method preassumes that instance list has space for this sample.
DDS::ReturnCode_t
WriteDataContainer::enqueue(
  DataSampleListElement* sample,
  DDS::InstanceHandle_t instance_handle)
{
  // Get the PublicationInstance pointer from InstanceHandle_t.
  PublicationInstance* const instance =
    get_handle_instance(instance_handle);
  // Extract the instance queue.
  DataSampleList& instance_list = instance->samples_;

  if (this->watchdog_.get()) {
    instance->last_sample_tv_ = instance->cur_sample_tv_;
    instance->cur_sample_tv_ = ACE_OS::gettimeofday();
    this->watchdog_->execute((void const *)instance, false);
  }

  //
  // Enqueue to the next_send_sample_ thread of unsent_data_
  // will link samples with the next_sample/previous_sample and
  // also next_send_sample_.
  // This would save time when we actually send the data.

  unsent_data_.enqueue_tail_next_send_sample(sample);

  //
  // Add this sample to the INSTANCE scope list.
  instance_list.enqueue_tail_next_instance_sample(sample);

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::reenqueue_all(ReaderIdSeq const & rds,
                                  DDS::LifespanQosPolicy const & lifespan)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->lock_,
                   DDS::RETCODE_ERROR);

  // Make a copy of sending_data_ and sent_data_;

  if (sending_data_.size_ > 0) {
    this->copy_and_append(this->resend_data_,
                          sending_data_,
                          rds,
                          lifespan);
  }

  if (sent_data_.size_ > 0) {
    this->copy_and_append(this->resend_data_,
                          sent_data_,
                          rds,
                          lifespan);

    if (DCPS_debug_level > 9) {
      RepoIdConverter converter(publication_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::reenqueue_all: ")
                 ACE_TEXT("domain %d topic %C publication %C copying HISTORY to resend.\n"),
                 this->domain_id_,
                 this->topic_name_,
                 std::string(converter).c_str()));
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::register_instance(
  DDS::InstanceHandle_t&      instance_handle,
  DataSample*&                  registered_sample)
{
  PublicationInstance* instance = 0;
  auto_ptr<PublicationInstance> safe_instance;

  if (instance_handle == DDS::HANDLE_NIL) {
    // registered the instance for the first time.
    ACE_NEW_RETURN(instance,
                   PublicationInstance(registered_sample),
                   DDS::RETCODE_ERROR);

    ACE_auto_ptr_reset(safe_instance, instance);

    instance_handle = this->writer_->get_next_handle();

    int const insert_attempt = OpenDDS::DCPS::bind(instances_, instance_handle, instance);

    if (0 != insert_attempt) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::register_instance, ")
                 ACE_TEXT("failed to insert instance handle=%X\n"),
                 instance));
      return DDS::RETCODE_ERROR;
    } // if (0 != insert_attempt)

    instance->instance_handle_ = instance_handle;

  } else {
    int const find_attempt = find(instances_, instance_handle, instance);
    ACE_auto_ptr_reset(safe_instance, instance);

    if (0 != find_attempt) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::register_instance, ")
                 ACE_TEXT("The provided instance handle=%X is not a valid")
                 ACE_TEXT("handle.\n"),
                 instance_handle));

      return DDS::RETCODE_ERROR;
    } // if (0 != insert_attempt)

    // don't need this - the PublicationInstances already has a sample.
    registered_sample->release();

    instance->unregistered_ = false;
  }

  // The registered_sample is shallow copied.
  registered_sample = instance->registered_sample_->duplicate();

  if (this->watchdog_.get()) {
    this->watchdog_->schedule_timer(instance);
  }

  safe_instance.release();  // Safe to relinquish ownership.

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::unregister(
  DDS::InstanceHandle_t instance_handle,
  DataSample*&            registered_sample,
  bool                    dup_registered_sample)
{
  PublicationInstance* instance = 0;

  int const find_attempt = find(instances_, instance_handle, instance);

  if (0 != find_attempt) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("WriteDataContainer::unregister, ")
                      ACE_TEXT("The instance(handle=%X) ")
                      ACE_TEXT("is not registered yet.\n"),
                      instance_handle),
                     DDS::RETCODE_PRECONDITION_NOT_MET);
  } // if (0 != insert_attempt)

  instance->unregistered_ = true;

  if (dup_registered_sample) {
    // The registered_sample is shallow copied.
    registered_sample = instance->registered_sample_->duplicate();
  }

  // Unregister the instance with typed DataWriter.
  this->writer_->unregistered(instance_handle);

  if (this->watchdog_.get())
    this->watchdog_->cancel_timer(instance);

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::dispose(DDS::InstanceHandle_t instance_handle,
                            DataSample*&            registered_sample,
                            bool                    dup_registered_sample)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->lock_,
                   DDS::RETCODE_ERROR);

  PublicationInstance* instance = 0;

  int const find_attempt = find(instances_, instance_handle, instance);

  if (0 != find_attempt) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("WriteDataContainer::dispose, ")
                      ACE_TEXT("The instance(handle=%X) ")
                      ACE_TEXT("is not registered yet.\n"),
                      instance_handle),
                     DDS::RETCODE_PRECONDITION_NOT_MET);
  }

  if (dup_registered_sample) {
    // The registered_sample is shallow copied.
    registered_sample = instance->registered_sample_->duplicate();
  }

  // Note: The DDS specification is unclear as to if samples in the process
  // of being sent should be removed or not.
  // The advantage of calling remove_sample() on them is that the
  // cached allocator memory for them is freed.  The disadvantage
  // is that the slow reader may see multiple disposes without
  // any write sample between them and hence not temporarily move into the
  // Alive state.
  // We have choosen to NOT remove the sending samples.

  DataSampleList& instance_list = instance->samples_;

  while (instance_list.size_ > 0) {
    bool released = false;
    DDS::ReturnCode_t ret
    = remove_oldest_sample(instance_list, released);

    if (ret != DDS::RETCODE_OK) {
      return ret;
    }
  }

  if (this->watchdog_.get())
    this->watchdog_->cancel_timer(instance);

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::num_samples(DDS::InstanceHandle_t handle,
                                size_t&                 size)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->lock_,
                   DDS::RETCODE_ERROR);
  PublicationInstance* instance = 0;

  int const find_attempt = find(instances_, handle, instance);

  if (0 != find_attempt) {
    return DDS::RETCODE_ERROR;

  } else {
    size = instance->samples_.size_;
    return DDS::RETCODE_OK;
  }
}

DataSampleList
WriteDataContainer::get_unsent_data()
{
  DBG_ENTRY_LVL("WriteDataContainer","get_unsent_data",6);

  //
  // The samples in unsent_data are added to the sending_data
  // during enqueue.
  //
  DataSampleList list = this->unsent_data_;

  //
  // The unsent_data_ already linked with the
  // next_send_sample during enqueue.
  // Append the unsent_data_ to current sending_data_
  // list.
  sending_data_.enqueue_tail_next_send_sample(list);

  //
  // Clear the unsent data list.
  //
  this->unsent_data_.reset();

  // Signal if there is no pending data.
  //
  // N.B. If a mutex cannot be obtained it is possible for this
  //      method to return successfully without broadcasting the
  //      condition.
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->lock_,
                   list);

  if (!pending_data())
    empty_condition_.broadcast();

  //
  // Return the moved list.
  //
  return list;
}

DataSampleList
WriteDataContainer::get_resend_data()
{
  DBG_ENTRY_LVL("WriteDataContainer","get_resend_data",6);

  //
  // The samples in unsent_data are added to the sending_data
  // during enqueue.
  //
  DataSampleList list = this->resend_data_;

  //
  // The unsent_data_ already linked with the
  // next_send_sample during enqueue.
  // Append the unsent_data_ to current sending_data_
  // list.
  released_data_.enqueue_tail_next_send_sample(list);

  //
  // Clear the unsent data list.
  //
  this->resend_data_.reset();
  //
  // Return the moved list.
  //
  return list;
}

DataSampleList
WriteDataContainer::get_sending_data()
{
  return this->sending_data_;
}

DataSampleList
WriteDataContainer::get_sent_data()
{
  return this->sent_data_;
}

bool
WriteDataContainer::pending_data()
{
  return this->sending_data_.size_ != 0
         || this->unsent_data_.size_ != 0;
}

void
WriteDataContainer::data_delivered(const DataSampleListElement* sample)
{
  DBG_ENTRY_LVL("WriteDataContainer","data_delivered",6);

  if (DCPS_debug_level >= 2) {
    ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::data_delivered")
                          ACE_TEXT(" %X \n"), sample));
  }

  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard,
            this->lock_);

  // Delivered samples _must_ be on sending_data_ or released_data_
  // list. If it is not found in one of the lists, an invariant
  // exception is declared.

  // The element now needs to be removed from the sending_data_
  // list, and appended to the end of the sent_data_ list.

  // Search the released_data_ list last.  Data is moved to this list
  // when it is removed from the sample list, but it is still in used
  // by transport.  We are now been notified by transport, so we can
  // now release the element.
  //
  PublicationInstance* instance = sample->handle_;
  DataSampleListElement* stale =
    released_data_.dequeue_next_send_sample(sample);
  if (stale) {
    release_buffer(stale);
  } else {
    //
    // Search the sending_data_ list first.
    //
    if (sending_data_.dequeue_next_send_sample(sample)) {
      // in sending_data_ list
    } else {
      // The sample is neither in the sending_data_ nor the
      // released_data_.
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::data_delivered, ")
                 ACE_TEXT("The delivered sample is not in sending_data_ and ")
                 ACE_TEXT("released_data_ list.\n")));
      return;
    }

    if (instance->waiting_list_.head_ != 0) {
      // Remove the delivered sample from the instance sample list
      // and release.
      stale = instance->samples_.dequeue_next_instance_sample(sample);
      if (stale == 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("WriteDataContainer::data_delivered, ")
                   ACE_TEXT("dequeue_next_instance_sample from instance ")
                   ACE_TEXT("list failed\n")));
        return;
      }

      release_buffer(stale);
    } else {
      if (DCPS_debug_level > 9) {
        RepoIdConverter converter(publication_id_);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) WriteDataContainer::data_delivered: ")
                   ACE_TEXT("domain %d topic %C publication %C pushed to HISTORY.\n"),
                   this->domain_id_,
                   this->topic_name_,
                   std::string(converter).c_str()));
      }

      DataSampleHeader::set_flag(HISTORIC_SAMPLE_FLAG, sample->sample_);
      sent_data_.enqueue_tail_next_send_sample(sample);
    }
  }

  this->wakeup_blocking_writers (stale, instance);

  // Signal if there is no pending data.
  if (!pending_data())
    empty_condition_.broadcast();
}

void
WriteDataContainer::data_dropped(const DataSampleListElement* sample,
                                 bool dropped_by_transport)
{
  DBG_ENTRY_LVL("WriteDataContainer","data_dropped",6);

  if (DCPS_debug_level >= 2) {
    ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::data_dropped")
                          ACE_TEXT(" sample %X dropped_by_transport %d\n"),
                          sample, dropped_by_transport));
  }

  // If the transport initiates the data dropping, we need do same thing
  // as data_delivered. e.g. remove the sample from the internal list
  // and the instance list. We do not need aquire the lock here since
  // the data_delivered acquires the lock.
  if (dropped_by_transport) {
    this->data_delivered(sample);
    return;
  }

  //The data_dropped could be called from the thread initiating sample remove
  //which already hold the lock. In this case, it's not necessary to acquire
  //lock here. It also could be called from the transport thread in a delayed
  //notification, it's necessary to acquire lock here to protect the internal
  //structures in this class.

  ACE_GUARD (ACE_Recursive_Thread_Mutex,
    guard,
    this->lock_);

  // The dropped sample is either in released_data_ list or
  // sending_data_ list. Otherwise an exception will be raised.
  //
  // We are now been notified by transport, so we can
  // now release the sample from released_data_ list and
  // keep the sample from the sending_data_ list still in
  // sample list since we will send it.
  DataSampleListElement* stale = 0;

  if (sending_data_.dequeue_next_send_sample(sample)) {
    // else: The data_dropped is called as a result of remove_sample()
    // called from reenqueue_all() which supports the TRANSIENT_LOCAL
    // qos. The samples that are sending by transport are dropped from
    // transport and will be moved to the unsent list for resend.
    unsent_data_.enqueue_tail_next_send_sample(sample);

  } else {
    stale = released_data_.dequeue_next_send_sample(sample);
    if (stale) {
      // The remove_sample is requested when sample list size
      // reaches limit. In this case, the oldest sample is
      // moved to released_data_ already.
      release_buffer(stale);

    } else {
      // The sample is neither in not in the
      // released_data_ list.
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::data_dropped, ")
                 ACE_TEXT("The dropped sample is not in released_data_ ")
                 ACE_TEXT("list.\n")));
    }
  }

  this->wakeup_blocking_writers (stale, sample->handle_);

  if (!pending_data())
    empty_condition_.broadcast();
}

DDS::ReturnCode_t
WriteDataContainer::remove_oldest_sample(
  DataSampleList& instance_list,
  bool& released)
{
  DataSampleListElement* stale = 0;

  //
  // Remove the oldest sample from the instance list.
  //
  if (instance_list.dequeue_head_next_instance_sample(stale) == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("WriteDataContainer::remove_oldest_sample, ")
                      ACE_TEXT("dequeue_head_next_sample failed\n")),
                     DDS::RETCODE_ERROR);
  }

  //
  // Remove the stale data from the next_sample_ list.  The
  // sending_data_/next_send_sample_ list is not managed within the
  // container, it is only used external to the container and does
  // not need to be managed internally.
  //
  // The next_sample_ link is being used in one of the sent_data_,
  // sending_data_, or unsent_data lists.  Removal from the doubly
  // linked list needs to repair the list only when the stale sample
  // is either the head or tail of the list.
  //

  //
  // Locate the head of the list that the stale data is in.
  //
  DataSampleListElement* head = stale;

  while (head->previous_send_sample_ != 0) {
    head = head->previous_send_sample_;
  }

  //
  // Identify the list that the stale data is in.
  // The stale data should be in one of the sent_data_, sending_data_
  // or unsent_data_. It should not be in released_data_ list since
  // this function is the only place a sample is moved from
  // sending_data_ to released_data_ list.

  // Remove the element from the internal list.
  bool result = false;

  if (head == this->sending_data_.head_) {
    // Move the element to the released_data_ list since it is still
    // in use, and we need to wait until it is told by the transport.
    //
    result = this->sending_data_.dequeue_next_send_sample(stale) != 0;
    released_data_.enqueue_tail_next_send_sample(stale);
    released = false;

  } else if (head == this->sent_data_.head_) {
    // No one is using the data sample, so we can release it back to
    // its allocator.
    //
    result = this->sent_data_.dequeue_next_send_sample(stale) != 0;
    release_buffer(stale);
    released = true;

    if (DCPS_debug_level > 9) {
      RepoIdConverter converter(publication_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::remove_oldest_sample: ")
                 ACE_TEXT("domain %d topic %C publication %C sample removed from HISTORY.\n"),
                 this->domain_id_,
                 this->topic_name_,
                 std::string(converter).c_str()));
    }

  } else if (head == this->unsent_data_.head_) {
    //
    // No one is using the data sample, so we can release it back to
    // its allocator.
    //
    result = this->unsent_data_.dequeue_next_send_sample(stale) != 0;
    release_buffer(stale);
    released = true;

  } else {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("WriteDataContainer::remove_oldest_sample, ")
                      ACE_TEXT("The oldest sample is not in any internal list.\n")),
                     DDS::RETCODE_ERROR);
  }

  // Signal if there is no pending data.
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     guard,
                     this->lock_,
                     DDS::RETCODE_ERROR);

    if (!pending_data())
      empty_condition_.broadcast();
  }

  if (result == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("WriteDataContainer::remove_oldest_sample, ")
                      ACE_TEXT("dequeue_next_send_sample from internal list failed.\n")),
                     DDS::RETCODE_ERROR);

  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::obtain_buffer(DataSampleListElement*& element,
                                  DDS::InstanceHandle_t handle)
{
  PublicationInstance* instance = get_handle_instance(handle);

  ACE_NEW_MALLOC_RETURN(
    element,
    static_cast<DataSampleListElement*>(
      sample_list_element_allocator_.malloc(
        sizeof(DataSampleListElement))),
    DataSampleListElement(publication_id_,
                          this->writer_,
                          instance,
                          &transport_send_element_allocator_,
                          &transport_customized_element_allocator_),
    DDS::RETCODE_ERROR);

  // Extract the current instance queue.
  DataSampleList& instance_list = instance->samples_;
  DDS::ReturnCode_t ret = DDS::RETCODE_OK;

  bool oldest_released = true;
  DataSampleListElement* stale = instance_list.head_;

  // Release the oldest sample if the size reaches the max size of
  // the sample list.
  if (instance_list.size_ > depth_) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("WriteDataContainer::obtain_buffer, ")
               ACE_TEXT("The instance list size %d exceeds depth %d\n"),
               instance_list.size_,
               depth_));
    ret = DDS::RETCODE_ERROR;

  } else if (instance_list.size_ == depth_) {
    // The remove_oldest_sample() method removes the oldest sample
    // from instance list and removes it from the internal lists.
    ret = this->remove_oldest_sample(instance_list, oldest_released);
  }

  if (ret != DDS::RETCODE_OK) {
    this->release_buffer(element);
    return ret;
  }

  // Each write/enqueue just writes one element and hence the number
  // of samples will reach the size of sample list at some point.
  // We need remove enough to accomodate the new element.

  if (should_block_) {
    // Need wait when waiting list is not empty or the oldest sample
    // is still being used.
    bool const need_wait =
      instance->waiting_list_.head_ != 0
      || !oldest_released ? true : false;

    if (need_wait) {
      // Add the newly allocated sample to waiting list.
      instance->waiting_list_.enqueue_tail_next_instance_sample(element);

      // wait for all "released" samples to be delivered
      // Timeout value from Qos.RELIABILITY.max_blocking_time
      ACE_Time_Value abs_timeout = duration_to_absolute_time_value (max_blocking_time_);
      bool waited = false;
      while (!shutdown_ && ACE_OS::gettimeofday() < abs_timeout) {
        waited = true;
        waiting_on_release_ = true; // reduces broadcast to only when waiting

        if (DCPS_debug_level >= 2) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) WriteDataContainer::obtain_buffer, ")
                                ACE_TEXT ("wait for condition, oldest_released %d waiting %X\n"),
                                oldest_released, instance->waiting_list_.head_));
        }

        // lock is released while waiting and aquired before returning
        // from wait.
        int const wait_result = condition_.wait(&abs_timeout);

        if (wait_result == 0) { // signalled
          if (element->space_available_ == true) {
            break;
          } // else continue wait

        } else {
          if (errno == ETIME) {
            ret = DDS::RETCODE_TIMEOUT;

          } else {
            // Other errors from wait.
            ret = DDS::RETCODE_ERROR;
          }

          break;
        }
      }
      if (!waited) {
        // max-blocking_time_ was so short we did not even try to wait
        ret = DDS::RETCODE_TIMEOUT;
      }
      if (ret != DDS::RETCODE_OK) {
        // Remove from the waiting list if wait() timed out or return
        // other errors.
        if (instance->waiting_list_.dequeue_next_instance_sample(element) == false) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("WriteDataContainer::obtain_buffer, ")
                            ACE_TEXT("dequeue_next_instance_sample from ")
                            ACE_TEXT("waiting list failed\n")),
                           DDS::RETCODE_ERROR);
        }
      }
    } // wait_needed

  } else {
    if (!oldest_released) {
      // The oldest sample is still being used by the transport,
      // then we need force the Transport to drop the oldest sample.
      // The transport will call data_dropped to remove the oldest
      // sample from the released_data_ list.
      this->writer_->remove_sample(stale);
    }
  }

  // Cleanup
  if (ret != DDS::RETCODE_OK) {
    release_buffer(element);

  } else {
    data_holder_.enqueue_tail_next_sample(element);
  }

  return ret;
}

void
WriteDataContainer::release_buffer(DataSampleListElement* element)
{
  data_holder_.dequeue_next_sample(element);
  // Release the memeory to the allocator.
  ACE_DES_FREE(element,
               sample_list_element_allocator_.free,
               DataSampleListElement);
}

void
WriteDataContainer::unregister_all()
{
  DBG_ENTRY_LVL("WriteDataContainer","unregister_all",6);

  shutdown_ = true;

  {
    //The internal list needs protection since this call may result from the
    //the delete_datawriter call which does not acquire the lock in advance.

    ACE_GUARD(ACE_Recursive_Thread_Mutex,
              guard,
              this->lock_);

    // Tell transport remove all control messages currently
    // transport is processing.
    (void) this->writer_->remove_all_msgs();

    // Broadcast to wake up all waiting threads.
    if (waiting_on_release_) {
      condition_.broadcast();
    }
  }
  DDS::ReturnCode_t ret;
  DataSample* registered_sample;
  PublicationInstanceMapType::iterator it = instances_.begin();

  while (it != instances_.end()) {
    // Release the instance data.
    ret = dispose(it->first, registered_sample, false);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::unregister_all, ")
                 ACE_TEXT("dispose instance %X failed\n"),
                 it->first));
    }

    // Mark the instance unregistered.
    ret = unregister(it->first, registered_sample, false);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::unregister_all, ")
                 ACE_TEXT("unregister instance %X failed\n"),
                 it->first));
    }

    PublicationInstance* instance = it->second;

    delete instance;

    // Get the next iterator before erase the instance handle.
    PublicationInstanceMapType::iterator it_next = it;
    ++it_next;
    // Remove the instance from the instance list.
    unbind(instances_, it->first);
    it = it_next;
  }

  ACE_UNUSED_ARG(registered_sample);
}

PublicationInstance*
WriteDataContainer::get_handle_instance(DDS::InstanceHandle_t handle)
{
  PublicationInstance* instance = 0;

  if (0 != find(instances_, handle, instance)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("WriteDataContainer::get_handle_instance, ")
               ACE_TEXT("lookup for %d failed\n"),
               handle));
  }

  return instance;
}

void
WriteDataContainer::copy_and_append(DataSampleList& list,
                                    DataSampleList const & appended,
                                    ReaderIdSeq const & rds,
                                    DDS::LifespanQosPolicy const & lifespan)
{
  CORBA::ULong const num_rds = rds.length();
  CORBA::ULong const num_iters_per_sample =
    num_rds / MAX_READERS_PER_ELEM + 1;

  for (DataSampleListElement* cur = appended.head_;
       cur != 0;
       cur = cur->next_send_sample_) {
    // Do not copy and append data that has exceeded the configured
    // lifespan.
    if (resend_data_expired(*cur, lifespan))
      continue;

    CORBA::ULong num_rds_left = num_rds;

    for (CORBA::ULong i = 0; i < num_iters_per_sample; ++i) {
      DataSampleListElement* element = 0;
      ACE_NEW_MALLOC(element,
                     static_cast<DataSampleListElement*>(
                       sample_list_element_allocator_.malloc(
                         sizeof(DataSampleListElement))),
                     DataSampleListElement(*cur));

      // @todo Does ACE_NEW_MALLOC throw?  Where's the check for
      //       allocation failure, i.e. element == 0?

      element->num_subs_ =
        num_rds_left <= MAX_READERS_PER_ELEM
        ? num_rds_left
        : MAX_READERS_PER_ELEM;

      for (CORBA::ULong j = 0; j < element->num_subs_; ++j)
        element->subscription_ids_[j] = rds[j + i * MAX_READERS_PER_ELEM];

      list.enqueue_tail_next_send_sample(element);
      num_rds_left -= element->num_subs_;
    }
  }
}

bool
WriteDataContainer::persist_data()
{
  bool result = true;

  // ------------------------------------------------------------
  // Transfer sent data to data DURABILITY cache.
  // ------------------------------------------------------------
  if (this->durability_cache_) {
    // A data durability cache is available for TRANSIENT or
    // PERSISTENT data durability.  Cache the data samples.

    /**
     * @todo We should only cache data that is not in the
     *       "released_data_" list, i.e. not still in use outside of
     *       this instance of WriteDataContainer.
     */
    bool const inserted =
      this->durability_cache_->insert(this->domain_id_,
                                      this->topic_name_,
                                      this->type_name_,
                                      this->sent_data_,
                                      this->durability_service_);

    result = inserted;

    if (!inserted)
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::persist_data, ")
                 ACE_TEXT("failed to make data durable for ")
                 ACE_TEXT("(domain, topic, type) = (%d, %C, %C)\n"),
                 this->domain_id_,
                 this->topic_name_,
                 this->type_name_));
  }

  return result;
}

void WriteDataContainer::reschedule_deadline()
{
  for (PublicationInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter) {
    if (iter->second->deadline_timer_id_ != -1) {
      if (this->watchdog_->reset_timer_interval(iter->second->deadline_timer_id_) == -1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) WriteDataContainer::reschedule_deadline %p\n")
                   ACE_TEXT("reset_timer_interval")));
      }
    }
  }
}

void
WriteDataContainer::wait_pending()
{
  ACE_Time_Value pending_timeout =
    TheServiceParticipant->pending_timeout();

  ACE_Time_Value* pTimeout = 0;

  if (pending_timeout != ACE_Time_Value::zero) {
    pTimeout = &pending_timeout;
    pending_timeout += ACE_OS::gettimeofday();
  }

  while (true) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

    if (!pending_data())
      break;

    empty_condition_.wait(pTimeout);
  }
}

void
WriteDataContainer::get_instance_handles(InstanceHandleVec& instance_handles)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard,
            this->lock_);
  PublicationInstanceMapType::iterator it = instances_.begin();

  while (it != instances_.end()) {
    instance_handles.push_back(it->second->instance_handle_);
    ++it;
  }
}

void
WriteDataContainer::wakeup_blocking_writers (DataSampleListElement* stale,
                                            PublicationInstance* instance)
{
  if (stale && instance->waiting_list_.head_ != 0) {
      // Mark the first waiting sample will be next to add to instance
      // list.
      instance->waiting_list_.head_->space_available_ = true;
      // Remove this waiting sample from waiting list.
      DataSampleListElement* waiting = 0;

      if (instance->waiting_list_.dequeue_head_next_instance_sample(waiting) == false) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("WriteDataContainer::data_delivered, ")
                   ACE_TEXT("dequeue_head_next_instance_sample from waiting ")
                   ACE_TEXT("list failed\n")));
        return;
      }


      if (waiting_on_release_) {
        waiting_on_release_ = false;
        // Broadcast the blocked enqueue threads.
        if (DCPS_debug_level >= 2) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::wakeup_blocking_writers ")
                                ACE_TEXT("removed sample %X and broadcast to wake up ")
                                ACE_TEXT("blocking threads for available spot \n"),
                                stale));
        }

        condition_.broadcast();
      }
  }
}

} // namespace OpenDDS
} // namespace DCPS
