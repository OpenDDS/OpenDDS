/// -*- C++ -*-
///
/// $Id$
#ifndef TAO_DDS_DCPS_WRITE_DATA_CONTAINER_H
#define TAO_DDS_DCPS_WRITE_DATA_CONTAINER_H


#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsDataWriterRemoteC.h"
#include "DataSampleList.h"
#include "OfferedDeadlineWatchdog.h"

#include "ace/Synch_T.h"

#include <map>
#include <set>
#include <memory>


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace OpenDDS
{
  namespace DCPS
  {
    class DataWriterImpl;
    class DataDurabilityCache;

    typedef std::map< ::DDS::InstanceHandle_t, PublicationInstance*> PublicationInstanceMapType;

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
     * DataSampleListElement.  The data_holder_ holds all
     * DataSampleListElement in the writing order via the
     * next_sample_/previous_sample_ thread. The instance list in
     * PublicationInstance links samples via the next_instance_sample_
     * thread.
     *
     * There are four state transition lists - unsent, sending, sent
     * and released during the data writing. These lists are linked
     * via the next_send_sample_/previous_send_sample_ thread. Any
     * DataSampleListElement should be in one of these four lists and
     * SHOULD NOT be shared between these four lists. A normal
     * transition of a DataSampleListElement would be
     * unsent->sending->sent, but a DataSampleListElement could be
     * moved from sending to released list when the instance reaches
     * maximum samples allowed and the transport is still using the
     * sample. A DataSampleListElement is removed from released or
     * sent list after it's delivered or dropped and is freed when
     * it's removed from release list or when the instance queue needs
     * more space.  The real data sample will be freed when the
     * reference counting goes 0.  The resend list is only used when
     * the datawriter uses TRANSIENT_LOCAL_DURABILITY_QOS. It holds
     * the DataSampleElements for the data sample duplicates of the
     * sending and sent list and are pushed to the released list after
     * giving to the transport.
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
    class OpenDDS_Dcps_Export WriteDataContainer
    {
    public:

      friend class DataWriterImpl;


      /**
       * No default constructor, must be initialized.
       */
      WriteDataContainer(
        /// Depth of the instance sample queue.
        CORBA::Long    depth,
        /// Should the write wait for available space?
        bool           should_block ,
        /// The timeout for write.
        ACE_Time_Value max_blocking_time,
        /// The number of chunks that the DataSampleListElementAllocator
        /// needs allocate.
        size_t         n_chunks,
        /// Domain ID.
        ::DDS::DomainId_t domain_id,
        /// Topic name.
        char const *   topic_name,
        /// Type name.
        char const *   type_name,
        /// The data durability cache for unsent data.
        DataDurabilityCache * durability_cache,
        /// DURABILITY_SERVICE QoS specific to the DataWriter.
        ::DDS::DurabilityServiceQosPolicy const & durability_service,
        std::auto_ptr<OfferedDeadlineWatchdog> & watchdog
        );

      /**
       * Default destructor.
       */
      ~WriteDataContainer();

      /**
       * Enqueue the data sample in its instance thread. This method
       * assumes there is an available space for the sample in the
       * instance list.
      */
      ::DDS::ReturnCode_t
        enqueue (
          DataSampleListElement* sample,
          ::DDS::InstanceHandle_t instance);

      /**
       * Create a resend list with the copies of all current "sending"
       * and "sent" samples. The samples will be sent to the
       *  subscribers specified.
       */
      ::DDS::ReturnCode_t
      reenqueue_all (OpenDDS::DCPS::ReaderIdSeq const & rds,
                     ::DDS::LifespanQosPolicy const & lifespan);

      /**
       * Dynamically allocate a PublicationInstance object and add to
       * the instances_ list.
       *
       * @note: The registered_sample is an input and output parameter.
       *        A shallow copy of the sample data will be given to
       *        datawriter as part of the control message.
       */
      ::DDS::ReturnCode_t
      register_instance (::DDS::InstanceHandle_t&  instance_handle,
			 DataSample*&              registered_sample);

      /**
       * Remove the provided instance from the instances_ list.
       * The registered sample data will be released upon the deletion
       * of the PublicationInstance. A shallow copy of the sample data
       * will be given to datawriter as part of the control message if
       * the dup_registered_sample is true.
       *
       * This method returns error if the instance is not registered.
       */
      ::DDS::ReturnCode_t unregister (
          ::DDS::InstanceHandle_t handle,
          DataSample*& registered_sample,
          DataWriterImpl*         writer,
          bool                    dup_registered_sample = true
        );


      /**
       * Delete the samples for the provided instance.
       * A shallow copy of the sample data will be given to datawriter
       * as part of the control message if the dup_registered_sample
       * is true.
       *
       * This method returns error if the instance is not registered.
       */
      ::DDS::ReturnCode_t dispose (
          ::DDS::InstanceHandle_t handle,
          DataSample*& registered_sample,
          bool                    dup_registered_sample = true
        );

      /**
       * Return the number of samples for the given instance.
       */
      ::DDS::ReturnCode_t num_samples (
          ::DDS::InstanceHandle_t handle,
          size_t&                 size
        );

      /**
       * Obtain a list of data that has not yet been sent.  The data
       * on the list returned is moved from the internal unsent_data_
       * list to the internal sending_data_ list as part of this call.
       * The entire list is linked via the
       * DataSampleListElement.next_send_sample_ link as well.
       */
      DataSampleList get_unsent_data() ;

      /**
       * Obtain a list of data for resending. This is only used when 
       * TRANSIENT_LOCAL_DURABILITY_QOS is used. The data on the list
       * returned is moved from the resend list to the released list
       * as part of this call.
       */
      DataSampleList get_resend_data() ;

      /**
       * Obtain a list of data that are still referencing by the
       * transport but have not been acknowledged by transport as
       * delivered or dropped.
       */
      DataSampleList get_sending_data() ;

      /**
       * Obtain a list of data that has already been sent
       * successfully.
       */
      DataSampleList get_sent_data() ;

      /**
       * Acknowledge the delivery of data.  The sample that resides in
       * this container will be moved from sending_data_ list to the
       * internal sent_data_ list or released from the released_data_
       * list.  If there is any threads waiting for available space
       *  then it needs wake up these threads.
       */
      void data_delivered(DataSampleListElement* sample);

      /**
       * This method is called by the transport to notify the sample
       * is dropped.  Which the transport was told to do by the
       * publication code by calling
       * TransportInterface::remove_sample(). If the sample was
       * "sending" the it is moved to "unsent" list. If the sample was
       * "released" then the sample is released. If there are any
       * threads waiting for available space then it needs wake up
       * these threads. The dropped_by_transport flag true indicates
       * the dropping initiated by transport when the transport send
       * strategy is in a MODE_TERMINATED. The dropped_by_transport
       * flag false indicates the dropping is initiated by the
       * remove_sample and data_dropped() is a result of
       * remove_sample().
       */
      void data_dropped (DataSampleListElement* element,
			 bool dropped_by_transport);

      /**
       * Allocate a DataSampleListElement object and check the space
       * avaliability in the instance list for newly allocated element.
       * For the blocking write case, if the instance list size reaches
       * depth_, then the new element will be added to the waiting list
       * and is blocked until a previous sample is delivered or dropped
       * by the transport. If there are several threads waiting then
       * the first one in the waiting list can enqueue, others continue
       * waiting.
       * For the non-blocking write case, it asks the transport to drop
       * the oldest sample.
       */
      ::DDS::ReturnCode_t obtain_buffer (
        DataSampleListElement*& element,
        ::DDS::InstanceHandle_t handle,
        DataWriterImpl*         writer);

      /**
       * Release the memory previously allocated.
       * This method is corresponding to the obtain_buffer method. If
       * the memory is allocated by some allocator then the memory
       * needs to be released to the allocator.
       */
      void release_buffer (DataSampleListElement* element);

      /**
       * Unregister all instances managed by this data containers.
       */
      void unregister_all (DataWriterImpl* writer);

      /**
       * @todo remove/document this!
       */
      PublicationInstance* get_handle_instance (
          ::DDS::InstanceHandle_t handle);


      /**
       * Copy sent data to data DURABILITY cache.
       */
      bool persist_data ();

      // Reset time interval for each instance.
      void reschedule_deadline ();

    private:

      // --------------------------
      // Preventing copying
      // --------------------------
      WriteDataContainer (WriteDataContainer const &);
      WriteDataContainer & operator= (WriteDataContainer const &);
      // --------------------------

      void copy_and_append (DataSampleList& list, 
                            DataSampleList const & appended, 
                            OpenDDS::DCPS::ReaderIdSeq const & rds,
                            ::DDS::LifespanQosPolicy const & lifespan);

      /**
       * Remove the oldest sample (head) from the instance history list.
       * This method also updates the internal lists to reflect
       * the change.
       * If the sample is in the unsent_data_ or sent_data_ list then
       * it will be released. If the sample is in the sending_data_ list
       * then it will be moved to released_data_ list. Otherwise an error
       * is returned.
       * The "released" boolean value indicates whether the sample is
       * released.
       */
      ::DDS::ReturnCode_t remove_oldest_sample (
        DataSampleList& instance_list,
        bool& released);

      /**
       * Get an instance handle for a new instance.
       * This method should be called under the protection of a lock
       * to ensure that the handle is unique for the container.
       */
      ::DDS::InstanceHandle_t get_next_handle ();

    private:

      /// List of data that has not been sent yet.
      DataSampleList   unsent_data_ ;

      /// List of data that is currently being sent.
      DataSampleList   sending_data_ ;

      /// List of data that has already been sent.
      DataSampleList   sent_data_ ;

      /// List of data that has been released, but it
      /// still in use externally (by the transport).
      DataSampleList   released_data_ ;

      /// The list of all samples written to this datawriter in
      /// writing order.
      DataSampleList   data_holder_;

      /// List of the data reenqueued to support the
      /// TRANSIENT_LOCAL_DURABILITY_QOS policy. It duplicates the
      /// samples in sent and sending list. These
      /// DataSampleListElement will be appended to released_data_
      /// list after passing to the transport. 
      DataSampleList   resend_data_ ;

      /// The individual instance queue threads in the data.
      PublicationInstanceMapType instances_;

      /// The publication Id from repo.
      PublicationId    publication_id_ ;

      /// The maximum size of an instance sample list which are to
      /// be maintained in the container.
      /// It corresponds to the QoS.HISTORY.depth value for
      /// QoS.HISTORY.kind==KEEP_LAST and corresponds to the
      /// QoS.RESOURCE_LIMITS.max_samples_per_instance for
      /// the case of QoS.HISTORY.kind==KEEP_ALL.
      CORBA::Long                     depth_;

      /// Flag to indicate whether the write operation should block
      /// to wait for space to become avaliable.
      /// Is true when the DataWriter's QoS has RELIABILITY.kind=RELIABLE
      /// and HISTORY.kind=KEEP_ALL.
      bool                            should_block_;

      /// The maximum time to block on write operation.
      /// This comes from DataWriter's QoS HISTORY.max_blocking_time
      ACE_Time_Value                  max_blocking_time_;

      /// The block waiting flag.
      bool                            waiting_on_release_;

      /// This lock is used to protect the container and the map
      /// in the type-specific DataWriter.
      /// This lock can be accessiable via the datawriter.
      /// This lock is made to be globally accessable for
      /// performance concern. The lock is acquired as the external
      /// call (e.g. FooDataWriterImpl::write) started and the
      /// same lock will be used by the transport thread to notify
      /// the datawriter the data is delivered. Other internal
      /// operations will not lock.
      ACE_Recursive_Thread_Mutex                lock_;
      ACE_Condition<ACE_Recursive_Thread_Mutex> condition_;

      /// The number of chunks that sample_list_element_allocator_
      /// needs initialize.
      size_t                                    n_chunks_;

      /// The cached allocator to allocate DataSampleListElement
      /// objects.
      DataSampleListElementAllocator sample_list_element_allocator_;

      /// The allocator for TransportSendElement.
      /// The TransportSendElement allocator is put here because it
      /// needs the number of chunks information that WriteDataContainer
      /// has.
      TransportSendElementAllocator  transport_send_element_allocator_;

      /// The flag indicates the datawriter will be destroyed.
      bool  shutdown_;

      /// The instance handle for the next new instance.
      ::DDS::InstanceHandle_t next_handle_;

      /// Domain ID.
      ::DDS::DomainId_t const domain_id_;

      /// Topic name.
      char const * const topic_name_;

      /// Type name.
      char const * const type_name_;

      /// Pointer to the data durability cache.
      /**
       * This a pointer to the data durability cache owned by the
       * Service Participant singleton, which means this cache is also
       * a singleton.
       */
      DataDurabilityCache * const durability_cache_;

      /// DURABILITY_SERVICE QoS specific to the DataWriter.
      ::DDS::DurabilityServiceQosPolicy const & durability_service_;

      std::auto_ptr<OfferedDeadlineWatchdog> & watchdog_;
    };

  } /// namespace OpenDDS
} /// namespace DCPS

#endif /* TAO_DDS_DCPS_WRITE_DATA_CONTAINER_H */
