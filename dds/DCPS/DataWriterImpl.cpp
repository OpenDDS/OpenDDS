// -*- C++ -*-
//
// $Id$


#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataWriterImpl.h"
#include "DomainParticipantImpl.h"
#include "PublisherImpl.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "TopicImpl.h"
#include "PublisherImpl.h"
#include "PublicationInstance.h"
#include "Serializer.h"
#include "Transient_Kludge.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "BuiltInTopicUtils.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "dds/DCPS/transport/framework/EntryExit.h"
#include "tao/ORB_Core.h"
#include "ace/Reactor.h"

namespace TAO
{
  namespace DCPS
  {

    //TBD - add check for enabled in most methods.
    //      currently this is not needed because auto_enable_created_entities
    //      cannot be false.
#if 0
    // Emacs trick to align code with first column
    // This will cause emacs to emit bogus alignment message
    // For now just disregard them.
  }}
#endif

DataWriterImpl::DataWriterImpl (void)
  : data_dropped_count_ (0),
    data_delivered_count_ (0),
    control_dropped_count_ (0),
    control_delivered_count_ (0),
    n_chunks_ (TheServiceParticipant->n_chunks ()),
    association_chunk_multiplier_(TheServiceParticipant->association_chunk_multiplier ()),
    topic_id_ (0),
    topic_servant_ (0),
    qos_ (TheServiceParticipant->initial_DataWriterQos ()),
    listener_mask_(DEFAULT_STATUS_KIND_MASK),
    fast_listener_ (0),
    participant_servant_ (0),
    domain_id_ (0),
    publisher_servant_(0),
    publication_id_ (0),
    sequence_number_ (),
    data_container_ (0),
    mb_allocator_(0),
    db_allocator_(0),
    header_allocator_(0),
    reactor_ (0),
    liveliness_check_interval_ (ACE_Time_Value::zero),
    last_liveliness_activity_time_ (ACE_Time_Value::zero),
    cancel_timer_ (false),
    is_bit_ (false),
    initialized_ (false)
{
  liveliness_lost_status_.total_count = 0;
  liveliness_lost_status_.total_count_change = 0;

  offered_deadline_missed_status_.total_count = 0;
  offered_deadline_missed_status_.total_count_change = 0;
  offered_deadline_missed_status_.last_instance_handle = ::DDS::HANDLE_NIL;

  offered_incompatible_qos_status_.total_count = 0;
  offered_incompatible_qos_status_.total_count_change = 0;
  offered_incompatible_qos_status_.last_policy_id = 0;
  offered_incompatible_qos_status_.policies.length(0);

  publication_match_status_.total_count = 0;
  publication_match_status_.total_count_change = 0;
  publication_match_status_.last_subscription_handle = ::DDS::HANDLE_NIL;
}

// This method is called when there are no longer any reference to the
// the servant.
DataWriterImpl::~DataWriterImpl (void)
{
  DBG_ENTRY_LVL ("DataWriterImpl","~DataWriterImpl", 5);

  if (initialized_)
    {
      participant_servant_->_remove_ref ();
      publisher_servant_->_remove_ref ();
      topic_servant_->_remove_ref ();

      delete data_container_;
      delete mb_allocator_;
      delete db_allocator_;
      delete header_allocator_;
    }
}

// this method is called when delete_datawriter is called.
void
DataWriterImpl::cleanup ()
{
  if (cancel_timer_)
    {
      // The cancel_timer will call handle_close to
      // remove_ref.
      int num_handlers = reactor_->cancel_timer (this, 0);
      ACE_UNUSED_ARG (num_handlers);
      cancel_timer_ = false;
    }

  topic_servant_->remove_entity_ref ();
}

void
DataWriterImpl::init ( ::DDS::Topic_ptr                       topic,
           TopicImpl                             *topic_servant,
           const ::DDS::DataWriterQos &           qos,
           ::DDS::DataWriterListener_ptr          a_listener,
           TAO::DCPS::DomainParticipantImpl*      participant_servant,
           ::DDS::Publisher_ptr                   publisher,
           TAO::DCPS::PublisherImpl*              publisher_servant,
           ::DDS::DataWriter_ptr                  dw_local,
           TAO::DCPS::DataWriterRemote_ptr        dw_remote
           )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  DBG_ENTRY_LVL ("DataWriterImpl","init", 5);
  topic_objref_ = ::DDS::Topic::_duplicate (topic);
  topic_servant_ = topic_servant;
  topic_servant_->_add_ref ();
  topic_servant_->add_entity_ref ();
  topic_name_    = topic_servant_->get_name ();
  topic_id_      = topic_servant_->get_id ();

#if !defined (DDS_HAS_MINIMUM_BIT)
  is_bit_ = ACE_OS::strcmp (topic_name_.in (), BUILT_IN_PARTICIPANT_TOPIC) == 0
    || ACE_OS::strcmp (topic_name_.in (), BUILT_IN_TOPIC_TOPIC) == 0
    || ACE_OS::strcmp (topic_name_.in (), BUILT_IN_SUBSCRIPTION_TOPIC) == 0
    || ACE_OS::strcmp (topic_name_.in (), BUILT_IN_PUBLICATION_TOPIC) == 0;
