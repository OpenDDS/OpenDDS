// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h"
#include  "DataReaderImpl.h"
#include  "tao/ORB_Core.h"
#include  "SubscriptionInstance.h"
#include  "ReceivedDataElementList.h"
#include  "DomainParticipantImpl.h"
#include  "Service_Participant.h"
#include  "Qos_Helper.h"
#include  "TopicImpl.h"
#include  "SubscriberImpl.h"
#include  "BuiltInTopicUtils.h"
#include  "ace/Reactor.h"

#if !defined (__ACE_INLINE__)
# include "DataReaderImpl.inl"
#endif /* ! __ACE_INLINE__ */

namespace TAO
{
  namespace DCPS
  {
    
    DataReaderImpl::DataReaderImpl (void) :
        rd_allocator_(0),
        qos_ (TheServiceParticipant->initial_DataReaderQos ()),
        topic_servant_ (0),
        topic_desc_(0),
        listener_mask_(DEFAULT_STATUS_KIND_MASK),
        fast_listener_ (0),
        participant_servant_ (0),
        domain_id_ (0),
        subscriber_servant_(0),
        subscription_id_ (0),
        n_chunks_ (TheServiceParticipant->n_chunks ()),
        reactor_(0),
        liveliness_lease_duration_ (ACE_Time_Value::zero),
        liveliness_timer_id_(-1),
        is_bit_ (false),
        initialized_ (false)
      {
        CORBA::ORB_var orb = TheServiceParticipant->get_ORB ();
        reactor_ = orb->orb_core()->reactor();

        liveliness_changed_status_.active_count = 0 ;
        liveliness_changed_status_.inactive_count = 0 ;
        liveliness_changed_status_.active_count_change = 0 ;
        liveliness_changed_status_.inactive_count_change = 0 ;

        requested_deadline_missed_status_.total_count = 0 ;
        requested_deadline_missed_status_.total_count_change = 0 ;
        requested_deadline_missed_status_.last_instance_handle =
            ::DDS::HANDLE_NIL ;

        requested_incompatible_qos_status_.total_count = 0 ;
        requested_incompatible_qos_status_.total_count_change = 0 ;
        requested_incompatible_qos_status_.last_policy_id = 0 ;
        requested_incompatible_qos_status_.policies.length(0) ;

        subscription_match_status_.total_count = 0 ;
        subscription_match_status_.total_count_change = 0 ;
        subscription_match_status_.last_publication_handle =
            ::DDS::HANDLE_NIL ;

        sample_lost_status_.total_count = 0 ;
        sample_lost_status_.total_count_change = 0 ;

        sample_rejected_status_.total_count = 0 ;
        sample_rejected_status_.total_count_change = 0 ;
        sample_rejected_status_.last_reason =
            ::DDS::REJECTED_BY_INSTANCE_LIMIT ;
        sample_rejected_status_.last_instance_handle = ::DDS::HANDLE_NIL ;
     }
      
    // This method is called when there are no longer any reference to the 
    // the servant.
    DataReaderImpl::~DataReaderImpl (void)
      {
        if (initialized_)
          {
            participant_servant_->_remove_ref ();
            subscriber_servant_->_remove_ref ();
            topic_servant_->_remove_ref ();
            delete rd_allocator_ ;
          }
      }

