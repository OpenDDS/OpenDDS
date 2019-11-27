/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ace/Condition_Recursive_Thread_Mutex.h"
#include "WriteDataContainer.h"
#include "DataSampleHeader.h"
#include "InstanceDataSampleList.h"
#include "DataWriterImpl.h"
#include "MessageTracker.h"
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
#include "DataDurabilityCache.h"
#endif
#include "PublicationInstance.h"
#include "Util.h"
#include "Time_Helper.h"
#include "GuidConverter.h"
#include "OfferedDeadlineWatchdog.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "tao/debug.h"

#include "ace/Auto_Ptr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @todo Refactor this code and DataReaderImpl::data_expired() to
 *       a common function.
 */
bool
resend_data_expired(const DataSampleElement& element,
                    const DDS::LifespanQosPolicy& lifespan)
{
  if (lifespan.duration.sec != DDS::DURATION_INFINITE_SEC
      || lifespan.duration.nanosec != DDS::DURATION_INFINITE_NSEC) {
    // Finite lifespan.  Check if data has expired.

    const DDS::Time_t tmp = {
      element.get_header().source_timestamp_sec_ + lifespan.duration.sec,
      element.get_header().source_timestamp_nanosec_ + lifespan.duration.nanosec
    };
    const SystemTimePoint expiration_time(time_to_time_value(tmp));
    const SystemTimePoint now = SystemTimePoint::now();

    if (now >= expiration_time) {
      if (DCPS_debug_level >= 8) {
        const TimeDuration diff = now - expiration_time;
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("OpenDDS (%P|%t) Data to be sent ")
                   ACE_TEXT("expired by %d seconds, %d microseconds.\n"),
                   diff.value().sec(),
                   diff.value().usec()));
      }

      return true; // Data expired.
    }
  }

  return false;
}

WriteDataContainer::WriteDataContainer(
  DataWriterImpl* writer,
  CORBA::Long max_samples_per_instance,
  CORBA::Long history_depth,
  CORBA::Long max_durable_per_instance,
  DDS::Duration_t max_blocking_time,
  size_t n_chunks,
  DDS::DomainId_t domain_id,
  const char* topic_name,
  const char* type_name,
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  DataDurabilityCache* durability_cache,
  const DDS::DurabilityServiceQosPolicy& durability_service,
#endif
  CORBA::Long max_instances,
  CORBA::Long max_total_samples)
  : transaction_id_(0),
    publication_id_(GUID_UNKNOWN),
    writer_(writer),
    max_samples_per_instance_(max_samples_per_instance),
    history_depth_(history_depth),
    max_durable_per_instance_(max_durable_per_instance),
    max_num_instances_(max_instances),
    max_num_samples_(max_total_samples),
    max_blocking_time_(max_blocking_time),
    waiting_on_release_(false),
    condition_(lock_, ConditionAttributesMonotonic()),
    empty_condition_(lock_, ConditionAttributesMonotonic()),
    wfa_condition_(wfa_lock_, ConditionAttributesMonotonic()),
    n_chunks_(n_chunks),
    sample_list_element_allocator_(2 * n_chunks_),
    shutdown_(false),
    domain_id_(domain_id),
    topic_name_(topic_name),
    type_name_(type_name)
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  , durability_cache_(durability_cache)
  , durability_service_(durability_service)
#endif
{

  if (DCPS_debug_level >= 2) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) WriteDataContainer "
               "sample_list_element_allocator %x with %d chunks\n",
               &sample_list_element_allocator_, n_chunks_));
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
    if (TransportRegistry::instance()->released()) {
      for (DataSampleElement* e; sending_data_.dequeue_head(e);) {
        release_buffer(e);
      }
    }
    if (sending_data_.size() && DCPS_debug_level) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: WriteDataContainer::~WriteDataContainer() - ")
                 ACE_TEXT("destroyed with %d samples sending.\n"),
                 this->sending_data_.size()));
    }
  }

  if (this->sent_data_.size() > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) WriteDataContainer::~WriteDataContainer() - ")
               ACE_TEXT("destroyed with %d samples sent.\n"),
               this->sent_data_.size()));
  }

  if (this->orphaned_to_transport_.size() > 0) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::~WriteDataContainer() - ")
                 ACE_TEXT("destroyed with %d samples orphaned_to_transport.\n"),
                 this->orphaned_to_transport_.size()));
    }
  }

  if (!shutdown_) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("WriteDataContainer::~WriteDataContainer, ")
               ACE_TEXT("The container has not been cleaned.\n")));
  }
}