#endif // !defined (DDS_HAS_MINIMUM_BIT)

  qos_ = qos;
  //Note: OK to _duplicate(nil).
  listener_ = ::DDS::DataWriterListener::_duplicate(a_listener);

  if (! CORBA::is_nil (listener_.in()))
    {
      fast_listener_ =
        reference_to_servant<DDS::DataWriterListener> (listener_.in());
    }
  participant_servant_ = participant_servant;
  participant_servant_->_add_ref ();
  domain_id_ = participant_servant_->get_domain_id ();

  publisher_objref_  = ::DDS::Publisher::_duplicate (publisher);
  publisher_servant_ = publisher_servant;
  publisher_servant_->_add_ref ();
  dw_local_objref_   = ::DDS::DataWriter::_duplicate (dw_local);
  dw_remote_objref_  = TAO::DCPS::DataWriterRemote::_duplicate (dw_remote);

  initialized_ = true;
}

void
DataWriterImpl::add_associations ( ::TAO::DCPS::RepoId yourId,
           const ReaderAssociationSeq & readers
           )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  DBG_ENTRY_LVL ("DataWriterImpl","add_associations", 5);
  if (entity_deleted_ == true)
  {
    if (DCPS_debug_level >= 1)
      ACE_DEBUG ((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataWriterImpl::add_associations")
      ACE_TEXT(" This is a deleted datawriter, ignoring add.\n")));
    return;
  }

  if (0 == publication_id_)
  {
    publication_id_ = yourId;
  }


  {
    // I am not sure this guard is necessary for
    // publisher_servant_->add_associations but better safe than sorry.
    // 1/11/06 SHH can cause deadlock so avoid getting the lock.
    //ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);

    // add associations to the transport before using
    // Built-In Topic support and telling the listener.
    this->publisher_servant_->add_associations( readers, this, qos_) ;
  }

}


void
DataWriterImpl::fully_associated ( ::TAO::DCPS::RepoId,
           size_t                  num_remote_associations,
           const AssociationData*  remote_associations)
{
  DBG_ENTRY_LVL ("DataWriterImpl","fully_associated", 5);

  {
    // protect readers_
    ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);
    CORBA::ULong readers_old_length = readers_.length();
    readers_.length(readers_old_length + num_remote_associations);
    for (CORBA::ULong reader_cnt = 0; reader_cnt < num_remote_associations; reader_cnt++)
    {
      readers_[reader_cnt+readers_old_length] = remote_associations[reader_cnt].remote_id_;
    }
  }

  ReaderIdSeq rd_ids;
  rd_ids.length (num_remote_associations);

  for (CORBA::ULong i = 0; i < num_remote_associations; i++)
  {
    rd_ids[i] = remote_associations[i].remote_id_;
  }

  if (! is_bit_)
  {
    ::DDS::InstanceHandleSeq handles;
    // Create the list of readers repo id.

    if (this->bit_lookup_instance_handles (rd_ids, handles) == false)
      return;

    {
      // protect subscription_handles_, publication_match_status_
      // and status changed flags.
      ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);

      CORBA::ULong rd_len = handles.length ();
      CORBA::ULong sub_len = subscription_handles_.length();
      subscription_handles_.length (sub_len + rd_len);

      for (CORBA::ULong i = 0; i < rd_len; i++)
      {
        // update the publication_match_status_
        publication_match_status_.total_count ++;
        publication_match_status_.total_count_change ++;
        subscription_handles_[sub_len + i] = handles[i];
        if (id_to_handle_map_.bind (rd_ids[i], handles[i]) != 0)
        {
          ACE_DEBUG ((LM_DEBUG, "(%P|%t)ERROR: DataWriterImpl::fully_associated "
            "insert %d - %X to id_to_handle_map_ failed \n", rd_ids[i], handles[i]));
          return;
        }
        publication_match_status_.last_subscription_handle = handles[i];
      }


      set_status_changed_flag (::DDS::PUBLICATION_MATCH_STATUS, true);

      ::DDS::DataWriterListener* listener
        = listener_for (::DDS::PUBLICATION_MATCH_STATUS);

      if (listener != 0)
      {
        listener->on_publication_match (dw_local_objref_.in (),
          publication_match_status_);

        // TBD - why does the spec say to change this but not
        // change the ChangeFlagStatus after a listener call?
        publication_match_status_.total_count_change = 0;
      }

      delete [] remote_associations;
    }
  }

  // Support TRANSIENT_LOCAL_DURABILITY_QOS instead of using TheTransientKludge.
  if (this->qos_.durability.kind == DDS::TRANSIENT_LOCAL_DURABILITY_QOS)
    //if (TheTransientKludge->is_enabled ())
    {
      // The above condition is only true for the DCPSInfo Server.

      // kludge over a kludge
      // This sleep ensures that the newly created connection used
      // by Built-In Topics is established end-to-end
      // before the BIT data is resent.
      // Atleast hopefully that will happen in 3 seconds.
      // !!! this sleep caused other problems.
      //     Consider this double kludge at a later time.
      //     Could optimize this by only sleeping when
      //     it is the first time the TransportBlob is seen
      //     (in other words when the connection is being established).
      //     But actaully we still need a small sleep because
      //     although the conneciton is established already the
      //     subscriber side association may not yet be established.
      //     ACE_OS::sleep(2);

      // This is a very limited implementation of
      // DURABILITY.kind=TRANSIENT_LOCAL
      // It suffers from resending the history to every subscription.

      // Tell the WriteDataContainer to resend all sending/sent
      // samples.
      this->data_container_->reenqueue_all (rd_ids);
      this->publisher_servant_->data_available(this, true) ;
    }
}



