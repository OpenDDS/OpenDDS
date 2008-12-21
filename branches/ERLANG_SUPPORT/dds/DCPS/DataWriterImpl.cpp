// $Id$


#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataWriterImpl.h"
#include "DomainParticipantImpl.h"
#include "PublisherImpl.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "TopicImpl.h"
#include "PublicationInstance.h"
#include "Serializer.h"
#include "Transient_Kludge.h"
#include "DataDurabilityCache.h"
#include "OfferedDeadlineWatchdog.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "BuiltInTopicUtils.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "Util.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "tao/ORB_Core.h"
#include "ace/Reactor.h"
#include "ace/Auto_Ptr.h"

#include <sstream>

namespace OpenDDS
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
    topic_id_ ( GUID_UNKNOWN ),
    topic_servant_ (0),
    qos_ (TheServiceParticipant->initial_DataWriterQos ()),
    listener_mask_(DEFAULT_STATUS_KIND_MASK),
    fast_listener_ (0),
    participant_servant_ (0),
    domain_id_ (0),
    publisher_servant_(0),
    publication_id_ ( GUID_UNKNOWN ),
    sequence_number_ (),
    data_container_ (0),
    mb_allocator_(0),
    db_allocator_(0),
    header_allocator_(0),
    reactor_ (0),
    liveliness_check_interval_ (ACE_Time_Value::zero),
    last_liveliness_activity_time_ (ACE_Time_Value::zero),
    last_deadline_missed_total_count_ (0),
    watchdog_ (),
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
  DBG_ENTRY_LVL ("DataWriterImpl","~DataWriterImpl",6);

  if (initialized_)
  {
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
    (void) reactor_->cancel_timer (this, 0);
    cancel_timer_ = false;
  }

  // release our Topic_var
  topic_objref_ = ::DDS::Topic::_nil();
  topic_servant_->remove_entity_ref ();
  topic_servant_->_remove_ref ();
  topic_servant_ = 0;

  dw_local_objref_ = ::DDS::DataWriter::_nil();
  deactivate_remote_object(dw_remote_objref_.in());
  dw_remote_objref_ = ::OpenDDS::DCPS::DataWriterRemote::_nil();
}

void
DataWriterImpl::init (
    ::DDS::Topic_ptr                       topic,
    TopicImpl *                            topic_servant,
    const ::DDS::DataWriterQos &           qos,
    ::DDS::DataWriterListener_ptr          a_listener,
    OpenDDS::DCPS::DomainParticipantImpl * participant_servant,
    OpenDDS::DCPS::PublisherImpl *         publisher_servant,
    ::DDS::DataWriter_ptr                  dw_local,
    OpenDDS::DCPS::DataWriterRemote_ptr    dw_remote)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  DBG_ENTRY_LVL ("DataWriterImpl","init",6);
  topic_objref_ = ::DDS::Topic::_duplicate (topic);
  topic_servant_ = topic_servant;
  topic_servant_->_add_ref ();
  topic_servant_->add_entity_ref ();
  topic_name_    = topic_servant_->get_name ();
  topic_id_      = topic_servant_->get_id ();
  type_name_     = topic_servant_->get_type_name ();

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
      fast_listener_ = listener_.in();
    }

  // Only store the participant pointer, since it is our "grand"
  // parent, we will exist as long as it does.
  participant_servant_ = participant_servant;
  domain_id_ = participant_servant_->get_domain_id ();

  // Only store the publisher pointer, since it is our parent, we will
  // exist as long as it does.
  publisher_servant_ = publisher_servant;
  dw_local_objref_   = ::DDS::DataWriter::_duplicate (dw_local);
  dw_remote_objref_  = OpenDDS::DCPS::DataWriterRemote::_duplicate (dw_remote);

  CORBA::ORB_var orb = TheServiceParticipant->get_ORB ();
  this->reactor_ = orb->orb_core()->reactor();

  initialized_ = true;
}

void
DataWriterImpl::add_associations ( ::OpenDDS::DCPS::RepoId yourId,
                                   const ReaderAssociationSeq & readers )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  DBG_ENTRY_LVL ("DataWriterImpl","add_associations",6);

  if (DCPS_debug_level >= 1)
  {
    ::OpenDDS::DCPS::GuidConverter writerConverter( yourId);
    ::OpenDDS::DCPS::GuidConverter readerConverter(
      const_cast< ::OpenDDS::DCPS::RepoId*>( &readers[ 0].readerId)
    );
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataWriterImpl::add_associations - ")
      ACE_TEXT("bit %d local %s remote %s num remotes %d \n"),
      is_bit_,
      (const char*) writerConverter,
      (const char*) readerConverter,
      readers.length()
    ));
  }

  if (entity_deleted_ == true)
  {
    if (DCPS_debug_level >= 1)
      ACE_DEBUG ((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataWriterImpl::add_associations")
      ACE_TEXT(" This is a deleted datawriter, ignoring add.\n")));
    return;
  }

  if (GUID_UNKNOWN == publication_id_)
  {
    publication_id_ = yourId;
  }

  {
    ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);
    // Add to pending_readers_ 

    CORBA::ULong len = readers.length();

    for (CORBA::ULong i = 0; i < len; ++i)
    {
      if( OpenDDS::DCPS::insert(pending_readers_, readers[i].readerId) == -1) {
        ::OpenDDS::DCPS::GuidConverter converter(
          const_cast< ::OpenDDS::DCPS::RepoId*>( &readers[ i].readerId)
        );
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::add_associations: ")
          ACE_TEXT("failed to mark %s as pending.\n"),
          (const char*) converter
        ));

      } else if (DCPS_debug_level > 0) {
        ::OpenDDS::DCPS::GuidConverter converter(
          const_cast< ::OpenDDS::DCPS::RepoId*>( &readers[ i].readerId)
        );
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataWriterImpl::add_associations: ")
          ACE_TEXT("marked %s as pending.\n"),
          (const char*) converter
        ));
      }
    }
  }

  {
    // I am not sure this guard is necessary for
    // publisher_servant_->add_associations but better safe than sorry.
    // 1/11/06 SHH can cause deadlock so avoid getting the lock.
    //ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);

    // add associations to the transport before using
    // Built-In Topic support and telling the listener.
    this->publisher_servant_->add_associations( readers, this, qos_);
  }
}