DDS::ReturnCode_t
WriteDataContainer::enqueue_control(DataSampleElement* control_sample)
{
  // Enqueue to the next_send_sample_ thread of unsent_data_
  // will link samples with the next_sample/previous_sample and
  // also next_send_sample_.
  // This would save time when we actually send the data.

  unsent_data_.enqueue_tail(control_sample);

  return DDS::RETCODE_OK;
}

// This method assumes that instance list has space for this sample.
DDS::ReturnCode_t
WriteDataContainer::enqueue(
  DataSampleElement* sample,
  DDS::InstanceHandle_t instance_handle)
{
  // Get the PublicationInstance pointer from InstanceHandle_t.
  PublicationInstance_rch instance =
    get_handle_instance(instance_handle);
  // Extract the instance queue.
  InstanceDataSampleList& instance_list = instance->samples_;

  if (this->writer_->watchdog_.in()) {
    instance->last_sample_tv_ = instance->cur_sample_tv_;
    instance->cur_sample_tv_.set_to_now();
    this->writer_->watchdog_->execute(*this->writer_, instance, false);
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
                                  const OPENDDS_STRING& filterClassName,
                                  const FilterEvaluator* eval,
                                  const DDS::StringSeq& expression_params
#endif
                                  )
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   lock_,
                   DDS::RETCODE_ERROR);

  ssize_t total_size = 0;
  for (PublicationInstanceMapType::iterator it = instances_.begin();
       it != instances_.end(); ++it) {
    const ssize_t durable = std::min(it->second->samples_.size(),
                                     ssize_t(max_durable_per_instance_));
    total_size += durable;
    it->second->durable_samples_remaining_ = durable;
  }

  copy_and_prepend(resend_data_,
                   sending_data_,
                   reader_id,
                   lifespan,
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
                   filterClassName, eval, expression_params,
#endif
                   total_size);

  copy_and_prepend(resend_data_,
                   sent_data_,
                   reader_id,
                   lifespan,
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
                   filterClassName, eval, expression_params,
#endif
                   total_size);

  if (DCPS_debug_level > 9 && resend_data_.size()) {
    GuidConverter converter(publication_id_);
    GuidConverter reader(reader_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) WriteDataContainer::reenqueue_all: ")
               ACE_TEXT("domain %d topic %C publication %C copying ")
               ACE_TEXT("sending/sent to resend to %C.\n"),
               domain_id_,
               topic_name_,
               OPENDDS_STRING(converter).c_str(),
               OPENDDS_STRING(reader).c_str()));
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::register_instance(
  DDS::InstanceHandle_t&      instance_handle,
  Message_Block_Ptr&          registered_sample)
{
  PublicationInstance_rch instance;

  if (instance_handle == DDS::HANDLE_NIL) {
    if (max_num_instances_ > 0
        && max_num_instances_ <= (CORBA::Long) instances_.size()) {
      return DDS::RETCODE_OUT_OF_RESOURCES;
    }

    // registered the instance for the first time.
    instance.reset(new PublicationInstance(move(registered_sample)), keep_count());

    instance_handle = this->writer_->get_next_handle();

    int const insert_attempt = OpenDDS::DCPS::bind(instances_, instance_handle, instance);

    if (0 != insert_attempt) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::register_instance, ")
                 ACE_TEXT("failed to insert instance handle=%X\n"),
                 instance.in()));
      return DDS::RETCODE_ERROR;
    } // if (0 != insert_attempt)

    instance->instance_handle_ = instance_handle;

  } else {

    int const find_attempt = find(instances_, instance_handle, instance);

    if (0 != find_attempt) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("WriteDataContainer::register_instance, ")
                 ACE_TEXT("The provided instance handle=%X is not a valid")
                 ACE_TEXT("handle.\n"),
                 instance_handle));

      return DDS::RETCODE_ERROR;
    } // if (0 != find_attempt)

    instance->unregistered_ = false;
  }

  // The registered_sample is shallow copied.
  registered_sample.reset(instance->registered_sample_->duplicate());

  if (this->writer_->watchdog_.in()) {
    this->writer_->watchdog_->schedule_timer(instance);
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::unregister(
  DDS::InstanceHandle_t   instance_handle,
  Message_Block_Ptr& registered_sample,
  bool                    dup_registered_sample)
{
  PublicationInstance_rch instance;

  int const find_attempt = find(instances_, instance_handle, instance);

  if (0 != find_attempt) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("WriteDataContainer::unregister, ")
                      ACE_TEXT("The instance(handle=%X) ")
                      ACE_TEXT("is not registered yet.\n"),
                      instance_handle),
                     DDS::RETCODE_PRECONDITION_NOT_MET);
  } // if (0 != find_attempt)

  instance->unregistered_ = true;

  if (dup_registered_sample) {
    // The registered_sample is shallow copied.
    registered_sample.reset(instance->registered_sample_->duplicate());
  }

  if (this->writer_->watchdog_.in())
    this->writer_->watchdog_->cancel_timer(instance);

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::dispose(DDS::InstanceHandle_t instance_handle,
                            Message_Block_Ptr&    registered_sample,
                            bool                  dup_registered_sample)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   lock_,
                   DDS::RETCODE_ERROR);

  PublicationInstance_rch instance;

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
    registered_sample.reset(instance->registered_sample_->duplicate());
  }

  // Note: The DDS specification is unclear as to if samples in the process
  // of being sent should be removed or not.
  // The advantage of calling remove_sample() on them is that the
  // cached allocator memory for them is freed.  The disadvantage
  // is that the slow reader may see multiple disposes without
  // any write sample between them and hence not temporarily move into the
  // Alive state.
  // We have chosen to NOT remove the sending samples.

  InstanceDataSampleList& instance_list = instance->samples_;

  while (instance_list.size() > 0) {
    bool released = false;
    DDS::ReturnCode_t ret
    = remove_oldest_sample(instance_list, released);

    if (ret != DDS::RETCODE_OK) {
      return ret;
    }
  }

  if (this->writer_->watchdog_.in())
    this->writer_->watchdog_->cancel_timer(instance);
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::num_samples(DDS::InstanceHandle_t handle,
                                size_t&                 size)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   lock_,
                   DDS::RETCODE_ERROR);
  PublicationInstance_rch instance;

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
                   lock_,
                   0);

  for (PublicationInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter)
  {
    size += iter->second->samples_.size();
  }

  return size;
}

