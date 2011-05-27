// -*- C++ -*-
//
// $Id$

#ifndef TAO_DDS_DCPS_DATAREADER_H
#define TAO_DDS_DCPS_DATAREADER_H

#include  "EntityImpl.h"
#include  "dds/DdsDcpsTopicC.h"
#include  "dds/DdsDcpsSubscriptionS.h"
#include  "dds/DdsDcpsDomainC.h"
#include  "dds/DdsDcpsTopicC.h"
#include  "Definitions.h"
#include  "TopicImpl.h"
#include  "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include  "dds/DCPS/transport/framework/TransportReceiveListener.h"
#include  "SubscriptionInstance.h"
#include  "SubscriberImpl.h"
#include  "InstanceState.h"
#include  "DomainParticipantImpl.h"
#include  "Cached_Allocator_With_Overflow_T.h"


#include  "ace/String_Base.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO 
{
  namespace DCPS 
  {
    class SubscriberImpl;
    class DomainParticipantImpl;
    class SubscriptionInstance ;
    
    typedef Cached_Allocator_With_Overflow<ReceivedDataElement, ACE_Null_Mutex>
                ReceivedDataAllocator;

    /// Keeps track of a DataWriter's liveliness for a DataReader.
    class WriterInfo {
      public:
        WriterInfo (); // needed for Hash_Map_Manager

        WriterInfo (DataReaderImpl* reader,
                    PublicationId   writer_id);

        /// check to see if this writer is alive (called by handle_timeout).
        /// @param next_timeout next time this DataWriter will become not active (not alive)
        ///      if no sample or liveliness message is received.
        /// @returns absolute time when the Writer will become not active (if no activity)
        ///          of ACE_Time_Value::zero if the writer is already or became not alive
        ACE_Time_Value check_activity (const ACE_Time_Value& now);

        /// called when a sample or other activity is received from this writer.
        int received_activity (const ACE_Time_Value& when);

        /// returns 1 if the DataWriter is lively; otherwise returns 0.
        int is_alive () { return is_alive_; };

      private:
        /// Timestamp of last write/dispose/assert_liveliness from this DataWriter
        ACE_Time_Value last_liveliness_activity_time_;

        /// 1 if the DataWrite is "alive"
        int is_alive_;

        /// The DataReader owning this WriterInfo
        DataReaderImpl* reader_;

        /// DCPSInfoRepo ID of the DataWriter
        PublicationId writer_id_;
      };



    /**
    * @class DataReaderImpl
    *
    * @brief Implements the ::TAO::DCPS::ReaderRemote interfaces and 
    *        ::DDS::DataReader interfaces.
    *
    * See the DDS specification, OMG formal/04-12-02, for a description of  
    * the interface this class is implementing.
    * 
    * This class must be inherited by the type-specific datareader which 
    * is specific to the data-type associated with the topic.
    *
    */
    class TAO_DdsDcps_Export DataReaderImpl 
      : public virtual POA_TAO::DCPS::DataReaderRemote,
        public virtual EntityImpl,
        public virtual TransportReceiveListener,
        public virtual ACE_Event_Handler
    {
    public:

      typedef std::set< ::DDS::InstanceHandle_t > SubscriptionInstances;
      
      //Constructor 
      DataReaderImpl (void);
      
      //Destructor 
      virtual ~DataReaderImpl (void);
      

      virtual void add_associations (
          ::TAO::DCPS::RepoId yourId,
          const TAO::DCPS::WriterAssociationSeq & writers
          ACE_ENV_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual void remove_associations (
          const TAO::DCPS::WriterIdSeq & writers
          ACE_ENV_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual void update_incompatible_qos (
          const TAO::DCPS::IncompatibleQosStatus & status
          ACE_ENV_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

    /**
    * This is used to retrieve the listener for a certain status change.
    * If this datareader has a registered listener and the status kind
    * is in the listener mask then the listener is returned.
    * Otherwise, the query for the listener is propagated up to the 
    * factory/subscriber.
    */
    ::POA_DDS::DataReaderListener* listener_for (::DDS::StatusKind kind);

    
    /// Handle the assert liveliness timeout.
    virtual int handle_timeout (const ACE_Time_Value &tv,
                                const void *arg);

    /// tell instances when a DataWriter transitions to being alive
    void writer_became_alive (PublicationId         writer_id,
                              const ACE_Time_Value& when);

    /// tell instances when a DataWriter transitions to NOT_ALIVE
    void writer_became_dead (PublicationId         writer_id,
                             const ACE_Time_Value& when);

    virtual int handle_close (ACE_HANDLE,
                              ACE_Reactor_Mask);

    /**
     * cleanup the DataWriter.
     */
    void cleanup ();

    virtual void init (
        TopicImpl*                    a_topic,
        const ::DDS::DataReaderQos &  qos,
        ::DDS::DataReaderListener_ptr a_listener,
        DomainParticipantImpl*        participant,
        SubscriberImpl*               subscriber,
        ::DDS::Subscriber_ptr         subscriber_objref,
        DataReaderRemote_ptr          dr_remote_objref
        ACE_ENV_ARG_DECL
      )
        ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

      virtual ::DDS::ReturnCode_t delete_contained_entities (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::ReturnCode_t set_qos (
          const ::DDS::DataReaderQos & qos
          ACE_ENV_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual void get_qos (
          ::DDS::DataReaderQos & qos
          ACE_ENV_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::ReturnCode_t set_listener (
          ::DDS::DataReaderListener_ptr a_listener,
          ::DDS::StatusKindMask mask
          ACE_ENV_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::DataReaderListener_ptr get_listener (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::TopicDescription_ptr get_topicdescription (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::Subscriber_ptr get_subscriber (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::SampleRejectedStatus get_sample_rejected_status (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::LivelinessChangedStatus get_liveliness_changed_status (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::RequestedDeadlineMissedStatus get_requested_deadline_missed_status (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::RequestedIncompatibleQosStatus * get_requested_incompatible_qos_status (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::SubscriptionMatchStatus get_subscription_match_status (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::SampleLostStatus get_sample_lost_status (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::ReturnCode_t wait_for_historical_data (
          const ::DDS::Duration_t & max_wait
          ACE_ENV_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::ReturnCode_t get_matched_publications (
          ::DDS::InstanceHandleSeq & publication_handles
          ACE_ENV_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::ReturnCode_t get_matched_publication_data (
          ::DDS::PublicationBuiltinTopicData & publication_data,
          ::DDS::InstanceHandle_t publication_handle
          ACE_ENV_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::ReturnCode_t enable (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));

      virtual ::DDS::StatusKindMask get_status_changes (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        ));
      
      /// update liveliness info for this writer.
      void writer_activity(PublicationId writer_id);

      /// process a message that has been received - could be control or a data sample.
      virtual void data_received(const ReceivedDataSample& sample) ;

      SubscriberImpl* get_subscriber_servant ();

      RepoId DataReaderImpl::get_subscription_id() const ;
      void set_subscription_id(RepoId subscription_id) ;

      ::DDS::DataReader_ptr get_dr_obj_ref();
      //virtual TAO::DCPS::DataReaderRemote_ptr get_datareaderremote_obj_ref () = 0;

      char *get_topic_name() const { return topic_servant_->get_name() ; }

      bool have_sample_states(::DDS::SampleStateMask sample_states) const ;
      bool have_view_states(::DDS::ViewStateMask view_states) const ;
      bool have_instance_states(::DDS::InstanceStateMask instance_states) const;
/*
      virtual void demarshal(const ReceivedDataSample& sample) = 0 ;
*/
      virtual void demarshal(const ReceivedDataSample& sample) ;
      virtual void dispose(const ReceivedDataSample& sample) ;

      CORBA::Long get_depth() const { return depth_ ; }
      size_t get_n_chunks() const { return n_chunks_ ; }

      void liveliness_lost() ;

    protected:

      // type specific DataReader's part of enable.
      virtual ::DDS::ReturnCode_t enable_specific (
          ACE_ENV_SINGLE_ARG_DECL
        )
        ACE_THROW_SPEC ((
          CORBA::SystemException
        )) = 0;

      void sample_info(::DDS::SampleInfoSeq & info_seq,
                       size_t start_idx, size_t count,
                       ReceivedDataElement *ptr) ;

      CORBA::Long total_samples() const ;
      
      void set_sample_lost_status(const ::DDS::SampleLostStatus& status) ;
      void set_sample_rejected_status(
              const ::DDS::SampleRejectedStatus& status) ;

      SubscriptionInstances           instances_ ;

      ReceivedDataAllocator          *rd_allocator_ ;
      ::DDS::DataReaderQos            qos_;
      
      ::DDS::SampleRejectedStatus sample_rejected_status_;
      ::DDS::SampleLostStatus sample_lost_status_;

      /// lock protecting sample container as well as statuses.
      ACE_Recursive_Thread_Mutex                sample_lock_;

    private:
      friend class WriterInfo;

      TopicImpl*                      topic_servant_;
      ::DDS::TopicDescription_var     topic_desc_ ;
      ::DDS::StatusKindMask           listener_mask_;
      ::DDS::DataReaderListener_var   listener_;
      ::POA_DDS::DataReaderListener*  fast_listener_;
      DomainParticipantImpl*          participant_servant_;
      ::DDS::DomainId_t               domain_id_;
      SubscriberImpl*                 subscriber_servant_;
      ::DDS::Subscriber_var           subscriber_objref_;
      DataReaderRemote_var            dr_remote_objref_;
      RepoId                          subscription_id_;

      CORBA::Long                     depth_ ;
      size_t                          n_chunks_ ;

      ACE_Recursive_Thread_Mutex                publication_handle_lock_ ;

      ::DDS::InstanceHandleSeq        publication_handles_;

      // Status conditions.
      ::DDS::LivelinessChangedStatus      liveliness_changed_status_ ;
      ::DDS::RequestedDeadlineMissedStatus  requested_deadline_missed_status_ ;
      ::DDS::RequestedIncompatibleQosStatus requested_incompatible_qos_status_ ;
      // YD: should this be publication_match_status_ ?
      ::DDS::SubscriptionMatchStatus         subscription_match_status_ ;


      /// The orb's reactor to be used to register the liveliness 
      /// timer.
      ACE_Reactor*               reactor_;
      /// The time interval for checking liveliness.
      ACE_Time_Value             liveliness_lease_duration_;
      /// liveliness timer id; -1 if no timer is set
      int liveliness_timer_id_;
      
      /// Flag indicates that this datareader is a builtin topic
      /// datareader.
      bool                       is_bit_;

      /// Flag indicates that the init() is called.
      bool                       initialized_;

      typedef ACE_Hash_Map_Manager_Ex<PublicationId,
                                      WriterInfo,
                                      ACE_Hash<PublicationId>,
                                      ACE_Equal_To<PublicationId>,
                                      ACE_Null_Mutex>        WriterMapType;


      /// publications writing to this reader.
      WriterMapType writers_;
    };

  } // namespace DCPS
} // namespace TAO

#if defined (__ACE_INLINE__)
# include "DataReaderImpl.inl"
#endif  /* __ACE_INLINE__ */


#endif /* TAO_DDS_DCPS_DATAREADER_H  */