void
DataWriterImpl::fully_associated ( ::OpenDDS::DCPS::RepoId myid,
                                   size_t num_remote_associations,
                                   const AssociationData* remote_associations)
{
  DBG_ENTRY_LVL ("DataWriterImpl","fully_associated",6);

  if (DCPS_debug_level >= 1)
  {
    ::OpenDDS::DCPS::GuidConverter writerConverter( myid);
    ::OpenDDS::DCPS::GuidConverter readerConverter(
      const_cast< ::OpenDDS::DCPS::RepoId*>( &remote_associations[ 0].remote_id_)
    );
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataWriterImpl::fully_associated - ")
      ACE_TEXT("bit %d local %s remote %s num remotes %d \n"),
      is_bit_,
      (const char*) writerConverter,
      (const char*) readerConverter,
      num_remote_associations
    ));
  }

  CORBA::ULong len = 0;
  ReaderIdSeq rd_ids;

  {
    // protect readers_
    ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);

    for (CORBA::ULong i = 0; i < num_remote_associations; ++i)
    {
      // If the reader is not in pending association list, which indicates it's already
      // removed by remove_association. In other words, the remove_association()  
      // is called before fully_associated() call.
      if (OpenDDS::DCPS::remove (pending_readers_, remote_associations[i].remote_id_) == -1)
      {
        ::OpenDDS::DCPS::GuidConverter converter(
          const_cast< ::OpenDDS::DCPS::RepoId*>( &remote_associations[ i].remote_id_)
        );
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataWriterImpl::fully_associated: ")
          ACE_TEXT("reader %s is not in pending list ")
          ACE_TEXT("because remove_association is already called.\n"),
          (const char*) converter
        ));
        continue;
      }
      
      // The reader is in the pending reader, now add it to fully associated reader
      // list.
      ++len;
      rd_ids.length (len);
      rd_ids[len - 1] = remote_associations[i].remote_id_;
      
      if (OpenDDS::DCPS::insert (readers_, remote_associations[i].remote_id_) == -1)
      {
        ::OpenDDS::DCPS::GuidConverter converter(
          const_cast< ::OpenDDS::DCPS::RepoId*>( &remote_associations[ i].remote_id_)
        );
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::fully_associated: ")
          ACE_TEXT("insert %s from pending failed.\n"),
          (const char*) converter
        ));
      }
    }
  }

  if (len == 0)
    return;

  if (! is_bit_)
  {
    ::DDS::InstanceHandleSeq handles;
    // Create the list of readers repo id.

    if (this->bit_lookup_instance_handles (rd_ids, handles) == false)
      return;

    {
      // protect publication_match_status_ and status changed flags.
      ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);

      CORBA::ULong rd_len = handles.length ();

      for (CORBA::ULong i = 0; i < rd_len; ++i)
      {
        // update the publication_match_status_
        ++publication_match_status_.total_count;
        ++publication_match_status_.total_count_change;
        if (bind(id_to_handle_map_, rd_ids[i], handles[i]) != 0)
        {
          ::OpenDDS::DCPS::GuidConverter converter(
            const_cast< ::OpenDDS::DCPS::RepoId*>( &rd_ids[ i])
          );
          ACE_DEBUG((LM_WARNING,
            ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::fully_associated: ")
            ACE_TEXT("id_to_handle_map_%s = 0x%x failed.\n"),
            (const char*) converter,
            handles[i]
          ));
          return;

        } else if( DCPS_debug_level > 4) {
          ::OpenDDS::DCPS::GuidConverter converter(
            const_cast< ::OpenDDS::DCPS::RepoId*>( &rd_ids[ i])
          );
          ACE_DEBUG((LM_WARNING,
            ACE_TEXT("(%P|%t) DataWriterImpl::fully_associated: ")
            ACE_TEXT("id_to_handle_map_%s = 0x%x.\n"),
            (const char*) converter,
            handles[i]
          ));
        }
        publication_match_status_.last_subscription_handle = handles[i];
      }

      set_status_changed_flag (::DDS::PUBLICATION_MATCH_STATUS, true);
    }

    ::DDS::DataWriterListener* listener =
        listener_for (::DDS::PUBLICATION_MATCH_STATUS);

    if (listener != 0)
    {
      listener->on_publication_match (dw_local_objref_.in (),
        publication_match_status_);

      // TBD - why does the spec say to change this but not
      // change the ChangeFlagStatus after a listener call?
      publication_match_status_.total_count_change = 0;
    }
    notify_status_condition();
    delete [] remote_associations;
  }

  // Support DURABILITY QoS
  if( this->qos_.durability.kind > ::DDS::VOLATILE_DURABILITY_QOS) {
    // Tell the WriteDataContainer to resend all sending/sent
    // samples.
    this->data_container_->reenqueue_all (rd_ids,
                                          this->qos_.lifespan);

    // Acquire the data writer container lock to avoid deadlock. The
    // thread calling fully_associated() has to acquire lock in the
    // same order as the write()/register() operation.

    // Since the thread calling fully_associated() is the reactor
    // thread, it may have some performance penalty. If the
    // performance is an issue, we may need a new thread to handle the
    // data_availble() calls.
    ACE_GUARD (ACE_Recursive_Thread_Mutex,
               guard,
               this->get_lock());
    this->publisher_servant_->data_available(this, true);
  }
}