ACE_UINT64
WriteDataContainer::get_unsent_data(SendStateDataSampleList& list)
{
  DBG_ENTRY_LVL("WriteDataContainer","get_unsent_data",6);
  //
  // The samples in unsent_data are added to the local datawriter
  // list and enqueued to the sending_data_ signifying they have
  // been passed to the transport to send in a transaction
  //
  list = this->unsent_data_;

  // Increment send counter for this send operation
  ++transaction_id_;

  // Mark all samples with current send counter
  SendStateDataSampleList::iterator iter = list.begin();
  while (iter != list.end()) {
    iter->set_transaction_id(this->transaction_id_);
    ++iter;
  }

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

  //
  // Return the moved list.
  //
  return transaction_id_;
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
         || this->orphaned_to_transport_.size() != 0
         || this->unsent_data_.size() != 0;
}

void
WriteDataContainer::data_delivered(const DataSampleElement* sample)
{
  DBG_ENTRY_LVL("WriteDataContainer","data_delivered",6);

  if (DCPS_debug_level >= 2) {
    ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::data_delivered")
                          ACE_TEXT(" %@\n"), sample));
  }

  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard,
            lock_);

  // Delivered samples _must_ be on sending_data_ list

  // If it is not found in one of the lists, an invariant
  // exception is declared.

  // The element now needs to be removed from the sending_data_
  // list, and appended to the end of the sent_data_ list here

  DataSampleElement* stale = const_cast<DataSampleElement*>(sample);

  // If sample is on a SendStateDataSampleList it should be on the
  // sending_data_ list signifying it was given to the transport to
  // deliver and now the transport is signaling it has been delivered
  if (!sending_data_.dequeue(sample)) {
    //
    // Should be on sending_data_.  If it is in sent_data_
    // or unsent_data there was a problem.
    //
    SendStateDataSampleList* send_lists[] = {
      &sent_data_,
      &unsent_data_,
      &orphaned_to_transport_};
    const SendStateDataSampleList* containing_list =
      SendStateDataSampleList::send_list_containing_element(stale, send_lists);

    if (containing_list == &this->sent_data_) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("WriteDataContainer::data_delivered, ")
                 ACE_TEXT("The delivered sample is not in sending_data_ and ")
                 ACE_TEXT("WAS IN sent_data_.\n")));
    } else if (containing_list == &this->unsent_data_) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("WriteDataContainer::data_delivered, ")
                 ACE_TEXT("The delivered sample is not in sending_data_ and ")
                 ACE_TEXT("WAS IN unsent_data_ list.\n")));
    } else {

      //No-op: elements may be removed from all WriteDataContainer lists during shutdown
      //and inform transport of their release.  Transport will call data-delivered on the
      //elements as it processes the removal but they will already be gone from the send lists.
      if (stale->get_header().message_id_ != SAMPLE_DATA) {
        //this message was a control message so release it
        if (DCPS_debug_level > 9) {
          GuidConverter converter(publication_id_);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) WriteDataContainer::data_delivered: ")
                     ACE_TEXT("domain %d topic %C publication %C control message delivered.\n"),
                     this->domain_id_,
                     this->topic_name_,
                     OPENDDS_STRING(converter).c_str()));
        }
        writer_->controlTracker.message_delivered();
      }

      if (containing_list == &this->orphaned_to_transport_) {
        orphaned_to_transport_.dequeue(sample);
        release_buffer(stale);

      } else if (!containing_list) {
        // samples that were retrieved from get_resend_data()
        SendStateDataSampleList::remove(stale);
        release_buffer(stale);
      }

      if (!pending_data())
        empty_condition_.broadcast();
    }

    return;
  }
  ACE_GUARD(ACE_SYNCH_MUTEX, wfa_guard, wfa_lock_);
  SequenceNumber acked_seq = stale->get_header().sequence_;
  SequenceNumber prev_max = acked_sequences_.cumulative_ack();

  if (stale->get_header().message_id_ != SAMPLE_DATA) {
    //this message was a control message so release it
    if (DCPS_debug_level > 9) {
      GuidConverter converter(publication_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::data_delivered: ")
                 ACE_TEXT("domain %d topic %C publication %C control message delivered.\n"),
                 this->domain_id_,
                 this->topic_name_,
                 OPENDDS_STRING(converter).c_str()));
    }
    release_buffer(stale);
    stale = 0;
    writer_->controlTracker.message_delivered();
  } else {

    if (max_durable_per_instance_) {
      DataSampleHeader::set_flag(HISTORIC_SAMPLE_FLAG, sample->get_sample());
      sent_data_.enqueue_tail(sample);

    } else {
      if (InstanceDataSampleList::on_some_list(sample)) {
        PublicationInstance_rch inst = sample->get_handle();
        inst->samples_.dequeue(sample);
      }
      release_buffer(stale);
      stale = 0;
    }

    if (DCPS_debug_level > 9) {
      GuidConverter converter(publication_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::data_delivered: ")
                 ACE_TEXT("domain %d topic %C publication %C %s.\n"),
                 this->domain_id_,
                 this->topic_name_,
                 OPENDDS_STRING(converter).c_str(),
                 max_durable_per_instance_
                 ? ACE_TEXT("stored for durability")
                 : ACE_TEXT("released")));
    }

    this->wakeup_blocking_writers (stale);
  }
  if (DCPS_debug_level > 9) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::data_delivered: ")
                         ACE_TEXT("Inserting acked_sequence: %q\n"),
                         acked_seq.getValue()));
  }

  acked_sequences_.insert(acked_seq);

  if (prev_max == SequenceNumber::SEQUENCENUMBER_UNKNOWN() ||
      prev_max < acked_sequences_.cumulative_ack()) {

    if (DCPS_debug_level > 9) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WriteDataContainer::data_delivered - ")
                 ACE_TEXT("broadcasting wait_for_acknowledgments update.\n")));
    }

    wfa_condition_.broadcast();
  }

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
  // and the instance list. We do not need acquire the lock here since
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
    lock_);

  // The dropped sample should be in the sending_data_ list.
  // Otherwise an exception will be raised.
  //
  // We are now been notified by transport, so we can
  // keep the sample from the sending_data_ list still in
  // sample list since we will send it.

  DataSampleElement* stale = const_cast<DataSampleElement*>(sample);

  // If sample is on a SendStateDataSampleList it should be on the
  // sending_data_ list signifying it was given to the transport to
  // deliver and now the transport is signaling it has been dropped

  if (sending_data_.dequeue(sample)) {
    // else: The data_dropped is called as a result of remove_sample()
    // called from reenqueue_all() which supports the TRANSIENT_LOCAL
    // qos. The samples that are sending by transport are dropped from
    // transport and will be moved to the unsent list for resend.
    unsent_data_.enqueue_tail(sample);

  } else {
    //
    // If it is in sent_data_ or unsent_data there was a problem.
    //
    SendStateDataSampleList* send_lists[] = {
      &sent_data_,
      &unsent_data_,
      &orphaned_to_transport_};
    const SendStateDataSampleList* containing_list =
      SendStateDataSampleList::send_list_containing_element(stale, send_lists);

    if (containing_list == &this->sent_data_) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("WriteDataContainer::data_dropped, ")
                 ACE_TEXT("The dropped sample is not in sending_data_ and ")
                 ACE_TEXT("WAS IN sent_data_.\n")));
    } else if (containing_list == &this->unsent_data_) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("WriteDataContainer::data_dropped, ")
                 ACE_TEXT("The dropped sample is not in sending_data_ and ")
                 ACE_TEXT("WAS IN unsent_data_ list.\n")));
    } else {

      //No-op: elements may be removed from all WriteDataContainer lists during shutdown
      //and inform transport of their release.  Transport will call data-dropped on the
      //elements as it processes the removal but they will already be gone from the send lists.
      if (stale->get_header().message_id_ != SAMPLE_DATA) {
        //this message was a control message so release it
        if (DCPS_debug_level > 9) {
          GuidConverter converter(publication_id_);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) WriteDataContainer::data_dropped: ")
                     ACE_TEXT("domain %d topic %C publication %C control message dropped.\n"),
                     this->domain_id_,
                     this->topic_name_,
                     OPENDDS_STRING(converter).c_str()));
        }
        writer_->controlTracker.message_dropped();
      }

      if (containing_list == &this->orphaned_to_transport_) {
        orphaned_to_transport_.dequeue(sample);
        release_buffer(stale);
        if (!pending_data())
          empty_condition_.broadcast();

      } else if (!containing_list) {
        // samples that were retrieved from get_resend_data()
        SendStateDataSampleList::remove(stale);
        release_buffer(stale);
      }
    }

    return;
  }

  this->wakeup_blocking_writers (stale);

  if (!pending_data())
    empty_condition_.broadcast();
}

