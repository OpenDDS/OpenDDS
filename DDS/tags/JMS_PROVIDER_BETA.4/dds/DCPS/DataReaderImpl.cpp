// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataReaderImpl.h"
#include "tao/ORB_Core.h"
#include "SubscriptionInstance.h"
#include "ReceivedDataElementList.h"
#include "DomainParticipantImpl.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "RepoIdConverter.h"
#include "TopicImpl.h"
#include "Serializer.h"
#include "SubscriberImpl.h"
#include "Transient_Kludge.h"
#include "Util.h"
#include "RequestedDeadlineWatchdog.h"
#include "QueryConditionImpl.h"

#include "dds/DCPS/transport/framework/EntryExit.h"
#if !defined (DDS_HAS_MINIMUM_BIT)
#include "BuiltInTopicUtils.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "ace/Reactor.h"
#include "ace/Auto_Ptr.h"

#include <sstream>

#if !defined (__ACE_INLINE__)
# include "DataReaderImpl.inl"
#endif /* ! __ACE_INLINE__ */

namespace OpenDDS
{
  namespace DCPS
  {

#if 0
    // Emacs trick to align code with first column
    // This will cause emacs to emit bogus alignment message
    // For now just disregard them.
  }}
#endif


DataReaderImpl::DataReaderImpl (void) :
  rd_allocator_(0),
  qos_ (TheServiceParticipant->initial_DataReaderQos ()),
  reverse_sample_lock_(sample_lock_),
  next_handle_(0),
  topic_servant_ (0),
  topic_desc_(0),
  listener_mask_(DEFAULT_STATUS_KIND_MASK),
  fast_listener_ (0),
  participant_servant_ (0),
  domain_id_ (0),
  subscriber_servant_(0),
  subscription_id_ ( GUID_UNKNOWN ),
  n_chunks_ (TheServiceParticipant->n_chunks ()),
  reactor_(0),
  liveliness_lease_duration_ (ACE_Time_Value::zero),
  liveliness_timer_id_(-1),
  last_deadline_missed_total_count_ (0),
  watchdog_ (),
  is_bit_ (false),
  initialized_ (false),
  always_get_history_ (false),
  statistics_enabled_( false),
  raw_latency_buffer_size_( 0),
  raw_latency_buffer_type_( DataCollector< double>::KeepOldest)
{
  CORBA::ORB_var orb = TheServiceParticipant->get_ORB ();
  reactor_ = orb->orb_core()->reactor();

  liveliness_changed_status_.alive_count = 0;
  liveliness_changed_status_.not_alive_count = 0;
  liveliness_changed_status_.alive_count_change = 0;
  liveliness_changed_status_.not_alive_count_change = 0;
  liveliness_changed_status_.last_publication_handle =
    ::DDS::HANDLE_NIL;

  requested_deadline_missed_status_.total_count = 0;
  requested_deadline_missed_status_.total_count_change = 0;
  requested_deadline_missed_status_.last_instance_handle =
    ::DDS::HANDLE_NIL;

  requested_incompatible_qos_status_.total_count = 0;
  requested_incompatible_qos_status_.total_count_change = 0;
  requested_incompatible_qos_status_.last_policy_id = 0;
  requested_incompatible_qos_status_.policies.length(0);

  subscription_match_status_.total_count = 0;
  subscription_match_status_.total_count_change = 0;
  subscription_match_status_.current_count = 0;
  subscription_match_status_.current_count_change = 0;
  subscription_match_status_.last_publication_handle =
    ::DDS::HANDLE_NIL;

  sample_lost_status_.total_count = 0;
  sample_lost_status_.total_count_change = 0;

  sample_rejected_status_.total_count = 0;
  sample_rejected_status_.total_count_change = 0;
  sample_rejected_status_.last_reason = ::DDS::NOT_REJECTED;
  sample_rejected_status_.last_instance_handle = ::DDS::HANDLE_NIL;

  this->budget_exceeded_status_.total_count = 0;
  this->budget_exceeded_status_.total_count_change = 0;
  this->budget_exceeded_status_.last_instance_handle = ::DDS::HANDLE_NIL;
}

// This method is called when there are no longer any reference to the
// the servant.
DataReaderImpl::~DataReaderImpl (void)
{
  DBG_ENTRY_LVL ("DataReaderImpl","~DataReaderImpl",6);

  if (initialized_)
    {
      delete rd_allocator_;
    }

  CORBA::Long count = this->total_samples();
  if( count > 0) {
    RepoIdConverter converter(subscription_id_);
    ACE_DEBUG((LM_WARNING,
      ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::~DataReaderImpl() - ")
      ACE_TEXT("reader %s terminating with %d samples unread.\n"),
      std::string(converter).c_str(),
      count
    ));
  }
}

// this method is called when delete_datawriter is called.
void
DataReaderImpl::cleanup ()
{
  {
  ACE_GUARD (ACE_Recursive_Thread_Mutex,
    guard,
    this->sample_lock_);

  if (liveliness_timer_id_ != -1)
    {
      (void) reactor_->cancel_timer (this);
    }
  }

  topic_servant_->remove_entity_ref ();
  topic_servant_->_remove_ref ();
  dr_local_objref_ = ::DDS::DataReader::_nil();
  deactivate_remote_object(dr_remote_objref_.in());
  dr_remote_objref_ = DataReaderRemote::_nil();
}


void DataReaderImpl::init (
                           TopicImpl*         a_topic,
                           const ::DDS::DataReaderQos &  qos,
                           const DataReaderQosExt &      ext_qos,
                           ::DDS::DataReaderListener_ptr a_listener,
                           DomainParticipantImpl*        participant,
                           SubscriberImpl*               subscriber,
                           ::DDS::DataReader_ptr         dr_objref,
               ::OpenDDS::DCPS::DataReaderRemote_ptr dr_remote_objref
                           )
  ACE_THROW_SPEC ((
                   CORBA::SystemException
                   ))
{
  topic_servant_ = a_topic;
  topic_servant_->_add_ref ();

  topic_servant_->add_entity_ref ();

  CORBA::String_var topic_name = topic_servant_->get_name ();

#if !defined (DDS_HAS_MINIMUM_BIT)
  is_bit_ = ACE_OS::strcmp (topic_name.in (), BUILT_IN_PARTICIPANT_TOPIC) == 0
    || ACE_OS::strcmp (topic_name.in (), BUILT_IN_TOPIC_TOPIC) == 0
    || ACE_OS::strcmp (topic_name.in (), BUILT_IN_SUBSCRIPTION_TOPIC) == 0
    || ACE_OS::strcmp (topic_name.in (), BUILT_IN_PUBLICATION_TOPIC) == 0;
#endif // !defined (DDS_HAS_MINIMUM_BIT)

  qos_ = qos;
  always_get_history_ = ext_qos.durability.always_get_history;

  listener_ = ::DDS::DataReaderListener::_duplicate (a_listener);

  if (! CORBA::is_nil (listener_.in()))
    {
      fast_listener_ = listener_.in ();
    }

  // Only store the participant pointer, since it is our "grand"
  // parent, we will exist as long as it does
  participant_servant_ = participant;
  domain_id_ = participant_servant_->get_domain_id ();
  topic_desc_ =
    participant_servant_->lookup_topicdescription(topic_name.in ());

  // Only store the subscriber pointer, since it is our parent, we
  // will exist as long as it does.
  subscriber_servant_ = subscriber;
  dr_local_objref_    = ::DDS::DataReader::_duplicate (dr_objref);
  dr_remote_objref_   =
    ::OpenDDS::DCPS::DataReaderRemote::_duplicate (dr_remote_objref );

  initialized_ = true;
}

DDS::InstanceHandle_t
DataReaderImpl::get_instance_handle()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  RepoIdConverter converter(subscription_id_);
  return DDS::InstanceHandle_t(converter);
}