void
DataWriterImpl::remove_associations ( const ReaderIdSeq & readers,
              ::CORBA::Boolean notify_lost
              )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  CORBA::ULong num_removed_readers = readers.length();
  {
    ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);

    CORBA::ULong num_orig_readers = readers_.length();

    for (CORBA::ULong rm_idx = 0; rm_idx < num_removed_readers; rm_idx++)
    {
      for (CORBA::ULong orig_idx = 0;
        orig_idx < num_orig_readers;
        orig_idx++)
      {
        if (readers_[orig_idx] == readers[rm_idx])
        {
          // move last element to this position.
          if (orig_idx < num_orig_readers - 1)
          {
            readers_[orig_idx] = readers_[num_orig_readers - 1];
          }
          num_orig_readers --;
          readers_.length (num_orig_readers);
          break;
        }
      }
    }

    if (! is_bit_)
    {
      ::DDS::InstanceHandleSeq handles;

      // The reader should be in the id_to_handle map at this time so
      // log with error.
      if (this->cache_lookup_instance_handles (readers, handles) == false)
      {
        ACE_ERROR ((LM_ERROR, "(%P|%t)DataWriterImpl::remove_associations "
          "cache_lookup_instance_handles failed\n"));
        return;
      }

      CORBA::ULong subed_len = subscription_handles_.length();
      CORBA::ULong rd_len = handles.length ();
      for (CORBA::ULong rd_index = 0; rd_index < rd_len; rd_index++)
      {
        for (CORBA::ULong subed_index = 0;
          subed_index < subed_len;
          subed_index++)
        {
          if (subscription_handles_[subed_index] == handles[rd_index])
          {
            // move last element to this position.
            if (subed_index < subed_len - 1)
            {
              subscription_handles_[subed_index]
              = subscription_handles_[subed_len - 1];
            }
            subed_len --;
            subscription_handles_.length (subed_len);
            break;
          }
        }
      }
    }

    for (CORBA::ULong i = 0; i < num_removed_readers; i++)
    {
      id_to_handle_map_.unbind (readers[i]);
    }
  }

  this->publisher_servant_->remove_associations (readers, this->publication_id_);
  // If this remove_association is invoked when the InfoRepo
  // detects a lost reader then make a callback to notify
  // subscription lost.
  if (notify_lost)
    {
      this->notify_publication_lost (readers);
    }
}


void DataWriterImpl::remove_all_associations ()
{

  TAO::DCPS::ReaderIdSeq readers;

  CORBA::ULong size = readers_.length();
  readers.length(size);

  for (CORBA::ULong i = 0; i < size; i++)
    {
      readers[i] = readers_[i];
    }

  try
    {
      if (0 < size)
  {
    CORBA::Boolean dont_notify_lost = 0;
    this->remove_associations(readers, dont_notify_lost);
  }
    }
  catch (const CORBA::Exception&)
    {
    }
}


void
DataWriterImpl::update_incompatible_qos ( const TAO::DCPS::IncompatibleQosStatus & status
            )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  ::DDS::DataWriterListener* listener
      = listener_for (::DDS::OFFERED_INCOMPATIBLE_QOS_STATUS);

  ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);

  set_status_changed_flag (::DDS::OFFERED_INCOMPATIBLE_QOS_STATUS, true);

  // copy status and increment change
  offered_incompatible_qos_status_.total_count = status.total_count;
  offered_incompatible_qos_status_.total_count_change += status.count_since_last_send;
  offered_incompatible_qos_status_.last_policy_id = status.last_policy_id;
  offered_incompatible_qos_status_.policies = status.policies;

  if (listener != 0)
    {
      listener->on_offered_incompatible_qos (dw_local_objref_.in (),
               offered_incompatible_qos_status_);

      // TBD - why does the spec say to change this but not
      // change the ChangeFlagStatus after a listener call?
      offered_incompatible_qos_status_.total_count_change = 0;
    }
}

::DDS::ReturnCode_t
DataWriterImpl::set_qos (
       const ::DDS::DataWriterQos & qos
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

void
DataWriterImpl::get_qos (
       ::DDS::DataWriterQos & qos
       )
  ACE_THROW_SPEC ((
       CORBA::SystemException
       ))
{
  qos = qos_;
}

::DDS::ReturnCode_t
DataWriterImpl::set_listener ( ::DDS::DataWriterListener_ptr a_listener,
             ::DDS::StatusKindMask mask
             )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  listener_mask_ = mask;
  //note: OK to duplicate  and reference_to_servant a nil object ref
  listener_ = ::DDS::DataWriterListener::_duplicate(a_listener);
  fast_listener_
    = reference_to_servant<DDS::DataWriterListener> (listener_.in ());
  return ::DDS::RETCODE_OK;
}

::DDS::DataWriterListener_ptr
DataWriterImpl::get_listener ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  return ::DDS::DataWriterListener::_duplicate (listener_.in ());
}

::DDS::Topic_ptr
DataWriterImpl::get_topic ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  return ::DDS::Topic::_duplicate (topic_objref_.in ());
}

::DDS::Publisher_ptr
DataWriterImpl::get_publisher ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  return ::DDS::Publisher::_duplicate (publisher_objref_.in ());
}

::DDS::LivelinessLostStatus
DataWriterImpl::get_liveliness_lost_status ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
        guard,
        this->lock_,
        ::DDS::LivelinessLostStatus ());
  set_status_changed_flag (::DDS::LIVELINESS_LOST_STATUS, false);
  ::DDS::LivelinessLostStatus status = liveliness_lost_status_;
  liveliness_lost_status_.total_count_change = 0;
  return status;
}