void
WriteDataContainer::remove_excess_durable()
{
  if (!max_durable_per_instance_)
    return;

  size_t n_released = 0;

  for (PublicationInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter) {

    CORBA::Long durable_allowed = max_durable_per_instance_;
    InstanceDataSampleList& instance_list = iter->second->samples_;

    for (DataSampleElement* it = instance_list.tail(), *prev; it; it = prev) {
      prev = InstanceDataSampleList::prev(it);

      if (DataSampleHeader::test_flag(HISTORIC_SAMPLE_FLAG, it->get_sample())) {

        if (durable_allowed) {
          --durable_allowed;
        } else {
          instance_list.dequeue(it);
          sent_data_.dequeue(it);
          release_buffer(it);
          ++n_released;
        }
      }
    }
  }

  if (n_released && DCPS_debug_level > 9) {
    const GuidConverter converter(publication_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) WriteDataContainer::remove_excess_durable: ")
               ACE_TEXT("domain %d topic %C publication %C %B samples removed ")
               ACE_TEXT("from durable data.\n"), domain_id_, topic_name_,
               OPENDDS_STRING(converter).c_str(), n_released));
  }
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
  SendStateDataSampleList* send_lists[] = {
    &sending_data_,
    &sent_data_,
    &unsent_data_,
    &orphaned_to_transport_};
  const SendStateDataSampleList* containing_list =
    SendStateDataSampleList::send_list_containing_element(stale, send_lists);

  //
  // Identify the list that the stale data is in.
  // The stale data should be in one of the sent_data_, sending_data_
  // or unsent_data_. It should not be in released_data_ list since
  // this function is the only place a sample is moved from
  // sending_data_ to released_data_ list.

  // Remove the element from the internal list.
  bool result = false;

  if (containing_list == &this->sending_data_) {
    if (DCPS_debug_level > 2) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("WriteDataContainer::remove_oldest_sample, ")
                 ACE_TEXT("removing from sending_data_ so must notify transport to remove sample\n")));
    }

    // This means transport is still using the sample that needs to
    // be released currently so notify transport that sample is being removed.

    if (this->writer_->remove_sample(stale)) {
      if (this->sent_data_.dequeue(stale)) {
        release_buffer(stale);
        result = true;
      }

    } else {
      if (this->sending_data_.dequeue(stale)) {
        this->orphaned_to_transport_.enqueue_tail(stale);
      } else if (this->sent_data_.dequeue(stale)) {
        release_buffer(stale);
        result = true;
      }
      result = true;
    }
    released = true;

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
                 OPENDDS_STRING(converter).c_str()));
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
                 OPENDDS_STRING(converter).c_str()));
    }
  } else {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("WriteDataContainer::remove_oldest_sample, ")
                      ACE_TEXT("The oldest sample is not in any internal list.\n")),
                     DDS::RETCODE_ERROR);
  }

  if (!pending_data()) {
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
WriteDataContainer::obtain_buffer_for_control(DataSampleElement*& element)
{
  DBG_ENTRY_LVL("WriteDataContainer","obtain_buffer_for_control", 6);

  ACE_NEW_MALLOC_RETURN(
    element,
    static_cast<DataSampleElement*>(
      sample_list_element_allocator_.malloc(
        sizeof(DataSampleElement))),
    DataSampleElement(publication_id_,
                      this->writer_,
                      PublicationInstance_rch()),
    DDS::RETCODE_ERROR);

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
WriteDataContainer::obtain_buffer(DataSampleElement*& element,
                                  DDS::InstanceHandle_t handle)
{
  DBG_ENTRY_LVL("WriteDataContainer","obtain_buffer", 6);

  remove_excess_durable();

  PublicationInstance_rch instance = get_handle_instance(handle);

  if (!instance) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  ACE_NEW_MALLOC_RETURN(
    element,
    static_cast<DataSampleElement*>(
      sample_list_element_allocator_.malloc(
        sizeof(DataSampleElement))),
    DataSampleElement(publication_id_,
                          this->writer_,
                          instance),
    DDS::RETCODE_ERROR);

  // Extract the current instance queue.
  InstanceDataSampleList& instance_list = instance->samples_;
  DDS::ReturnCode_t ret = DDS::RETCODE_OK;

  bool need_to_set_abs_timeout = true;
  MonotonicTimePoint abs_timeout;

  //max_num_samples_ covers ResourceLimitsQosPolicy max_samples and
  //max_instances and max_instances * depth
  while ((instance_list.size() >= max_samples_per_instance_) ||
         ((this->max_num_samples_ > 0) &&
         ((CORBA::Long) this->num_all_samples () >= this->max_num_samples_))) {

    if (this->writer_->qos_.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS) {
      if (instance_list.size() >= history_depth_) {
        if (DCPS_debug_level >= 2) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::obtain_buffer")
                     ACE_TEXT(" instance %d attempting to remove")
                     ACE_TEXT(" its oldest sample (reliable)\n"),
                     handle));
        }
        bool oldest_released = false;
        ret = remove_oldest_sample(instance_list, oldest_released);
        if (oldest_released) {
          break;
        }
      }
      // Reliable writers can wait
      if (need_to_set_abs_timeout) {
        abs_timeout = MonotonicTimePoint(MonotonicTimePoint::now() + TimeDuration(max_blocking_time_));
        need_to_set_abs_timeout = false;
      }
      if (!shutdown_ && MonotonicTimePoint::now() < abs_timeout) {
        if (DCPS_debug_level >= 2) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::obtain_buffer")
                                ACE_TEXT(" instance %d waiting for samples to be released by transport\n"),
                      handle));
        }

        waiting_on_release_ = true;
        const int wait_result = condition_.wait(&abs_timeout.value());

        if (wait_result == 0) {
          remove_excess_durable();

        } else {
          if (errno == ETIME) {
            if (DCPS_debug_level >= 2) {
              ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::obtain_buffer")
                                    ACE_TEXT(" instance %d timed out waiting for samples to be released by transport\n"),
                          handle));
            }
            ret = DDS::RETCODE_TIMEOUT;

          } else {
            ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) ERROR: WriteDataContainer::obtain_buffer condition_.wait()")
                                  ACE_TEXT("%p\n")));
            ret = DDS::RETCODE_ERROR;
          }
        }

      } else {
        //either shutdown has been signaled or max_blocking_time
        //has surpassed so treat as timeout
        ret = DDS::RETCODE_TIMEOUT;
      }

    } else {
      //BEST EFFORT
      bool oldest_released = false;

      //try to remove stale samples from this instance
      // The remove_oldest_sample() method removes the oldest sample
      // from instance list and removes it from the internal lists.
      if (instance_list.size() > 0) {
        if (DCPS_debug_level >= 2) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::obtain_buffer")
                                ACE_TEXT(" instance %d attempting to remove")
                                ACE_TEXT(" its oldest sample\n"),
                                handle));
        }
        ret = remove_oldest_sample(instance_list, oldest_released);
      }
      //else try to remove stale samples from other instances which are full
      if (ret == DDS::RETCODE_OK && !oldest_released) {
        if (DCPS_debug_level >= 2) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::obtain_buffer")
                                ACE_TEXT(" instance %d attempting to remove")
                                ACE_TEXT(" oldest sample from any full instances\n"),
                                handle));
        }
        PublicationInstanceMapType::iterator it = instances_.begin();

        while (!oldest_released && it != instances_.end() && ret == DDS::RETCODE_OK) {
          if (it->second->samples_.size() >= max_samples_per_instance_) {
            ret = remove_oldest_sample(it->second->samples_, oldest_released);
          }
          ++it;
        }
      }
      //else try to remove stale samples from other non-full instances
      if (ret == DDS::RETCODE_OK && !oldest_released) {
        if (DCPS_debug_level >= 2) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::obtain_buffer")
                                ACE_TEXT(" instance %d attempting to remove")
                                ACE_TEXT(" oldest sample from any instance with samples currently\n"),
                                handle));
        }
        PublicationInstanceMapType::iterator it = instances_.begin();

        while (!oldest_released && it != instances_.end() && ret == DDS::RETCODE_OK) {
          if (it->second->samples_.size() > 0) {
            ret = remove_oldest_sample(it->second->samples_, oldest_released);
          }
          ++it;
        }
      }
      if (!oldest_released) {
        //This means that no instances have samples to remove and yet
        //still hitting resource limits.
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("WriteDataContainer::obtain_buffer, ")
                   ACE_TEXT("hitting resource limits with no samples to remove\n")));
        ret = DDS::RETCODE_ERROR;
      }
    }  //END BEST EFFORT

    if (ret != DDS::RETCODE_OK) {
      if (DCPS_debug_level >= 2) {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::obtain_buffer")
                              ACE_TEXT(" instance %d could not obtain buffer for sample")
                              ACE_TEXT(" releasing allotted sample and returning\n"),
                              handle));
      }
      this->release_buffer(element);
      return ret;
    }
  }  //END WHILE

  data_holder_.enqueue_tail(element);

  return ret;
}