void DataReaderImpl::add_associations (::OpenDDS::DCPS::RepoId yourId,
                                       const OpenDDS::DCPS::WriterAssociationSeq & writers)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  //
  // The following block is for diagnostic purposes only.
  //
  if (DCPS_debug_level >= 1)
  {
    RepoIdConverter reader_converter(yourId);
    RepoIdConverter writer_converter(writers[0].writerId);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::add_associations - ")
      ACE_TEXT("bit %d local %s remote %s num remotes %d \n"),
      is_bit_,
      std::string(reader_converter).c_str(),
      std::string(writer_converter).c_str(),
      writers.length()
    ));
  }

  //
  // This block prevents adding associations to deleted readers.
  // Presumably this is a "good thing(tm)".
  //
  if (entity_deleted_ == true)
  {
    if (DCPS_debug_level >= 1)
      ACE_DEBUG ((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::add_associations")
      ACE_TEXT(" This is a deleted datareader, ignoring add.\n")));
    return;
  }

  //
  // We are being called back from the repository before we are done
  // processing after our call to the repository that caused this call
  // (from the repository) to be made.
  //
  if (GUID_UNKNOWN == subscription_id_)
  {
    // add_associations was invoked before DCSPInfoRepo::add_subscription() returned.
    subscription_id_ = yourId;
  }

  //
  // We do the following while holding the publication_handle_lock_.
  //
  {
    ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

    //
    // For each writer in the list of writers to associate with, we
    // create a WriterInfo and a WriterStats object and store them in
    // our internal maps.
    //
    CORBA::ULong wr_len = writers.length ();
    for (CORBA::ULong i = 0; i < wr_len; i++)
    {
      PublicationId writer_id = writers[i].writerId;
      this->writers_.insert(
        // This insertion is idempotent.
        WriterMapType::value_type(
          writer_id,
          WriterInfo( this, writer_id)
        )
      );
      this->statistics_.insert(
        StatsMapType::value_type(
          writer_id,
          WriterStats(
            this->raw_latency_buffer_size_,
            this->raw_latency_buffer_type_
          )
        )
      );
      if( DCPS_debug_level > 4) {
        RepoIdConverter converter(writer_id);
        ACE_DEBUG((LM_DEBUG,
          "(%P|%t) DataReaderImpl::add_associations: "
          "inserted writer %s.\n",
          std::string(converter).c_str()
        ));
      }
    }

    //
    // Propagate the add_associations processing down into the Transport
    // layer here.  This will establish the transport support and reserve
    // usage of an existing connection or initiate creation of a new
    // connection if no suitable connection is available.
    //
    this->subscriber_servant_->add_associations(writers, this, qos_);

    // Check if any publications have already sent a REQUEST_ACK message.
    wr_len = writers.length ();
    for( unsigned int index = 0; index < wr_len; ++index) {
      WriterMapType::iterator where
        = this->writers_.find( writers[ index].writerId);
      if( where != this->writers_.end()) {
        ACE_Time_Value now = ACE_OS::gettimeofday();

        ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);
        if( where->second.should_ack( now)) {
          SequenceNumber sequence = where->second.last_sequence();
          ::DDS::Time_t timenow = time_value_to_time(now);
          bool result = this->send_sample_ack(
                          writers[ index].writerId,
                          sequence.value_,
                          timenow
                        );
          if( result) {
            where->second.clear_acks( sequence);
          }
        }
      }
    }

    //
    // LIVELINESS policy timers are managed here.
    //
    if (liveliness_lease_duration_  != ACE_Time_Value::zero)
    {
      // this call will start the timer if it is not already set
      ACE_Time_Value now = ACE_OS::gettimeofday ();
      if (DCPS_debug_level >= 5) {
        RepoIdConverter converter(subscription_id_);
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::add_associations: ")
          ACE_TEXT("starting/resetting liveliness timer for reader %s\n"),
          std::string(converter).c_str()
        ));
      }
      this->handle_timeout (now, this);
    }
    // else - no timer needed when LIVELINESS.lease_duration is INFINITE

  }
  //
  // We no longer hold the publication_handle_lock_.
  //

  //
  // We only do the following processing for readers that are *not*
  // readers of Builtin Topics.
  //
  if (! is_bit_)
    {
      //
      // We derive a list of writer Id values corresponding to the writer
      // argument list.
      //
      WriterIdSeq wr_ids;
      CORBA::ULong wr_len = writers.length ();
      wr_ids.length (wr_len);

      for (CORBA::ULong i = 0; i < wr_len; ++i)
      {
        wr_ids[i] = writers[i].writerId;
      }

      //
      // Here we convert the list of writer Id values to local handle
      // values.
      //
      ::DDS::InstanceHandleSeq handles;
      if (this->bit_lookup_instance_handles (wr_ids, handles) == false)
        return;

      //
      // We acquire the publication_handle_lock_ for the remainder of our
      // processing.
      //
      ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);
      wr_len = handles.length ();

      for( unsigned int index = 0; index < wr_len; ++index) {
        // This insertion is idempotent.
        this->id_to_handle_map_.insert(
          RepoIdToHandleMap::value_type( wr_ids[ index], handles[ index])
        );
        if( DCPS_debug_level > 4) {
          RepoIdConverter converter(wr_ids[index]);
          ACE_DEBUG((LM_WARNING,
            ACE_TEXT("(%P|%t) DataReaderImpl::add_associations: ")
            ACE_TEXT("id_to_handle_map_[ %s] = 0x%x.\n"),
            std::string(converter).c_str(),
            handles[index]
          ));
        }
      }

      // We need to adjust these after the insertions have all completed
      // since insertions are not guaranteed to increase the number of
      // currently matched publications.
      int matchedPublications = this->id_to_handle_map_.size();
      this->subscription_match_status_.current_count_change
        = matchedPublications - this->subscription_match_status_.current_count;
      this->subscription_match_status_.current_count = matchedPublications;

      ++this->subscription_match_status_.total_count;
      ++this->subscription_match_status_.total_count_change;

      this->subscription_match_status_.last_publication_handle
        = handles[ wr_len - 1];

      set_status_changed_flag (::DDS::SUBSCRIPTION_MATCH_STATUS, true);

      ::DDS::DataReaderListener* listener
        = listener_for (::DDS::SUBSCRIPTION_MATCH_STATUS);
      if( listener != 0) {
        listener->on_subscription_match(
          dr_local_objref_.in (),
          this->subscription_match_status_
        );

        // TBD - why does the spec say to change this but not change
        //       the ChangeFlagStatus after a listener call?

        // Client will look at it so next time it looks the change should be 0
        this->subscription_match_status_.total_count_change = 0;
        this->subscription_match_status_.current_count_change = 0;
      }
      notify_status_condition();
    }
}


void DataReaderImpl::remove_associations (
                                          const OpenDDS::DCPS::WriterIdSeq & writers,
                                          ::CORBA::Boolean notify_lost
                                          )
  ACE_THROW_SPEC ((
                   CORBA::SystemException
                   ))
{
  DBG_ENTRY_LVL("DataReaderImpl","remove_associations",6);

  if (DCPS_debug_level >= 1)
  {
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(writers[0]);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::remove_associations: ")
      ACE_TEXT("bit %d local %s remote %s num remotes %d \n"),
      is_bit_,
      std::string(reader_converter).c_str(),
      std::string(writer_converter).c_str(),
      writers.length ()
    ));
  }

  ::DDS::InstanceHandleSeq handles;

  ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

  // This is used to hold the list of writers which were actually
  // removed, which is a proper subset of the writers which were
  // requested to be removed.
  WriterIdSeq updated_writers;

  //Remove the writers from writer list. If the supplied writer
  //is not in the cached writers list then it is already removed.
  //We just need remove the writers in the list that have not been
  //removed.
  CORBA::ULong wr_len = writers.length ();
  for (CORBA::ULong i = 0; i < wr_len; i++)
  {
    PublicationId writer_id = writers[i];

    WriterMapType::iterator it = this->writers_.find (writer_id);
    if (it != this->writers_.end())
    {
      it->second.removed ();
    }

    if (this->writers_.erase(writer_id) == 0)
    {
      if (DCPS_debug_level >= 1)
      {
        RepoIdConverter converter(writer_id);
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::remove_associations: ")
          ACE_TEXT("the writer local %s was already removed.\n"),
          std::string(converter).c_str()
        ));
      }
    }
    else
    {
      CORBA::Long len = updated_writers.length ();
      updated_writers.length (len + 1);
      updated_writers[len] = writer_id;
    }
  }

  wr_len = updated_writers.length ();

  // Return now if the supplied writers have been removed already.
  if (wr_len == 0)
  {
    return;
  }

  if (! is_bit_)
  {
    // The writer should be in the id_to_handle map at this time.  Note
    // it if it not there.
    if (this->cache_lookup_instance_handles (updated_writers, handles) == false)
    {
      if( DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::remove_associations: ")
          ACE_TEXT("cache_lookup_instance_handles failed.\n")
        ));
      }
    }

    for (CORBA::ULong i = 0; i < wr_len; ++i)
    {
      id_to_handle_map_.erase( updated_writers[i]);
    }
  }

  this->subscriber_servant_->remove_associations(updated_writers, this->subscription_id_);

  // Mirror the add_associations SUBSCRIPTION_MATCHED_STATUS processing.
  if( !this->is_bit_) {
    // Derive the change in the number of publications writing to this reader.
    int matchedPublications = this->id_to_handle_map_.size();
    this->subscription_match_status_.current_count_change
      = matchedPublications - this->subscription_match_status_.current_count;

    // Only process status if the number of publications has changed.
    if( this->subscription_match_status_.current_count_change != 0) {
      this->subscription_match_status_.current_count = matchedPublications;

      /// Section 7.1.4.1: total_count will not decrement.

      /// @TODO: Reconcile this with the verbiage in section 7.1.4.1
      this->subscription_match_status_.last_publication_handle
        = handles[ wr_len - 1];

      set_status_changed_flag (::DDS::SUBSCRIPTION_MATCH_STATUS, true);

      ::DDS::DataReaderListener* listener
        = listener_for (::DDS::SUBSCRIPTION_MATCH_STATUS);
      if( listener != 0) {
        listener->on_subscription_match(
          dr_local_objref_.in (),
          this->subscription_match_status_
        );

        // Client will look at it so next time it looks the change should be 0
        this->subscription_match_status_.total_count_change = 0;
        this->subscription_match_status_.current_count_change = 0;
      }
      notify_status_condition();
    }
  }

  // If this remove_association is invoked when the InfoRepo
  // detects a lost writer then make a callback to notify
  // subscription lost.
  if (notify_lost)
  {
    this->notify_subscription_lost (handles);
  }
}


void DataReaderImpl::remove_all_associations ()
{
  DBG_ENTRY_LVL("DataReaderImpl","remove_all_associations",6);

  OpenDDS::DCPS::WriterIdSeq writers;

  ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

  int size = writers_.size();
  writers.length(size);
  WriterMapType::iterator curr_writer = writers_.begin();
  WriterMapType::iterator end_writer = writers_.end();

  int i = 0;
  while (curr_writer != end_writer)
    {
      writers[i++] = curr_writer->first;
      ++curr_writer;
    }

  try
    {
      CORBA::Boolean dont_notify_lost = 0;
      if (0 < size)
        {
          remove_associations(writers, dont_notify_lost);
        }
    }
  catch (const CORBA::Exception&)
    {
    }
}


