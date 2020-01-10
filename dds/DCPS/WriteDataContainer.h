/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_WRITE_DATA_CONTAINER_H
#define OPENDDS_DCPS_WRITE_DATA_CONTAINER_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsCoreC.h"
#include "DataSampleElement.h"
#include "SendStateDataSampleList.h"
#include "WriterDataSampleList.h"
#include "DisjointSequence.h"
#include "PoolAllocator.h"
#include "PoolAllocationBase.h"
#include "Message_Block_Ptr.h"
#include "TimeTypes.h"

#include "ace/Synch_Traits.h"
#include "ace/Condition_T.h"
#include "ace/Condition_Thread_Mutex.h"
#include "ace/Condition_Recursive_Thread_Mutex.h"

#include <memory>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class InstanceDataSampleList;
class DataWriterImpl;
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
class DataDurabilityCache;
#endif
class FilterEvaluator;

typedef OPENDDS_MAP(DDS::InstanceHandle_t, PublicationInstance_rch)
  PublicationInstanceMapType;

/**
 * @class WriteDataContainer
 *
 * @brief A container for instances sample data.
 *
 * This container is instantiated per DataWriter. It maintains
 * list of PublicationInstance objects which is internally
 * referenced by the instance handle.
 *
 * This container contains threaded lists of all data written to a
 * given DataWriter. The real data sample is represented by the
 * DataSampleElement.  The data_holder_ holds all
 * DataSampleElement in the writing order via the
 * next_writer_sample_/previous_writer_sample_ thread. The instance list in
 * PublicationInstance links samples via the next_instance_sample_
 * thread.
 *
 * There are three state transition lists used during write operations:
 * unsent, sending, and sent.  These lists are linked
 * via the next_send_sample_/previous_send_sample_ thread. Any
 * DataSampleElement should be in one of these three lists and
 * SHOULD NOT be shared between these three lists.
 * A normal transition of a DataSampleElement would be
 * unsent->sending->sent.  A DataSampleElement transitions from unsent
 * to sent naturally during a write operation when get_unsent_data is called.
 * A DataSampleElement transitions from sending to sent when data_delivered
 * is called notifying the container that the transport is finished with the
 * sample and it can be marked as a historical sample.
 * A DataSampleElement may transition back to unsent from sending if
 * the transport notifies that the sample was dropped and should be resent.
 * A DataSampleElement is removed from sent list and freed when the instance
 * queue or container (for another instance) needs more space.
 * A DataSampleElement may be removed and freed directly from sending when
 * and unreliable writer needs space.  In this case the transport will be
 * notified that the sample should be removed.
 * A DataSampleElement may also be removed directly from unsent if an unreliable
 * writer needs space for new samples.
 * Note: The real data sample will be freed when the reference counting goes 0.
 * The resend list is only used when the datawriter uses
 * TRANSIENT_LOCAL_DURABILITY_QOS. It holds the DataSampleElements
 * for the data sample duplicates of the sending and sent list and
 * hands this list off to the transport.
 *
 *
 *
 * @note: 1) The PublicationInstance object is not removed from
 *           this container until the instance is
 *           unregistered. The same instance handle is reused for
 *           re-registration. The instance data is deleted when
 *           this container is deleted. This would simplify
 *           instance data memory management. An alternative way
 *           is to remove the handle from the instance list when
 *           unregister occurs and delete the instance data after
 *           the transport is done with the instance, but we do
 *           not have a way to know when the transport is done
 *           since we have the sending list for all instances in
 *           the same datawriter.
 *        2) This container has the ownership of the instance data
 *           samples when the data is written. The type-specific
 *           datawriter that allocates the memory for the sample
 *           data gives the ownership to its base class.
 *        3) It is the responsibility of the owner of objects of
 *           this class to ensure that access to the lists are
 *           properly locked.  This means using the same
 *           lock/condition to access the lists via the enqueue(),
 *           get*(), and data_delivered() methods.  For the case
 *           where these are all called from the same (client)
 *           thread, this should be a recursive lock so that: 1)
 *           we do not deadlock; and, 2) we incur the cost of
 *           obtaining the lock only once.
 */
class OpenDDS_Dcps_Export WriteDataContainer : public PoolAllocationBase {
public:

  friend class DataWriterImpl;