void
WriteDataContainer::release_buffer(DataSampleElement* element)
{
  if (element->get_header().message_id_ == SAMPLE_DATA)
    data_holder_.dequeue(element);
  // Release the memory to the allocator.
  ACE_DES_FREE(element,
               sample_list_element_allocator_.free,
               DataSampleElement);
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
              lock_);
    // Tell transport remove all control messages currently
    // transport is processing.
    (void) this->writer_->remove_all_msgs();

    // Broadcast to wake up all waiting threads.
    if (waiting_on_release_) {
      condition_.broadcast();
    }
  }
  DDS::ReturnCode_t ret;
  Message_Block_Ptr registered_sample;
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

    // Get the next iterator before erase the instance handle.
    PublicationInstanceMapType::iterator it_next = it;
    ++it_next;
    // Remove the instance from the instance list.
    unbind(instances_, it->first);
    it = it_next;
  }

  ACE_UNUSED_ARG(registered_sample);
}

PublicationInstance_rch
WriteDataContainer::get_handle_instance(DDS::InstanceHandle_t handle)
{
  PublicationInstance_rch instance;

  if (0 != find(instances_, handle, instance)) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ")
               ACE_TEXT("WriteDataContainer::get_handle_instance, ")
               ACE_TEXT("lookup for %d failed\n"), handle));
  }

  return instance;
}