void DataReaderImpl::update_incompatible_qos (
    const OpenDDS::DCPS::IncompatibleQosStatus & status)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ::DDS::DataReaderListener* listener =
    listener_for (::DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS);

  ACE_GUARD (ACE_Recursive_Thread_Mutex,
             guard,
             this->publication_handle_lock_);

  if( this->requested_incompatible_qos_status_.total_count == status.total_count) {
    // This test should make the method idempotent.
    return;
  }

  set_status_changed_flag(::DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS,
                          true);

  // copy status and increment change
  requested_incompatible_qos_status_.total_count = status.total_count;
  requested_incompatible_qos_status_.total_count_change +=
    status.count_since_last_send;
  requested_incompatible_qos_status_.last_policy_id =
    status.last_policy_id;
  requested_incompatible_qos_status_.policies = status.policies;

  if (listener != 0)
    {
      listener->on_requested_incompatible_qos (dr_local_objref_.in (),
                                               requested_incompatible_qos_status_);


      // TBD - why does the spec say to change total_count_change but not
      // change the ChangeFlagStatus after a listener call?

      // client just looked at it so next time it looks the
      // change should be 0
      requested_incompatible_qos_status_.total_count_change = 0;
    }
  notify_status_condition();
}

::DDS::ReadCondition_ptr DataReaderImpl::create_readcondition (
    ::DDS::SampleStateMask sample_states,
    ::DDS::ViewStateMask view_states,
    ::DDS::InstanceStateMask instance_states)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_, 0);
  DDS::ReadCondition_var rc = new ReadConditionImpl(this, sample_states,
    view_states, instance_states);
  read_conditions_.insert(rc);
  return rc._retn();
}

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
::DDS::QueryCondition_ptr DataReaderImpl::create_querycondition (
    ::DDS::SampleStateMask sample_states,
    ::DDS::ViewStateMask view_states,
    ::DDS::InstanceStateMask instance_states,
    const char* query_expression,
    const ::DDS::StringSeq& query_parameters)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_, 0);
  ::DDS::QueryCondition_var qc = new QueryConditionImpl(this, sample_states,
    view_states, instance_states, query_expression, query_parameters);
  ::DDS::ReadCondition_var rc = ::DDS::ReadCondition::_duplicate(qc);
  read_conditions_.insert(rc);
  return qc._retn();
}
#endif

bool DataReaderImpl::has_readcondition(::DDS::ReadCondition_ptr a_condition)
{
  //sample lock already held
  ::DDS::ReadCondition_var rc = ::DDS::ReadCondition::_duplicate(a_condition);
  return read_conditions_.find(rc) != read_conditions_.end();
}

::DDS::ReturnCode_t DataReaderImpl::delete_readcondition (
    ::DDS::ReadCondition_ptr a_condition)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
    ::DDS::RETCODE_OUT_OF_RESOURCES);
  ::DDS::ReadCondition_var rc = ::DDS::ReadCondition::_duplicate(a_condition);
  return read_conditions_.erase(rc)
    ? ::DDS::RETCODE_OK : ::DDS::RETCODE_PRECONDITION_NOT_MET;
}

::DDS::ReturnCode_t DataReaderImpl::delete_contained_entities ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (DCPS_debug_level >= 2)
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT("(%P|%t) ")
                ACE_TEXT("DataReaderImpl::delete_contained_entities\n")));
  return ::DDS::RETCODE_OK;
}

::DDS::ReturnCode_t DataReaderImpl::set_qos (
                                             const ::DDS::DataReaderQos & qos
                                             )
  ACE_THROW_SPEC ((
                   CORBA::SystemException
                   ))
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos))
  {
    if (qos_ == qos)
      return ::DDS::RETCODE_OK;

    if (enabled_.value())
    {
      if (! Qos_Helper::changeable (qos_, qos))
      {
        return ::DDS::RETCODE_IMMUTABLE_POLICY;
      }
      else
      {
        try
        {
          DCPSInfo_var repo = TheServiceParticipant->get_repository(domain_id_);
          ::DDS::SubscriberQos subscriberQos;
          this->subscriber_servant_->get_qos(subscriberQos);
          repo->update_subscription_qos(this->participant_servant_->get_domain_id(),
                                        this->participant_servant_->get_id(),
                                        this->subscription_id_,
                                        qos,
                                        subscriberQos);
        }
        catch (const CORBA::SystemException& sysex)
        {
          sysex._tao_print_exception (
            "ERROR: System Exception"
            " in DataReaderImpl::set_qos");
          return ::DDS::RETCODE_ERROR;
        }
        catch (const CORBA::UserException& userex)
        {
          userex._tao_print_exception (
            "ERROR:  Exception"
            " in DataReaderImpl::set_qos");
          return ::DDS::RETCODE_ERROR;
        }
      }
    }

    // Reset the deadline timer if the period has changed.
    if (qos_.deadline.period.sec != qos.deadline.period.sec
      || qos_.deadline.period.nanosec != qos.deadline.period.nanosec)
    {
      if (qos_.deadline.period.sec == ::DDS::DURATION_INFINITY_SEC
        && qos_.deadline.period.nanosec == ::DDS::DURATION_INFINITY_NSEC)
      {
        ACE_auto_ptr_reset (this->watchdog_,
          new RequestedDeadlineWatchdog (
          this->reactor_,
          this->sample_lock_,
          qos.deadline,
          this,
          this->dr_local_objref_.in (),
          this->requested_deadline_missed_status_,
          this->last_deadline_missed_total_count_));
      }
      else if (qos.deadline.period.sec == ::DDS::DURATION_INFINITY_SEC
        && qos.deadline.period.nanosec == ::DDS::DURATION_INFINITY_NSEC)
      {
        this->watchdog_->cancel_all ();
        this->watchdog_.reset ();
      }
      else
      {
        this->watchdog_->reset_interval (
          duration_to_time_value (qos.deadline.period));
      }
    }

    qos_ = qos;

    return ::DDS::RETCODE_OK;
  }
  else
  {
    return ::DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

void DataReaderImpl::get_qos (
                              ::DDS::DataReaderQos & qos
                              )
  ACE_THROW_SPEC ((
                   CORBA::SystemException
                   ))
{
  qos = qos_;
}

::DDS::ReturnCode_t DataReaderImpl::set_listener (
                                                  ::DDS::DataReaderListener_ptr a_listener,
                                                  ::DDS::StatusKindMask mask
                                                  )
  ACE_THROW_SPEC ((
                   CORBA::SystemException
                   ))
{
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = ::DDS::DataReaderListener::_duplicate(a_listener);
  fast_listener_ = listener_.in ();
  return ::DDS::RETCODE_OK;
}

::DDS::DataReaderListener_ptr DataReaderImpl::get_listener (
                                                            )
  ACE_THROW_SPEC ((
                   CORBA::SystemException
                   ))
{
  return ::DDS::DataReaderListener::_duplicate (listener_.in ());
}


::DDS::TopicDescription_ptr DataReaderImpl::get_topicdescription (
                                                                  )
  ACE_THROW_SPEC ((
                   CORBA::SystemException
                   ))
{
  return ::DDS::TopicDescription::_duplicate (topic_desc_.in ());
}

::DDS::Subscriber_ptr DataReaderImpl::get_subscriber (
                                                      )
  ACE_THROW_SPEC ((
                   CORBA::SystemException
                   ))
{
  return ::DDS::Subscriber::_duplicate (subscriber_servant_);
}

::DDS::SampleRejectedStatus DataReaderImpl::get_sample_rejected_status (
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

  liveliness_changed_status_.alive_count_change = 0;
  liveliness_changed_status_.not_alive_count_change = 0;

  return status;
}

::DDS::RequestedDeadlineMissedStatus
DataReaderImpl::get_requested_deadline_missed_status ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (this->sample_lock_);

  set_status_changed_flag(::DDS::REQUESTED_DEADLINE_MISSED_STATUS,
                          false);

  this->requested_deadline_missed_status_.total_count_change =
    this->requested_deadline_missed_status_.total_count
    - this->last_deadline_missed_total_count_;

  // ::DDS::RequestedDeadlineMissedStatus::last_instance_handle field
  // is updated by the RequestedDeadlineWatchdog.

  // Update for next status check.
  this->last_deadline_missed_total_count_ =
    this->requested_deadline_missed_status_.total_count;

  ::DDS::RequestedDeadlineMissedStatus const status =
      requested_deadline_missed_status_;

  return status;
}

::DDS::RequestedIncompatibleQosStatus *
DataReaderImpl::get_requested_incompatible_qos_status ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (
    this->publication_handle_lock_);

  set_status_changed_flag(::DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS,
                          false);
  ::DDS::RequestedIncompatibleQosStatus* status
      = new ::DDS::RequestedIncompatibleQosStatus;
  *status = requested_incompatible_qos_status_;
  requested_incompatible_qos_status_.total_count_change = 0;
  return status;
}

::DDS::SubscriptionMatchStatus
DataReaderImpl::get_subscription_match_status ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (
    this->publication_handle_lock_);

  set_status_changed_flag (::DDS::SUBSCRIPTION_MATCH_STATUS, false);
  ::DDS::SubscriptionMatchStatus status = subscription_match_status_;
  subscription_match_status_.total_count_change = 0;
  subscription_match_status_.current_count_change = 0;

  return status ;
}

::DDS::SampleLostStatus
DataReaderImpl::get_sample_lost_status ()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> justMe (this->sample_lock_);

  set_status_changed_flag (::DDS::SAMPLE_LOST_STATUS, false);
  ::DDS::SampleLostStatus status = sample_lost_status_;
  sample_lost_status_.total_count_change = 0;
  return status;
}