void
DataWriterImpl::remove_associations ( const ReaderIdSeq & readers,
                                      ::CORBA::Boolean notify_lost )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  if (DCPS_debug_level >= 1)
  {
    ::OpenDDS::DCPS::GuidConverter writerConverter( this->publication_id_);
    ::OpenDDS::DCPS::GuidConverter readerConverter(
      const_cast< ::OpenDDS::DCPS::RepoId*>( &readers[ 0])
    );
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataWriterImpl::remove_associations: ")
      ACE_TEXT("bit %d local %s remote %s num remotes %d\n"),
      is_bit_,
      (const char*) writerConverter,
      (const char*) readerConverter,
      readers.length()
    ));
  }

  ReaderIdSeq fully_associated_readers;
  CORBA::ULong fully_associated_len = 0;
  ReaderIdSeq rds;
  CORBA::ULong rds_len = 0;
  ::DDS::InstanceHandleSeq handles;

  {
    ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);

    //Remove the readers from fully associated reader list. 
    //If the supplied reader is not in the cached reader list then it is 
    //already removed. We just need remove the readers in the list that have
    //not been removed.

    CORBA::ULong len = readers.length();
    for (CORBA::ULong i = 0; i < len; ++i)
    {
      //Remove the readers from fully associated reader list. If it's not
      //in there, the fully_associated() is not called yet and remove it
      //from pending list.

      if (OpenDDS::DCPS::remove (readers_, readers[i]) == 0)
      {
        ++ fully_associated_len;
        fully_associated_readers.length (fully_associated_len);
        fully_associated_readers [fully_associated_len - 1] = readers[i]; 

        ++ rds_len;
        rds.length (rds_len);
        rds [rds_len - 1] = readers[i];         
      }
      else if (OpenDDS::DCPS::remove (pending_readers_, readers[i]) == 0)
      {
        ++ rds_len;
        rds.length (rds_len);
        rds [rds_len - 1] = readers[i]; 

        ::OpenDDS::DCPS::GuidConverter converter(
          const_cast< ::OpenDDS::DCPS::RepoId*>( &readers[ i])
        );
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataWriterImpl::remove_associations: ")
          ACE_TEXT("removing reader %s before fully_associated() call.\n"),
          (const char*) converter
        ));
      }
      //else reader is already removed which indicates remove_association()
      //is called multiple times.
    }
  
    if (fully_associated_len > 0 && ! is_bit_)
    {
      // The reader should be in the id_to_handle map at this time so
      // log with error.
      if (this->cache_lookup_instance_handles (fully_associated_readers, handles) == false)
      {
        ACE_ERROR ((LM_ERROR, "(%P|%t) ERROR: DataWriterImpl::remove_associations: "
          "cache_lookup_instance_handles failed, notify %d \n", notify_lost));
        return;
      }

      for (CORBA::ULong i = 0; i < fully_associated_len; ++i)
      {
        id_to_handle_map_.erase(fully_associated_readers[i]);
      }
    }
  }

  if (rds_len > 0)
  {
    this->publisher_servant_->remove_associations (rds,
      this->publication_id_);
  }

  // If this remove_association is invoked when the InfoRepo
  // detects a lost reader then make a callback to notify
  // subscription lost.
  if (notify_lost && handles.length () > 0)
  {
    this->notify_publication_lost (handles);
  }
}



void DataWriterImpl::remove_all_associations ()
{

  OpenDDS::DCPS::ReaderIdSeq readers;

  CORBA::ULong size = readers_.size();
  readers.length(size);
 
  IdSet::iterator itEnd = readers_.end ();
  int i = 0;
  for (IdSet::iterator it = readers_.begin (); it != itEnd; ++it)
  {
     readers[i ++] = *it;
  }

  try
  {
    if (0 < size)
    {
      CORBA::Boolean dont_notify_lost = false;
      this->remove_associations(readers, dont_notify_lost);
    }
  }
  catch (const CORBA::Exception&)
  {
  }
}


void
DataWriterImpl::update_incompatible_qos (
    const OpenDDS::DCPS::IncompatibleQosStatus & status)
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  ::DDS::DataWriterListener* listener =
    listener_for (::DDS::OFFERED_INCOMPATIBLE_QOS_STATUS);

  ACE_GUARD (ACE_Recursive_Thread_Mutex, guard, this->lock_);

#if 0
  if( this->offered_incompatible_qos_status_.total_count == status.total_count) {
    // This test should make the method idempotent.
    return;
  }
#endif

  set_status_changed_flag (::DDS::OFFERED_INCOMPATIBLE_QOS_STATUS, true);

  // copy status and increment change
  offered_incompatible_qos_status_.total_count = status.total_count;
  offered_incompatible_qos_status_.total_count_change +=
    status.count_since_last_send;
  offered_incompatible_qos_status_.last_policy_id = status.last_policy_id;
  offered_incompatible_qos_status_.policies = status.policies;

  if (listener != 0)
  {
    listener->on_offered_incompatible_qos (dw_local_objref_.in (),
                                           offered_incompatible_qos_status_);

    // TBD - Why does the spec say to change this but not change the
    //       ChangeFlagStatus after a listener call?
    offered_incompatible_qos_status_.total_count_change = 0;
  }
  notify_status_condition();
}

::DDS::ReturnCode_t
DataWriterImpl::set_qos (const ::DDS::DataWriterQos & qos)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos))
  {
    if (enabled_.value())
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
            ::DDS::PublisherQos publisherQos;
            this->publisher_servant_->get_qos(publisherQos);
            CORBA::Boolean status
              = repo->update_publication_qos(this->participant_servant_->get_domain_id(),
                                         this->participant_servant_->get_id(),
                                         this->publication_id_,
                                         qos,
                                         publisherQos);
            if (status == 0)
            {
              ACE_ERROR_RETURN ((LM_ERROR,
                ACE_TEXT("(%P|%t) "
                "DataWriterImpl::set_qos, ")
                ACE_TEXT("qos is not compatible. \n")),
                ::DDS::RETCODE_ERROR);
            }

          }
          catch (const CORBA::SystemException& sysex)
          {
            sysex._tao_print_exception (
              "ERROR: System Exception"
              " in DataWriterImpl::set_qos");
            return ::DDS::RETCODE_ERROR;
          }
          catch (const CORBA::UserException& userex)
          {
            userex._tao_print_exception (
              "ERROR:  Exception"
              " in DataWriterImpl::set_qos");
            return ::DDS::RETCODE_ERROR;
          }
        }
      }

      // Reset the deadline timer if the period has changed.
      if (qos_.deadline.period.sec != qos.deadline.period.sec
          || qos_.deadline.period.nanosec != qos.deadline.period.nanosec)
      {
        if (this->watchdog_.get ())
        {
          this->watchdog_->reset_interval (
            duration_to_time_value (qos.deadline.period));
        }
      }

      qos_ = qos;

      return ::DDS::RETCODE_OK;
    }
    if (! (qos_ == qos))
    {
      // Reset the deadline timer if the period has changed.
      if (qos_.deadline.period.sec != qos.deadline.period.sec
          || qos_.deadline.period.nanosec != qos.deadline.period.nanosec)
      {
        if (this->watchdog_.get ())
        {
          this->watchdog_->reset_interval (
            duration_to_time_value (qos.deadline.period));
        }
      }

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
DataWriterImpl::get_qos (::DDS::DataWriterQos & qos)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  qos = qos_;
}