void
WriteDataContainer::copy_and_prepend(SendStateDataSampleList& list,
                                     const SendStateDataSampleList& appended,
                                     const RepoId& reader_id,
                                     const DDS::LifespanQosPolicy& lifespan,
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
                                     const OPENDDS_STRING& filterClassName,
                                     const FilterEvaluator* eval,
                                     const DDS::StringSeq& params,
#endif
                                     ssize_t& max_resend_samples)
{
  for (SendStateDataSampleList::const_reverse_iterator cur = appended.rbegin();
       cur != appended.rend() && max_resend_samples; ++cur) {

    if (resend_data_expired(*cur, lifespan))
      continue;

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    if (eval && writer_->filter_out(*cur, filterClassName, *eval, params))
      continue;
#endif

    PublicationInstance_rch inst = cur->get_handle();

    if (!inst) {
      // *cur is a control message, just skip it
      continue;
    }

    if (inst->durable_samples_remaining_ == 0)
      continue;
    --inst->durable_samples_remaining_;

    DataSampleElement* element = 0;
    ACE_NEW_MALLOC(element,
                   static_cast<DataSampleElement*>(
                     sample_list_element_allocator_.malloc(
                       sizeof(DataSampleElement))),
                   DataSampleElement(*cur));

    element->set_num_subs(1);
    element->set_sub_id(0, reader_id);

    list.enqueue_head(element);
    --max_resend_samples;
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

    //
    //  We only cache data that is not still in use outside of
    //  this instance of WriteDataContainer
    //  (only cache samples in sent_data_ meaning transport has delivered).
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
      if (this->writer_->watchdog_->reset_timer_interval(iter->second->deadline_timer_id_) == -1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) WriteDataContainer::reschedule_deadline %p\n")
                   ACE_TEXT("reset_timer_interval")));
      }
    }
  }
}