::DDS::ReturnCode_t
DataReaderImpl::wait_for_historical_data (
    const ::DDS::Duration_t & /* max_wait */)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  // Add your implementation here
  return 0;
}

::DDS::ReturnCode_t
DataReaderImpl::get_matched_publications (
    ::DDS::InstanceHandleSeq & publication_handles)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::get_matched_publications: ")
                         ACE_TEXT(" Entity is not enabled. \n")),
                        ::DDS::RETCODE_NOT_ENABLED);
    }

  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    this->publication_handle_lock_,
                    ::DDS::RETCODE_ERROR);

  // Copy out the handles for the current set of publications.
  int index = 0;
  publication_handles.length( this->id_to_handle_map_.size());
  for( RepoIdToHandleMap::iterator
       current = this->id_to_handle_map_.begin();
       current != this->id_to_handle_map_.end();
       ++current, ++index
     ) {
    publication_handles[ index] = current->second;
  }

  return ::DDS::RETCODE_OK;
}

#if !defined (DDS_HAS_MINIMUM_BIT)
::DDS::ReturnCode_t
DataReaderImpl::get_matched_publication_data (
    ::DDS::PublicationBuiltinTopicData & publication_data,
    ::DDS::InstanceHandle_t publication_handle)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::")
                         ACE_TEXT("get_matched_publication_data: ")
                         ACE_TEXT("Entity is not enabled. \n")),
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
#endif // !defined (DDS_HAS_MINIMUM_BIT)

::DDS::ReturnCode_t
DataReaderImpl::enable ()
  ACE_THROW_SPEC ((CORBA::SystemException))
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
      // use 2147483647L because that is the greatest value a signed
      // CORBA::Long can have.
      // WARNING: The client risks running out of memory in this case.
      depth_ = 2147483647L;
    }

  if (qos_.resource_limits.max_samples != ::DDS::LENGTH_UNLIMITED)
    {
      n_chunks_ = qos_.resource_limits.max_samples;
    }
  //else using value from Service_Participant

  // enable the type specific part of this DataReader
  this->enable_specific ();

  //Note: the QoS used to set n_chunks_ is Changable=No so
  // it is OK that we cannot change the size of our allocators.
  rd_allocator_ = new ReceivedDataAllocator(n_chunks_);
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


  // Setup the requested deadline watchdog if the configured deadline
  // period is not the default (infinite).
  ::DDS::Duration_t const deadline_period = this->qos_.deadline.period;
  if (this->watchdog_.get () == 0
    && (deadline_period.sec != ::DDS::DURATION_INFINITY_SEC
    || deadline_period.nanosec != ::DDS::DURATION_INFINITY_NSEC))
  {
    ACE_auto_ptr_reset (this->watchdog_,
      new RequestedDeadlineWatchdog (
      this->reactor_,
      this->sample_lock_,
      this->qos_.deadline,
      this,
      this->dr_local_objref_.in (),
      this->requested_deadline_missed_status_,
      this->last_deadline_missed_total_count_));
  }

  this->set_enabled ();

  CORBA::String_var name = topic_servant_->get_name ();

  subscriber_servant_->reader_enabled(
                      dr_remote_objref_.in (),
                      dr_local_objref_.in (),
                      this,
                                      name.in(),
                                      topic_servant_->get_id());

  return ::DDS::RETCODE_OK;
}


void
DataReaderImpl::writer_activity(PublicationId writer_id)
{
  // caller should have the sample_lock_ !!!

  WriterMapType::iterator iter = writers_.find(writer_id);
  if( iter != writers_.end()) {
      ACE_Time_Value when = ACE_OS::gettimeofday ();
      iter->second.received_activity (when);

  } else if( DCPS_debug_level > 4) {
    // This may not be an error since it could happen that the sample
    // is delivered to the datareader after the write is dis-associated
    // with this datareader.
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(writer_id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::writer_activity: ")
      ACE_TEXT("reader %s is not associated with writer %s.\n"),
      std::string(reader_converter).c_str(),
      std::string(writer_converter).c_str()
    ));
  }
}

void
DataReaderImpl::data_received(const ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("DataReaderImpl","data_received",6);

  // ensure some other thread is not changing the sample container
  // or statuses related to samples.
  ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);

  if( DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << sample.header_ << std::ends;
    RepoIdConverter converter(subscription_id_);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::data_received: ")
      ACE_TEXT("%s received sample: %s.\n"),
      std::string(converter).c_str(),
      buffer.str().c_str()
    ));
  }

  switch (sample.header_.message_id_)
    {
    case SAMPLE_DATA:
    case INSTANCE_REGISTRATION:
      {
        DataSampleHeader const & header = sample.header_;

        this->writer_activity(header.publication_id_);

        // Verify data has not exceeded its lifespan.
        if (this->data_expired (header))
        {
          // Data expired.  Do not demarshal the data.  Simply allow
          // the caller to deallocate the data buffer.
          break;
        }

        // This adds the reader to the set/list of readers with data.
        this->subscriber_servant_->data_received(this);

        // Only gather statistics about real samples, not registration data, etc.
        if (header.message_id_ == SAMPLE_DATA) {
          this->process_latency (sample);
        }

        // This also adds to the sample container and makes any callbacks
        // and condition modifications.

        SubscriptionInstance* instance = 0;
        bool is_new_instance = false;
        this->dds_demarshal(sample, instance, is_new_instance);

        WriterMapType::iterator where
          = this->writers_.find( header.publication_id_);
        if( where != this->writers_.end()) {
          where->second.last_sequence( SequenceNumber(header.sequence_));

          ACE_Time_Value now = ACE_OS::gettimeofday();
          if( where->second.should_ack( now)) {
            ::DDS::Time_t timenow = time_value_to_time(now);
            bool result = this->send_sample_ack(
                            header.publication_id_,
                            header.sequence_,
                            timenow
                          );
            if( result) {
              where->second.clear_acks( SequenceNumber(header.sequence_));
            }
          }

        } else {
          RepoIdConverter subscriptionBuffer( this->subscription_id_);
          RepoIdConverter publicationBuffer(  header.publication_id_);
          ACE_DEBUG((LM_WARNING,
            ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::data_received() - ")
            ACE_TEXT("subscription %s failed to find ")
            ACE_TEXT("publication data for %s.\n"),
            std::string( subscriptionBuffer).c_str(),
            std::string( publicationBuffer).c_str()
          ));
        }

        if (this->watchdog_.get ())
        {
          instance->last_sample_tv_ = instance->cur_sample_tv_;
          instance->cur_sample_tv_ = ACE_OS::gettimeofday (); 

          if (is_new_instance)
          {
            this->watchdog_->schedule_timer (instance);
          }
          else
          {
            this->watchdog_->execute ((void const *)instance, false);
          }
        }

        this->notify_read_conditions();
      }
      break;

    case REQUEST_ACK:
      {
        this->writer_activity( sample.header_.publication_id_);

        SequenceNumber ack;
        ::DDS::Duration_t delay;
        ::TAO::DCPS::Serializer serializer(
          sample.sample_,
          sample.header_.byte_order_ != TAO_ENCAP_BYTE_ORDER
        );
        serializer >> ack.value_;
        serializer >> delay;

        if( DCPS_debug_level > 9) {
          RepoIdConverter debugConverter( sample.header_.publication_id_);
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DataReaderImpl::data_received() - ")
            ACE_TEXT("publication %s received REQUEST_ACK for sequence 0x%x ")
            ACE_TEXT("valid for the next %d seconds.\n"),
            std::string( debugConverter).c_str(),
            ACE_UINT16(ack.value_),
            delay.sec
          ));
        }

        WriterMapType::iterator where
          = this->writers_.find( sample.header_.publication_id_);
        if( where != this->writers_.end()) {
          ACE_Time_Value now      = ACE_OS::gettimeofday();
          ACE_Time_Value deadline = now + duration_to_time_value( delay);

          where->second.ack_deadline( ack, deadline);

          if( where->second.should_ack( now)) {
            ::DDS::Time_t timenow = time_value_to_time(now);
            bool result = this->send_sample_ack(
                            sample.header_.publication_id_,
                            ack.value_,
                            timenow
                          );
            if( result) {
              where->second.clear_acks( ack);
            }
          }

        } else {
          RepoIdConverter subscriptionBuffer( this->subscription_id_);
          RepoIdConverter publicationBuffer(  sample.header_.publication_id_);
          ACE_DEBUG((LM_WARNING,
            ACE_TEXT("(%P|%t) WARNING: DataReaderImpl::data_received() - ")
            ACE_TEXT("subscription %s failed to find ")
            ACE_TEXT("publication data for %s.\n"),
            std::string( subscriptionBuffer).c_str(),
            std::string( publicationBuffer).c_str()
          ));
        }

      }
      break;

    case DATAWRITER_LIVELINESS:
      {
        this->writer_activity(sample.header_.publication_id_);

        // tell all instances they got a liveliness message
        for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
             iter != instances_.end();
             ++iter)
          {
            SubscriptionInstance *ptr = iter->second;

            ptr->instance_state_.lively (sample.header_.publication_id_);
          }

      }
      break;

    case DISPOSE_INSTANCE:
      {
        this->writer_activity(sample.header_.publication_id_);
        SubscriptionInstance* instance = 0;
        this->dispose(sample, instance);
        if (this->watchdog_.get ())
          this->watchdog_->cancel_timer (instance);
      }
      this->notify_read_conditions();
      break;

    case UNREGISTER_INSTANCE:
      {
        this->writer_activity(sample.header_.publication_id_);
        SubscriptionInstance* instance = 0;
        this->unregister(sample, instance);
        if (this->watchdog_.get ())
          this->watchdog_->cancel_timer (instance);
      }
      this->notify_read_conditions();
      break;


    default:
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: DataReaderImpl::data_received"
                 "unexpected message_id = %d\n",
                 sample.header_.message_id_));
      break;
    }
}