  /**
   * No default constructor, must be initialized.
   */
  WriteDataContainer(
    /// The writer which owns this container.
    DataWriterImpl*  writer,
    /// Max samples kept within each instance
    CORBA::Long      max_samples_per_instance,
    CORBA::Long history_depth,
    /// Max durable samples sent for each instance
    CORBA::Long      max_durable_per_instance,
    /// The timeout for write.
    DDS::Duration_t max_blocking_time,
    /// The number of chunks that the DataSampleElementAllocator
    /// needs allocate.
    size_t           n_chunks,
    /// Domain ID.
    DDS::DomainId_t  domain_id,
    /// Topic name.
    char const *     topic_name,
    /// Type name.
    char const *     type_name,
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    /// The data durability cache for unsent data.
    DataDurabilityCache * durability_cache,
    /// DURABILITY_SERVICE QoS specific to the DataWriter.
    DDS::DurabilityServiceQosPolicy const & durability_service,
#endif
    /// maximum number of instances, 0 for unlimited
    CORBA::Long      max_instances,
    /// maximum total number of samples, 0 for unlimited
    CORBA::Long      max_total_samples);

  ~WriteDataContainer();

  DDS::ReturnCode_t
  enqueue_control(DataSampleElement* control_sample);

  /**
   * Enqueue the data sample in its instance thread. This method
   * assumes there is an available space for the sample in the
   * instance list.
  */
  DDS::ReturnCode_t
  enqueue(
    DataSampleElement* sample,
    DDS::InstanceHandle_t instance);

  /**
   * Create a resend list with the copies of all current "sending"
   * and "sent" samples. The samples will be sent to the
   *  subscriber specified.
   */
  DDS::ReturnCode_t reenqueue_all(const RepoId& reader_id,
                                  const DDS::LifespanQosPolicy& lifespan
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
                                  ,
                                  const OPENDDS_STRING& filterClassName,
                                  const FilterEvaluator* eval,
                                  const DDS::StringSeq& params
#endif
                                  );

  /**
   * Dynamically allocate a PublicationInstance object and add to
   * the instances_ list.
   *
   * @note: The registered_sample is an input and output parameter.
   *        A shallow copy of the sample data will be given to
   *        datawriter as part of the control message.
   */
  DDS::ReturnCode_t
  register_instance(DDS::InstanceHandle_t&  instance_handle,
                    Message_Block_Ptr&      registered_sample);

  /**
   * Remove the provided instance from the instances_ list.
   * The registered sample data will be released upon the deletion
   * of the PublicationInstance. A shallow copy of the sample data
   * will be given to datawriter as part of the control message if
   * the dup_registered_sample is true.
   *
   * This method returns error if the instance is not registered.
   */
  DDS::ReturnCode_t unregister(
    DDS::InstanceHandle_t handle,
    Message_Block_Ptr&    registered_sample,
    bool dup_registered_sample = true);

  /**
   * Delete the samples for the provided instance.
   * A shallow copy of the sample data will be given to datawriter
   * as part of the control message if the dup_registered_sample
   * is true.
   *
   * This method returns error if the instance is not registered.
   */
  DDS::ReturnCode_t dispose(
    DDS::InstanceHandle_t handle,
    Message_Block_Ptr& registered_sample,
    bool dup_registered_sample = true);

  /**
   * Return the number of samples for the given instance.
   */
  DDS::ReturnCode_t num_samples(
    DDS::InstanceHandle_t handle,
    size_t& size);

  /**
   * Return the number of samples for all instances.
   */
  size_t num_all_samples();

  /**
   * Obtain a list of data that has not yet been sent.  The data
   * on the list returned is moved from the internal unsent_data_
   * list to the internal sending_data_ list as part of this call.
   * The entire list is linked via the
   * DataSampleElement.next_send_sample_ link as well.
   */
   ACE_UINT64 get_unsent_data(SendStateDataSampleList& list);

  /**
   * Obtain a list of data for resending. This is only used when
   * TRANSIENT_LOCAL_DURABILITY_QOS is used. The data on the list
   * returned is not put on any SendStateDataSampleList.
   */
  SendStateDataSampleList get_resend_data();

  /**
   * Acknowledge the delivery of data.  The sample that resides in
   * this container will be moved from sending_data_ list to the
   * internal sent_data_ list. If there are any threads waiting for
   * available space, it wakes up these threads.
   */
  void data_delivered(const DataSampleElement* sample);