::DDS::OfferedDeadlineMissedStatus
DataWriterImpl::get_offered_deadline_missed_status ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
        guard,
        this->lock_,
        ::DDS::OfferedDeadlineMissedStatus ());
  set_status_changed_flag (::DDS::OFFERED_DEADLINE_MISSED_STATUS, false);
  ::DDS::OfferedDeadlineMissedStatus status = offered_deadline_missed_status_;
  offered_deadline_missed_status_.total_count_change = 0;
  return status;
}

::DDS::OfferedIncompatibleQosStatus *
DataWriterImpl::get_offered_incompatible_qos_status ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
        guard,
        this->lock_,
        0);
  set_status_changed_flag (::DDS::OFFERED_INCOMPATIBLE_QOS_STATUS, false);
  ::DDS::OfferedIncompatibleQosStatus* status
      = new ::DDS::OfferedIncompatibleQosStatus;
  *status = offered_incompatible_qos_status_;
  offered_incompatible_qos_status_.total_count_change = 0;
  return status;
}

::DDS::PublicationMatchStatus
DataWriterImpl::get_publication_match_status ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
        guard,
        this->lock_,
        ::DDS::PublicationMatchStatus ());
  set_status_changed_flag (::DDS::PUBLICATION_MATCH_STATUS, false);
  ::DDS::PublicationMatchStatus status = publication_match_status_;
  publication_match_status_.total_count_change = 0;
  return status;
}

void
DataWriterImpl::assert_liveliness ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  // This operation need only be used if the LIVELINESS setting
  // is either MANUAL_BY_PARTICIPANT or MANUAL_BY_TOPIC.
  // Otherwise, it has no effect.

  // This will do nothing in current implementation since we only
  // support the AUTOMATIC liveliness qos for datawriter.

  // ACE_Time_Value now = ACE_OS::gettimeofday ();
  // send_liveliness (now);
}

::DDS::ReturnCode_t
DataWriterImpl::get_matched_subscriptions ( ::DDS::InstanceHandleSeq & subscription_handles
              )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::get_matched_subscriptions, ")
       ACE_TEXT(" Entity is not enabled. \n")),
      ::DDS::RETCODE_NOT_ENABLED);
    }
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
        guard,
        this->lock_,
        ::DDS::RETCODE_ERROR);
  subscription_handles = subscription_handles_;
  return ::DDS::RETCODE_OK;
}

#if !defined (DDS_HAS_MINIMUM_BIT)
::DDS::ReturnCode_t
DataWriterImpl::get_matched_subscription_data ( ::DDS::SubscriptionBuiltinTopicData & subscription_data,
            ::DDS::InstanceHandle_t subscription_handle
            )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::get_matched_subscription_data, ")
       ACE_TEXT(" Entity is not enabled. \n")),
      ::DDS::RETCODE_NOT_ENABLED);
    }

  BIT_Helper_1 < ::DDS::SubscriptionBuiltinTopicDataDataReader,
    ::DDS::SubscriptionBuiltinTopicDataDataReader_var,
    ::DDS::SubscriptionBuiltinTopicDataSeq > hh;

  ::DDS::SubscriptionBuiltinTopicDataSeq data;

  ::DDS::ReturnCode_t ret
      = hh.instance_handle_to_bit_data(participant_servant_,
               BUILT_IN_SUBSCRIPTION_TOPIC,
               subscription_handle,
               data);
  if (ret == ::DDS::RETCODE_OK)
    {
      subscription_data = data[0];
    }

  return ret;
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)

