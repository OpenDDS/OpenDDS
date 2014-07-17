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
#include "InstanceDataSampleList.h"
#include "DataWriterImpl.h"
#include "MessageTracker.h"
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
#include "DataDurabilityCache.h"
#endif
#include "PublicationInstance.h"
#include "Util.h"
#include "Qos_Helper.h"
#include "GuidConverter.h"
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
resend_data_expired(DataSampleElement const & element,
                    DDS::LifespanQosPolicy const & lifespan)
{
  if (lifespan.duration.sec != DDS::DURATION_INFINITE_SEC
      || lifespan.duration.nanosec != DDS::DURATION_INFINITE_NSEC) {
    // Finite lifespan.  Check if data has expired.

    DDS::Time_t const tmp = {
      element.get_header().source_timestamp_sec_ + lifespan.duration.sec,
      element.get_header().source_timestamp_nanosec_ + lifespan.duration.nanosec
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
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  DataDurabilityCache* durability_cache,
  DDS::DurabilityServiceQosPolicy const & durability_service,
#endif
  std::auto_ptr<OfferedDeadlineWatchdog>& watchdog,
  CORBA::Long     max_instances,
  CORBA::Long     max_total_samples)
  : writer_(writer),
    depth_(depth),
    max_num_instances_(max_instances),
    max_num_samples_(max_total_samples),
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
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    durability_cache_(durability_cache),
    durability_service_(durability_service),
#endif
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
  if (this->unsent_data_.size() > 0) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: WriteDataContainer::~WriteDataContainer() - ")
               ACE_TEXT("destroyed with %d samples unsent.\n"),
               this->unsent_data_.size()));
  }

  if (this->sending_data_.size() > 0) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: WriteDataContainer::~WriteDataContainer() - ")
               ACE_TEXT("destroyed with %d samples sending.\n"),
               this->sending_data_.size()));
  }

  if (this->sent_data_.size() > 0) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::~WriteDataContainer() - ")
                 ACE_TEXT("destroyed with %d samples sent.\n"),
                 this->sent_data_.size()));
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
  DataSampleElement* sample,
  DDS::InstanceHandle_t instance_handle)
{
  // Get the PublicationInstance pointer from InstanceHandle_t.
  PublicationInstance* const instance =
    get_handle_instance(instance_handle);
  // Extract the instance queue.
  InstanceDataSampleList& instance_list = instance->samples_;

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

  unsent_data_.enqueue_tail(sample);

  //
  // Add this sample to the INSTANCE scope list.
  instance_list.enqueue_tail(sample);

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::reenqueue_all(const RepoId& reader_id,
                                  const DDS::LifespanQosPolicy& lifespan
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
                                  ,
                                  const FilterEvaluator* eval,
                                  const DDS::StringSeq& expression_params
#endif
                                  )
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->lock_,
                   DDS::RETCODE_ERROR);

  // Make a copy of sending_data_ and sent_data_;

  if (sending_data_.size() > 0) {
    this->copy_and_append(this->resend_data_,
                          sending_data_,
                          reader_id,
                          lifespan
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
                          , eval, expression_params
#endif
                          );
  }

  if (sent_data_.size() > 0) {
    this->copy_and_append(this->resend_data_,
                          sent_data_,
                          reader_id,
                          lifespan
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
                          , eval, expression_params
#endif
                          );

    if (DCPS_debug_level > 9) {
      GuidConverter converter(publication_id_);
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
    if (max_num_instances_ > 0
        && max_num_instances_ <= (CORBA::Long) instances_.size()) {
      return DDS::RETCODE_OUT_OF_RESOURCES;
    }

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

  InstanceDataSampleList& instance_list = instance->samples_;

  while (instance_list.size() > 0) {
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
    size = instance->samples_.size();
    return DDS::RETCODE_OK;
  }
}

size_t
WriteDataContainer::num_all_samples()
{
  size_t size = 0;

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->lock_,
                   0);

  for (PublicationInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter)
  {
    size += iter->second->samples_.size();
  }

  return size;
}

