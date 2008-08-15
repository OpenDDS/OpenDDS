// -*- C++ -*-
//
// $Id$


#ifndef TAO_DDS_DCPS_DATAWRITER_H
#define TAO_DDS_DCPS_DATAWRITER_H

#include "dds/DdsDcpsPublicationS.h"
#include "dds/DdsDcpsDataWriterRemoteS.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "WriteDataContainer.h"
#include "Definitions.h"
#include "DataSampleList.h"
#include "DataSampleHeader.h"
#include "TopicImpl.h"
#include "AssociationData.h"

#include "ace/Event_Handler.h"

#include <map>
#include <memory>
#include <set>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

namespace OpenDDS
{
  namespace DCPS
  {
    class PublisherImpl;
    class DomainParticipantImpl;
    class OfferedDeadlineWatchdog;

    /**
    * @class DataWriterImpl
    *
    * @brief Implements the ::OpenDDS::DCPS::DataWriterRemote interfaces and
    *        ::DDS::DataWrite interfaces.
    *
    * See the DDS specification, OMG formal/04-12-02, for a description of
    * the interface this class is implementing.
    *
    * This class must be inherited by the type-specific datawriter which
    * is specific to the data-type associated with the topic.
    *
    * @note: This class is responsiable for allocating memory for the
    *        header message block
    *        (MessageBlock + DataBlock + DataSampleHeader) and the
    *        DataSampleListElement.
    *        The data-type datawriter is responsiable for allocating
    *        memory for the sample data message block.
    *        (e.g. MessageBlock + DataBlock + Foo data). But it gives
    *        up ownership to this WriteDataContainer.
    */
    class OpenDDS_Dcps_Export DataWriterImpl
      : public virtual DDS::DataWriter,
        public virtual EntityImpl,
        public virtual TransportSendListener,
        public virtual ACE_Event_Handler
      // DataWriterLocal is conceptual - no need to implement - just
      // use DataWriterImpl
      // public virtual OpenDDS::DCPS::DataWriterLocal
    {
    public:

      ///Constructor
      DataWriterImpl (void);

      ///Destructor
      virtual ~DataWriterImpl (void);

      virtual ::DDS::ReturnCode_t set_qos (const ::DDS::DataWriterQos & qos)
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual void get_qos (::DDS::DataWriterQos & qos)
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::ReturnCode_t set_listener (
          ::DDS::DataWriterListener_ptr a_listener,
          ::DDS::StatusKindMask mask)
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::DataWriterListener_ptr get_listener ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::Topic_ptr get_topic ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::Publisher_ptr get_publisher ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::LivelinessLostStatus get_liveliness_lost_status ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::OfferedDeadlineMissedStatus get_offered_deadline_missed_status ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::OfferedIncompatibleQosStatus * get_offered_incompatible_qos_status ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::PublicationMatchStatus get_publication_match_status ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual void assert_liveliness ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::ReturnCode_t get_matched_subscriptions (
          ::DDS::InstanceHandleSeq & subscription_handles)
        ACE_THROW_SPEC ((CORBA::SystemException));

#if !defined (DDS_HAS_MINIMUM_BIT)
      virtual ::DDS::ReturnCode_t get_matched_subscription_data (
          ::DDS::SubscriptionBuiltinTopicData & subscription_data,
          ::DDS::InstanceHandle_t subscription_handle)
        ACE_THROW_SPEC ((CORBA::SystemException));
#endif // !defined (DDS_HAS_MINIMUM_BIT)

      virtual ::DDS::ReturnCode_t enable ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual ::DDS::StatusKindMask get_status_changes ()
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual void add_associations (::OpenDDS::DCPS::RepoId yourId,
                                     const ReaderAssociationSeq & readers)
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual void remove_associations (const ReaderIdSeq & readers,
                                        ::CORBA::Boolean callback)
        ACE_THROW_SPEC ((CORBA::SystemException));

      virtual void update_incompatible_qos (
          const OpenDDS::DCPS::IncompatibleQosStatus & status)
        ACE_THROW_SPEC ((CORBA::SystemException));

      /**
       * cleanup the DataWriter.
       */
      void cleanup ();

      /**
       * Initialize the data members.
       */
      virtual void init (
          ::DDS::Topic_ptr                       topic,
          TopicImpl*                             topic_servant,
          const ::DDS::DataWriterQos &           qos,
          ::DDS::DataWriterListener_ptr          a_listener,
          OpenDDS::DCPS::DomainParticipantImpl*  participant_servant,
          OpenDDS::DCPS::PublisherImpl*          publisher_servant,
          ::DDS::DataWriter_ptr                  dw_local,
          OpenDDS::DCPS::DataWriterRemote_ptr    dw_remote)
        ACE_THROW_SPEC ((CORBA::SystemException));

      /**
       * Delegate to the WriteDataContainer to register and tell
       * the transport to broadcast the registered instance.
       */
      ::DDS::ReturnCode_t
      register_instance (::DDS::InstanceHandle_t& handle,
                         DataSample* data,
                         const ::DDS::Time_t & source_timestamp)
        ACE_THROW_SPEC ((CORBA::SystemException));

      /**
       * Delegate to the WriteDataContainer to unregister and tell
       * the transport to broadcast the unregistered instance.
       */
      ::DDS::ReturnCode_t unregister (::DDS::InstanceHandle_t handle,
                                      const ::DDS::Time_t & source_timestamp)
        ACE_THROW_SPEC ((CORBA::SystemException));

      /**
       * Delegate to the WriteDataContainer to queue the instance
       * sample and finally tell the transport to send the sample.
       */
      ::DDS::ReturnCode_t write (DataSample* sample,
                                 ::DDS::InstanceHandle_t handle,
                                 const ::DDS::Time_t & source_timestamp)
        ACE_THROW_SPEC ((CORBA::SystemException));

      /**
       * Delegate to the WriteDataContainer to dispose all data
       * samples for a given instance and tell the transport to
       * broadcast the disposed instance.
       */
      ::DDS::ReturnCode_t dispose (::DDS::InstanceHandle_t handle,
                                   const ::DDS::Time_t & source_timestamp)
        ACE_THROW_SPEC ((CORBA::SystemException));

      /**
       * Return the number of samples for a given instance.
       */
      ::DDS::ReturnCode_t num_samples (::DDS::InstanceHandle_t handle,
                                       size_t&                 size);

      /**
       * Retreive the unsent data from the WriteDataContainer.
       */
      DataSampleList get_unsent_data ();
      DataSampleList get_resend_data ();

      /**
       * Cache the publication repository id after adding
       * datawriter/publication to repository.
       */
      void set_publication_id (RepoId publication_id);

      /**
       * Accessor of the repository id of this datawriter/publication.
       */
      RepoId get_publication_id ();

      /**
       * Delegate to WriteDataContainer to unregister all instances.
       */
      void unregister_all ();

      /**
       * This is called by transport to notify that the sample is
       * delivered and it is delegated to WriteDataContainer
       * to adjust the internal data sample threads.
       */
      void data_delivered (DataSampleListElement* sample);

      /**
       * This is called by transport to notify that the control
       * message is delivered.
       */
      void control_delivered (ACE_Message_Block* sample);

      /**
       * Accessor of the associated topic name.
       */
      const char* get_topic_name ();

      /**
       * Get associated topic type name.
       */
      char const * get_type_name () const;

      /**
       * This method is called when there is no more space in the
       * instance sample list for a non-blocking write. It requests
       * the transport to drop the oldest sample.
       *
       * The dropped_by_transport parameter will be passed all way to
       * the transport and is used when the data_dropped() is called
       * back.
       *
       * @see WriterDataContainer::data_dropped() comment for the
       *      dropped_by_transport parameter.
       */
      void remove_sample(DataSampleListElement* element,
                         bool dropped_by_transport = false);

      /**
       * This mothod is called by transport to notify the instance
       * sample is dropped and it delegates to WriteDataContainer
       * to update the internal list.
       */
      void data_dropped (DataSampleListElement* element,
                         bool dropped_by_transport);

      /**
       * This is called by transport to notify that the control
       * message is dropped.
       */
      void control_dropped (ACE_Message_Block* sample,
                            bool dropped_by_transport);

      /**
       * Tell transport to remove all control messages requested
       * by this datawriter.
       * This is called during datawriter shutdown.
       */
      int remove_all_control_msgs();

      /**
       * Accessor of the WriterDataContainer's lock.
       */
      // ciju: Seems this is no longer being used.
      // Was wrong. Still required.
      ACE_INLINE
      ACE_Recursive_Thread_Mutex&      get_lock ()
      {
        return data_container_->lock_;
      }

      /**
       * This method is called when an instance is unregistered from
       * the WriteDataContainer.
       *
       * The subclass must provide the implementation to unregister
       * the instance from its own map.
       */
      virtual void unregistered (::DDS::InstanceHandle_t instance_handle);

      /**
       * This is used to retrieve the listener for a certain status
       * change.
       *
       * If this datawriter has a registered listener and the status
       * kind is in the listener mask then the listener is returned.
       * Otherwise, the query for the listener is propagated up to the
       * factory/publisher.
       */
      ::DDS::DataWriterListener* listener_for (::DDS::StatusKind kind);

      /// Handle the assert liveliness timeout.
      virtual int handle_timeout (const ACE_Time_Value &tv,
                                  const void *arg);

      virtual int handle_close (ACE_HANDLE,
                                ACE_Reactor_Mask);

      void remove_all_associations();

      void notify_publication_disconnected (const ReaderIdSeq& subids);
      void notify_publication_reconnected (const ReaderIdSeq& subids);
      void notify_publication_lost (const ReaderIdSeq& subids);

      void notify_connection_deleted ();

      /// Statistics counter.
      int         data_dropped_count_;
      int         data_delivered_count_;
      int         control_dropped_count_;
      int         control_delivered_count_;

      /// Called by transport after transport received the
      /// FULLY_ASSOCIATED ack from the associated subscriber.
      void fully_associated (::OpenDDS::DCPS::RepoId yourId,
                             size_t                  num_remote_associations,
                             const AssociationData*  remote_associations);

      /**
       * This method create a header message block and chain with
       * the sample data. The header contains the information
       * needed. e.g. message id, length of whole message...
       * The fast allocator is used to allocate the message block,
       * data block and header.
       */
      ::DDS::ReturnCode_t
      create_sample_data_message (DataSample* data,
                                  ::DDS::InstanceHandle_t instance_handle,
                                  ACE_Message_Block*& message,
                                  const ::DDS::Time_t& source_timestamp);

      /// Make sent data available beyond the lifetime of this
      /// @c DataWriter.
      bool persist_data ();

    protected:

      /**
       * Accessor of the cached publisher servant.
       */
      PublisherImpl* get_publisher_servant ();

      // type specific DataWriter's part of enable.
      virtual ::DDS::ReturnCode_t enable_specific ()
        ACE_THROW_SPEC ((CORBA::SystemException)) = 0;

      /// The number of chunks for the cached allocator.
      size_t                     n_chunks_;

      /// The multiplier for allocators affected by associations
      size_t                     association_chunk_multiplier_;

      /**
       *  Attempt to locate an existing instance for the given handle.
       */
      PublicationInstance* get_handle_instance (
        ::DDS::InstanceHandle_t handle);

    private:

      void notify_publication_lost (const ::DDS::InstanceHandleSeq& handles);

      /**
       * This method create a header message block and chain with
       * the registered sample. The header contains the information
       * needed. e.g. message id, length of whole message...
       * The fast allocator is not used for the header.
       */
      ACE_Message_Block*
      create_control_message (enum MessageId message_id,
                              ACE_Message_Block* data,
                              const ::DDS::Time_t& source_timestamp);

      /// Send the liveliness message.
      void send_liveliness (const ACE_Time_Value& now);

      /// Lookup the instance handles by the subscription repo ids
      /// via the bit datareader.
      bool bit_lookup_instance_handles (const ReaderIdSeq& ids,
                                         ::DDS::InstanceHandleSeq & hdls);
 
      /// Lookup the cache to get the instance handle by the
      /// subscription repo ids.
      bool cache_lookup_instance_handles (const ReaderIdSeq& ids,
                                         ::DDS::InstanceHandleSeq & hdls);

      friend class ::DDS_TEST; // allows tests to get at dw_remote_objref_

      /// The name of associated topic.
      CORBA::String_var               topic_name_;
      /// The type name of associated topic.
      CORBA::String_var               type_name_;
      /// The associated topic repository id.
      RepoId                          topic_id_;
      /// The object reference of the associated topic.
      ::DDS::Topic_var                topic_objref_;
      /// The topic servant.
      TopicImpl*                      topic_servant_;
      /// The qos policy list of this datawriter.
      ::DDS::DataWriterQos            qos_;
      /// The StatusKind bit mask indicates which status condition change
      /// can be notified by the listener of this entity.
      ::DDS::StatusKindMask           listener_mask_;
      /// Used to notify the entity for relevant events.
      ::DDS::DataWriterListener_var   listener_;
      /// The datawriter listener servant.
      ::DDS::DataWriterListener*  fast_listener_;
      /// The participant servant which creats the publisher that
      /// creates this datawriter.
      DomainParticipantImpl*          participant_servant_;
      /// The domain id.
      ::DDS::DomainId_t               domain_id_;
      /// The publisher servant which creates this datawrite.
      PublisherImpl*                  publisher_servant_;
      /// the object reference of the local datawriter
      ::DDS::DataWriter_var           dw_local_objref_;
      /// The object reference of the remote datawriter.
      ::OpenDDS::DCPS::DataWriterRemote_var dw_remote_objref_;
      /// The repository id of this datawriter/publication.
      PublicationId                   publication_id_;
      /// The sequence number unique in DataWriter scope.
      /// Not used in first implementation.
      SequenceNumber                  sequence_number_;
      /// The sample data container.
      WriteDataContainer*             data_container_;
      /// The lock to protect the activate subscriptions
      /// and status changes.
      ACE_Recursive_Thread_Mutex                lock_;

      typedef std::map<RepoId, DDS::InstanceHandle_t, GUID_tKeyLessThan> RepoIdToHandleMap;

      RepoIdToHandleMap               id_to_handle_map_;

      typedef std::set<RepoId, GUID_tKeyLessThan> IdSet;

      IdSet                           readers_;

      /// Status conditions.
      ::DDS::LivelinessLostStatus         liveliness_lost_status_ ;
      ::DDS::OfferedDeadlineMissedStatus  offered_deadline_missed_status_ ;
      ::DDS::OfferedIncompatibleQosStatus offered_incompatible_qos_status_ ;
      ::DDS::PublicationMatchStatus       publication_match_status_ ;

      /**
       * @todo The publication_lost_status_ and
       *       publication_reconnecting_status_ are left here for
       *       future use when we add get_publication_lost_status()
       *       and get_publication_reconnecting_status() methods.
       */
      // Statistics of the lost publications due to lost connection.
      // PublicationLostStatus               publication_lost_status_;
      // Statistics of the publications that associates with a
      // reconnecting datalink.
      // PublicationReconnectingStatus       publication_reconnecting_status_;

      /// The message block allocator.
      MessageBlockAllocator*     mb_allocator_;
      /// The data block allocator.
      DataBlockAllocator*        db_allocator_;
      /// The header data allocator.
      DataSampleHeaderAllocator* header_allocator_;

      /// The orb's reactor to be used to register the liveliness
      /// timer.
      ACE_Reactor*               reactor_;
      /// The time interval for sending liveliness message.
      ACE_Time_Value             liveliness_check_interval_;
      /// Timestamp of last write/dispose/assert_liveliness.
      ACE_Time_Value             last_liveliness_activity_time_;
      /// Total number of offered deadlines missed during last offered
      /// deadline status check.
      CORBA::Long last_deadline_missed_total_count_;
      /// Watchdog responsible for reporting missed offered
      /// deadlines.
      std::auto_ptr<OfferedDeadlineWatchdog> watchdog_;
      /// The flag indicates whether the liveliness timer is scheduled and
      /// needs be cancelled.
      bool                       cancel_timer_;

      /// Flag indicates that this datawriter is a builtin topic
      /// datawriter.
      bool                       is_bit_;

      /// Flag indicates that the init() is called.
      bool                       initialized_;

      IdSet                  pending_readers_;
   };

  } // namespace DCPS
} // namespace OpenDDS

#endif /* DDSDCPSPUBLICATION_DATAWRITER_H_  */