::DDS::ReturnCode_t
DataWriterImpl::set_listener ( ::DDS::DataWriterListener_ptr a_listener,
                               ::DDS::StatusKindMask mask)
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  listener_mask_ = mask;
  //note: OK to duplicate  and reference_to_servant a nil object ref
  listener_ = ::DDS::DataWriterListener::_duplicate(a_listener);
  fast_listener_ = listener_.in ();
  return ::DDS::RETCODE_OK;
}

::DDS::DataWriterListener_ptr
DataWriterImpl::get_listener ()
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  return ::DDS::DataWriterListener::_duplicate (listener_.in ());
}

::DDS::Topic_ptr
DataWriterImpl::get_topic ()
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  return ::DDS::Topic::_duplicate (topic_objref_.in ());
}

::DDS::Publisher_ptr
DataWriterImpl::get_publisher ()
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  return ::DDS::Publisher::_duplicate (publisher_servant_);
}

::DDS::LivelinessLostStatus
DataWriterImpl::get_liveliness_lost_status ()
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
DataWriterImpl::get_offered_deadline_missed_status ()
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    this->lock_,
                    ::DDS::OfferedDeadlineMissedStatus ());

  set_status_changed_flag (::DDS::OFFERED_DEADLINE_MISSED_STATUS, false);

  this->offered_deadline_missed_status_.last_instance_handle =
    ::DDS::HANDLE_NIL;

  this->offered_deadline_missed_status_.total_count_change =
    this->offered_deadline_missed_status_.total_count
    - this->last_deadline_missed_total_count_;

  // Update for next status check.
  this->last_deadline_missed_total_count_ =
    this->offered_deadline_missed_status_.total_count;

  ::DDS::OfferedDeadlineMissedStatus const status =
      offered_deadline_missed_status_;

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
  ::DDS::OfferedIncompatibleQosStatus* status =
      new ::DDS::OfferedIncompatibleQosStatus;
  *status = offered_incompatible_qos_status_;
  offered_incompatible_qos_status_.total_count_change = 0;
  return status;
}

::DDS::PublicationMatchStatus
DataWriterImpl::get_publication_match_status ()
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
DataWriterImpl::assert_liveliness ()
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
DataWriterImpl::get_matched_subscriptions (
  ::DDS::InstanceHandleSeq & subscription_handles)
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  if (enabled_ == false)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("DataWriterImpl::get_matched_subscriptions: ")
                       ACE_TEXT(" Entity is not enabled. \n")),
                      ::DDS::RETCODE_NOT_ENABLED);
  }
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    this->lock_,
                    ::DDS::RETCODE_ERROR);

  // Copy out the handles for the current set of subscriptions.
  int index = 0;
  subscription_handles.length( this->id_to_handle_map_.size());
  for( RepoIdToHandleMap::iterator
       current = this->id_to_handle_map_.begin();
       current != this->id_to_handle_map_.end();
       ++current, ++index
     ) {
    subscription_handles[ index] = current->second;
  }

  return ::DDS::RETCODE_OK;
}