SendStateDataSampleList
WriteDataContainer::get_unsent_data()
{
  DBG_ENTRY_LVL("WriteDataContainer","get_unsent_data",6);

  //
  // The samples in unsent_data are added to the sending_data
  // during enqueue.
  //
  SendStateDataSampleList list = this->unsent_data_;

  //
  // The unsent_data_ already linked with the
  // next_send_sample during enqueue.
  // Append the unsent_data_ to current sending_data_
  // list.
  sending_data_.enqueue_tail(list);

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

SendStateDataSampleList
WriteDataContainer::get_resend_data()
{
  DBG_ENTRY_LVL("WriteDataContainer","get_resend_data",6);

  //
  // The samples in unsent_data are added to the sending_data
  // during enqueue.
  //
  SendStateDataSampleList list = this->resend_data_;

  if (list.size()) {
    //
    // The unsent_data_ already linked with the
    // next_send_sample during enqueue.
    // Append the unsent_data_ to current sending_data_
    // list.
    released_data_.enqueue_tail(list);
  }

  //
  // Clear the unsent data list.
  //
  this->resend_data_.reset();
  //
  // Return the moved list.
  //
  return list;
}

bool
WriteDataContainer::pending_data()
{
  return this->sending_data_.size() != 0
         || this->unsent_data_.size() != 0
         || this->released_data_.size() != 0;
}

void
WriteDataContainer::data_delivered(const DataSampleElement* sample)
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
  PublicationInstance* instance = sample->get_handle();
  bool dequeued =
    released_data_.dequeue(sample);
  DataSampleElement* stale = const_cast<DataSampleElement*>(sample);
  if (dequeued) {
    release_buffer(stale);
  } else {
    //
    // Search the sending_data_ list first.
    //
    if (sending_data_.dequeue(sample)) {
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

    if (instance->waiting_list_.head() != 0) {
      // Remove the delivered sample from the instance sample list
      // and release.
      if (!instance->samples_.dequeue(sample)) {
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
        GuidConverter converter(publication_id_);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) WriteDataContainer::data_delivered: ")
                   ACE_TEXT("domain %d topic %C publication %C pushed to HISTORY.\n"),
                   this->domain_id_,
                   this->topic_name_,
                   std::string(converter).c_str()));
      }

      DataSampleHeader::set_flag(HISTORIC_SAMPLE_FLAG, sample->get_sample());
      sent_data_.enqueue_tail(sample);
    }
  }

  this->wakeup_blocking_writers (stale, instance);

  // Signal if there is no pending data.
  if (!pending_data())
    empty_condition_.broadcast();
}