::DDS::ReturnCode_t
DataWriterImpl::enable ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  //TDB - check if factory is enabled and then enable all entities
  // (don't need to do it for now because
  //  entity_factory.autoenable_created_entities is always = 1)

  //if (factory Entity is not enabled.)
  //{
  //  return ::DDS::RETCODE_PRECONDITION_NOT_MET;
  //}

  // Note: do configuration based on QoS in enable() because
  //       before enable is called the QoS can be changed -- even
  //       for Changeable=NO


  // Configure WriteDataContainer constructor parameters from qos.

  bool should_block
    = (qos_.history.kind == ::DDS::KEEP_ALL_HISTORY_QOS
       && qos_.reliability.kind == ::DDS::RELIABLE_RELIABILITY_QOS);

  ACE_Time_Value max_blocking_time = ACE_Time_Value::zero;
  if (should_block)
    {
      max_blocking_time.sec (qos_.reliability.max_blocking_time.sec);
      max_blocking_time.usec(qos_.reliability.max_blocking_time.nanosec/1000);
    }

  CORBA::Long depth = 0;
  if (qos_.history.kind == ::DDS::KEEP_ALL_HISTORY_QOS)
    {
      // The spec says qos_.history.depth is "has no effect"
      // when history.kind = KEEP_ALL so use max_samples_per_instance
      depth = qos_.resource_limits.max_samples_per_instance;
    }
  else // qos_.history.kind == ::DDS::KEEP_LAST_HISTORY_QOS
    {
      depth = qos_.history.depth;
    }

  if (depth == ::DDS::LENGTH_UNLIMITED)
    {
      // ::DDS::LENGTH_UNLIMITED is negative so make it a positive
      // value that is for all intents and purposes unlimited
      // and we can use it for comparisons.
      // use 2147483647L because that is the greatest value a signed
      // CORBA::Long can have.
      // WARNING: The client risks running out of memory in this case.
      depth = 2147483647L;
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
  data_container_
    = new WriteDataContainer (depth,
            should_block,
            max_blocking_time,
            n_chunks_);

  // +1 because we might allocate one before releasing another
  // TBD - see if this +1 can be removed.
  mb_allocator_ = new MessageBlockAllocator (n_chunks_ * association_chunk_multiplier_);
  db_allocator_ = new DataBlockAllocator (n_chunks_+1);
  header_allocator_ = new DataSampleHeaderAllocator (n_chunks_+1);
  if (DCPS_debug_level >= 2)
    {
      ACE_DEBUG((LM_DEBUG,"(%P|%t) DataWriterImpl::enable-mb"
     " Cached_Allocator_With_Overflow %x with %d chunks\n",
     mb_allocator_, n_chunks_));
      ACE_DEBUG((LM_DEBUG,"(%P|%t) DataWriterImpl::enable-db"
     " Cached_Allocator_With_Overflow %x with %d chunks\n",
     db_allocator_, n_chunks_));
      ACE_DEBUG((LM_DEBUG,"(%P|%t) DataWriterImpl::enable-header"
     " Cached_Allocator_With_Overflow %x with %d chunks\n",
     header_allocator_, n_chunks_));
    }

  if (qos_.liveliness.lease_duration.sec != ::DDS::DURATION_INFINITY_SEC
      || qos_.liveliness.lease_duration.nanosec != ::DDS::DURATION_INFINITY_NSEC)
    {
      liveliness_check_interval_ = duration_to_time_value (qos_.liveliness.lease_duration);
      liveliness_check_interval_ *= TheServiceParticipant->liveliness_factor ()/100.0;

      CORBA::ORB_var orb = TheServiceParticipant->get_ORB ();
      reactor_ = orb->orb_core()->reactor();

      if (reactor_->schedule_timer(this,
           0,
           liveliness_check_interval_,
           liveliness_check_interval_) == -1)
  {
    ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::init, ")
          ACE_TEXT(" %p. \n"), "schedule_timer"));
  }
      else
  {
    cancel_timer_ = true;
    this->_add_ref ();
  }
    }

  this->set_enabled ();

  return publisher_servant_->writer_enabled (
               dw_remote_objref_.in(),
               dw_local_objref_.in (),
               topic_name_.in (),
               topic_id_);
}

::DDS::StatusKindMask
DataWriterImpl::get_status_changes ( )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, this->lock_, 0);
  return EntityImpl::get_status_changes ();
}

::DDS::ReturnCode_t
DataWriterImpl::register_instance( ::DDS::InstanceHandle_t& handle,
           DataSample* data,
           const ::DDS::Time_t & source_timestamp
           )
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  DBG_ENTRY_LVL("DataWriterImpl","register_instance",5);
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::register_instance, ")
       ACE_TEXT(" Entity is not enabled. \n")),
      ::DDS::RETCODE_NOT_ENABLED);
    }

  ::DDS::ReturnCode_t ret
      = this->data_container_->register_instance(handle,
             data) ;
  if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) DataWriterImpl::register_instance, ")
       ACE_TEXT(" register instance with container failed. \n")),
      ret);
    }

  // Add header with the registration sample data.
  ACE_Message_Block* registered_sample
    = this->create_control_message(INSTANCE_REGISTRATION,
           data,
           source_timestamp) ;

  SendControlStatus status;
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publisher_servant_->get_pi_lock());

    status = this->publisher_servant_->send_control(publication_id_,
                this,
                registered_sample) ;
  }

  if (status == SEND_CONTROL_ERROR)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::register_instance, ")
       ACE_TEXT(" send_control failed. \n")),
      ::DDS::RETCODE_ERROR);
    }

  return ret;
}


::DDS::ReturnCode_t
DataWriterImpl::unregister ( ::DDS::InstanceHandle_t handle,
           const ::DDS::Time_t & source_timestamp
           )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  DBG_ENTRY_LVL("DataWriterImpl","unregister",5);
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::unregister, ")
       ACE_TEXT(" Entity is not enabled. \n")),
      ::DDS::RETCODE_NOT_ENABLED);
    }


  DataSample* unregistered_sample_data;

  ::DDS::ReturnCode_t ret
      = this->data_container_->unregister(handle,
            unregistered_sample_data,
            this) ;

  if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::unregister, ")
       ACE_TEXT(" unregister with container failed. \n")),
      ret);
    }

  ACE_Message_Block* message
    = this->create_control_message(UNREGISTER_INSTANCE,
           unregistered_sample_data,
           source_timestamp) ;

  SendControlStatus status;
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publisher_servant_->get_pi_lock());

    status = this->publisher_servant_->send_control(publication_id_,
                this,
                message) ;
  }

  if (status == SEND_CONTROL_ERROR)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::unregister, ")
       ACE_TEXT(" send_control failed. \n")),
      ::DDS::RETCODE_ERROR);
    }

  return ret;
}

::DDS::ReturnCode_t
DataWriterImpl::write ( DataSample* data,
      ::DDS::InstanceHandle_t handle,
      const ::DDS::Time_t & source_timestamp
      )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  DBG_ENTRY_LVL("DataWriterImpl","write",5);
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::write, ")
       ACE_TEXT(" Entity is not enabled. \n")),
      ::DDS::RETCODE_NOT_ENABLED);
    }

  DataSampleListElement* element;
  ::DDS::ReturnCode_t ret
      = this->data_container_->obtain_buffer(element,
               handle,
               this) ;

  if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: ")
       ACE_TEXT("DataWriterImpl::write, ")
       ACE_TEXT("obtain_buffer returned %d.\n"),
       ret),
      ret);
    }

  ret = create_sample_data_message (data,
            handle,
            element->sample_,
            source_timestamp);

  if (ret != ::DDS::RETCODE_OK)
    {
      return ret;
    }

  ret = this->data_container_->enqueue( element,
          handle) ;

  if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: ")
       ACE_TEXT("DataWriterImpl::write, ")
       ACE_TEXT("enqueue failed.\n")),
      ret);
    }

  last_liveliness_activity_time_ = ACE_OS::gettimeofday ();
  return this->publisher_servant_->data_available(this) ;
}