#if !defined (DDS_HAS_MINIMUM_BIT)
::DDS::ReturnCode_t
DataWriterImpl::get_matched_subscription_data (
    ::DDS::SubscriptionBuiltinTopicData & subscription_data,
    ::DDS::InstanceHandle_t subscription_handle)
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  if (enabled_ == false)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::")
                       ACE_TEXT("get_matched_subscription_data: ")
                       ACE_TEXT("Entity is not enabled. \n")),
      ::DDS::RETCODE_NOT_ENABLED);
  }

  BIT_Helper_1 < ::DDS::SubscriptionBuiltinTopicDataDataReader,
                 ::DDS::SubscriptionBuiltinTopicDataDataReader_var,
                 ::DDS::SubscriptionBuiltinTopicDataSeq > hh;

  ::DDS::SubscriptionBuiltinTopicDataSeq data;

  ::DDS::ReturnCode_t ret =
      hh.instance_handle_to_bit_data(participant_servant_,
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
DataWriterImpl::enable ()
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

  bool const should_block =
    (qos_.history.kind == ::DDS::KEEP_ALL_HISTORY_QOS
     && qos_.reliability.kind == ::DDS::RELIABLE_RELIABILITY_QOS);

  ACE_Time_Value max_blocking_time = ACE_Time_Value::zero;
  if (should_block)
  {
    max_blocking_time =
      duration_to_time_value (qos_.reliability.max_blocking_time);
  }

  CORBA::Long const depth =
    get_instance_sample_list_depth (
      qos_.history.kind,
      qos_.history.depth,
      qos_.resource_limits.max_samples_per_instance);

  if (qos_.resource_limits.max_samples != ::DDS::LENGTH_UNLIMITED)
  {
    n_chunks_ = qos_.resource_limits.max_samples;
  }
  //else using value from Service_Participant

  // enable the type specific part of this DataWriter
  this->enable_specific ();

  // Get data durability cache if DataWriter QoS requires durable
  // samples.  Publisher servant retains ownership of the cache.
  DataDurabilityCache* const durability_cache =
    TheServiceParticipant->get_data_durability_cache (qos_.durability);

  //Note: the QoS used to set n_chunks_ is Changable=No so
  // it is OK that we cannot change the size of our allocators.
  data_container_ = new WriteDataContainer (depth,
                                            should_block,
                                            max_blocking_time,
                                            n_chunks_,
                                            domain_id_,
                                            get_topic_name (),
                                            get_type_name (),
                                            durability_cache,
                                            qos_.durability_service);

  // +1 because we might allocate one before releasing another
  // TBD - see if this +1 can be removed.
  mb_allocator_ = new MessageBlockAllocator (n_chunks_ * association_chunk_multiplier_);
  db_allocator_ = new DataBlockAllocator (n_chunks_+1);
  header_allocator_ = new DataSampleHeaderAllocator (n_chunks_+1);
  if (DCPS_debug_level >= 2)
  {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) DataWriterImpl::enable-mb"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               mb_allocator_,
               n_chunks_));

    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) DataWriterImpl::enable-db"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               db_allocator_,
               n_chunks_));

    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) DataWriterImpl::enable-header"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               header_allocator_,
               n_chunks_));
  }

  if (qos_.liveliness.lease_duration.sec != ::DDS::DURATION_INFINITY_SEC
      || qos_.liveliness.lease_duration.nanosec != ::DDS::DURATION_INFINITY_NSEC)
  {
    liveliness_check_interval_ =
      duration_to_time_value (qos_.liveliness.lease_duration);
    liveliness_check_interval_ *=
      TheServiceParticipant->liveliness_factor ()/100.0;

    if (reactor_->schedule_timer(this,
                                 0,
                                 liveliness_check_interval_,
                                 liveliness_check_interval_) == -1)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::enable: ")
                  ACE_TEXT(" %p. \n"), "schedule_timer"));
    }
    else
    {
      cancel_timer_ = true;
      this->_add_ref ();
    }
  }

  // Setup the offered deadline watchdog if the configured deadline
  // period is not the default (infinite).
  ::DDS::Duration_t const deadline_period = this->qos_.deadline.period;
  if (deadline_period.sec != ::DDS::DURATION_INFINITY_SEC
      || deadline_period.nanosec != ::DDS::DURATION_INFINITY_NSEC)
  {
    ACE_auto_ptr_reset (this->watchdog_,
                        new OfferedDeadlineWatchdog (
                          this->reactor_,
                          this->lock_,
                          this->qos_.deadline,
                          this,
                          this->dw_local_objref_.in (),
                          this->offered_deadline_missed_status_,
                          this->last_deadline_missed_total_count_));
  }

  this->set_enabled ();

  ::DDS::ReturnCode_t const writer_enabled_result =
      publisher_servant_->writer_enabled (dw_remote_objref_.in(),
                                          dw_local_objref_.in (),
                                          topic_name_.in (),
                                          topic_id_);

  // Move cached data from the durability cache to the unsent data
  // queue.
  if (durability_cache != 0)
  {
    if (!durability_cache->get_data (this->domain_id_,
                                     get_topic_name (),
                                     get_type_name (),
                                     this,
                                     this->mb_allocator_,
                                     this->db_allocator_,
                                     this->qos_.lifespan))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::enable: ")
                  ACE_TEXT("unable to retrieve durable data\n")));
    }
  }

  return writer_enabled_result;
}

 
::DDS::ReturnCode_t
DataWriterImpl::register_instance(::DDS::InstanceHandle_t& handle,
                                  DataSample* data,
                                  const ::DDS::Time_t & source_timestamp)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  DBG_ENTRY_LVL("DataWriterImpl","register_instance",6);
  if (enabled_ == false)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("DataWriterImpl::register_instance: ")
                       ACE_TEXT(" Entity is not enabled. \n")),
                      ::DDS::RETCODE_NOT_ENABLED);
  }

  ::DDS::ReturnCode_t ret =
      this->data_container_->register_instance(handle, data);
  if (ret != ::DDS::RETCODE_OK)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::register_instance: ")
                       ACE_TEXT("register instance with container failed.\n")),
                      ret);
  }

  // Add header with the registration sample data.
  ACE_Message_Block* registered_sample =
    this->create_control_message(INSTANCE_REGISTRATION,
                                 data,
                                 source_timestamp);

  SendControlStatus status;
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publisher_servant_->get_pi_lock());

    status = this->publisher_servant_->send_control(publication_id_,
                                                    this,
                                                    registered_sample);
  }

  if (status == SEND_CONTROL_ERROR)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("DataWriterImpl::register_instance: ")
                       ACE_TEXT("send_control failed.\n")),
                      ::DDS::RETCODE_ERROR);
  }

  return ret;
}


::DDS::ReturnCode_t
DataWriterImpl::unregister ( ::DDS::InstanceHandle_t handle,
                             const ::DDS::Time_t & source_timestamp)
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  DBG_ENTRY_LVL("DataWriterImpl","unregister",6);
  if (enabled_ == false)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::unregister: ")
                       ACE_TEXT(" Entity is not enabled.\n")),
                      ::DDS::RETCODE_NOT_ENABLED);
  }


  DataSample* unregistered_sample_data;

  ::DDS::ReturnCode_t ret =
      this->data_container_->unregister(handle,
                                        unregistered_sample_data,
                                        this);

  if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("DataWriterImpl::unregister: ")
                         ACE_TEXT(" unregister with container failed. \n")),
                        ret);
    }

  ACE_Message_Block* message =
    this->create_control_message(UNREGISTER_INSTANCE,
                                 unregistered_sample_data,
                                 source_timestamp);

  SendControlStatus status;
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publisher_servant_->get_pi_lock());

    status = this->publisher_servant_->send_control(publication_id_,
                                                    this,
                                                    message);
  }

  if (status == SEND_CONTROL_ERROR)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::unregister: ")
                       ACE_TEXT(" send_control failed. \n")),
                      ::DDS::RETCODE_ERROR);
  }

  return ret;
}