bool
DataReaderImpl::send_sample_ack(
  const RepoId& publication,
  ACE_INT16 sequence,
  ::DDS::Time_t when
)
{
  size_t dataSize = sizeof( sequence);
  dataSize += _dcps_find_size( publication);

  ACE_Message_Block* data;
  ACE_NEW_RETURN( data, ACE_Message_Block( dataSize), false);

  bool doSwap    = this->subscriber_servant_->swap_bytes();
  bool byteOrder = (doSwap? !TAO_ENCAP_BYTE_ORDER: TAO_ENCAP_BYTE_ORDER);

  ::TAO::DCPS::Serializer serializer( data, doSwap);
  serializer << publication;
  serializer << sequence;

  DataSampleHeader outbound_header;
  outbound_header.message_id_               = SAMPLE_ACK;
  outbound_header.byte_order_               = byteOrder,
  outbound_header.message_length_           = data->total_length ();
  outbound_header.sequence_                 = 0;
  outbound_header.source_timestamp_sec_     = when.sec;
  outbound_header.source_timestamp_nanosec_ = when.nanosec;
  outbound_header.coherency_group_          = 0;
  outbound_header.publication_id_           = this->subscription_id_;

  ACE_Message_Block* sample_ack;
  ACE_NEW_RETURN(
    sample_ack,
    ACE_Message_Block(
      outbound_header.max_marshaled_size(),
      ACE_Message_Block::MB_DATA,
      data // cont
    ), false
  );
  sample_ack << outbound_header;

  if( DCPS_debug_level > 0) {
    RepoIdConverter subscriptionBuffer( this->subscription_id_);
    RepoIdConverter publicationBuffer(  publication);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::send_sample_ack() - ")
      ACE_TEXT("%s sending SAMPLE_ACK message with sequence 0x%x ")
      ACE_TEXT("to publication %s.\n"),
      std::string( subscriptionBuffer).c_str(),
      ACE_UINT16(sequence),
      std::string( publicationBuffer).c_str()
    ));
  }

  return this->subscriber_servant_->send_response( publication, sample_ack);
}

void DataReaderImpl::notify_read_conditions()
{
  //sample lock is already held
  ReadConditionSet local_read_conditions = read_conditions_;
  ACE_GUARD(Reverse_Lock_t, unlock_guard, reverse_sample_lock_);
  for (ReadConditionSet::iterator it = local_read_conditions.begin(),
    end = local_read_conditions.end(); it != end; ++it)
    {
      dynamic_cast<ConditionImpl*>(it->in())->signal_all();
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
  subscription_id_ = subscription_id;
}

char *
DataReaderImpl::get_topic_name() const
{
  return topic_servant_->get_name();
}

bool DataReaderImpl::have_sample_states(
                                        ::DDS::SampleStateMask sample_states) const
{
  //!!! caller should have acquired sample_lock_

  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter)
    {
      SubscriptionInstance *ptr = iter->second;

      for (ReceivedDataElement *item = ptr->rcvd_sample_.head_;
           item != 0; item = item->next_data_sample_)
        {
          if (item->sample_state_ & sample_states)
            {
              return true;
            }
        }
    }
  return false;
}

bool
DataReaderImpl::have_view_states(::DDS::ViewStateMask view_states) const
{
  //!!! caller should have acquired sample_lock_
  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter)
    {
      SubscriptionInstance *ptr = iter->second;

      if (ptr->instance_state_.view_state() & view_states)
        {
          return true;
        }
    }
  return false;
}

bool DataReaderImpl::have_instance_states(
                                          ::DDS::InstanceStateMask instance_states) const
{
  //!!! caller should have acquired sample_lock_
  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter)
    {
      SubscriptionInstance *ptr = iter->second;

      if (ptr->instance_state_.instance_state() & instance_states)
        {
          return true;
        }
    }
  return false;
}

/// Fold-in the three separate loops of have_sample_states(),
/// have_view_states(), and have_instance_states().  Takes the sample_lock_.
bool DataReaderImpl::contains_sample(::DDS::SampleStateMask sample_states,
  ::DDS::ViewStateMask view_states, ::DDS::InstanceStateMask instance_states) 
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, false);
  for (SubscriptionInstanceMapType::iterator iter = instances_.begin(),
    end = instances_.end(); iter != end; ++iter)
    {
      SubscriptionInstance& inst = *iter->second;
      if ((inst.instance_state_.view_state() & view_states) &&
        (inst.instance_state_.instance_state() & instance_states))
        {
          //if the sample state mask is "don't care" we can skip the inner loop
          //(as long as there's at least one sample)
          if ((sample_states & ::DDS::ANY_SAMPLE_STATE)
            == ::DDS::ANY_SAMPLE_STATE && inst.rcvd_sample_.head_)
            {
              return true;
            }
          for (ReceivedDataElement* item = inst.rcvd_sample_.head_; item != 0;
            item = item->next_data_sample_)
            {
              if (item->sample_state_ & sample_states)
                {
                  return true;
                }
            }
        }
    }
  return false;
}

::DDS::DataReaderListener*
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

void DataReaderImpl::sample_info(::DDS::SampleInfo & sample_info,
                                 const ReceivedDataElement *ptr)
{

  sample_info.sample_rank = 0;

  // generation_rank =
  //    (MRSIC.disposed_generation_count +
  //     MRSIC.no_writers_generation_count)
  //  - (S.disposed_generation_count +
  //     S.no_writers_generation_count)
  //
  sample_info.generation_rank =
    (sample_info.disposed_generation_count +
     sample_info.no_writers_generation_count) -
    sample_info.generation_rank;

  // absolute_generation_rank =
  //     (MRS.disposed_generation_count +
  //      MRS.no_writers_generation_count)
  //   - (S.disposed_generation_count +
  //      S.no_writers_generation_count)
  //
  sample_info.absolute_generation_rank =
    (ptr->disposed_generation_count_ +
     ptr->no_writers_generation_count_) -
    sample_info.absolute_generation_rank;
}

CORBA::Long DataReaderImpl::total_samples() const
{
  //!!! caller should have acquired sample_lock_
  CORBA::Long count(0);
  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter)
    {
      SubscriptionInstance *ptr = iter->second;

      count += ptr->rcvd_sample_.size_;
    }

  return count;
}

int
DataReaderImpl::handle_timeout (const ACE_Time_Value &tv,
                                const void * arg)
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
    guard,
    this->sample_lock_,
    ::DDS::RETCODE_ERROR);

  if (liveliness_timer_id_ != -1)
  {
    if (arg == this)
    {

      if (DCPS_debug_level >= 5) {
        RepoIdConverter converter(subscription_id_);
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::handle_timeout: ")
          ACE_TEXT(" canceling timer for reader %s.\n"),
          std::string(converter).c_str()
        ));
      }

      // called from add_associations and there is already a timer
      // so cancel the existing timer.
      if (reactor_->cancel_timer (this->liveliness_timer_id_, &arg) == -1)
      {
        // this could fail because the reactor's call and
        // the add_associations' call to this could overlap
        // so it is not a failure.
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::handle_timeout: ")
          ACE_TEXT(" %p. \n"), "cancel_timer" ));
      }
      liveliness_timer_id_ = -1;
    }
  }

  ACE_Time_Value smallest(ACE_Time_Value::max_time);
  ACE_Time_Value next_absolute;
  int alive_writers = 0;

  // Iterate over each writer to this reader
  for (WriterMapType::iterator iter = writers_.begin();
    iter != writers_.end();
    ++iter)
  {
    // deal with possibly not being alive or
    // tell when it will not be alive next (if no activity)
    next_absolute = iter->second.check_activity(tv);

    if (next_absolute != ACE_Time_Value::max_time)
    {
      alive_writers++;
      if (next_absolute < smallest)
      {
        smallest = next_absolute;
      }
    }
  }

  if (DCPS_debug_level >= 5) {
    RepoIdConverter converter(subscription_id_);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::handle_timeout: ")
      ACE_TEXT("reader %s has %d live writers; from_reactor=%d\n"),
      std::string(converter).c_str(),
      alive_writers,
      arg == this ? 0 : 1
    ));
  }

  if (alive_writers)
  {
    ACE_Time_Value relative;
    ACE_Time_Value now = ACE_OS::gettimeofday ();
    // compare the time now with the earliest(smallest) deadline we found
    if (now < smallest)
      relative = smallest - now;
    else
      relative = ACE_Time_Value(0,1); // ASAP

   liveliness_timer_id_ = reactor_->schedule_timer(this, 0, relative);
    if (liveliness_timer_id_ == -1)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::handle_timeout: ")
        ACE_TEXT(" %p. \n"), "schedule_timer" ));
    }
  }
  else {
    // no live writers so no need to schedule a timer
    // but be sure we don't try to cancel the timer later.
    liveliness_timer_id_ = -1;
  }

  return 0;
}


void
DataReaderImpl::release_instance (::DDS::InstanceHandle_t handle)
{
  ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);
  SubscriptionInstance* instance = this->get_handle_instance (handle);

  if (instance == 0)
  {
    ACE_ERROR ((LM_ERROR, "(%P|%t) DataReaderImpl::release_instance "
      "could not find the instance by handle 0x%x\n", handle));
    return;
  }

  if (instance->rcvd_sample_.size_ == 0)
  {
    this->instances_.erase (handle);
    this->release_instance_i (handle);
  }
  else
  {
    ACE_ERROR ((LM_ERROR, "(%P|%t) DataReaderImpl::release_instance "
      "Can not release the instance (handle=0x%x) since it still has samples.\n",
      handle));
  }
}