::DDS::ReturnCode_t DataWriterImpl::dispose ( ::DDS::InstanceHandle_t handle,
                const ::DDS::Time_t & source_timestamp
                )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  DBG_ENTRY_LVL("DataWriterImpl","dispose",5);
  if (enabled_ == false)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::dispose, ")
       ACE_TEXT(" Entity is not enabled. \n")),
      ::DDS::RETCODE_NOT_ENABLED);
    }

  DataSample* registered_sample_data;
  ::DDS::ReturnCode_t ret
      = this->data_container_->dispose(handle,
               registered_sample_data) ;

  if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: ")
       ACE_TEXT("DataWriterImpl::dispose, ")
       ACE_TEXT("dispose failed.\n")),
      ret);
    }

  ACE_Message_Block* message
    = this->create_control_message(DISPOSE_INSTANCE,
           registered_sample_data,
           source_timestamp) ;

  SendControlStatus status;
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publisher_servant_->get_pi_lock());

    status  = this->publisher_servant_->send_control(publication_id_,
                 this,
                 message) ;
  }

  if (status == SEND_CONTROL_ERROR)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::dispose, ")
       ACE_TEXT(" send_control failed. \n")),
      ::DDS::RETCODE_ERROR);
    }

  return ret;
}


::DDS::ReturnCode_t
DataWriterImpl::num_samples ( ::DDS::InstanceHandle_t handle,
            size_t&                 size
            )
{
  return data_container_->num_samples (handle, size);
}


DataSampleList
DataWriterImpl::get_unsent_data()
{
  return data_container_->get_unsent_data ();
}


DataSampleList
DataWriterImpl::get_resend_data()
{
  return data_container_->get_resend_data ();
}


void
DataWriterImpl::unregister_all ()
{
  if (cancel_timer_)
    {
      // The cancel_timer will call handle_close to
      // remove_ref.
      int num_handlers = reactor_->cancel_timer (this, 0);
      ACE_UNUSED_ARG (num_handlers);
      cancel_timer_ = false;
    }
  data_container_->unregister_all (this);
}

void
DataWriterImpl::set_publication_id ( RepoId publication_id )
{
  publication_id_ = publication_id;
  data_container_->publication_id_ = publication_id;
  //data_container_->pub_id (publication_id);
}

RepoId
DataWriterImpl::get_publication_id()
{
  return publication_id_;
}

const char*
DataWriterImpl::get_topic_name ()
{
  return topic_name_.in ();
}

ACE_Message_Block*
DataWriterImpl::create_control_message (enum MessageId message_id,
          ACE_Message_Block* data,
          const ::DDS::Time_t& source_timestamp)
{
  DataSampleHeader header_data;
  header_data.message_id_ = message_id;
  //header_data.last_sample_
  header_data.byte_order_
    = this->publisher_servant_->swap_bytes() ? !TAO_ENCAP_BYTE_ORDER : TAO_ENCAP_BYTE_ORDER;
  header_data.message_length_ = data->total_length ();
  header_data.sequence_ = 0;
  header_data.source_timestamp_sec_ = source_timestamp.sec;
  header_data.source_timestamp_nanosec_ = source_timestamp.nanosec;
  header_data.coherency_group_ = 0;
  header_data.publication_id_ = publication_id_;

  ACE_Message_Block* message;
  size_t max_marshaled_size = header_data.max_marshaled_size ();

  ACE_NEW_MALLOC_RETURN (message,
       static_cast<ACE_Message_Block*>(
               mb_allocator_->malloc (
                    sizeof (ACE_Message_Block))),
       ACE_Message_Block(max_marshaled_size,
             ACE_Message_Block::MB_DATA,
             data, //cont
             0, //data
             0, //allocator_strategy
             0, //locking_strategy
             ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
             ACE_Time_Value::zero,
             ACE_Time_Value::max_time,
             db_allocator_,
             mb_allocator_),
       0);

  message << header_data;
  return message ;
}


::DDS::ReturnCode_t
DataWriterImpl::create_sample_data_message ( DataSample* data,
               ::DDS::InstanceHandle_t instance_handle,
               ACE_Message_Block*& message,
               const ::DDS::Time_t& source_timestamp)
{
  PublicationInstance* instance =
    data_container_->get_handle_instance(instance_handle);

  if (0 == instance)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
       ACE_TEXT("(%P|%t) DataWriterImpl::create_sample_data_message ")
       ACE_TEXT("failed to find instance for handle %d\n"),
       instance_handle),
      ::DDS::RETCODE_ERROR);
    }

  DataSampleHeader header_data;
  header_data.message_id_ = SAMPLE_DATA;
  //header_data.last_sample_
  header_data.byte_order_
    = this->publisher_servant_->swap_bytes() ? !TAO_ENCAP_BYTE_ORDER : TAO_ENCAP_BYTE_ORDER;
  header_data.message_length_ = data->total_length ();
  header_data.sequence_ = instance->sequence_.value_;
  instance->sequence_ ++;
  header_data.source_timestamp_sec_ = source_timestamp.sec;
  header_data.source_timestamp_nanosec_ = source_timestamp.nanosec;
  header_data.coherency_group_ = 0;
  header_data.publication_id_ = publication_id_;

  size_t max_marshaled_size = header_data.max_marshaled_size ();

  ACE_NEW_MALLOC_RETURN (message,
       static_cast<ACE_Message_Block*> (
                mb_allocator_->malloc (
                     sizeof (ACE_Message_Block))),
       ACE_Message_Block(max_marshaled_size,
             ACE_Message_Block::MB_DATA,
             data, //cont
             0, //data
             header_allocator_, //allocator_strategy
             0, //locking_strategy
             ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
             ACE_Time_Value::zero,
             ACE_Time_Value::max_time,
             db_allocator_,
             mb_allocator_),
       ::DDS::RETCODE_ERROR);

  message << header_data;
  return ::DDS::RETCODE_OK;
}