    // this method is called when delete_datawriter is called.
    void
    DataReaderImpl::cleanup ()
    {
      if (liveliness_timer_id_ != -1)
        {
          int num_handlers = reactor_->cancel_timer (this);
          ACE_UNUSED_ARG (num_handlers);
        }
               
      topic_servant_->remove_entity_ref ();
    }

     
    void DataReaderImpl::init (
        TopicImpl*         a_topic,
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
      ))
    {
      topic_servant_ = a_topic ;
      topic_servant_->_add_ref ();

      topic_servant_->add_entity_ref ();

      CORBA::String_var topic_name = topic_servant_->get_name ();

      is_bit_ = ACE_OS::strcmp (topic_name.in (), BUILT_IN_PARTICIPANT_TOPIC) == 0
                || ACE_OS::strcmp (topic_name.in (), BUILT_IN_TOPIC_TOPIC) == 0
                || ACE_OS::strcmp (topic_name.in (), BUILT_IN_SUBSCRIPTION_TOPIC) == 0 
                || ACE_OS::strcmp (topic_name.in (), BUILT_IN_PUBLICATION_TOPIC) == 0;

      qos_ = qos;
      listener_ = ::DDS::DataReaderListener::_duplicate (a_listener);

      if (! CORBA::is_nil (listener_.in()))
        {
          fast_listener_ = reference_to_servant<POA_DDS::DataReaderListener, 
                                                DDS::DataReaderListener_ptr> 
                              (listener_.in() ACE_ENV_ARG_PARAMETER);
          ACE_CHECK;
        }
      participant_servant_ = participant;
      participant_servant_->_add_ref ();
      domain_id_ =
            participant_servant_->get_domain_id (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_CHECK;
      
      topic_desc_ = participant_servant_->lookup_topicdescription(topic_name.in ()) ;

      subscriber_servant_ = subscriber ;
      subscriber_objref_ = ::DDS::Subscriber::_duplicate (subscriber_objref);
      subscriber_servant_->_add_ref ();
      dr_remote_objref_  = ::TAO::DCPS::DataReaderRemote::_duplicate (dr_remote_objref);
     
      initialized_ = true;
    }


    void DataReaderImpl::add_associations (
        ::TAO::DCPS::RepoId yourId,
        const TAO::DCPS::WriterAssociationSeq & writers
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {

      if (entity_deleted_ == true)
        {
          if (DCPS_debug_level >= 1) 
            ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT("(%P|%t) DataReaderImpl::add_associations")
                      ACE_TEXT(" This is a deleted datareader, ignoring add.\n")));
          return;
        }

      ::DDS::InstanceHandleSeq handles;

      if (0 == subscription_id_)
        {
          // add_associations was invoked before DCSPInfoRepo::add_subscription() returned.
          subscription_id_ = yourId;
        }

      {
        ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

        // keep track of writers associate with this reader
        CORBA::ULong wr_len = writers.length ();
        for (CORBA::ULong i = 0; i < wr_len; i++)
          {
            PublicationId writer_id = writers[i].writerId;
            WriterInfo info(this, writer_id);
            if (this->writers_.bind(writer_id, info) != 0)
              {
                ACE_ERROR((LM_ERROR,
                            "(%P|%t) DataReaderImpl::add_associations: "
                            "ERROR: Unable to insert writer publisher_id %d.\n",
                            writer_id));
              }

            // TBD should we set the status flag?

            liveliness_changed_status_.active_count++;
            // don't increment the change count because we don't call the listner
            // TBD - does this make sense?
            //liveliness_changed_status_.active_count_change++;

            // inactive count remains the same - this is a new writer
            //liveliness_changed_status_.inactive_count xx;
            //liveliness_changed_status_.inactive_count_change xx;

            if (liveliness_changed_status_.active_count < 0)
              {
                ACE_ERROR ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                            ACE_TEXT(" invalid liveliness_changed_status active count.\n")));
                return;
              }
            if (liveliness_changed_status_.inactive_count < 0)
              {
                ACE_ERROR ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                            ACE_TEXT(" invalid liveliness_changed_status inactive count.\n")));
                return;
              }
            if (this->writers_.current_size () !=
                  unsigned (liveliness_changed_status_.active_count +
                   liveliness_changed_status_.inactive_count) )
              {
                ACE_ERROR ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                            ACE_TEXT(" liveliness count does not match the number of writers.\n")));
                return;
              }

            //TBD - I don't think we should call the listner->on_liveliness_changed
            //      even though this new association is "alive".
            //      ?? or is it not alive until the first sample is recieved?

          }

        if (liveliness_lease_duration_  != ACE_Time_Value::zero)
          {
            // this call will start the timer if it is not already set
            ACE_Time_Value now = ACE_OS::gettimeofday ();
            if (DCPS_debug_level >= 5)
              ACE_DEBUG((LM_DEBUG, 
                        "(%P|%t) DataReaderImpl::add_associations"
                        " Starting/resetting liveliness timer for reader %d\n",
                        subscription_id_ ));
            this->handle_timeout (now, this);
          }
        // else - no timer needed when LIVELINESS.lease_duration is INFINITE

        // add associations to the transport before using
        // Built-In Topic support and telling the listener.
        this->subscriber_servant_->add_associations(writers, this, qos_) ;    
      }

      if (! is_bit_)
        {
          WriterIdSeq wr_ids;
          CORBA::ULong wr_len = writers.length ();
          wr_ids.length (wr_len);

          for (CORBA::ULong i = 0; i < wr_len; i++)
            {
              wr_ids[i] = writers[i].writerId;
            }

          // TBD: Remove the condition check after we change to default support 
          //      builtin topics.
          if (TheServiceParticipant->get_BIT () == true)
            {
              BIT_Helper_2 < ::DDS::PublicationBuiltinTopicDataDataReader,
                            ::DDS::PublicationBuiltinTopicDataDataReader_var,
                            ::DDS::PublicationBuiltinTopicDataSeq,
                            WriterIdSeq > hh;

              ::DDS::ReturnCode_t ret =
                hh.repo_ids_to_instance_handles(participant_servant_, 
                                              BUILT_IN_PUBLICATION_TOPIC,
                                              wr_ids, 
                                              handles);
         
              if (ret != ::DDS::RETCODE_OK)
                {
                  ACE_ERROR ((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                              ACE_TEXT(" failed to transfer repo ids to instance handles\n")));
                  return;
                }
            }
          else
            {
              handles.length (wr_len);
              for (CORBA::ULong i = 0; i < wr_len; i++)
                {
                  handles[i] = wr_ids[i];
                }
            }
      }

      ::POA_DDS::DataReaderListener* listener 
        = listener_for (::DDS::SUBSCRIPTION_MATCH_STATUS);

      ::DDS::SubscriptionMatchStatus subscription_match_status;

      ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);
      CORBA::ULong wr_len = handles.length ();
      CORBA::ULong pub_len = publication_handles_.length();
      publication_handles_.length (pub_len + wr_len);

      for (CORBA::ULong i = 0; i < wr_len; i++)
        {
          subscription_match_status_.total_count ++; 
          subscription_match_status_.total_count_change ++;
          publication_handles_[pub_len + i] = handles[i];
          subscription_match_status_.last_publication_handle = handles[i];
        }

      set_status_changed_flag (::DDS::SUBSCRIPTION_MATCH_STATUS, true);
        
      subscription_match_status = subscription_match_status_;
      
      if (listener != 0)
        {
          listener->on_subscription_match (dr_remote_objref_.in (), 
                                           subscription_match_status
                                           ACE_ENV_ARG_PARAMETER);
          ACE_CHECK;

          // TBD - why does the spec say to change this but not
          // change the ChangeFlagStatus after a listener call?

          // Client will look at it so next time it looks the change should be 0
          subscription_match_status_.total_count_change = 0;

        }

    }

    void DataReaderImpl::remove_associations (
        const TAO::DCPS::WriterIdSeq & writers
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ::DDS::InstanceHandleSeq handles;

        if (! is_bit_)
          {
            // TBD: Remove the condition check after we change to default support 
            //      builtin topics.
            if (TheServiceParticipant->get_BIT () == true)
              {
                BIT_Helper_2 < ::DDS::PublicationBuiltinTopicDataDataReader,
                            ::DDS::PublicationBuiltinTopicDataDataReader_var,
                            ::DDS::PublicationBuiltinTopicDataSeq,
                            WriterIdSeq > hh;

                ::DDS::ReturnCode_t ret 
                  = hh.repo_ids_to_instance_handles(participant_servant_, 
                                                  BUILT_IN_PUBLICATION_TOPIC,
                                                  writers, 
                                                  handles);
         
                if (ret != ::DDS::RETCODE_OK)
                  {
                    ACE_ERROR ((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::remove_associations, ")
                                ACE_TEXT(" failed to transfer repo ids to instance handles\n")));
                    return;
                  }
              }
            else
              {
                CORBA::ULong wr_len = writers.length ();
                handles.length (wr_len);
                for (CORBA::ULong i = 0; i < wr_len; i++)
                  {
                    handles[i] = writers[i];
                  }
              }
          }

        ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

        // keep track of writers associate with this reader
        CORBA::ULong wr_len = writers.length ();
        for (CORBA::ULong i = 0; i < wr_len; i++)
          {
            PublicationId writer_id = writers[i];

            int was_alive = 0;
            WriterMapType::ENTRY* entry;
            if (this->writers_.find(writer_id, entry)  == 0)
              was_alive = entry->int_id_.is_alive ();

            if (this->writers_.unbind(writer_id) != 0)
              {
                ACE_ERROR((LM_ERROR,
                            "(%P|%t) DataReaderImpl::remove_associations: "
                            "ERROR: Unable to remove writer publisher_id %d.\n",
                            writer_id));
              }

            // No need to cancel the liveliness timer becaue it will
            // have no impact if there are no writers or existing writers are alive

            //TBD ? should we tell the sample instances that the writer has been removed?

            //TBD should we set the status flag?

            if (was_alive)
              {
                liveliness_changed_status_.active_count--;
                liveliness_changed_status_.active_count_change--;
              }
            else
              {
                liveliness_changed_status_.inactive_count--;
                liveliness_changed_status_.inactive_count_change--;
              }

            if (liveliness_changed_status_.active_count < 0)
              {
                ACE_ERROR ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                            ACE_TEXT(" invalid liveliness_changed_status active count.\n")));
                return;
              }
            if (liveliness_changed_status_.inactive_count < 0)
              {
                ACE_ERROR ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                            ACE_TEXT(" invalid liveliness_changed_status inactive count.\n")));
                return;
              }
            if (this->writers_.current_size () !=
                  unsigned (liveliness_changed_status_.active_count +
                    liveliness_changed_status_.inactive_count) )
              {
                ACE_ERROR ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                            ACE_TEXT(" liveliness count does not match the number of writers.\n")));
                return;
              }

            //TBD - I don't think we should call the listner->on_liveliness_changed
            //      even though this write is dead because it is removed.
          }

        CORBA::ULong pubed_len = publication_handles_.length();
        //CORBA::ULong wr_len = handles.length();
        for (CORBA::ULong wr_index = 0; wr_index < wr_len; wr_index++)
          {

            for (CORBA::ULong pubed_index = 0; 
                 pubed_index < pubed_len; 
                 pubed_index++)
              {
                 if (publication_handles_[pubed_index] == handles[wr_index])
                  {
                    // move last element to this position.
                    if (pubed_index < pubed_len - 1)
                      {
                        publication_handles_[pubed_index] 
                        = publication_handles_[pubed_len - 1];
                      }
                    pubed_len --;
                    publication_handles_.length (pubed_len);
                    break;
                  }
              }
          }

        this->subscriber_servant_->remove_associations(writers);

      }


    void DataReaderImpl::remove_all_associations ()
      {

        TAO::DCPS::WriterIdSeq writers;

        ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

        int size = writers_.current_size();
        writers.length(size);
        WriterMapType::iterator curr_writer = writers_.begin();
        WriterMapType::iterator end_writer = writers_.end();

        int i = 0;
        while (curr_writer != end_writer)
          {
            writers[i++] = (*curr_writer).ext_id_;
            curr_writer.advance();
          }

        ACE_TRY_NEW_ENV
          {
            if (0 < size)
              {
                remove_associations(writers ACE_ENV_ARG_PARAMETER);
              }
          }
        ACE_CATCHANY
          {
          }
        ACE_ENDTRY;
      }


    void DataReaderImpl::update_incompatible_qos (
        const TAO::DCPS::IncompatibleQosStatus & status
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ::POA_DDS::DataReaderListener* listener 
          = listener_for (::DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS);

        ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

        set_status_changed_flag(::DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS,
                                true) ;

        // copy status and increment change
        requested_incompatible_qos_status_.total_count = status.total_count;
        requested_incompatible_qos_status_.total_count_change +=
            status.count_since_last_send;
        requested_incompatible_qos_status_.last_policy_id =
            status.last_policy_id;
        requested_incompatible_qos_status_.policies = status.policies;
          
        if (listener != 0)
          {

            listener->on_requested_incompatible_qos (dr_remote_objref_.in (), 
                requested_incompatible_qos_status_);
            
            ACE_CHECK;

            // TBD - why does the spec say to change total_count_change but not
            // change the ChangeFlagStatus after a listener call?

            // client just looked at it so next time it looks the
            // change should be 0
            requested_incompatible_qos_status_.total_count_change = 0;
       }
      }

    ::DDS::ReturnCode_t DataReaderImpl::delete_contained_entities (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        if (DCPS_debug_level >= 2) 
          ACE_DEBUG ((LM_DEBUG,
                     ACE_TEXT("(%P|%t) ")
                     ACE_TEXT("DataReaderImpl::delete_contained_entities\n")));
        return ::DDS::RETCODE_OK;
      }
      
    ::DDS::ReturnCode_t DataReaderImpl::set_qos (
        const ::DDS::DataReaderQos & qos
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos))
        {
          if (enabled_.value()) 
          {
            if (! Qos_Helper::changeable (qos_, qos))
            {
              return ::DDS::RETCODE_IMMUTABLE_POLICY;
            }
          }
          if (! (qos_ == qos))
            {
              qos_ = qos;
              // TBD - when there are changable QoS supported
              //       this code may need to do something
              //       with the changed values.
              // TBD - when there are changable QoS then we
              //       need to tell the DCPSInfo/repo about
              //       the changes in Qos.
              // repo->set_qos(qos_);
            }
          return ::DDS::RETCODE_OK;
        }
        else 
        {
          return ::DDS::RETCODE_INCONSISTENT_POLICY;
        }
      }
      
    void DataReaderImpl::get_qos (
        ::DDS::DataReaderQos & qos
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        qos = qos_ ;
      }
      
    ::DDS::ReturnCode_t DataReaderImpl::set_listener (
        ::DDS::DataReaderListener_ptr a_listener,
        ::DDS::StatusKindMask mask
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        listener_mask_ = mask;
        //note: OK to duplicate  and reference_to_servant a nil object ref
        listener_ = ::DDS::DataReaderListener::_duplicate(a_listener);
        fast_listener_ 
          = reference_to_servant< ::POA_DDS::DataReaderListener, 
                                  ::DDS::DataReaderListener_ptr > 
            (listener_.in () ACE_ENV_ARG_PARAMETER);
        ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);
        return ::DDS::RETCODE_OK;
      }
      
    ::DDS::DataReaderListener_ptr DataReaderImpl::get_listener (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        return ::DDS::DataReaderListener::_duplicate (listener_.in ());
      }


    ::DDS::TopicDescription_ptr DataReaderImpl::get_topicdescription (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        return ::DDS::TopicDescription::_duplicate (topic_desc_.in ()) ;
      }
      
    ::DDS::Subscriber_ptr DataReaderImpl::get_subscriber (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        return ::DDS::Subscriber::_duplicate (subscriber_objref_.in ());
      }
      
    ::DDS::SampleRejectedStatus DataReaderImpl::get_sample_rejected_status (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (this->sample_lock_);

        set_status_changed_flag (::DDS::SAMPLE_REJECTED_STATUS, false);
        ::DDS::SampleRejectedStatus status = sample_rejected_status_;
        sample_rejected_status_.total_count_change = 0;
        return status;
      }
      
    ::DDS::LivelinessChangedStatus DataReaderImpl::get_liveliness_changed_status (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (this->sample_lock_);

        set_status_changed_flag(::DDS::LIVELINESS_CHANGED_STATUS,
                                false);
        ::DDS::LivelinessChangedStatus status =
            liveliness_changed_status_;

        liveliness_changed_status_.active_count_change = 0;
        liveliness_changed_status_.inactive_count_change = 0;

        return status ;
      }
      
    ::DDS::RequestedDeadlineMissedStatus
          DataReaderImpl::get_requested_deadline_missed_status (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (this->sample_lock_);

        set_status_changed_flag(::DDS::REQUESTED_DEADLINE_MISSED_STATUS,
                                false);
        ::DDS::RequestedDeadlineMissedStatus status =
            requested_deadline_missed_status_;
        requested_deadline_missed_status_.total_count_change = 0 ;
        return status ;
      }
      
    ::DDS::RequestedIncompatibleQosStatus * DataReaderImpl::get_requested_incompatible_qos_status (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (this->publication_handle_lock_);

        set_status_changed_flag(::DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS,
                                false);
        ::DDS::RequestedIncompatibleQosStatus* status 
          = new ::DDS::RequestedIncompatibleQosStatus;
        *status = requested_incompatible_qos_status_;
        requested_incompatible_qos_status_.total_count_change = 0 ;
        return status;
      }
      
    ::DDS::SubscriptionMatchStatus DataReaderImpl::get_subscription_match_status (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (this->publication_handle_lock_);

        set_status_changed_flag (::DDS::SUBSCRIPTION_MATCH_STATUS, false);
        ::DDS::SubscriptionMatchStatus status = subscription_match_status_;
        subscription_match_status_.total_count_change = 0;

        return status  ;
      }
      
    ::DDS::SampleLostStatus DataReaderImpl::get_sample_lost_status (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (this->sample_lock_);

        set_status_changed_flag (::DDS::SAMPLE_LOST_STATUS, false);
        ::DDS::SampleLostStatus status = sample_lost_status_;
        sample_lost_status_.total_count_change = 0;
        return status;
      }
      
    ::DDS::ReturnCode_t DataReaderImpl::wait_for_historical_data (
        const ::DDS::Duration_t & max_wait
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ACE_UNUSED_ARG( max_wait) ;
        // Add your implementation here
        return 0;
      }
      
    ::DDS::ReturnCode_t DataReaderImpl::get_matched_publications (
        ::DDS::InstanceHandleSeq & publication_handles
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        if (enabled_ == false)
          {
            ACE_ERROR_RETURN ((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::get_matched_publications, ")
                              ACE_TEXT(" Entity is not enabled. \n")),
                              ::DDS::RETCODE_NOT_ENABLED);
          }

        ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                          guard,
                          this->publication_handle_lock_,
                          ::DDS::RETCODE_ERROR);

        publication_handles = publication_handles_;

        return ::DDS::RETCODE_OK;
      }
      
    ::DDS::ReturnCode_t DataReaderImpl::get_matched_publication_data (
        ::DDS::PublicationBuiltinTopicData & publication_data,
        ::DDS::InstanceHandle_t publication_handle
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        if (enabled_ == false)
          {
            ACE_ERROR_RETURN ((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::get_matched_publication_data, ")
                              ACE_TEXT(" Entity is not enabled. \n")),
                              ::DDS::RETCODE_NOT_ENABLED);
          }

        ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                          guard,
                          this->publication_handle_lock_,
                          ::DDS::RETCODE_ERROR);

        BIT_Helper_1 < ::DDS::PublicationBuiltinTopicDataDataReader,
                       ::DDS::PublicationBuiltinTopicDataDataReader_var,
                       ::DDS::PublicationBuiltinTopicDataSeq > hh;

        ::DDS::PublicationBuiltinTopicDataSeq data;

        ::DDS::ReturnCode_t ret 
          = hh.instance_handle_to_bit_data(participant_servant_, 
                                           BUILT_IN_PUBLICATION_TOPIC, 
                                           publication_handle, 
                                           data);
        if (ret == ::DDS::RETCODE_OK)
          {
            publication_data = data[0];
          }

        return ret;
      }
      
    ::DDS::ReturnCode_t DataReaderImpl::enable (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        if (qos_.history.kind == ::DDS::KEEP_ALL_HISTORY_QOS)
          {
            // The spec says qos_.history.depth is "has no effect" 
            // when history.kind = KEEP_ALL so use max_samples_per_instance
            depth_ = qos_.resource_limits.max_samples_per_instance;
          }
        else // qos_.history.kind == ::DDS::KEEP_LAST_HISTORY_QOS
          {
            depth_ = qos_.history.depth;
          }

        if (depth_ == ::DDS::LENGTH_UNLIMITED)
          {
            // ::DDS::LENGTH_UNLIMITED is negative so make it a positive
            // value that is for all intents and purposes unlimited
            // and we can use it for comparisons.
            // WARNING: The client risks running out of memory in this case.
            depth_ = LONG_MAX;
          }

        if (qos_.resource_limits.max_samples != ::DDS::LENGTH_UNLIMITED)
          {
            n_chunks_ = qos_.resource_limits.max_samples;
          }
        //else using value from Service_Participant

        // enable the type specific part of this DataWriter
        this->enable_specific ();

        //Note: the QoS used to set n_chunks_ is Changable=No so
        // it is OK that we cannot change the size of our allocators.
        rd_allocator_ = new ReceivedDataAllocator(n_chunks_) ;
        if (DCPS_debug_level >= 2) 
          ACE_DEBUG((LM_DEBUG,"(%P|%t) DataReaderImpl::enable"
                     " Cached_Allocator_With_Overflow %x with %d chunks\n",
                      rd_allocator_, n_chunks_));

        if ((qos_.liveliness.lease_duration.sec !=
             ::DDS::DURATION_INFINITY_SEC) ||
            (qos_.liveliness.lease_duration.nanosec !=
             ::DDS::DURATION_INFINITY_NSEC))
          {
            liveliness_lease_duration_ =
                duration_to_time_value (qos_.liveliness.lease_duration);
          }

        this->set_enabled () ;

        CORBA::String_var name = topic_servant_->get_name ();
 
        subscriber_servant_->reader_enabled(dr_remote_objref_.in (),
                                            name.in(),
                                            topic_servant_->get_id()
                                            ACE_ENV_ARG_PARAMETER);
        ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);

        return ::DDS::RETCODE_OK;
      }
      
    ::DDS::StatusKindMask DataReaderImpl::get_status_changes (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
      {
        ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                          guard,
                          this->sample_lock_,
                          ::DDS::RETCODE_ERROR);

        return EntityImpl::get_status_changes (ACE_ENV_SINGLE_ARG_PARAMETER);
      }


      void 
      DataReaderImpl::writer_activity(PublicationId writer_id)
      {
        // caller should have the sample_lock_ !!!

        WriterMapType::ENTRY* entry;
        if (this->writers_.find(writer_id, entry)  == 0)
          {
            ACE_Time_Value when = ACE_OS::gettimeofday ();
            entry->int_id_.received_activity (when);
          }
        else
          ACE_ERROR((LM_ERROR,"(%P|%t) DataReaderImpl::writer_activity"
                     " reader %d is not associated with writer %d\n",
                     this->subscription_id_, writer_id));
      }

      void DataReaderImpl::data_received(const ReceivedDataSample& sample)
      {
        // ensure some other thread is not changing the sample container
        // or statuses related to samples.
        ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);

        switch (sample.header_.message_id_)
        {
          case SAMPLE_DATA:
          case INSTANCE_REGISTRATION:
            {
              this->writer_activity(sample.header_.publication_id_);
              // This also adds to the sample container
              this->demarshal(sample) ;

              this->subscriber_servant_->data_received(this) ;
            }
            break ;

          case DATAWRITER_LIVELINESS:
            {
              this->writer_activity(sample.header_.publication_id_);

              // tell all instances they got a liveliness message
              for (SubscriptionInstances::const_iterator pos = instances_.begin() ;
                  pos != instances_.end() ;
                  ++pos)
                {
                  ::DDS::InstanceHandle_t handle = *pos ;
                  SubscriptionInstance *ptr = 
                      reinterpret_cast<SubscriptionInstance *> (handle);

                  ptr->instance_state_.lively (sample.header_.publication_id_);
                }

            }
            break ;

          case DISPOSE_INSTANCE:
            this->writer_activity(sample.header_.publication_id_);
            this->dispose(sample);
            break ;

          default:
            ACE_ERROR((LM_ERROR,
                "(%P|%t) %T ERRRO: DataReaderImpl::data_received"
                "unexpected message_id = %d\n",
                sample.header_.message_id_));
            break ;
        }
      }


      SubscriberImpl* DataReaderImpl::get_subscriber_servant ()
      {
        return subscriber_servant_;
      }
      
      RepoId DataReaderImpl::get_subscription_id() const
      {
        return subscription_id_;
      }

      void DataReaderImpl::set_subscription_id(RepoId subscription_id)
      {
        subscription_id_ = subscription_id ;
      }

      bool DataReaderImpl::have_sample_states(
            ::DDS::SampleStateMask sample_states) const
      {
        //!!! caller should have acquired sample_lock_

        for (SubscriptionInstances::const_iterator pos = instances_.begin() ;
             pos != instances_.end() ;
             ++pos)
        {
          ::DDS::InstanceHandle_t handle = *pos ;
          SubscriptionInstance *ptr = 
              reinterpret_cast<SubscriptionInstance *> (handle) ;
          for (ReceivedDataElement *item = ptr->rcvd_sample_.head_ ;
               item != 0 ; item = item->next_data_sample_)
          {
            if (item->sample_state_ & sample_states)
            {
              return true ;
            }
          }
        } 
        return false ;
      }
           
      bool DataReaderImpl::have_view_states(
            ::DDS::ViewStateMask view_states) const 
      {
        //!!! caller should have acquired sample_lock_
        for (SubscriptionInstances::const_iterator pos = instances_.begin() ;
             pos != instances_.end() ;
             ++pos)
        {
          ::DDS::InstanceHandle_t handle = *pos ;
          SubscriptionInstance *ptr = 
              reinterpret_cast<SubscriptionInstance *> (handle) ;
          if (ptr->instance_state_.view_state() & view_states)
          {
            return true ;
          }
        }
        return false ;
      }

      bool DataReaderImpl::have_instance_states(
            ::DDS::InstanceStateMask instance_states) const
      {
        //!!! caller should have acquired sample_lock_
        for (SubscriptionInstances::const_iterator pos = instances_.begin() ;
             pos != instances_.end() ;
             ++pos)
        {
          ::DDS::InstanceHandle_t handle = *pos ;
          SubscriptionInstance *ptr = 
              reinterpret_cast<SubscriptionInstance *> (handle) ;
          if (ptr->instance_state_.instance_state() & instance_states)
          {
            return true ;
          }
        }
        return false ;
      }

      ::POA_DDS::DataReaderListener*
      DataReaderImpl::listener_for (::DDS::StatusKind kind)
      {
        // per 2.1.4.3.1 Listener Access to Plain Communication Status
        // use this entities factory if listener is mask not enabled
        // for this kind.
        if ((listener_mask_ & kind) == 0)
          {
            return subscriber_servant_->listener_for (kind);
          }
        else
          {
            return fast_listener_;
          }
      }

      void DataReaderImpl::sample_info(::DDS::SampleInfoSeq & info_seq,
                                       size_t start_idx, size_t count,
                                       ReceivedDataElement *ptr)
      {
        size_t end_idx = start_idx + count - 1 ;
        for (size_t i = start_idx ; i <= end_idx ; i++)
        {
          info_seq[i].sample_rank = count - (i - start_idx + 1) ;

          // generation_rank =
          //    (MRSIC.disposed_generation_count + 
          //     MRSIC.no_writers_generation_count)
          //  - (S.disposed_generation_count + 
          //     S.no_writers_generation_count)
          //
          //  info_seq[end_idx] == MRSIC
          //  info_seq[i].generation_rank == 
          //      (S.disposed_generation_count + 
          //      (S.no_writers_generation_count) -- calculated in 
          //            InstanceState::sample_info
         
          info_seq[i].generation_rank =
              (info_seq[end_idx].disposed_generation_count +
               info_seq[end_idx].no_writers_generation_count) -
              info_seq[i].generation_rank ;

          // absolute_generation_rank =
          //     (MRS.disposed_generation_count + 
          //      MRS.no_writers_generation_count)
          //   - (S.disposed_generation_count + 
          //      S.no_writers_generation_count)
          //
          // ptr == MRS
          // info_seq[i].absolute_generation_rank == 
          //    (S.disposed_generation_count + 
          //     S.no_writers_generation_count)-- calculated in
          //            InstanceState::sample_info
          //
          info_seq[i].absolute_generation_rank =
              (ptr->disposed_generation_count_ +
               ptr->no_writers_generation_count_) -
              info_seq[i].absolute_generation_rank ;
        }
      }

      CORBA::Long DataReaderImpl::total_samples() const
      {
        //!!! caller should have acquired sample_lock_
        CORBA::Long count(0) ;
        for (SubscriptionInstances::const_iterator pos = instances_.begin() ;
             pos != instances_.end() ;
             ++pos)
        {
          ::DDS::InstanceHandle_t handle = *pos ;
          SubscriptionInstance *ptr = 
              reinterpret_cast<SubscriptionInstance *> (handle) ;

          count += ptr->rcvd_sample_.size_ ;
        }
        
        return count ;
      }

    int 
    DataReaderImpl::handle_timeout (const ACE_Time_Value &tv,
                                    const void *arg)
    {
      ACE_UNUSED_ARG(arg);

      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                        guard,
                        this->sample_lock_,
                        ::DDS::RETCODE_ERROR);

      if (liveliness_timer_id_ != -1)
        {
          this->_remove_ref();


          if (arg == this)
            {

              if (DCPS_debug_level >= 5)
                ACE_DEBUG((LM_DEBUG, 
                          "(%P|%t) DataReaderImpl::handle_timeout"
                          " canceling timer for reader %d\n",
                          subscription_id_ ));

              // called from add_associations and there is already a timer
              // so cancel the existing timer.
              if (reactor_->cancel_timer (this) == -1)
                {
                  // this could fail becaue the reactor's call and
                  // the add_associations' call to this could overlap
                  // so it is not a failure.
                  ACE_DEBUG((LM_DEBUG, 
                    ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::handle_timeout, ")
                    ACE_TEXT(" %p. \n"), "cancel_timer" ));
                }
              liveliness_timer_id_ = -1;
            }
        }
      
      ACE_Time_Value smallest(ACE_Time_Value::max_time) ; 
      ACE_Time_Value next_absolute ;
      WriterMapType::ENTRY* entry;
      int alive_writers = 0;

      // Iterate over each writer to this reader
      for (WriterMapType::ITERATOR itr(this->writers_);
          itr.next(entry);
          itr.advance())
        {
          // deal with possibly not being alive or
          // tell when it will not be alive next (if no activity)
          next_absolute = entry->int_id_.check_activity(tv);

          if (next_absolute != ACE_Time_Value::max_time)
            {
              alive_writers++;
              if (next_absolute < smallest)
                {
                  smallest = next_absolute;
                } 
            }
        }

      if (DCPS_debug_level >= 5)
        ACE_DEBUG((LM_DEBUG, 
                    "(%P|%t) %T DataReaderImpl::handle_timeout"
                    " reader %d has %d live writers; from_reator=%d\n",
                    subscription_id_, alive_writers, arg == this ? 0 : 1));

      if (alive_writers)
        {
          ACE_Time_Value relative;
          ACE_Time_Value now = ACE_OS::gettimeofday ();
          if (now < next_absolute)
            relative = next_absolute - now;
          else
            relative = ACE_Time_Value(0,1); // ASAP
        
          liveliness_timer_id_ = reactor_->schedule_timer(this, 0, relative);
          if (liveliness_timer_id_ == -1)
            {
              ACE_ERROR((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::handle_timeout, ")
                  ACE_TEXT(" %p. \n"), "schedule_timer" ));
            }
          else
          {
            this->_add_ref();
          }
        }
      else {
        // no live writers so no need to schedule a timer
        // but be sure we don't try to cancel the timer later.
        liveliness_timer_id_ = -1;
      }
    
      return 0;
    }

    WriterInfo::WriterInfo ()
    : last_liveliness_activity_time_(ACE_OS::gettimeofday()),
      is_alive_(1),
      reader_(0),
      writer_id_(0)
    {
    }

    WriterInfo::WriterInfo (DataReaderImpl* reader,
                            PublicationId   writer_id)
    : last_liveliness_activity_time_(ACE_OS::gettimeofday()),
      is_alive_(1),
      reader_(reader),
      writer_id_(writer_id)
    {
      if (DCPS_debug_level >= 5)
        ACE_DEBUG((LM_DEBUG,"(%P|%t) WriterInfo::WriterInfo"
                  " added writer %d to reader %d", 
                  reader_->subscription_id_, writer_id_));
    }

    ACE_Time_Value 
    WriterInfo::check_activity (const ACE_Time_Value& now)
    {
      ACE_Time_Value expires_at = ACE_Time_Value::max_time;
      if (is_alive_)
        {
          expires_at = this->last_liveliness_activity_time_ +
            reader_->liveliness_lease_duration_ ;
          if (expires_at <= now)
            { 
              is_alive_ = 0;
              // let all instances know this write is not alive.
              reader_->writer_became_dead(writer_id_, now);
              expires_at = ACE_Time_Value::max_time;
            }
        }
      return expires_at;
    }

    void
    DataReaderImpl::writer_became_alive (PublicationId   writer_id,
                                         const ACE_Time_Value& when)
    {
      ACE_UNUSED_ARG(when) ;
      if (DCPS_debug_level >= 5)
        ACE_DEBUG((LM_DEBUG,
                  "(%P|%t) DataReaderImpl::writer_became_alive writer %d to reader %d\n", 
                  this->subscription_id_, writer_id));
      
      // caller should already have the samples_lock_ !!!

      // NOTE: each instance will change to ALIVE_STATE when they recieve a sample

      ::POA_DDS::DataReaderListener* listener
          = listener_for (::DDS::LIVELINESS_CHANGED_STATUS);

      set_status_changed_flag(::DDS::LIVELINESS_CHANGED_STATUS,
                              true) ;

      liveliness_changed_status_.active_count++;
      liveliness_changed_status_.inactive_count--;
      liveliness_changed_status_.active_count_change++;
      liveliness_changed_status_.inactive_count_change--;

      if (liveliness_changed_status_.active_count < 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                      ACE_TEXT(" invalid liveliness_changed_status active count.\n")));
          return;
        }
      if (liveliness_changed_status_.inactive_count < 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                      ACE_TEXT(" invalid liveliness_changed_status inactive count.\n")));
          return;
        }
      if (this->writers_.current_size () !=
            unsigned (liveliness_changed_status_.active_count +
              liveliness_changed_status_.inactive_count) )
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                      ACE_TEXT(" liveliness count does not match the number of writers.\n")));
          return;
        }


      // this call will start the livilness timer if it is not already set
      ACE_Time_Value now = ACE_OS::gettimeofday ();
      this->handle_timeout (now, this);

      if (listener != 0)
        {
          listener->on_liveliness_changed (dr_remote_objref_.in (),
                                           liveliness_changed_status_);
          ACE_CHECK;

          liveliness_changed_status_.active_count_change = 0;
          liveliness_changed_status_.inactive_count_change = 0;
        }

    }

    void
    DataReaderImpl::writer_became_dead (PublicationId   writer_id,
                                        const ACE_Time_Value& when)
    {
      if (DCPS_debug_level >= 5)
        ACE_DEBUG((LM_DEBUG,
                  "(%P|%t) DataReaderImpl::writer_became_dead writer %d to reader %d\n", 
                  this->subscription_id_, writer_id));

      // caller should already have the samples_lock_ !!!

      set_status_changed_flag(::DDS::LIVELINESS_LOST_STATUS,
                              true) ;

      liveliness_changed_status_.active_count--;
      liveliness_changed_status_.inactive_count++;
      liveliness_changed_status_.active_count_change--;
      liveliness_changed_status_.inactive_count_change++;

      if (liveliness_changed_status_.active_count < 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                      ACE_TEXT(" invalid liveliness_changed_status active count.\n")));
          return;
        }
      if (liveliness_changed_status_.inactive_count < 0)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                      ACE_TEXT(" invalid liveliness_changed_status inactive count.\n")));
          return;
        }
      if (this->writers_.current_size () !=
            unsigned (liveliness_changed_status_.active_count +
              liveliness_changed_status_.inactive_count) )
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::add_associations, ")
                      ACE_TEXT(" liveliness count does not match the number of writers.\n")));
          return;
        }


      //loop through all instances telling them this writer is dead
      for (SubscriptionInstances::const_iterator pos = instances_.begin() ;
           pos != instances_.end() ;
           ++pos)
        {
          ::DDS::InstanceHandle_t handle = *pos ;
          SubscriptionInstance *ptr = 
              reinterpret_cast<SubscriptionInstance *> (handle);

          ptr->instance_state_.writer_became_dead (
                   writer_id, liveliness_changed_status_.active_count, when);

        }

      ::POA_DDS::DataReaderListener* listener
          = listener_for (::DDS::LIVELINESS_CHANGED_STATUS);

      if (listener != 0)
        {
          listener->on_liveliness_changed (dr_remote_objref_.in (),
                                           liveliness_changed_status_);
          ACE_CHECK;

          liveliness_changed_status_.active_count_change = 0;
          liveliness_changed_status_.inactive_count_change = 0;
        }

    }


    int
    DataReaderImpl::handle_close (ACE_HANDLE,
                                  ACE_Reactor_Mask)
    {
      this->_remove_ref ();
      return 0;
    }


    void
    DataReaderImpl::set_sample_lost_status(
        const ::DDS::SampleLostStatus& status)
    {
      //!!! caller should have acquired sample_lock_
      sample_lost_status_ = status ;
    }
   

    void
    DataReaderImpl::set_sample_rejected_status(
        const ::DDS::SampleRejectedStatus& status)
    {
      //!!! caller should have acquired sample_lock_
      sample_rejected_status_ = status ;
    }


    void DataReaderImpl::demarshal(const ReceivedDataSample& sample)
    {
      ACE_UNUSED_ARG(sample) ;
    }

    void DataReaderImpl::dispose(const ReceivedDataSample& sample)
    {
      ACE_UNUSED_ARG(sample) ;
    }
 
    
  } // namespace DCPS
} // namespace TAO