void
WriteDataContainer::wait_pending()
{
  const TimeDuration pending_timeout(TheServiceParticipant->pending_timeout());
  MonotonicTimePoint timeout_at;
  const ACE_Time_Value_T<MonotonicClock>* timeout_ptr = 0;

  if (!pending_timeout.is_zero()) {
    timeout_at = MonotonicTimePoint::now() + pending_timeout;
    timeout_ptr = &timeout_at.value();
  }

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, lock_);
  const bool report = DCPS_debug_level > 0 && pending_data();
  if (report) {
    if (pending_timeout.is_zero()) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::wait_pending no timeout\n")));
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::wait_pending ")
        ACE_TEXT("timeout at %#T\n"),
        &pending_timeout.value()));
    }
  }

  while (pending_data()) {
    if (empty_condition_.wait(timeout_ptr) == -1 && pending_data()) {
      if (DCPS_debug_level) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) WriteDataContainer::wait_pending %p\n"),
                   ACE_TEXT("Timed out waiting for messages to be transported")));
        this->log_send_state_lists("WriteDataContainer::wait_pending - wait failed: ");
      }
      break;
    }
  }
  if (report) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::wait_pending done\n")));
  }
}

void
WriteDataContainer::get_instance_handles(InstanceHandleVec& instance_handles)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard,
            lock_);
  PublicationInstanceMapType::iterator it = instances_.begin();

  while (it != instances_.end()) {
    instance_handles.push_back(it->second->instance_handle_);
    ++it;
  }
}