::DDS::ReturnCode_t
DataWriterImpl::write (DataSample* data,
                       ::DDS::InstanceHandle_t handle,
                       const ::DDS::Time_t & source_timestamp)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  DBG_ENTRY_LVL("DataWriterImpl","write",6);
  if (enabled_ == false)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::write: ")
                       ACE_TEXT(" Entity is not enabled. \n")),
                      ::DDS::RETCODE_NOT_ENABLED);
  }

  DataSampleListElement* element;
  ::DDS::ReturnCode_t ret = this->data_container_->obtain_buffer(element,
                                                                 handle,
                                                                 this);

  if (ret != ::DDS::RETCODE_OK)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("DataWriterImpl::write: ")
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

  element->source_timestamp_.sec     = source_timestamp.sec;
  element->source_timestamp_.nanosec = source_timestamp.nanosec;

  ret = this->data_container_->enqueue(element,
                                       handle);

  if (ret != ::DDS::RETCODE_OK)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("DataWriterImpl::write: ")
                       ACE_TEXT("enqueue failed.\n")),
                      ret);
  }

  last_liveliness_activity_time_ = ACE_OS::gettimeofday ();

  // "Pet the dog" so that it doesn't callback on the listener the
  // next time deadline timer expires.
  if (this->watchdog_.get ())
    this->watchdog_->signal ();

  return this->publisher_servant_->data_available(this);
}

::DDS::ReturnCode_t
DataWriterImpl::dispose ( ::DDS::InstanceHandle_t handle,
                          const ::DDS::Time_t & source_timestamp)
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  DBG_ENTRY_LVL("DataWriterImpl","dispose",6);
  if (enabled_ == false)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::dispose: ")
                       ACE_TEXT(" Entity is not enabled. \n")),
                      ::DDS::RETCODE_NOT_ENABLED);
  }

  DataSample* registered_sample_data;
  ::DDS::ReturnCode_t ret =
      this->data_container_->dispose(handle,
                                     registered_sample_data);

  if (ret != ::DDS::RETCODE_OK)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: ")
                       ACE_TEXT("DataWriterImpl::dispose: ")
                       ACE_TEXT("dispose failed.\n")),
                      ret);
  }

  ACE_Message_Block* message =
    this->create_control_message(DISPOSE_INSTANCE,
                                 registered_sample_data,
                                 source_timestamp);

  SendControlStatus status;
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publisher_servant_->get_pi_lock());

    status  = this->publisher_servant_->send_control(publication_id_,
                                                     this,
                                                     message);
  }

  if (status == SEND_CONTROL_ERROR)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::dispose: ")
                       ACE_TEXT(" send_control failed. \n")),
                      ::DDS::RETCODE_ERROR);
  }

  return ret;
}


::DDS::ReturnCode_t
DataWriterImpl::num_samples ( ::DDS::InstanceHandle_t handle,
                              size_t&                 size)
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
    // The cancel_timer will call handle_close to remove_ref.
    (void) reactor_->cancel_timer (this, 0);
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

char const *
DataWriterImpl::get_type_name () const
{
  return type_name_.in ();
}

ACE_Message_Block*
DataWriterImpl::create_control_message (enum MessageId message_id,
                                        ACE_Message_Block* data,
                                        const ::DDS::Time_t& source_timestamp)
{
  DataSampleHeader header_data;
  header_data.message_id_ = message_id;
  //header_data.last_sample_
  header_data.byte_order_ =
    this->publisher_servant_->swap_bytes()
    ? !TAO_ENCAP_BYTE_ORDER
    : TAO_ENCAP_BYTE_ORDER;
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
                           mb_allocator_->malloc (sizeof (ACE_Message_Block))),
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
  return message;
}


::DDS::ReturnCode_t
DataWriterImpl::create_sample_data_message ( DataSample* data,
               ::DDS::InstanceHandle_t instance_handle,
               ACE_Message_Block*& message,
               const ::DDS::Time_t& source_timestamp)
{
  PublicationInstance* const instance =
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
  header_data.byte_order_ =
    this->publisher_servant_->swap_bytes()
    ? !TAO_ENCAP_BYTE_ORDER
    : TAO_ENCAP_BYTE_ORDER;
  header_data.message_length_ = data->total_length ();
  header_data.sequence_ = instance->sequence_.value_;
  ++instance->sequence_;
  header_data.source_timestamp_sec_ = source_timestamp.sec;
  header_data.source_timestamp_nanosec_ = source_timestamp.nanosec;

  if (qos_.lifespan.duration.sec != ::DDS::DURATION_INFINITY_SEC
    || qos_.lifespan.duration.nanosec != ::DDS::DURATION_INFINITY_NSEC)
  {
    header_data.lifespan_duration_ = true;
    header_data.lifespan_duration_sec_ = qos_.lifespan.duration.sec;
    header_data.lifespan_duration_nanosec_ = qos_.lifespan.duration.nanosec;
  }

  header_data.coherency_group_ = 0;
  header_data.publication_id_ = publication_id_;

  size_t max_marshaled_size = header_data.max_marshaled_size ();

  ACE_NEW_MALLOC_RETURN (message,
                         static_cast<ACE_Message_Block*> (
                           mb_allocator_->malloc (sizeof (ACE_Message_Block))),
                         ACE_Message_Block(max_marshaled_size,
                                           ACE_Message_Block::MB_DATA,
                                           data, //cont
                                           0, //data
                                           header_allocator_, //alloc_strategy
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
  DBG_ENTRY_LVL("DataWriterImpl","data_delivered",6);
  if( !(sample->publication_id_ == this->publication_id_))
  {
    ::OpenDDS::DCPS::GuidConverter sampleConverter( sample->publication_id_);
    ::OpenDDS::DCPS::GuidConverter writerConverter( this->publication_id_);
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::data_delivered: ")
      ACE_TEXT(" The publication id %s from delivered element ")
      ACE_TEXT("does not match the datawriter's id %s\n"),
      (const char*) sampleConverter,
      (const char*) writerConverter
    ));
    return;
  }

  this->data_container_->data_delivered (sample);

  ++data_delivered_count_;
}