  /**
   * This method is called by the transport to notify the sample
   * is dropped.  Which the transport was told to do by the
   * publication code by calling
   * TransportClient::remove_sample(). If the sample was
   * "sending" then it is moved to the "unsent" list. If there are any
   * threads waiting for available space then it needs wake up
   * these threads. The dropped_by_transport flag true indicates
   * the dropping initiated by transport when the transport send
   * strategy is in a MODE_TERMINATED. The dropped_by_transport
   * flag false indicates the dropping is initiated by the
   * remove_sample and data_dropped() is a result of
   * remove_sample().
   */
  void data_dropped(const DataSampleElement* element,
                    bool dropped_by_transport);

  DDS::ReturnCode_t obtain_buffer_for_control(DataSampleElement*& element);

  /**
   * Allocate a DataSampleElement object and check the space
   * availability for newly allocated element according to qos settings.
   * For the blocking write case, if resource limits or history qos limits
   * are reached, then it blocks for max blocking time for a previous sample
   * to be delivered or dropped by the transport. In non-blocking write
   * case, if resource limits or history qos limits are reached, will attempt
   * to remove oldest samples (forcing the transport to drop samples if necessary)
   * to make space.  If there are several threads waiting then
   * the first one in the waiting list can enqueue, others continue
   * waiting. Note: the lock should be held before calling this method
   */
  DDS::ReturnCode_t obtain_buffer(
    DataSampleElement*& element,
    DDS::InstanceHandle_t handle);

  /**
   * Release the memory previously allocated.
   * This method is corresponding to the obtain_buffer method. If
   * the memory is allocated by some allocator then the memory
   * needs to be released to the allocator.
   */
  void release_buffer(DataSampleElement* element);

  /**
   * Unregister all instances managed by this data containers.
   */
  void unregister_all();

  /**
   * @todo remove/document this!
   */
  PublicationInstance_rch get_handle_instance(
    DDS::InstanceHandle_t handle);

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  /**
   * Copy sent data to data DURABILITY cache.
   */
  bool persist_data();
#endif

  /// Reset time interval for each instance.
  void reschedule_deadline();

  /**
   * Block until pending samples have either been delivered
   * or dropped.
   */
  void wait_pending();

  /**
   * Returns a vector of handles for the instances registered for this
   * data writer.
   */
  typedef OPENDDS_VECTOR(DDS::InstanceHandle_t) InstanceHandleVec;
  void get_instance_handles(InstanceHandleVec& instance_handles);

  DDS::ReturnCode_t wait_ack_of_seq(const MonotonicTimePoint& abs_deadline, const SequenceNumber& sequence);

  bool sequence_acknowledged(const SequenceNumber sequence);

private:

  // A class, normally provided by an unit test, that needs access to
  // private methods/members.
  friend class ::DDS_TEST;

  // --------------------------
  // Preventing copying
  // --------------------------
  WriteDataContainer(WriteDataContainer const &);
  WriteDataContainer & operator= (WriteDataContainer const &);
  // --------------------------

  /**
   * Returns if pending data exists.  This includes
   * sending, and unsent data.
   */
  bool pending_data();

  void copy_and_prepend(SendStateDataSampleList& list,
                        const SendStateDataSampleList& appended,
                        const RepoId& reader_id,
                        const DDS::LifespanQosPolicy& lifespan,
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
                        const OPENDDS_STRING& filterClassName,
                        const FilterEvaluator* eval,
                        const DDS::StringSeq& params,
#endif
                        ssize_t& max_resend_samples);

  /**
   * Remove the oldest "n" samples from each instance list that are
   * in a state such that they could only be used for durability
   * purposes (see reenqueue_all).
   * "n" is determined by max_durable_per_instance_, so these samples
   * are truly unneeded -- there are max_durable_per_instance_ newer
   * samples available in the instance.
   */
  void remove_excess_durable();

  /**
   * Remove the oldest sample (head) from the instance history list.
   * This method also updates the internal lists to reflect
   * the change.
   * If the sample is in the unsent_data_ or sent_data_ list then
   * it will be released. If the sample is in the sending_data_ list
   * then the transport will be notified to release the sample, then
   * the sample will be released. Otherwise an error
   * is returned.
   * The "released" boolean value indicates whether the sample is
   * released.
   */
  DDS::ReturnCode_t remove_oldest_sample(
    InstanceDataSampleList& instance_list,
    bool& released);

  /**
   * Called when data has been dropped or delivered and any
   * blocked writers should be notified
   */
  void wakeup_blocking_writers (DataSampleElement* stale);

private:

  void log_send_state_lists (OPENDDS_STRING description);

  DisjointSequence acked_sequences_;

  /// List of data that has not been sent yet.
  SendStateDataSampleList   unsent_data_;

  /// Id used to keep track of which send transaction
  /// DataWriter is currently creating
  ACE_UINT64 transaction_id_;

  /// List of data that is currently being sent.
  SendStateDataSampleList   sending_data_;

  /// List of data that has already been sent.
  SendStateDataSampleList   sent_data_;

  /// List of data that has been released by WriteDataContainer
  /// but is still in process of delivery (or dropping) by transport
  SendStateDataSampleList  orphaned_to_transport_;

  /// The list of all samples written to this datawriter in
  /// writing order.
  WriterDataSampleList   data_holder_;

  /// List of the data reenqueued to support the
  /// TRANSIENT_LOCAL_DURABILITY_QOS policy. It duplicates the
  /// samples in sent and sending list. This
  /// list will be passed to the transport for re-sending.
  SendStateDataSampleList   resend_data_;

  /// The individual instance queue threads in the data.
  PublicationInstanceMapType instances_;

  /// The publication Id from repo.
  PublicationId    publication_id_;

  /// The writer that owns this container.
  DataWriterImpl*  writer_;

  /// The maximum size a container should allow for
  /// an instance sample list
  CORBA::Long                     max_samples_per_instance_;

  CORBA::Long history_depth_;

  /// The maximum number of samples from each instance that
  /// can be added to the resend_data_ for durability.
  CORBA::Long                     max_durable_per_instance_;

  /// The maximum number of instances allowed or zero
  /// to indicate unlimited.
  /// It corresponds to the QoS.RESOURCE_LIMITS.max_instances
  /// when QoS.RELIABILITY.kind == DDS::RELIABLE_RELIABILITY_QOS
  CORBA::Long                     max_num_instances_;

  /// The maximum number of samples allowed or zero
  /// to indicate unlimited.
  /// It corresponds to the QoS.RESOURCE_LIMITS.max_instances
  /// when QoS.RELIABILITY.kind == DDS::RELIABLE_RELIABILITY_QOS
  /// It also covers QoS.RESOURCE_LIMITS.max_samples and
  /// max_instances * max_samples_per_instance
  CORBA::Long                     max_num_samples_;

  /// The maximum time to block on write operation.
  /// This comes from DataWriter's QoS HISTORY.max_blocking_time
  DDS::Duration_t               max_blocking_time_;

  /// The block waiting flag.
  bool                            waiting_on_release_;

  /// This lock is used to protect the container and the map
  /// in the type-specific DataWriter.
  /// This lock can be accessible via the datawriter.
  /// This lock is made to be globally accessible for
  /// performance concern. The lock is acquired as the external
  /// call (e.g. FooDataWriterImpl::write) started and the
  /// same lock will be used by the transport thread to notify
  /// the datawriter the data is delivered. Other internal
  /// operations will not lock.
  ACE_Recursive_Thread_Mutex                lock_;
  ACE_Condition<ACE_Recursive_Thread_Mutex> condition_;
  ACE_Condition<ACE_Recursive_Thread_Mutex> empty_condition_;

  /// Lock used for wait_for_acks() processing.
  ACE_Thread_Mutex wfa_lock_;

  /// Used to block in wait_for_acks().
  ACE_Condition<ACE_Thread_Mutex> wfa_condition_;

  /// The number of chunks that sample_list_element_allocator_
  /// needs initialize.
  size_t n_chunks_;

  /// The cached allocator to allocate DataSampleElement
  /// objects.
  DataSampleElementAllocator sample_list_element_allocator_;

  /// The flag indicates the datawriter will be destroyed.
  bool shutdown_;

  /// Domain ID.
  DDS::DomainId_t const domain_id_;

  /// Topic name.
  char const * const topic_name_;

  /// Type name.
  char const * const type_name_;

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE

  /// Pointer to the data durability cache.
  /**
   * This a pointer to the data durability cache owned by the
   * Service Participant singleton, which means this cache is also
   * a singleton.
   */
  DataDurabilityCache * const durability_cache_;

  /// DURABILITY_SERVICE QoS specific to the DataWriter.
  DDS::DurabilityServiceQosPolicy const & durability_service_;

#endif
};

} /// namespace OpenDDS
} /// namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_WRITE_DATA_CONTAINER_H */