DDS::ReturnCode_t
WriteDataContainer::wait_ack_of_seq(const MonotonicTimePoint& abs_deadline, const SequenceNumber& sequence)
{
  const MonotonicTimePoint deadline(abs_deadline);
  DDS::ReturnCode_t ret = DDS::RETCODE_OK;
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, lock_, DDS::RETCODE_ERROR);
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, wfa_guard, wfa_lock_, DDS::RETCODE_ERROR);

  SequenceNumber last_acked = acked_sequences_.last_ack();
  SequenceNumber acked = acked_sequences_.cumulative_ack();
  if (sequence == last_acked && sequence == acked && sending_data_.size() != 0) {
    acked_sequences_.insert(sending_data_.head()->get_header().sequence_.previous());
  }

  guard.release();

  while (MonotonicTimePoint::now() < deadline) {

    if (!sequence_acknowledged(sequence)) {
      // lock is released while waiting and acquired before returning
      // from wait.
      int const wait_result = wfa_condition_.wait(&deadline.value());

      if (wait_result != 0) {
        if (errno == ETIME) {
          if (DCPS_debug_level >= 2) {
            ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::wait_ack_of_seq")
                                  ACE_TEXT(" timed out waiting for sequence %q to be acked\n"),
                                  sequence.getValue()));
          }
          ret = DDS::RETCODE_TIMEOUT;
        } else {
          ret = DDS::RETCODE_ERROR;
        }
      }
    } else {
      ret = DDS::RETCODE_OK;
      break;
    }
  }

  return ret;
}

bool
WriteDataContainer::sequence_acknowledged(const SequenceNumber sequence)
{
  if (sequence == SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    //return true here so that wait_for_acknowledgements doesn't block
    return true;
  }

  SequenceNumber acked = acked_sequences_.cumulative_ack();
  if (DCPS_debug_level >= 10) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) WriteDataContainer::sequence_acknowledged ")
                          ACE_TEXT("- cumulative ack is currently: %q\n"), acked.getValue()));
  }
  if (acked == SequenceNumber::SEQUENCENUMBER_UNKNOWN() || acked < sequence){
    //if acked_sequences_ is empty or its cumulative_ack is lower than
    //the requests sequence, return false
    return false;
  }
  return true;
}

void
WriteDataContainer::wakeup_blocking_writers (DataSampleElement* stale)
{
  if (!stale && waiting_on_release_) {
    waiting_on_release_ = false;

    condition_.broadcast();
  }
}

void
WriteDataContainer::log_send_state_lists (OPENDDS_STRING description)
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) WriteDataContainer::log_send_state_lists: %C -- unsent(%d), sending(%d), sent(%d), orphaned_to_transport(%d), num_all_samples(%d), num_instances(%d)\n",
             description.c_str(),
             unsent_data_.size(),
             sending_data_.size(),
             sent_data_.size(),
             orphaned_to_transport_.size(),
             num_all_samples(),
             instances_.size()));
}

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