void
DataWriterImpl::control_delivered(ACE_Message_Block* sample)
{
  DBG_ENTRY_LVL("DataWriterImpl","control_delivered",6);
  ++control_delivered_count_;
  sample->release ();
}

PublisherImpl*
DataWriterImpl::get_publisher_servant ()
{
  return publisher_servant_;
}

void
DataWriterImpl::remove_sample (DataSampleListElement* element,
                               bool dropped_by_transport)
{
  DBG_ENTRY_LVL("DataWriterImpl","remove_sample",6);
  publisher_servant_->remove_sample (element, dropped_by_transport);
}

void
DataWriterImpl::data_dropped (DataSampleListElement* element,
                              bool dropped_by_transport)
{
  DBG_ENTRY_LVL("DataWriterImpl","data_dropped",6);
  this->data_container_->data_dropped (element, dropped_by_transport);

  ++data_dropped_count_;
}

void
DataWriterImpl::control_dropped (ACE_Message_Block* sample,
                                 bool /* dropped_by_transport */)
{
  DBG_ENTRY_LVL("DataWriterImpl","control_dropped",6);
  ++control_dropped_count_;
  sample->release ();
}

int
DataWriterImpl::remove_all_control_msgs()
{
  DBG_ENTRY_LVL("DataWriterImpl","remove_all_control_msgs",6);
  return
    publisher_servant_->remove_all_control_msgs (this->publication_id_);
}

void
DataWriterImpl::unregistered(::DDS::InstanceHandle_t /* instance_handle */)
{
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
                                const void * /* arg */)
{
  ACE_Time_Value elapsed = tv - last_liveliness_activity_time_;

  if (elapsed >= liveliness_check_interval_)
  {
    //Not recent enough then send liveliness message.
    if (DCPS_debug_level > 9) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataWriterImpl::handle_timeout: ")
        ACE_TEXT("%s sending LIVELINESS message.\n"),
        (const char*) ::OpenDDS::DCPS::GuidConverter( this->publication_id_)
      ));
    }
    this->send_liveliness(tv);
  }
  else
  {
    // Recent enough. Schedule the interval.
    if (reactor_->cancel_timer (this) == -1)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("DataWriterImpl::handle_timeout: ")
                         ACE_TEXT(" %p. \n"),
                         "cancel_timer"),
                        -1);
    }

    ACE_Time_Value remain = liveliness_check_interval_ - elapsed;

    if (reactor_->schedule_timer(this,
                                 0,
                                 remain,
                                 liveliness_check_interval_) == -1)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("DataWriterImpl::handle_timeout: ")
                         ACE_TEXT(" %p. \n"),
                         "schedule_timer"),
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
  ACE_Message_Block* liveliness_msg =
    this->create_control_message(DATAWRITER_LIVELINESS, 0, t);

  SendControlStatus status;
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> justMe(publisher_servant_->get_pi_lock());

    status = this->publisher_servant_->send_control(publication_id_,
                                                    this,
                                                    liveliness_msg);
  }

  if (status == SEND_CONTROL_ERROR)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::send_liveliness: ")
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
  DBG_ENTRY_LVL("DataWriterImpl","notify_publication_disconnected",6);

  if (! is_bit_)
  {
    // Narrow to DDS::DCPS::DataWriterListener. If a DDS::DataWriterListener
    // is given to this DataWriter then narrow() fails.
    DataWriterListener_var the_listener =
      DataWriterListener::_narrow (this->listener_.in ());

    if (! CORBA::is_nil (the_listener.in ()))
    {
      PublicationDisconnectedStatus status;
      // Since this callback may come after remove_association which
      // removes the reader from id_to_handle map, we can ignore this
      // error.
      this->cache_lookup_instance_handles (subids,
                                           status.subscription_handles);
      the_listener->on_publication_disconnected (this->dw_local_objref_.in (),
                                                 status);
    }
  }
}



void
DataWriterImpl::notify_publication_reconnected (const ReaderIdSeq& subids)
{
  DBG_ENTRY_LVL("DataWriterImpl","notify_publication_reconnected",6);

  if (! is_bit_)
  {
    // Narrow to DDS::DCPS::DataWriterListener. If a
    // DDS::DataWriterListener is given to this DataWriter then
    // narrow() fails.
    DataWriterListener_var the_listener =
      DataWriterListener::_narrow (this->listener_.in ());

    if (! CORBA::is_nil (the_listener.in ()))
    {
      PublicationDisconnectedStatus status;

      // If it's reconnected then the reader should be in id_to_handle
      // map otherwise log with an error.
      if (this->cache_lookup_instance_handles (subids,
                                               status.subscription_handles) == false)
      {
        ACE_ERROR ((LM_ERROR,
                    "(%P|%t)ERROR: DataWriterImpl::"
                    "notify_publication_reconnected: "
                    "lookup_instance_handles failed\n"));
      }

      the_listener->on_publication_reconnected (this->dw_local_objref_.in (),
                                                status);
    }
  }
}


void
DataWriterImpl::notify_publication_lost (const ReaderIdSeq& subids)
{
  DBG_ENTRY_LVL("DataWriterImpl","notify_publication_lost",6);
  if (! is_bit_)
  {
    // Narrow to DDS::DCPS::DataWriterListener. If a
    // DDS::DataWriterListener is given to this DataWriter then
    // narrow() fails.
    DataWriterListener_var the_listener =
      DataWriterListener::_narrow (this->listener_.in ());

    if (! CORBA::is_nil (the_listener.in ()))
    {
      PublicationLostStatus status;

      // Since this callback may come after remove_association which removes
      // the reader from id_to_handle map, we can ignore this error.
      this->cache_lookup_instance_handles (subids,
                                           status.subscription_handles);
      the_listener->on_publication_lost (this->dw_local_objref_.in (),
      status);
    }
  }
}