void
DataWriterImpl::data_delivered (DataSampleListElement* sample)
{
  DBG_ENTRY_LVL("DataWriterImpl","data_delivered",5);
  if (sample->publication_id_ != this->publication_id_)
    {
      ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::data_delivered, ")
      ACE_TEXT(" The publication id %d from delivered element ")
      ACE_TEXT("does not match the datawriter's id %d\n"),
      sample->publication_id_, this->publication_id_));
      return;
    }

  this->data_container_->data_delivered (sample);

  data_delivered_count_ ++;
}

void
DataWriterImpl::control_delivered(ACE_Message_Block* sample)
{
  DBG_ENTRY_LVL("DataWriterImpl","control_delivered",5);
  control_delivered_count_ ++;
  sample->release ();
}

PublisherImpl*
DataWriterImpl::get_publisher_servant ()
{
  return publisher_servant_;
}

void
DataWriterImpl::remove_sample (DataSampleListElement* element, bool dropped_by_transport)
{
  DBG_ENTRY_LVL("DataWriterImpl","remove_sample",5);
  publisher_servant_->remove_sample (element, dropped_by_transport);
}

void
DataWriterImpl::data_dropped (DataSampleListElement* element,
            bool dropped_by_transport)
{
  DBG_ENTRY_LVL("DataWriterImpl","data_dropped",5);
  this->data_container_->data_dropped (element, dropped_by_transport);

  data_dropped_count_ ++;
}

void
DataWriterImpl::control_dropped (ACE_Message_Block* sample,
         bool dropped_by_transport)
{
  ACE_UNUSED_ARG (dropped_by_transport);
  DBG_ENTRY_LVL("DataWriterImpl","control_dropped",5);
  control_dropped_count_ ++;
  sample->release ();
}

int
DataWriterImpl::remove_all_control_msgs()
{
  DBG_ENTRY_LVL("DataWriterImpl","remove_all_control_msgs",5);
  return
    publisher_servant_->remove_all_control_msgs (this->publication_id_);
}

void
DataWriterImpl::unregistered(::DDS::InstanceHandle_t instance_handle)
{
  ACE_UNUSED_ARG (instance_handle);
}

::DDS::DataWriterListener*
DataWriterImpl::listener_for (::DDS::StatusKind kind)
{
  // per 2.1.4.3.1 Listener Access to Plain Communication Status
  // use this entities factory if listener is mask not enabled
  // for this kind.
  if ((listener_mask_ & kind) == 0)
    {
      return publisher_servant_->listener_for (kind);
    }
  else
    {
      return fast_listener_;
    }
}

int
DataWriterImpl::handle_timeout (const ACE_Time_Value &tv,
        const void *arg)
{
  ACE_UNUSED_ARG(arg);

  ACE_Time_Value elapsed = tv - last_liveliness_activity_time_;

  if (elapsed >= liveliness_check_interval_)
    {
      //Not recent enough then send liveliness message.
      this->send_liveliness(tv);
    }
  else
    {
      //Recent enough. Schedule the interval.
      if (reactor_->cancel_timer (this) == -1)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
           ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::handle_timeout, ")
           ACE_TEXT(" %p. \n"), "cancel_timer"),
          -1);
  }

      ACE_Time_Value remain = liveliness_check_interval_ - elapsed;

      if (reactor_->schedule_timer(
           this, 0, remain, liveliness_check_interval_) == -1)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
           ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::handle_timeout, ")
           ACE_TEXT(" %p. \n"), "schedule_timer"),
          -1);
  }
    }

  return 0;
}


int
DataWriterImpl::handle_close (ACE_HANDLE,
            ACE_Reactor_Mask)
{
  this->_remove_ref ();
  return 0;
}


void
DataWriterImpl::send_liveliness (const ACE_Time_Value& now)
{
  ::DDS::Time_t t = time_value_to_time (now);
  ACE_Message_Block* liveliness_msg
    = this->create_control_message(DATAWRITER_LIVELINESS, 0, t) ;

  SendControlStatus status;
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publisher_servant_->get_pi_lock());

    status = this->publisher_servant_->send_control(publication_id_,
                this,
                liveliness_msg) ;
  }

  if (status == SEND_CONTROL_ERROR)
    {
      ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::send_liveliness, ")
      ACE_TEXT(" send_control failed. \n")));
    }
  else
    {
      last_liveliness_activity_time_ = now;
    }
}


PublicationInstance*
DataWriterImpl::get_handle_instance (::DDS::InstanceHandle_t handle)
{
  PublicationInstance* instance = 0;
  if (0 != data_container_)
    {
      instance = data_container_->get_handle_instance(handle);
    }

  return instance;
}