OpenDDS::DCPS::WriterInfo::WriterInfo ()
  : last_liveliness_activity_time_(ACE_OS::gettimeofday()),
    state_(NOT_SET),
    reader_(0),
    writer_id_( GUID_UNKNOWN )
{
}

OpenDDS::DCPS::WriterInfo::WriterInfo (DataReaderImpl* reader,
                                   PublicationId   writer_id)
  : last_liveliness_activity_time_(ACE_OS::gettimeofday()),
    state_(NOT_SET),
    reader_(reader),
    writer_id_(writer_id)
{
  if (DCPS_debug_level >= 5) {
    RepoIdConverter writer_converter(writer_id);
    RepoIdConverter reader_converter(reader->subscription_id_);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) WriterInfo::WriterInfo: ")
      ACE_TEXT("writer %s added to reader %s.\n"),
      std::string(writer_converter).c_str(),
      std::string(reader_converter).c_str()
    ));
  }
}

ACE_Time_Value
OpenDDS::DCPS::WriterInfo::check_activity (const ACE_Time_Value& now)
{
  ACE_Time_Value expires_at = ACE_Time_Value::max_time;

  // We only need check the liveliness with the non-zero liveliness_lease_duration_.
  // if (state_ != DEAD && reader_->liveliness_lease_duration_ != ACE_Time_Value::zero)
  if (state_ == ALIVE && reader_->liveliness_lease_duration_ != ACE_Time_Value::zero)
  {
    expires_at = this->last_liveliness_activity_time_ +
      reader_->liveliness_lease_duration_;
    if (expires_at <= now)
    {
      // let all instances know this write is not alive.
      reader_->writer_became_dead(writer_id_, now, state_);
      expires_at = ACE_Time_Value::max_time;
    }
  }
  return expires_at;
}


void OpenDDS::DCPS::WriterInfo::removed ()
{
  reader_->writer_removed (writer_id_, this->state_);
}

SequenceNumber
OpenDDS::DCPS::WriterInfo::last_sequence() const
{
  // sample_lock_ is held by the caller.
  
  return this->last_sequence_;
}

void
OpenDDS::DCPS::WriterInfo::last_sequence(
  SequenceNumber sequence
)
{
  // sample_lock_ is held by the caller.

  if( this->last_sequence_ < sequence) {
    this->last_sequence_ = sequence;
  }
}

void
OpenDDS::DCPS::WriterInfo::clear_acks(
  SequenceNumber sequence
)
{
  // sample_lock_ is held by the caller.

  DeadlineList::iterator current
    = this->ack_deadlines_.begin();
  while( current != this->ack_deadlines_.end()) {
    if( current->first <= sequence) {
      current = this->ack_deadlines_.erase( current);

    } else {
      break;
    }
  }
}

bool
OpenDDS::DCPS::WriterInfo::should_ack(
  ACE_Time_Value now
)
{
  // sample_lock_ is held by the caller.

  if( this->ack_deadlines_.size() == 0) {
    return false;
  }

  DeadlineList::iterator current
    = this->ack_deadlines_.begin();
  while( current != this->ack_deadlines_.end()) {
    if( current->second < now) {
      // Remove any expired response deadlines.
      current = this->ack_deadlines_.erase( current);

    } else {
      // NOTE: This assumes no out of order reception.
      if( current->first <= this->last_sequence_) {
        return true;
      }
      ++current;
    }
  }
  return false;
}

void
OpenDDS::DCPS::WriterInfo::ack_deadline( SequenceNumber sequence, ACE_Time_Value when)
{
  // sample_lock_ is held by the caller.

  if( this->ack_deadlines_.size() == 0) {
    this->ack_deadlines_.push_back( std::make_pair( sequence, when));
    return;
  }

  for( DeadlineList::iterator current
         = this->ack_deadlines_.begin();
       current != this->ack_deadlines_.end();
       ++current) {
    // Insertion sort.
    if( sequence < current->first) {
      this->ack_deadlines_.insert(
        current,
        std::make_pair( sequence, when)
      );
      return;

    } else if( sequence == current->first) {
      // Only update the deadline to be *later* than any existing one.
      if( current->second < when) {
        current->second = when;
      }
      return;

    } else {
      break;
    }
  }
}

OpenDDS::DCPS::WriterStats::WriterStats(
  int amount,
  DataCollector< double>::OnFull type
) : stats_( amount, type)
{
}

void OpenDDS::DCPS::WriterStats::add_stat( const ACE_Time_Value& delay)
{
  double datum = static_cast<double>( delay.sec());
  datum += delay.usec() / 1000000.0;
  this->stats_.add( datum);
}

OpenDDS::DCPS::LatencyStatistics OpenDDS::DCPS::WriterStats::get_stats() const
{
  LatencyStatistics value;

  value.publication = GUID_UNKNOWN;
  value.n           = this->stats_.n();
  value.maximum     = this->stats_.maximum();
  value.minimum     = this->stats_.minimum();
  value.mean        = this->stats_.mean();
  value.variance    = this->stats_.var();

  return value;
}

void OpenDDS::DCPS::WriterStats::reset_stats()
{
  this->stats_.reset();
}

std::ostream& OpenDDS::DCPS::WriterStats::raw_data( std::ostream& str) const
{
  str << std::dec << this->stats_.size()
      << " samples out of " << this->stats_.n() << std::endl;
  return str << this->stats_;
}

void
DataReaderImpl::writer_removed (PublicationId   writer_id,
             WriterInfo::WriterState& state)
{
  if (DCPS_debug_level >= 5) {
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(writer_id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::writer_removed: ")
      ACE_TEXT("reader %s from writer %s.\n"),
      std::string(reader_converter).c_str(),
      std::string(writer_converter).c_str()
    ));
  }

  bool liveliness_changed = false;
  if (state == WriterInfo::ALIVE)
  {
    -- liveliness_changed_status_.alive_count;
    -- liveliness_changed_status_.alive_count_change;
    liveliness_changed = true;
  }

  if (state == WriterInfo::DEAD)
  {
    -- liveliness_changed_status_.not_alive_count;
    -- liveliness_changed_status_.not_alive_count_change;
    liveliness_changed = true;
  }

  if( liveliness_changed) {
    set_status_changed_flag(::DDS::LIVELINESS_CHANGED_STATUS, true);
    this->notify_liveliness_change ();
  }
}

void
DataReaderImpl::writer_became_alive (PublicationId writer_id,
                                     const ACE_Time_Value& /* when */,
                                     WriterInfo::WriterState& state)
{
  if (DCPS_debug_level >= 5) {
    std::stringstream buffer;
    buffer << state;
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(writer_id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::writer_became_alive: ")
      ACE_TEXT("reader %s from writer %s state %s.\n"),
      std::string(reader_converter).c_str(),
      std::string(writer_converter).c_str(),
      buffer.str().c_str()
    ));
  }

  // caller should already have the samples_lock_ !!!

  // NOTE: each instance will change to ALIVE_STATE when they receive a sample

  bool liveliness_changed = false;
  if (state != WriterInfo::ALIVE)
  {
    liveliness_changed_status_.alive_count++;
    liveliness_changed_status_.alive_count_change++;
    liveliness_changed = true;
  }

  if (state == WriterInfo::DEAD)
  {
    liveliness_changed_status_.not_alive_count--;
    liveliness_changed_status_.not_alive_count_change--;
    liveliness_changed = true;
  }

  set_status_changed_flag(::DDS::LIVELINESS_CHANGED_STATUS, true);

  if (liveliness_changed_status_.alive_count < 0)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_alive: ")
                  ACE_TEXT(" invalid liveliness_changed_status alive count - %d.\n"),
      liveliness_changed_status_.alive_count));
      return;
    }
  if (liveliness_changed_status_.not_alive_count < 0)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_alive: ")
                  ACE_TEXT(" invalid liveliness_changed_status not alive count - %d .\n"),
      liveliness_changed_status_.not_alive_count));
      return;
    }

  // Change the state to ALIVE since handle_timeout may call writer_became_dead
  // which need the current state info.
  state = WriterInfo::ALIVE;

  // Call listener only when there are liveliness status changes.
  if (liveliness_changed)
  {
    this->notify_liveliness_change ();
  }

  // this call will start the livilness timer if it is not already set
  ACE_Time_Value now = ACE_OS::gettimeofday ();
  this->handle_timeout (now, this);
}