void
WriteDataContainer::data_dropped(const DataSampleElement* sample,
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
  DataSampleElement* stale = 0;

  if (sending_data_.dequeue(sample)) {
    // else: The data_dropped is called as a result of remove_sample()
    // called from reenqueue_all() which supports the TRANSIENT_LOCAL
    // qos. The samples that are sending by transport are dropped from
    // transport and will be moved to the unsent list for resend.
    unsent_data_.enqueue_tail(sample);

  } else {
    if (released_data_.dequeue(sample)) {
      // The remove_sample is requested when sample list size
      // reaches limit. In this case, the oldest sample is
      // moved to released_data_ already.
      stale = const_cast<DataSampleElement*>(sample);
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

  this->wakeup_blocking_writers (stale, sample->get_handle());

  if (!pending_data())
    empty_condition_.broadcast();
}

DDS::ReturnCode_t
WriteDataContainer::remove_oldest_sample(
  InstanceDataSampleList& instance_list,
  bool& released)
{
  DataSampleElement* stale = 0;

  //
  // Remove the oldest sample from the instance list.
  //
  if (instance_list.dequeue_head(stale) == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("WriteDataContainer::remove_oldest_sample, ")
                      ACE_TEXT("dequeue_head_next_sample failed\n")),
                     DDS::RETCODE_ERROR);
  }

  //
  // Remove the stale data from the next_writer_sample_ list.  The
  // sending_data_/next_send_sample_ list is not managed within the
  // container, it is only used external to the container and does
  // not need to be managed internally.
  //
  // The next_writer_sample_ link is being used in one of the sent_data_,
  // sending_data_, or unsent_data lists.  Removal from the doubly
  // linked list needs to repair the list only when the stale sample
  // is either the head or tail of the list.
  //

  //
  // Locate the head of the list that the stale data is in.
  //
  std::vector<SendStateDataSampleList*> send_lists;
  send_lists.push_back(&sending_data_);
  send_lists.push_back(&sent_data_);
  send_lists.push_back(&unsent_data_);

  const SendStateDataSampleList* containing_list = SendStateDataSampleList::send_list_containing_element(stale, send_lists);


  //
  // Identify the list that the stale data is in.
  // The stale data should be in one of the sent_data_, sending_data_
  // or unsent_data_. It should not be in released_data_ list since
  // this function is the only place a sample is moved from
  // sending_data_ to released_data_ list.

  // Remove the element from the internal list.
  bool result = false;

  if (containing_list == &this->sending_data_) {
    // Move the element to the released_data_ list since it is still
    // in use, and we need to wait until it is told by the transport.
    //
    result = this->sending_data_.dequeue(stale) != 0;
    released_data_.enqueue_tail(stale);
    released = false;

  } else if (containing_list == &this->sent_data_) {
    // No one is using the data sample, so we can release it back to
    // its allocator.
    //
    result = this->sent_data_.dequeue(stale) != 0;
    release_buffer(stale);
    released = true;

    if (DCPS_debug_level > 9) {
      GuidConverter converter(publication_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::remove_oldest_sample: ")
                 ACE_TEXT("domain %d topic %C publication %C sample removed from HISTORY.\n"),
                 this->domain_id_,
                 this->topic_name_,
                 std::string(converter).c_str()));
    }

  } else if (containing_list == &this->unsent_data_) {
    //
    // No one is using the data sample, so we can release it back to
    // its allocator.
    //
    result = this->unsent_data_.dequeue(stale) != 0;
    release_buffer(stale);
    released = true;

    if (DCPS_debug_level > 9) {
      GuidConverter converter(publication_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::remove_oldest_sample: ")
                 ACE_TEXT("domain %d topic %C publication %C sample removed from unsent.\n"),
                 this->domain_id_,
                 this->topic_name_,
                 std::string(converter).c_str()));
    }
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
WriteDataContainer::obtain_buffer(DataSampleElement*& element,
                                  DDS::InstanceHandle_t handle)
{
  PublicationInstance* instance = get_handle_instance(handle);

  ACE_NEW_MALLOC_RETURN(
    element,
    static_cast<DataSampleElement*>(
      sample_list_element_allocator_.malloc(
        sizeof(DataSampleElement))),
    DataSampleElement(publication_id_,
                          this->writer_,
                          instance,
                          &transport_send_element_allocator_,
                          &transport_customized_element_allocator_),
    DDS::RETCODE_ERROR);

  // Extract the current instance queue.
  InstanceDataSampleList& instance_list = instance->samples_;
  DDS::ReturnCode_t ret = DDS::RETCODE_OK;

  bool oldest_released = true;
  DataSampleElement* stale = instance_list.head();

  // Release the oldest sample if the size reaches the max size of
  // the sample list.
  if (instance_list.size() > depth_) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("WriteDataContainer::obtain_buffer, ")
               ACE_TEXT("The instance list size %d exceeds depth %d\n"),
               instance_list.size(),
               depth_));
    ret = DDS::RETCODE_ERROR;

  } else if (instance_list.size() == depth_) {
    // The remove_oldest_sample() method removes the oldest sample
    // from instance list and removes it from the internal lists.
    ret = this->remove_oldest_sample(instance_list, oldest_released);
  }

  if (ret != DDS::RETCODE_OK) {
    this->release_buffer(element);
    return ret;
  }

  ACE_Time_Value abs_timeout;
  bool resource_limit_waited = false;

  if ( (max_num_samples_ > 0)
      && ((CORBA::Long) this->num_all_samples () >= max_num_samples_) ) {

    if ((max_num_instances_ > 0)
        && (max_num_samples_ < (max_num_instances_ * depth_)) ) {
      // Try to remove samples from other instances

      bool other_oldest_released = false;
      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                       guard,
                       this->lock_,
                       0);

      if (DCPS_debug_level >= 2) {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::obtain_buffer")
                              ACE_TEXT(" instance %d attempting to remove")
                              ACE_TEXT(" oldest sample from any full queues\n"),
                              handle));
      }

      PublicationInstanceMapType::iterator it = instances_.begin();

      // try removing any full instances first
      while (!other_oldest_released && it != instances_.end()) {
        if (it->second->samples_.size() == depth_) {
          ret = this->remove_oldest_sample(it->second->samples_, other_oldest_released);
        }
        ++it;
      }

      // wasn't able to release one of the full instances or there wasnt any then
      // remove from any that have more than 1
      if (!other_oldest_released) {
        if (DCPS_debug_level >= 2) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::obtain_buffer")
                                ACE_TEXT(" instance %d attempting to remove")
                                ACE_TEXT(" oldest sample from any queues with")
                                ACE_TEXT(" more than one sample\n"),
                                handle));
        }

        it = instances_.begin();

        // try removing any full instances first
        while (!other_oldest_released && it != instances_.end()) {
          if (it->second->samples_.size() > 1) {
            ret = this->remove_oldest_sample(it->second->samples_, other_oldest_released);
          }
          ++it;
        }
      }
    }


    // Still too many samples - need to wait
    if ( ((CORBA::Long) this->num_all_samples () >= max_num_samples_)
        && ((max_num_instances_ == 0)
             || (max_num_samples_ < (max_num_instances_ * depth_)) ) ) {

      resource_limit_waited = true;
      abs_timeout = duration_to_absolute_time_value (max_blocking_time_);
      bool waited = false;
      while (!shutdown_ && ACE_OS::gettimeofday() < abs_timeout) {
        waited = true;
        waiting_on_release_ = true; // reduces broadcast to only when waiting

        if (DCPS_debug_level >= 2) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) WriteDataContainer::obtain_buffer, ")
                                ACE_TEXT ("wait for condition, any releases, oldest_released %d\n"),
                                oldest_released));
        }

        // lock is released while waiting and aquired before returning
        // from wait.
        int const wait_result = condition_.wait(&abs_timeout);

        if (wait_result == 0) { // signaled
          if ((CORBA::Long) this->num_all_samples () < max_num_samples_) {
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
    } // end of Still too many samples - need to wait

    //
  }

  if (ret != DDS::RETCODE_OK) {
    this->release_buffer(element);
    return ret;
  }

  // Each write/enqueue just writes one element and hence the number
  // of samples will reach the size of sample list at some point.
  // We need remove enough to accommodate the new element.

  if (should_block_) {
    // Need wait when waiting list is not empty or the oldest sample
    // is still being used.
    bool const need_wait =
      instance->waiting_list_.head() != 0
      || !oldest_released ? true : false;

    if (need_wait) {
      // Add the newly allocated sample to waiting list.
      instance->waiting_list_.enqueue_tail(element);

      // wait for all "released" samples to be delivered
      // Timeout value from Qos.RELIABILITY.max_blocking_time
      if (!resource_limit_waited)
        abs_timeout = duration_to_absolute_time_value (max_blocking_time_);

      bool waited = false;
      while (!shutdown_ && ACE_OS::gettimeofday() < abs_timeout) {
        waited = true;
        waiting_on_release_ = true; // reduces broadcast to only when waiting

        if (DCPS_debug_level >= 2) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) WriteDataContainer::obtain_buffer, ")
                                ACE_TEXT ("wait for condition, oldest_released %d waiting %X\n"),
                                oldest_released, instance->waiting_list_.head()));
        }

        // lock is released while waiting and acquired before returning
        // from wait.
        int const wait_result = condition_.wait(&abs_timeout);

        if (wait_result == 0) { // signaled
          if (element->space_available() == true) {
            break;
          } // else continue wait

        } else {
          if (errno == ETIME) {
            ret = DDS::RETCODE_TIMEOUT;

          } else {
            // handle the race condition where the element is freed after
            // the timeout has occurred, but before this thread has
            // re-acquired the lock
            if (element->space_available() == true) {
              break;
            }
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
        if (instance->waiting_list_.dequeue(element) == false) {
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
    data_holder_.enqueue_tail(element);
  }

  return ret;
}

void
WriteDataContainer::release_buffer(DataSampleElement* element)
{
  data_holder_.dequeue(element);
  // Release the memory to the allocator.
  ACE_DES_FREE(element,
               sample_list_element_allocator_.free,
               DataSampleElement);
}

void
WriteDataContainer::unregister_all()
{
  //### Debug statements to track where test is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:WriteDataContainer::unregister_all --> enter\n"));
  DBG_ENTRY_LVL("WriteDataContainer","unregister_all",6);
  //### Debug statements to track where test is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:WriteDataContainer::unregister_all --> set shutdown_ = true\n"));
  shutdown_ = true;

  {
    //The internal list needs protection since this call may result from the
    //the delete_datawriter call which does not acquire the lock in advance.
    //### Debug statements to track where test is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:WriteDataContainer::unregister_all --> LOCKING lock_\n"));
    ACE_GUARD(ACE_Recursive_Thread_Mutex,
              guard,
              this->lock_);
    //### Debug statements to track where test is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:WriteDataContainer::unregister_all --> LOCKED lock_\n"));
    //### Debug statements to track where test is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:WriteDataContainer::unregister_all --> remove_all_messages\n"));
    // Tell transport remove all control messages currently
    // transport is processing.
    (void) this->writer_->remove_all_msgs();

    // Broadcast to wake up all waiting threads.
    if (waiting_on_release_) {
      //### Debug statements to track where test is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:WriteDataContainer::unregister_all --> broadcast\n"));
      condition_.broadcast();
    }
  }
  DDS::ReturnCode_t ret;
  DataSample* registered_sample;
  PublicationInstanceMapType::iterator it = instances_.begin();

  while (it != instances_.end()) {
    //### Debug statements to track where test is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:WriteDataContainer::unregister_all --> dispose of the instance data\n"));
    // Release the instance data.
    ret = dispose(it->first, registered_sample, false);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::unregister_all, ")
                 ACE_TEXT("dispose instance %X failed\n"),
                 it->first));
    }
    //### Debug statements to track where test is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:WriteDataContainer::unregister_all --> mark instance unregistered\n"));
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
  //### Debug statements to track where test is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:WriteDataContainer::unregister_all --> exit\n"));
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
WriteDataContainer::copy_and_append(SendStateDataSampleList& list,
                                    const SendStateDataSampleList& appended,
                                    const RepoId& reader_id,
                                    const DDS::LifespanQosPolicy& lifespan
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
                                    ,
                                    const FilterEvaluator* eval,
                                    const DDS::StringSeq& params
#endif
                                    )
{
  for (SendStateDataSampleList::const_iterator cur = appended.begin();
       cur != appended.end(); ++cur) {

    // Do not copy and append data that has exceeded the configured
    // lifespan.
    if (resend_data_expired(*cur, lifespan))
      continue;

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    if (eval && writer_->filter_out(*cur, *eval, params))
      continue;
#endif

    DataSampleElement* element = 0;
    ACE_NEW_MALLOC(element,
                    static_cast<DataSampleElement*>(
                      sample_list_element_allocator_.malloc(
                        sizeof(DataSampleElement))),
                    DataSampleElement(*cur));

    // TODO: Does ACE_NEW_MALLOC throw?  Where's the check for
    //       allocation failure, i.e. element == 0?

    element->set_num_subs(1);
    element->set_sub_id(0, reader_id);

    list.enqueue_tail(element);
  }
}

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
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
     * TODO: We should only cache data that is not in the
     *       "released_data_" list, i.e. not still in use outside of
     *       this instance of WriteDataContainer.
     */
    bool const inserted =
      this->durability_cache_->insert(this->domain_id_,
                                      this->topic_name_,
                                      this->type_name_,
                                      this->sent_data_,
                                      this->durability_service_
                                     );

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
#endif

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

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);
  const bool report = DCPS_debug_level > 0 && pending_data();
  if (report) {
    ACE_TCHAR date_time[50];
    ACE_TCHAR* const time =
      MessageTracker::timestamp(pending_timeout,
                                date_time,
                                50);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%T (%P|%t) WriteDataContainer::wait_pending timeout ")
               ACE_TEXT("at %s\n"),
               (pending_timeout == ACE_Time_Value::zero ?
                  ACE_TEXT("(no timeout)") : time)));
  }
  while (true) {

    if (!pending_data())
      break;

    if (empty_condition_.wait(pTimeout) == -1 && pending_data()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) WriteDataContainer::wait_pending %p\n"),
                 ACE_TEXT("Timed out waiting for messages to be transported")));
      break;
    }
  }
  if (report) {
    ACE_DEBUG((LM_DEBUG,
               "%T WriteDataContainer::wait_pending done\n"));
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
WriteDataContainer::wakeup_blocking_writers (DataSampleElement* stale,
                                            PublicationInstance* instance)
{
  if (stale && instance->waiting_list_.head() != 0) {
      // Mark the first waiting sample will be next to add to instance
      // list.
      instance->waiting_list_.head()->set_space_available(true);
      // Remove this waiting sample from waiting list.
      DataSampleElement* waiting = 0;

      if (instance->waiting_list_.dequeue_head(waiting) == false) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("WriteDataContainer::wakeup_blocking_writers, ")
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