void
DataWriterImpl::notify_publication_lost (const ::DDS::InstanceHandleSeq& handles)
{
  DBG_ENTRY_LVL("DataWriterImpl","notify_publication_lost",6);
  if (! is_bit_)
  {
    // Narrow to DDS::DCPS::DataWriterListener. If a
    // DDS::DataWriterListener is given to this DataWriter then
    // narrow() fails.
    DataWriterListener_var the_listener =
      DataWriterListener::_narrow (this->listener_.in ());

    if (! CORBA::is_nil (the_listener.in ()))
    {
      PublicationLostStatus status;

      CORBA::ULong len = handles.length ();
      status.subscription_handles.length (len);
      for (CORBA::ULong i = 0;i < len; ++ i)
      {
        status.subscription_handles[i] = handles[i];
      }

      the_listener->on_publication_lost (this->dw_local_objref_.in (),
          status);
    }
  }
}


void
DataWriterImpl::notify_connection_deleted ()
{
  DBG_ENTRY_LVL("DataWriterImpl","notify_connection_deleted",6);

  // Narrow to DDS::DCPS::DataWriterListener. If a DDS::DataWriterListener
  // is given to this DataWriter then narrow() fails.
  DataWriterListener_var the_listener =
    DataWriterListener::_narrow (this->listener_.in ());

  if (! CORBA::is_nil (the_listener.in ()))
    the_listener->on_connection_deleted (this->dw_local_objref_.in ());
}

bool
DataWriterImpl::bit_lookup_instance_handles (const ReaderIdSeq& ids,
                                             ::DDS::InstanceHandleSeq & hdls)
{
  if( DCPS_debug_level > 9) {
    CORBA::ULong const size = ids.length ();
    const char* separator = "";
    std::stringstream buffer;
    for( unsigned long i = 0; i < size; ++i) {
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter(
                 const_cast< ::OpenDDS::DCPS::RepoId*>( &ids[i])
               );
      buffer << separator << ids[i] << "(" << std::hex << handle << ")";
      separator = ", ";
    }
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataWriterImpl::bit_lookup_instance_handles: ")
      ACE_TEXT("searching for handles for reader Ids: %s.\n"),
      buffer.str().c_str()
    ));
  }

  if (TheServiceParticipant->get_BIT () == true
      && ! TheTransientKludge->is_enabled ())
  {
#if !defined (DDS_HAS_MINIMUM_BIT)
    BIT_Helper_2 < ::DDS::SubscriptionBuiltinTopicDataDataReader,
                   ::DDS::SubscriptionBuiltinTopicDataDataReader_var,
                   ::DDS::SubscriptionBuiltinTopicDataSeq,
                   ReaderIdSeq > hh;

    ::DDS::ReturnCode_t ret =
        hh.repo_ids_to_instance_handles(participant_servant_,
                                        BUILT_IN_SUBSCRIPTION_TOPIC,
                                        ids,
                                        hdls);

    if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataWriterImpl::")
        ACE_TEXT("bit_lookup_instance_handles: failed.\n")
      ));
      return false;

    } else if( DCPS_debug_level > 4) {
      CORBA::ULong const num_rds = ids.length ();
      for (CORBA::ULong i = 0; i < num_rds; ++i)
      {
        ACE_DEBUG((LM_WARNING,
          ACE_TEXT("(%P|%t) DataWriterImpl::bit_lookup_instance_handles: ")
          ACE_TEXT("reader %s has handle 0x%x.\n"),
          (const char*) ::OpenDDS::DCPS::GuidConverter(
                          const_cast< ::OpenDDS::DCPS::RepoId*>( &ids[ i])
                        ),
          hdls[i]
        ));
      }
    }
#endif // !defined (DDS_HAS_MINIMUM_BIT)
  }
  else
  {
    CORBA::ULong const num_rds = ids.length ();
    hdls.length (num_rds);
    for (CORBA::ULong i = 0; i < num_rds; ++i)
    {
      ::OpenDDS::DCPS::GuidConverter converter(
                                       const_cast< ::OpenDDS::DCPS::RepoId*>( &ids[i])
                                     );
      hdls[i] = converter;
      if( DCPS_debug_level > 4) {
        ACE_DEBUG((LM_WARNING,
          ACE_TEXT("(%P|%t) DataWriterImpl::bit_lookup_instance_handles: ")
          ACE_TEXT("using hash as handle for reader %s.\n"),
          (const char*) converter
        ));
      }
    }
  }

  return true;
}


bool
DataWriterImpl::cache_lookup_instance_handles (const ReaderIdSeq& ids,
                                               ::DDS::InstanceHandleSeq & hdls)
{
  bool ret = true;
  CORBA::ULong const num_ids = ids.length ();
  for (CORBA::ULong i = 0; i < num_ids; ++i)
  {
    hdls.length (i + 1);
    RepoIdToHandleMap::iterator iter = id_to_handle_map_.find(ids[i]);
    if (iter == id_to_handle_map_.end())
    {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) DataWriterImpl::cache_lookup_instance_handles: ")
        ACE_TEXT("could not find instance handle for writer %s.\n"),
        (const char*) ::OpenDDS::DCPS::GuidConverter(
                        const_cast< ::OpenDDS::DCPS::RepoId*>( &ids[ i])
                      )
      ));
      hdls[i] = -1;
      ret = false;
    }
    else
    {
      hdls[i] = iter->second;
      if( DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataWriterImpl::cache_lookup_instance_handles: ")
          ACE_TEXT("instance handle for writer %s == 0x%x.\n"),
          (const char*) ::OpenDDS::DCPS::GuidConverter(
                          const_cast< ::OpenDDS::DCPS::RepoId*>( &ids[ i])
                        ),
          hdls[i]
        ));
      }
    }
  }

  return ret;
}

bool
DataWriterImpl::persist_data ()
{
  return this->data_container_->persist_data ();
}



} // namespace DCPS
} // namespace OpenDDS