void
DataReaderImpl::writer_became_dead (PublicationId   writer_id,
                                    const ACE_Time_Value& when,
            WriterInfo::WriterState& state)
{
  if (DCPS_debug_level >= 5) {
    std::stringstream buffer;
    buffer << state;
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(writer_id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::writer_became_dead: ")
      ACE_TEXT("reader %s from writer%s state %s.\n"),
      std::string(reader_converter).c_str(),
      std::string(writer_converter).c_str(),
      buffer.str().c_str()
    ));
  }

  // caller should already have the samples_lock_ !!!
  bool liveliness_changed = false;

  if (state == OpenDDS::DCPS::WriterInfo::NOT_SET)
  {
    liveliness_changed_status_.not_alive_count++;
    liveliness_changed_status_.not_alive_count_change++;
    liveliness_changed = true;
  }

  if (state == WriterInfo::ALIVE)
  {
    liveliness_changed_status_.alive_count--;
    liveliness_changed_status_.alive_count_change--;
    liveliness_changed_status_.not_alive_count++;
    liveliness_changed_status_.not_alive_count_change++;
    liveliness_changed = true;
  }

  //update the state to DEAD.
  state = WriterInfo::DEAD;

  if (liveliness_changed_status_.alive_count < 0)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_dead: ")
                  ACE_TEXT(" invalid liveliness_changed_status alive count - %d.\n"),
      liveliness_changed_status_.alive_count));
      return;
    }
  if (liveliness_changed_status_.not_alive_count < 0)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::writer_became_dead: ")
                  ACE_TEXT(" invalid liveliness_changed_status not alive count - %d.\n"),
      liveliness_changed_status_.not_alive_count));
      return;
    }

    SubscriptionInstanceMapType::iterator iter = instances_.begin();
    SubscriptionInstanceMapType::iterator next = iter;

    while (iter != instances_.end())
    {
      ++next;
      SubscriptionInstance *ptr = iter->second;

      ptr->instance_state_.writer_became_dead (
        writer_id, liveliness_changed_status_.alive_count, when);

      iter = next;
    }


  // Call listener only when there are liveliness status changes.
  if (liveliness_changed)
  {
    set_status_changed_flag(::DDS::LIVELINESS_CHANGED_STATUS, true);
    this->notify_liveliness_change ();
  }
}


int
DataReaderImpl::handle_close (ACE_HANDLE,
                              ACE_Reactor_Mask)
{
  //this->_remove_ref ();
  return 0;
}


void
DataReaderImpl::set_sample_lost_status(
                                       const ::DDS::SampleLostStatus& status)
{
  //!!! caller should have acquired sample_lock_
  sample_lost_status_ = status;
}


void
DataReaderImpl::set_sample_rejected_status(
                                           const ::DDS::SampleRejectedStatus& status)
{
  //!!! caller should have acquired sample_lock_
  sample_rejected_status_ = status;
}


void DataReaderImpl::dispose(const ReceivedDataSample&,
                             SubscriptionInstance*&)
{
  if( DCPS_debug_level > 0) {
    ACE_DEBUG ((LM_DEBUG, "(%P|%t) DataReaderImpl::dispose()\n"));
  }
}


void DataReaderImpl::unregister(const ReceivedDataSample&,
                                SubscriptionInstance*&)
{
  if( DCPS_debug_level > 0) {
    ACE_DEBUG ((LM_DEBUG, "(%P|%t) DataReaderImpl::unregister()\n"));
  }
}


void DataReaderImpl::process_latency( const ReceivedDataSample& sample)
{
  StatsMapType::iterator location
    = this->statistics_.find( sample.header_.publication_id_);
  if( location != this->statistics_.end()) {
      // This starts as the current time.
      ACE_Time_Value  latency = ACE_OS::gettimeofday ();

      // The time interval starts at the send end.
      DDS::Duration_t then = {
                               sample.header_.source_timestamp_sec_,
                               sample.header_.source_timestamp_nanosec_
                             };

      // latency delay in ACE_Time_Value format.
      latency -= duration_to_time_value( then);

      if( this->statistics_enabled()) {
        location->second.add_stat( latency);
      }

      if( DCPS_debug_level > 9){
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::process_latency() - ")
          ACE_TEXT("measured latency of %dS, %dmS for current sample.\n"),
          latency.sec(),
          latency.msec()
        ));
      }

      // Check latency against the budget.
      if( time_value_to_duration( latency)
          > this->qos_.latency_budget.duration) {
        this->notify_latency( sample.header_.publication_id_);
      }

  } else if( DCPS_debug_level > 0){
    /// NB: This message is generated contemporaneously with a similar
    ///     message from writer_activity().  That message is not marked
    ///     as an error, so we follow that lead and leave this as an
    ///     informational message, guarded by debug level.  This seems
    ///     to be due to late samples (samples delivered after an
    ///     association has been torn down).  We may want to promote this
    ///     to a warning if other conditions causing this symptom are
    ///     discovered.
    RepoIdConverter reader_converter(subscription_id_);
    RepoIdConverter writer_converter(sample.header_.publication_id_);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::process_latency() - ")
      ACE_TEXT("reader %s is not associated with writer %s (late sample?).\n"),
      std::string(reader_converter).c_str(),
      std::string(writer_converter).c_str()
    ));
  }
}


void DataReaderImpl::notify_latency( PublicationId writer)
{
  // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
  // is given to this DataReader then narrow() fails.
  DataReaderListener_var listener
    = DataReaderListener::_narrow( this->listener_.in());
  if( ! CORBA::is_nil( listener.in())) {
    WriterIdSeq writerIds;
    writerIds.length(1);
    writerIds[ 0] = writer;

    ::DDS::InstanceHandleSeq handles;
    this->cache_lookup_instance_handles( writerIds, handles);

    if( handles.length() >= 1) {
      this->budget_exceeded_status_.last_instance_handle = handles[ 0];
    } else {
      this->budget_exceeded_status_.last_instance_handle = -1;
    }

    ++this->budget_exceeded_status_.total_count;
    ++this->budget_exceeded_status_.total_count_change;

    listener->on_budget_exceeded(
      this->dr_local_objref_.in(),
      this->budget_exceeded_status_
    );

    this->budget_exceeded_status_.total_count_change = 0;
  }
}

void
DataReaderImpl::get_latency_stats (
  ::OpenDDS::DCPS::LatencyStatisticsSeq & stats
)
ACE_THROW_SPEC (( ::CORBA::SystemException))
{
  stats.length( this->statistics_.size());
  int index = 0;
  for( StatsMapType::const_iterator current = this->statistics_.begin();
       current != this->statistics_.end();
       ++current, ++index) {
    stats[ index] = current->second.get_stats();
    stats[ index].publication = current->first;
  }
}

void
DataReaderImpl::reset_latency_stats ( void)
ACE_THROW_SPEC (( ::CORBA::SystemException))
{
  for( StatsMapType::iterator current = this->statistics_.begin();
       current != this->statistics_.end();
       ++current) {
    current->second.reset_stats();
  }
}

::CORBA::Boolean
DataReaderImpl::statistics_enabled ( void)
ACE_THROW_SPEC (( ::CORBA::SystemException))
{
  return this->statistics_enabled_;
}

void
DataReaderImpl::statistics_enabled (
  ::CORBA::Boolean statistics_enabled
)
ACE_THROW_SPEC (( ::CORBA::SystemException))
{
  this->statistics_enabled_ = statistics_enabled;
}

SubscriptionInstance*
DataReaderImpl::get_handle_instance (::DDS::InstanceHandle_t handle)
{
  SubscriptionInstanceMapType::iterator iter = instances_.find(handle);
  if (iter == instances_.end())
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("DataReaderImpl::get_handle_instance: ")
                  ACE_TEXT("lookup for 0x%x failed\n"),
                  handle));
      return 0;
    } // if (0 != instances_.find(handle, instance))
  return iter->second;
}


::DDS::InstanceHandle_t
DataReaderImpl::get_next_handle ()
{
  return ++next_handle_;
}


void
DataReaderImpl::notify_subscription_disconnected (const WriterIdSeq& pubids)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_disconnected",6);
  if (! this->is_bit_)
    {
      // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
      // is given to this DataReader then narrow() fails.
      DataReaderListener_var the_listener
        = DataReaderListener::_narrow (this->listener_.in ());
      if (! CORBA::is_nil (the_listener.in ()))
      {
        SubscriptionLostStatus status;

        // Since this callback may come after remove_association which removes
        // the writer from id_to_handle map, we can ignore this error.
        this->cache_lookup_instance_handles (pubids, status.publication_handles);
        the_listener->on_subscription_disconnected (this->dr_local_objref_.in (),
        status);
      }
    }
}


void
DataReaderImpl::notify_subscription_reconnected (const WriterIdSeq& pubids)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_reconnected",6);

  if (! this->is_bit_)
  {
    // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
    // is given to this DataReader then narrow() fails.
    DataReaderListener_var the_listener
      = DataReaderListener::_narrow (this->listener_.in ());
    if (! CORBA::is_nil (the_listener.in ()))
    {
      SubscriptionLostStatus status;

      // If it's reconnected then the reader should be in id_to_handle map otherwise
      // log with an error.
      if (this->cache_lookup_instance_handles (pubids, status.publication_handles) == false)
      {
        ACE_ERROR ((LM_ERROR, "(%P|%t) DataReaderImpl::notify_subscription_reconnected: "
          "cache_lookup_instance_handles failed.\n"));
      }
      the_listener->on_subscription_reconnected (this->dr_local_objref_.in (),
        status);
    }
  }
}


void
DataReaderImpl::notify_subscription_lost (const ::DDS::InstanceHandleSeq& handles)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_lost",6);

  if (! this->is_bit_)
  {
    // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
    // is given to this DataReader then narrow() fails.
    DataReaderListener_var the_listener
      = DataReaderListener::_narrow (this->listener_.in ());
    if (! CORBA::is_nil (the_listener.in ()))
    {
      SubscriptionLostStatus status;

      CORBA::ULong len = handles.length ();
      status.publication_handles.length (len);
      for (CORBA::ULong i = 0;i < len; ++ i)
      {
        status.publication_handles[i] = handles[i];
      }

      the_listener->on_subscription_lost (this->dr_local_objref_.in (),
        status);
    }
  }
}