void
DataWriterImpl::notify_publication_disconnected (const ReaderIdSeq& subids)
{
  DBG_ENTRY_LVL("DataWriterImpl","notify_publication_disconnected",5);

  if (! is_bit_)
  {
    // Narrow to DDS::DCPS::DataWriterListener. If a DDS::DataWriterListener
    // is given to this DataWriter then narrow() fails.
    DataWriterListener_var the_listener = DataWriterListener::_narrow (this->listener_.in ());

    if (! CORBA::is_nil (the_listener.in ()))
    {
      PublicationDisconnectedStatus status;
      // Since this callback may come after remove_association which removes
      // the reader from id_to_handle map, we can ignore this error.
      this->cache_lookup_instance_handles (subids, status.subscription_handles);
      the_listener->on_publication_disconnected (this->dw_local_objref_.in (),
        status);
    }
  }
}



void
DataWriterImpl::notify_publication_reconnected (const ReaderIdSeq& subids)
{
  DBG_ENTRY_LVL("DataWriterImpl","notify_publication_reconnected",5);

  if (! is_bit_)
  {
    // Narrow to DDS::DCPS::DataWriterListener. If a DDS::DataWriterListener
    // is given to this DataWriter then narrow() fails.
    DataWriterListener_var the_listener = DataWriterListener::_narrow (this->listener_.in ());

    if (! CORBA::is_nil (the_listener.in ()))
    {
      PublicationDisconnectedStatus status;

      // If it's reconnected then the reader should be in id_to_handle map otherwise
      // log with an error.
      if (this->cache_lookup_instance_handles (subids, status.subscription_handles) == false)
      {
        ACE_ERROR ((LM_ERROR, "(%P|%t)DataWriterImpl::notify_publication_reconnected "
          "cache_lookup_instance_handles failed\n"));
      }

      the_listener->on_publication_reconnected (this->dw_local_objref_.in (),
      status);
    }
  }
}


void
DataWriterImpl::notify_publication_lost (const ReaderIdSeq& subids)
{
  DBG_ENTRY_LVL("DataWriterImpl","notify_publication_lost",5);
  if (! is_bit_)
  {
    // Narrow to DDS::DCPS::DataWriterListener. If a DDS::DataWriterListener
    // is given to this DataWriter then narrow() fails.
    DataWriterListener_var the_listener = DataWriterListener::_narrow (this->listener_.in ());

    if (! CORBA::is_nil (the_listener.in ()))
    {
      PublicationLostStatus status;

      // Since this callback may come after remove_association which removes
      // the reader from id_to_handle map, we can ignore this error.
      this->cache_lookup_instance_handles (subids, status.subscription_handles);
      the_listener->on_publication_lost (this->dw_local_objref_.in (),
      status);
    }
  }
}


void
DataWriterImpl::notify_connection_deleted ()
{
  DBG_ENTRY_LVL("DataWriterImpl","notify_connection_deleted",5);

  // Narrow to DDS::DCPS::DataWriterListener. If a DDS::DataWriterListener
  // is given to this DataWriter then narrow() fails.
  DataWriterListener_var the_listener = DataWriterListener::_narrow (this->listener_.in ());

  if (! CORBA::is_nil (the_listener.in ()))
    the_listener->on_connection_deleted (this->dw_local_objref_.in ());
}

bool
DataWriterImpl::bit_lookup_instance_handles (const ReaderIdSeq& ids,
                ::DDS::InstanceHandleSeq & hdls)
{
  // TBD: Remove the condition check after we change to default support
  //      builtin topics.
  if (TheServiceParticipant->get_BIT () == true && ! TheTransientKludge->is_enabled ())
  {
#if !defined (DDS_HAS_MINIMUM_BIT)
    BIT_Helper_2 < ::DDS::SubscriptionBuiltinTopicDataDataReader,
      ::DDS::SubscriptionBuiltinTopicDataDataReader_var,
      ::DDS::SubscriptionBuiltinTopicDataSeq,
      ReaderIdSeq > hh;

    ::DDS::ReturnCode_t ret
      = hh.repo_ids_to_instance_handles(participant_servant_,
      BUILT_IN_SUBSCRIPTION_TOPIC,
      ids,
      hdls);

    if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::bit_lookup_instance_handles failed\n")));
      return false;
    }
#endif // !defined (DDS_HAS_MINIMUM_BIT)
  }
  else
  {
    CORBA::ULong num_rds = ids.length ();
    hdls.length (num_rds);
    for (CORBA::ULong i = 0; i < num_rds; i++)
    {
      hdls[i] = ids[i];
    }
  }

  return true;
}


bool
DataWriterImpl::cache_lookup_instance_handles (const ReaderIdSeq& ids,
					      ::DDS::InstanceHandleSeq & hdls)
{
  bool ret = true;
  CORBA::ULong num_ids = ids.length ();
  for (CORBA::ULong i = 0; i < num_ids; ++i)
  {
    hdls.length (i + 1);
    RepoIdToHandleMap::ENTRY* ientry;
    if (id_to_handle_map_.find (ids[i], ientry) != 0)
    {
      ACE_DEBUG ((LM_WARNING, "(%P|%t)DataWriterImpl::cache_lookup_instance_handles "
        "could not find instance handle for writer %d\n", ids[i]));
      hdls[i] = -1;
      ret = false;
    }
    else
    {
      hdls[i] = ientry->int_id_;
    }
  }

  return ret;
}


} // namespace DCPS
} // namespace TAO