void
DataReaderImpl::notify_subscription_lost (const WriterIdSeq& pubids)
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_subscription_lost",6);

  if (! this->is_bit_)
  {
    // Narrow to DDS::DCPS::DataReaderListener. If a DDS::DataReaderListener
    // is given to this DataReader then narrow() fails.
    DataReaderListener_var the_listener
      = DataReaderListener::_narrow (this->listener_.in ());
    if (! CORBA::is_nil (the_listener.in ()))
    {
      SubscriptionLostStatus status;

      // Since this callback may come after remove_association which removes
      // the writer from id_to_handle map, we can ignore this error.
      this->cache_lookup_instance_handles (pubids, status.publication_handles);
      the_listener->on_subscription_lost (this->dr_local_objref_.in (),
        status);
    }
  }
}


void
DataReaderImpl::notify_connection_deleted ()
{
  DBG_ENTRY_LVL("DataReaderImpl","notify_connection_deleted",6);

  // Narrow to DDS::DCPS::DataWriterListener. If a DDS::DataWriterListener
  // is given to this DataWriter then narrow() fails.
  DataReaderListener_var the_listener = DataReaderListener::_narrow (this->listener_.in ());

  if (! CORBA::is_nil (the_listener.in ()))
    the_listener->on_connection_deleted (this->dr_local_objref_.in ());
}

bool
DataReaderImpl::bit_lookup_instance_handles (const WriterIdSeq& ids,
                                               ::DDS::InstanceHandleSeq & hdls)
{
  if( DCPS_debug_level > 9) {
    CORBA::ULong const size = ids.length ();
    const char* separator = "";
    std::stringstream buffer;
    for( unsigned long i = 0; i < size; ++i) {
      buffer << separator << RepoIdConverter(ids[i]);
      separator = ", ";
    }
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::bit_lookup_instance_handles: ")
      ACE_TEXT("searching for handles for writer Ids: %s.\n"),
      buffer.str().c_str()
    ));
  }

  if (TheServiceParticipant->get_BIT () == true && ! TheTransientKludge->is_enabled ())
  {
#if !defined (DDS_HAS_MINIMUM_BIT)
    BIT_Helper_2 < ::DDS::PublicationBuiltinTopicDataDataReader,
      ::DDS::PublicationBuiltinTopicDataDataReader_var,
      ::DDS::PublicationBuiltinTopicDataSeq,
      WriterIdSeq > hh;

    DDS::ReturnCode_t ret = hh.repo_ids_to_instance_handles(ids, hdls);
    if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::bit_lookup_instance_handles: ")
        ACE_TEXT("failed.\n")
      ));
      return false;

    } else if( DCPS_debug_level > 4) {
      CORBA::ULong const num_wrts = ids.length ();
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) DataReaderImpl::bit_lookup_instance_handles: ")
        ACE_TEXT("%d writer handles processed.\n"),
        num_wrts
      ));
      for (CORBA::ULong i = 0; i < num_wrts; ++i)
      {
        RepoIdConverter converter(ids[i]);
        ACE_DEBUG((LM_WARNING,
          ACE_TEXT("(%P|%t) DataReaderImpl::bit_lookup_instance_handles: ")
          ACE_TEXT("writer %s has handle 0x%x.\n"),
          std::string(converter).c_str(),
          hdls[i]
        ));
      }
    }
#endif // !defined (DDS_HAS_MINIMUM_BIT)
  }
  else
  {
    CORBA::ULong num_wrts = ids.length ();
    hdls.length (num_wrts);
    for (CORBA::ULong i = 0; i < num_wrts; i++)
    {
      RepoIdConverter converter(ids[i]);
      hdls[i] = DDS::InstanceHandle_t(converter);
      if( DCPS_debug_level > 4) {
        ACE_DEBUG((LM_WARNING,
          ACE_TEXT("(%P|%t) DataReaderImpl::bit_lookup_instance_handles: ")
          ACE_TEXT("using hash as handle for writer %s.\n"),
          std::string(converter).c_str()
        ));
      }
    }
  }

  return true;
}

bool
DataReaderImpl::cache_lookup_instance_handles (const WriterIdSeq& ids,
                                              ::DDS::InstanceHandleSeq & hdls)
{
  bool ret = true;
  CORBA::ULong num_ids = ids.length ();
  for (CORBA::ULong i = 0; i < num_ids; ++i)
  {
    hdls.length (i + 1);
    RepoIdToHandleMap::iterator iter = id_to_handle_map_.find(ids[i]);
    if (iter == id_to_handle_map_.end())
    {
      RepoIdConverter converter(ids[i]);
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) DataReaderImpl::cache_lookup_instance_handles: ")
        ACE_TEXT("could not find instance handle for writer %s.\n"),
        std::string(converter).c_str()
      ));
      hdls[i] = -1;
      ret = false;
    }
    else
    {
      hdls[i] = iter->second;
      if( DCPS_debug_level > 7) {
        RepoIdConverter converter(ids[i]);
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderImpl::cache_lookup_instance_handles: ")
          ACE_TEXT("instance handle for writer %s == 0x%x.\n"),
          std::string(converter).c_str(),
          hdls[i]
        ));
      }
    }
  }

  return ret;
}

bool
DataReaderImpl::data_expired (DataSampleHeader const & header) const
{
  // Expire historic data if QoS indicates VOLATILE.
  if (!always_get_history_ && header.historic_sample_
    && qos_.durability.kind == ::DDS::VOLATILE_DURABILITY_QOS)
  {
    if (DCPS_debug_level >= 8)
    {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataReaderImpl::data_expired: ")
                 ACE_TEXT("Discarded historic data.\n")));
    }

    return true;  // Data expired. 
  }

  // The LIFESPAN_DURATION_FLAG is set when sample data is sent
  // with a non-default LIFESPAN duration value.
  if (header.lifespan_duration_)
  {
    // Finite lifespan.  Check if data has expired.

    ::DDS::Time_t const tmp =
      {
        header.source_timestamp_sec_ + header.lifespan_duration_sec_,
        header.source_timestamp_nanosec_ + header.lifespan_duration_nanosec_
      };

    // We assume that the publisher host's clock and subcriber host's
    // clock are synchronized (allowed by the spec).
    ACE_Time_Value const now (ACE_OS::gettimeofday ());
    ACE_Time_Value const expiration_time (
      OpenDDS::DCPS::time_to_time_value (tmp));
    if (now >= expiration_time)
    {
      if (DCPS_debug_level >= 8)
      {
        ACE_Time_Value const diff (now - expiration_time);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("OpenDDS (%P|%t) Received data ")
                   ACE_TEXT("expired by %d seconds, %d microseconds.\n"),
                   diff.sec (),
                   diff.usec ()));
      }

      return true;  // Data expired.
    }
  }

  return false;
}

bool DataReaderImpl::is_bit () const
{
  return this->is_bit_;
}

int
DataReaderImpl::num_zero_copies()
{
  int loans = 0;
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    this->sample_lock_,
                    1 /* assume we have loans */);

  for (SubscriptionInstanceMapType::iterator iter = instances_.begin();
       iter != instances_.end();
       ++iter)
    {
      SubscriptionInstance *ptr = iter->second;

      for (OpenDDS::DCPS::ReceivedDataElement *item = ptr->rcvd_sample_.head_;
            item != 0; item = item->next_data_sample_)
        {
            loans += item->zero_copy_cnt_.value();
        }
    }

    return loans;
}


void DataReaderImpl::notify_liveliness_change()
{
  ::DDS::DataReaderListener* listener
    = listener_for (::DDS::LIVELINESS_CHANGED_STATUS);
  if (listener != 0)
  {
    listener->on_liveliness_changed (dr_local_objref_.in (),
      liveliness_changed_status_);

    liveliness_changed_status_.alive_count_change = 0;
    liveliness_changed_status_.not_alive_count_change = 0;
  }
  notify_status_condition ();
  if( DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << "subscription " << RepoIdConverter(subscription_id_);
    buffer << ", listener at: 0x" << std::hex << this->fast_listener_;
    for( WriterMapType::iterator current = this->writers_.begin();
         current != this->writers_.end();
         ++current) {
      RepoId id = current->first;
      buffer << std::endl << "\tNOTIFY: writer[ " << RepoIdConverter(id) << "] == ";
      buffer << current->second.get_state();
    }
    buffer << std::endl;
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderImpl::notify_liveliness_change: ")
      ACE_TEXT("listener at 0x%x, mask 0x%x.\n")
      ACE_TEXT("\tNOTIFY: %s\n"),
      listener,
      listener_mask_,
      buffer.str().c_str()
    ));
  }
}

void DataReaderImpl::post_read_or_take()
{
  set_status_changed_flag(::DDS::DATA_AVAILABLE_STATUS, false);
  get_subscriber_servant()->set_status_changed_flag(
    ::DDS::DATA_ON_READERS_STATUS, false);
}


void DataReaderImpl::reschedule_deadline ()
{
  if (this->watchdog_.get() != 0)
  {
    for (SubscriptionInstanceMapType::iterator iter = this->instances_.begin();
      iter != this->instances_.end();
      ++iter)
    {
      if (iter->second->deadline_timer_id_ != -1)
      {
        if (this->watchdog_->reset_timer_interval (iter->second->deadline_timer_id_) == -1)
        {
          ACE_ERROR ((LM_ERROR, "(%P|%t)DataReaderImpl::reschedule_deadline "
            "%p\n", "reset_timer_interval"));
        }
      }
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

ostream& operator<<( ostream& str, OpenDDS::DCPS::WriterInfo::WriterState value)
{
  switch( value) {
    case OpenDDS::DCPS::WriterInfo::NOT_SET: return str << "NOT_SET";
    case OpenDDS::DCPS::WriterInfo::ALIVE:   return str << "ALIVE";
    case OpenDDS::DCPS::WriterInfo::DEAD:    return str << "DEAD";
    default:                                 return str << "UNSPECIFIED(" << int(value) << ")";
  }
}

