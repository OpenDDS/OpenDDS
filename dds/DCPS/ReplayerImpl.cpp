/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ReplayerImpl.h"
#include "FeatureDisabledQosCheck.h"
#include "DomainParticipantImpl.h"
#include "PublisherImpl.h"
#include "Service_Participant.h"
#include "GuidConverter.h"
#include "TopicImpl.h"
#include "PublicationInstance.h"
#include "SendStateDataSampleList.h"
#include "DataSampleElement.h"
#include "Serializer.h"
#include "Transient_Kludge.h"
#include "DataDurabilityCache.h"
#include "OfferedDeadlineWatchdog.h"
#include "MonitorFactory.h"
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
#include "CoherentChangeControl.h"
#endif
#include "AssociationData.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "BuiltInTopicUtils.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "Util.h"

#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"

#include "ace/Reactor.h"
#include "ace/Auto_Ptr.h"

#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


ReplayerImpl::ReplayerImpl()
  : data_dropped_count_(0),
  data_delivered_count_(0),
  n_chunks_(TheServiceParticipant->n_chunks()),
  association_chunk_multiplier_(TheServiceParticipant->association_chunk_multiplier()),
  qos_(TheServiceParticipant->initial_DataWriterQos()),
  participant_servant_(0),
  topic_id_(GUID_UNKNOWN),
  topic_servant_(0),
  listener_mask_(DEFAULT_STATUS_MASK),
  domain_id_(0),
  publisher_servant_(0),
  publication_id_(GUID_UNKNOWN),
  sequence_number_(SequenceNumber::SEQUENCENUMBER_UNKNOWN()),
  // data_container_(0),
  // liveliness_lost_(false),
  // last_deadline_missed_total_count_(0),
  is_bit_(false),
  empty_condition_(lock_),
  pending_write_count_(0)
{
  // liveliness_lost_status_.total_count = 0;
  // liveliness_lost_status_.total_count_change = 0;
  //
  // offered_deadline_missed_status_.total_count = 0;
  // offered_deadline_missed_status_.total_count_change = 0;
  // offered_deadline_missed_status_.last_instance_handle = DDS::HANDLE_NIL;

  offered_incompatible_qos_status_.total_count = 0;
  offered_incompatible_qos_status_.total_count_change = 0;
  offered_incompatible_qos_status_.last_policy_id = 0;
  offered_incompatible_qos_status_.policies.length(0);

  publication_match_status_.total_count = 0;
  publication_match_status_.total_count_change = 0;
  publication_match_status_.current_count = 0;
  publication_match_status_.current_count_change = 0;
  publication_match_status_.last_subscription_handle = DDS::HANDLE_NIL;

}

// This method is called when there are no longer any reference to the
// the servant.
ReplayerImpl::~ReplayerImpl()
{
  DBG_ENTRY_LVL("ReplayerImpl","~ReplayerImpl",6);
}

// this method is called when delete_datawriter is called.
DDS::ReturnCode_t
ReplayerImpl::cleanup()
{

  //     // Unregister all registered instances prior to deletion.
  //     // DDS::Time_t source_timestamp = time_value_to_time(ACE_OS::gettimeofday());
  //     // this->unregister_instances(source_timestamp);
  //
  //     // CORBA::String_var topic_name = this->get_Atopic_name();
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, DDS::RETCODE_ERROR);

    // Wait for pending samples to drain prior to removing associations
    // and unregistering the publication.
    while (this->pending_write_count_)
      this->empty_condition_.wait();

    // Call remove association before unregistering the datawriter
    // with the transport, otherwise some callbacks resulted from
    // remove_association may lost.
    this->remove_all_associations();

    // release our Topic_var
    topic_objref_ = DDS::Topic::_nil();
    topic_servant_->remove_entity_ref();
    topic_servant_->_remove_ref();
    topic_servant_ = 0;

  }

  // not just unregister but remove any pending writes/sends.
  // this->unregister_all();

  Discovery_rch disco = TheServiceParticipant->get_discovery(this->domain_id_);
  if (!disco->remove_publication(
        this->domain_id_,
        this->participant_servant_->get_id(),
        this->publication_id_)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("PublisherImpl::delete_datawriter, ")
                      ACE_TEXT("publication not removed from discovery.\n")),
                     DDS::RETCODE_ERROR);
  }
  return DDS::RETCODE_OK;
}

void
ReplayerImpl::init(
  DDS::Topic_ptr                         topic,
  TopicImpl *                            topic_servant,
  const DDS::DataWriterQos &             qos,
  ReplayerListener_rch                   a_listener,
  const DDS::StatusMask &                mask,
  OpenDDS::DCPS::DomainParticipantImpl * participant_servant,
  const DDS::PublisherQos&               publisher_qos)
{
  DBG_ENTRY_LVL("ReplayerImpl","init",6);
  topic_objref_ = DDS::Topic::_duplicate(topic);
  topic_servant_ = topic_servant;
  topic_servant_->_add_ref();
  topic_servant_->add_entity_ref();
  topic_name_    = topic_servant_->get_name();
  topic_id_      = topic_servant_->get_id();
  type_name_     = topic_servant_->get_type_name();

#if !defined (DDS_HAS_MINIMUM_BIT)
  is_bit_ = ACE_OS::strcmp(topic_name_.in(), BUILT_IN_PARTICIPANT_TOPIC) == 0
            || ACE_OS::strcmp(topic_name_.in(), BUILT_IN_TOPIC_TOPIC) == 0
            || ACE_OS::strcmp(topic_name_.in(), BUILT_IN_SUBSCRIPTION_TOPIC) == 0
            || ACE_OS::strcmp(topic_name_.in(), BUILT_IN_PUBLICATION_TOPIC) == 0;
#endif   // !defined (DDS_HAS_MINIMUM_BIT)

  qos_ = qos;

  //Note: OK to _duplicate(nil).
  listener_ = a_listener;
  listener_mask_ = mask;

  // Only store the participant pointer, since it is our "grand"
  // parent, we will exist as long as it does.
  participant_servant_ = participant_servant;
  domain_id_ = participant_servant_->get_domain_id();

  publisher_qos_ = publisher_qos;
}


DDS::ReturnCode_t ReplayerImpl::set_qos (const DDS::PublisherQos &  publisher_qos,
                                         const DDS::DataWriterQos & qos)
{

  OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(publisher_qos, DDS::RETCODE_UNSUPPORTED);

  if (Qos_Helper::valid(publisher_qos) && Qos_Helper::consistent(publisher_qos)) {
    if (publisher_qos_ == publisher_qos)
      return DDS::RETCODE_OK;

    // for the not changeable qos, it can be changed before enable
    if (!Qos_Helper::changeable(publisher_qos_, publisher_qos) && enabled_ == true) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      publisher_qos_ = publisher_qos;
    }
  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }

  OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_OWNERSHIP_STRENGTH_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_DURABILITY_SERVICE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);

  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (qos_ == qos)
      return DDS::RETCODE_OK;

    if (!Qos_Helper::changeable(qos_, qos) && enabled_ == true) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
      // DDS::PublisherQos publisherQos;
      // this->publisher_servant_->get_qos(publisherQos);
      DDS::PublisherQos publisherQos = this->publisher_qos_;
      const bool status
        = disco->update_publication_qos(this->participant_servant_->get_domain_id(),
                                        this->participant_servant_->get_id(),
                                        this->publication_id_,
                                        qos,
                                        publisherQos);

      if (!status) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) DataWriterImpl::set_qos, ")
                          ACE_TEXT("qos not updated. \n")),
                         DDS::RETCODE_ERROR);
      }
    }

    if (!(qos_ == qos)) {
      // Reset the deadline timer if the period has changed.
      // if (qos_.deadline.period.sec != qos.deadline.period.sec
      //     || qos_.deadline.period.nanosec != qos.deadline.period.nanosec) {
      //   if (qos_.deadline.period.sec == DDS::DURATION_INFINITE_SEC
      //       && qos_.deadline.period.nanosec == DDS::DURATION_INFINITE_NSEC) {
      //     ACE_auto_ptr_reset(this->watchdog_,
      //                        new OfferedDeadlineWatchdog(
      //                          this->reactor_,
      //                          this->lock_,
      //                          qos.deadline,
      //                          this,
      //                          this->dw_local_objref_.in(),
      //                          this->offered_deadline_missed_status_,
      //                          this->last_deadline_missed_total_count_));
      //
      //   } else if (qos.deadline.period.sec == DDS::DURATION_INFINITE_SEC
      //              && qos.deadline.period.nanosec == DDS::DURATION_INFINITE_NSEC) {
      //     this->watchdog_->cancel_all();
      //     this->watchdog_.reset();
      //
      //   } else {
      //     this->watchdog_->reset_interval(
      //       duration_to_time_value(qos.deadline.period));
      //   }
      // }

      qos_ = qos;
    }

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t ReplayerImpl::get_qos (DDS::PublisherQos &  publisher_qos,
                                         DDS::DataWriterQos & qos)
{
  qos = qos_;
  publisher_qos = publisher_qos_;
  return DDS::RETCODE_OK;
}


DDS::ReturnCode_t ReplayerImpl::set_listener (const ReplayerListener_rch & a_listener,
                                              DDS::StatusMask              mask)
{
  listener_ = a_listener;
  listener_mask_ = mask;
  return DDS::RETCODE_OK;
}

ReplayerListener_rch ReplayerImpl::get_listener ()
{
  return listener_;
}

DDS::ReturnCode_t
ReplayerImpl::enable()
{
  //According spec:
  // - Calling enable on an already enabled Entity returns OK and has no
  // effect.
  // - Calling enable on an Entity whose factory is not enabled will fail
  // and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

  // if (this->publisher_servant_->is_enabled() == false) {
  //   return DDS::RETCODE_PRECONDITION_NOT_MET;
  // }
  //
  const bool reliable = qos_.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS;

  if (qos_.resource_limits.max_samples != DDS::LENGTH_UNLIMITED) {
    n_chunks_ = qos_.resource_limits.max_samples;
  }
  // +1 because we might allocate one before releasing another
  // TBD - see if this +1 can be removed.
  mb_allocator_.reset(new MessageBlockAllocator(n_chunks_ * association_chunk_multiplier_));
  db_allocator_.reset(new DataBlockAllocator(n_chunks_+1));
  header_allocator_.reset(new DataSampleHeaderAllocator(n_chunks_+1));

  sample_list_element_allocator_.reset(new DataSampleElementAllocator(2 * n_chunks_));

  transport_send_element_allocator_.reset(new TransportSendElementAllocator(2 * n_chunks_,
                                                       sizeof(TransportSendElement)));
  transport_customized_element_allocator_.reset(new TransportCustomizedElementAllocator(2 * n_chunks_,
                                                             sizeof(TransportCustomizedElement)));

  if (DCPS_debug_level >= 2) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) ReplayerImpl::enable-mb"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               mb_allocator_.get(),
               n_chunks_));

    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) ReplayerImpl::enable-db"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               db_allocator_.get(),
               n_chunks_));

    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) ReplayerImpl::enable-header"
               " Cached_Allocator_With_Overflow %x with %d chunks\n",
               header_allocator_.get(),
               n_chunks_));
  }

  this->set_enabled();

  try {
    this->enable_transport(reliable,
                           this->qos_.durability.kind > DDS::VOLATILE_DURABILITY_QOS);

  } catch (const Transport::Exception&) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ReplayerImpl::enable, ")
               ACE_TEXT("Transport Exception.\n")));
    return DDS::RETCODE_ERROR;

  }

  const TransportLocatorSeq& trans_conf_info = connection_info();


  Discovery_rch disco = TheServiceParticipant->get_discovery(this->domain_id_);
  this->publication_id_ =
    disco->add_publication(this->domain_id_,
                           this->participant_servant_->get_id(),
                           this->topic_servant_->get_id(),
                           this,
                           this->qos_,
                           trans_conf_info,
                           this->publisher_qos_);

  if (this->publication_id_ == GUID_UNKNOWN) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ReplayerImpl::enable, ")
               ACE_TEXT("add_publication returned invalid id. \n")));
    return DDS::RETCODE_ERROR;
  }

  return DDS::RETCODE_OK;
}



void
ReplayerImpl::add_association(const RepoId&            yourId,
                              const ReaderAssociation& reader,
                              bool                     active)
{
  DBG_ENTRY_LVL("ReplayerImpl", "add_association", 6);

  if (DCPS_debug_level >= 1) {
    GuidConverter writer_converter(yourId);
    GuidConverter reader_converter(reader.readerId);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ReplayerImpl::add_association - ")
               ACE_TEXT("bit %d local %C remote %C\n"),
               is_bit_,
               OPENDDS_STRING(writer_converter).c_str(),
               OPENDDS_STRING(reader_converter).c_str()));
  }

  // if (entity_deleted_ == true) {
  //   if (DCPS_debug_level >= 1)
  //     ACE_DEBUG((LM_DEBUG,
  //                ACE_TEXT("(%P|%t) ReplayerImpl::add_association")
  //                ACE_TEXT(" This is a deleted datawriter, ignoring add.\n")));
  //
  //   return;
  // }

  if (GUID_UNKNOWN == publication_id_) {
    publication_id_ = yourId;
  }

  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);
    reader_info_.insert(std::make_pair(reader.readerId,
                                       ReaderInfo(TheServiceParticipant->publisher_content_filter() ? reader.filterExpression : "",
                                                  reader.exprParams, participant_servant_,
                                                  reader.readerQos.durability.kind > DDS::VOLATILE_DURABILITY_QOS)));
  }

  if (DCPS_debug_level > 4) {
    GuidConverter converter(publication_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ReplayerImpl::add_association(): ")
               ACE_TEXT("adding subscription to publication %C with priority %d.\n"),
               OPENDDS_STRING(converter).c_str(),
               qos_.transport_priority.value));
  }

  AssociationData data;
  data.remote_id_ = reader.readerId;
  data.remote_data_ = reader.readerTransInfo;
  data.remote_reliable_ =
    (reader.readerQos.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS);
  data.remote_durable_ =
    (reader.readerQos.durability.kind > DDS::VOLATILE_DURABILITY_QOS);

  if (!this->associate(data, active)) {
    //FUTURE: inform inforepo and try again as passive peer
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_ERROR,
                 ACE_TEXT("(%P|%t) ReplayerImpl::add_association: ")
                 ACE_TEXT("ERROR: transport layer failed to associate.\n")));
    }
    return;
  }

  if (active) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

    // Have we already received an association_complete() callback?
    if (assoc_complete_readers_.count(reader.readerId)) {
      assoc_complete_readers_.erase(reader.readerId);
      association_complete_i(reader.readerId);

      // Add to pending_readers_ -> pending means we are waiting
      // for the association_complete() callback.
    } else if (OpenDDS::DCPS::insert(pending_readers_, reader.readerId) == -1) {
      GuidConverter converter(reader.readerId);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ReplayerImpl::add_association: ")
                 ACE_TEXT("failed to mark %C as pending.\n"),
                 OPENDDS_STRING(converter).c_str()));

    } else {
      if (DCPS_debug_level > 0) {
        GuidConverter converter(reader.readerId);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) ReplayerImpl::add_association: ")
                   ACE_TEXT("marked %C as pending.\n"),
                   OPENDDS_STRING(converter).c_str()));
      }
    }
  } else {
    // In the current implementation, DataWriter is always active, so this
    // code will not be applicable.
    Discovery_rch disco = TheServiceParticipant->get_discovery(this->domain_id_);
    disco->association_complete(this->domain_id_,
                                this->participant_servant_->get_id(),
                                this->publication_id_, reader.readerId);
  }
}


ReplayerImpl::ReaderInfo::ReaderInfo(const char*            filter,
                                     const DDS::StringSeq&  params,
                                     DomainParticipantImpl* participant,
                                     bool                   durable)
  : expected_sequence_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
  , durable_(durable)
{
  ACE_UNUSED_ARG(filter);
  ACE_UNUSED_ARG(params);
  ACE_UNUSED_ARG(participant);
}


ReplayerImpl::ReaderInfo::~ReaderInfo()
{
}


void
ReplayerImpl::association_complete(const RepoId& remote_id)
{
  DBG_ENTRY_LVL("ReplayerImpl", "association_complete", 6);

  if (DCPS_debug_level >= 1) {
    GuidConverter writer_converter(this->publication_id_);
    GuidConverter reader_converter(remote_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ReplayerImpl::association_complete - ")
               ACE_TEXT("bit %d local %C remote %C\n"),
               is_bit_,
               OPENDDS_STRING(writer_converter).c_str(),
               OPENDDS_STRING(reader_converter).c_str()));
  }

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);
  if (OpenDDS::DCPS::remove(pending_readers_, remote_id) == -1) {
    // Not found in pending_readers_, defer calling association_complete_i()
    // until add_association() resumes and sees this ID in assoc_complete_readers_.
    assoc_complete_readers_.insert(remote_id);
  } else {
    association_complete_i(remote_id);
  }
}

void
ReplayerImpl::association_complete_i(const RepoId& remote_id)
{
  DBG_ENTRY_LVL("ReplayerImpl", "association_complete_i", 6);
  // bool reader_durable = false;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);
    if (OpenDDS::DCPS::insert(readers_, remote_id) == -1) {
      GuidConverter converter(remote_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ReplayerImpl::association_complete_i: ")
                 ACE_TEXT("insert %C from pending failed.\n"),
                 OPENDDS_STRING(converter).c_str()));
    }
    // RepoIdToReaderInfoMap::const_iterator it = reader_info_.find(remote_id);
    // if (it != reader_info_.end()) {
    //   reader_durable = it->second.durable_;
    // }
  }

  if (!is_bit_) {

    DDS::InstanceHandle_t handle =
      this->participant_servant_->id_to_handle(remote_id);

    {
      // protect publication_match_status_ and status changed flags.
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

      // update the publication_match_status_
      ++publication_match_status_.total_count;
      ++publication_match_status_.total_count_change;
      ++publication_match_status_.current_count;
      ++publication_match_status_.current_count_change;

      if (OpenDDS::DCPS::bind(id_to_handle_map_, remote_id, handle) != 0) {
        GuidConverter converter(remote_id);
        ACE_DEBUG((LM_WARNING,
                   ACE_TEXT("(%P|%t) ERROR: ReplayerImpl::association_complete_i: ")
                   ACE_TEXT("id_to_handle_map_%C = 0x%x failed.\n"),
                   OPENDDS_STRING(converter).c_str(),
                   handle));
        return;

      } else if (DCPS_debug_level > 4) {
        GuidConverter converter(remote_id);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) ReplayerImpl::association_complete_i: ")
                   ACE_TEXT("id_to_handle_map_%C = 0x%x.\n"),
                   OPENDDS_STRING(converter).c_str(),
                   handle));
      }

      publication_match_status_.last_subscription_handle = handle;

    }


    if (listener_.in()) {
      listener_->on_replayer_matched(this,
                                     publication_match_status_);

      // TBD - why does the spec say to change this but not
      // change the ChangeFlagStatus after a listener call?
      publication_match_status_.total_count_change = 0;
      publication_match_status_.current_count_change = 0;
    }

  }

}

void
ReplayerImpl::remove_associations(const ReaderIdSeq & readers,
                                  CORBA::Boolean      notify_lost)
{
  if (DCPS_debug_level >= 1) {
    GuidConverter writer_converter(publication_id_);
    GuidConverter reader_converter(readers[0]);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ReplayerImpl::remove_associations: ")
               ACE_TEXT("bit %d local %C remote %C num remotes %d\n"),
               is_bit_,
               OPENDDS_STRING(writer_converter).c_str(),
               OPENDDS_STRING(reader_converter).c_str(),
               readers.length()));
  }

  this->stop_associating(readers.get_buffer(), readers.length());

  ReaderIdSeq fully_associated_readers;
  CORBA::ULong fully_associated_len = 0;
  ReaderIdSeq rds;
  CORBA::ULong rds_len = 0;
  DDS::InstanceHandleSeq handles;

  {
    // Ensure the same acquisition order as in wait_for_acknowledgments().
    // ACE_GUARD(ACE_SYNCH_MUTEX, wfaGuard, this->wfaLock_);
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

    //Remove the readers from fully associated reader list.
    //If the supplied reader is not in the cached reader list then it is
    //already removed. We just need remove the readers in the list that have
    //not been removed.

    CORBA::ULong len = readers.length();

    for (CORBA::ULong i = 0; i < len; ++i) {
      //Remove the readers from fully associated reader list. If it's not
      //in there, the association_complete() is not called yet and remove it
      //from pending list.

      if (OpenDDS::DCPS::remove(readers_, readers[i]) == 0) {
        ++fully_associated_len;
        fully_associated_readers.length(fully_associated_len);
        fully_associated_readers [fully_associated_len - 1] = readers[i];

        // Remove this reader from the ACK sequence map if its there.
        // This is where we need to be holding the wfaLock_ obtained
        // above.
        RepoIdToSequenceMap::iterator where
          = this->idToSequence_.find(readers[i]);

        if (where != this->idToSequence_.end()) {
          this->idToSequence_.erase(where);

          // It is possible that this subscription was causing the wait
          // to continue, so give the opportunity to find out.
          // this->wfaCondition_.broadcast();
        }

        ++rds_len;
        rds.length(rds_len);
        rds [rds_len - 1] = readers[i];

      } else if (OpenDDS::DCPS::remove(pending_readers_, readers[i]) == 0) {
        ++rds_len;
        rds.length(rds_len);
        rds [rds_len - 1] = readers[i];

        GuidConverter converter(readers[i]);
        ACE_DEBUG((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ReplayerImpl::remove_associations: ")
                   ACE_TEXT("removing reader %C before association_complete() call.\n"),
                   OPENDDS_STRING(converter).c_str()));
      }
      reader_info_.erase(readers[i]);
      //else reader is already removed which indicates remove_association()
      //is called multiple times.
    }

    if (fully_associated_len > 0 && !is_bit_) {
      // The reader should be in the id_to_handle map at this time so
      // log with error.
      if (this->lookup_instance_handles(fully_associated_readers, handles) == false) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ReplayerImpl::remove_associations: "
                   "lookup_instance_handles failed, notify %d \n", notify_lost));
        return;
      }

      for (CORBA::ULong i = 0; i < fully_associated_len; ++i) {
        id_to_handle_map_.erase(fully_associated_readers[i]);
      }
    }

    // wfaGuard.release();

    // Mirror the PUBLICATION_MATCHED_STATUS processing from
    // association_complete() here.
    if (!this->is_bit_) {

      // Derive the change in the number of subscriptions reading this writer.
      int matchedSubscriptions =
        static_cast<int>(this->id_to_handle_map_.size());
      this->publication_match_status_.current_count_change =
        matchedSubscriptions - this->publication_match_status_.current_count;

      // Only process status if the number of subscriptions has changed.
      if (this->publication_match_status_.current_count_change != 0) {
        this->publication_match_status_.current_count = matchedSubscriptions;

        /// Section 7.1.4.1: total_count will not decrement.

        /// @TODO: Reconcile this with the verbiage in section 7.1.4.1
        /// TODO: Should rds_len really be fully_associated_len here??
        this->publication_match_status_.last_subscription_handle =
          handles[rds_len - 1];


        if (listener_.in()) {
          listener_->on_replayer_matched(
            this,
            this->publication_match_status_);

          // Listener consumes the change.
          this->publication_match_status_.total_count_change = 0;
          this->publication_match_status_.current_count_change = 0;
        }

      }
    }
  }

  for (CORBA::ULong i = 0; i < rds.length(); ++i) {
    this->disassociate(rds[i]);
  }

  // If this remove_association is invoked when the InfoRepo
  // detects a lost reader then make a callback to notify
  // subscription lost.
  if (notify_lost && handles.length() > 0) {
    this->notify_publication_lost(handles);
  }
}

void ReplayerImpl::remove_all_associations()
{
  this->stop_associating();

  OpenDDS::DCPS::ReaderIdSeq readers;
  CORBA::ULong size;
  CORBA::ULong num_pending_readers;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, lock_);

    num_pending_readers = static_cast<CORBA::ULong>(pending_readers_.size());
    size = static_cast<CORBA::ULong>(readers_.size()) + num_pending_readers;
    readers.length(size);

    RepoIdSet::iterator itEnd = readers_.end();
    int i = 0;

    for (RepoIdSet::iterator it = readers_.begin(); it != itEnd; ++it) {
      readers[i++] = *it;
    }

    itEnd = pending_readers_.end();
    for (RepoIdSet::iterator it = pending_readers_.begin(); it != itEnd; ++it) {
      readers[i++] = *it;
    }

    if (num_pending_readers > 0) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ReplayerImpl::remove_all_associations() - ")
                 ACE_TEXT("%d subscribers were pending and never fully associated.\n"),
                 num_pending_readers));
    }
  }

  try {
    if (0 < size) {
      CORBA::Boolean dont_notify_lost = false;
      this->remove_associations(readers, dont_notify_lost);
    }

  } catch (const CORBA::Exception&) {
  }
}

void
ReplayerImpl::register_for_reader(const RepoId& participant,
                                  const RepoId& writerid,
                                  const RepoId& readerid,
                                  const TransportLocatorSeq& locators,
                                  DiscoveryListener* listener)
{
  TransportClient::register_for_reader(participant, writerid, readerid, locators, listener);
}

void
ReplayerImpl::unregister_for_reader(const RepoId& participant,
                                    const RepoId& writerid,
                                    const RepoId& readerid)
{
  TransportClient::unregister_for_reader(participant, writerid, readerid);
}

void
ReplayerImpl::update_incompatible_qos(const IncompatibleQosStatus& status)
{


  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // copy status and increment change
  offered_incompatible_qos_status_.total_count = status.total_count;
  offered_incompatible_qos_status_.total_count_change +=
    status.count_since_last_send;
  offered_incompatible_qos_status_.last_policy_id = status.last_policy_id;
  offered_incompatible_qos_status_.policies = status.policies;

}

void
ReplayerImpl::update_subscription_params(const RepoId&         readerId,
                                         const DDS::StringSeq& params)
{
  ACE_UNUSED_ARG(readerId);
  ACE_UNUSED_ARG(params);
}

void
ReplayerImpl::inconsistent_topic()
{
  topic_servant_->inconsistent_topic();
}

bool
ReplayerImpl::check_transport_qos(const TransportInst&)
{
  // DataWriter does not impose any constraints on which transports
  // may be used based on QoS.
  return true;
}

const RepoId&
ReplayerImpl::get_repo_id() const
{
  return this->publication_id_;
}

CORBA::Long
ReplayerImpl::get_priority_value(const AssociationData&) const
{
  return this->qos_.transport_priority.value;
}

void
ReplayerImpl::data_delivered(const DataSampleElement* sample)
{
  DBG_ENTRY_LVL("ReplayerImpl","data_delivered",6);
  if (!(sample->get_pub_id() == this->publication_id_)) {
    GuidConverter sample_converter(sample->get_pub_id());
    GuidConverter writer_converter(publication_id_);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ReplayerImpl::data_delivered: ")
               ACE_TEXT(" The publication id %C from delivered element ")
               ACE_TEXT("does not match the datawriter's id %C\n"),
               OPENDDS_STRING(sample_converter).c_str(),
               OPENDDS_STRING(writer_converter).c_str()));
    return;
  }
  DataSampleElement* elem = const_cast<DataSampleElement*>(sample);
  // this->data_container_->data_delivered(sample);
  ACE_DES_FREE(elem, sample_list_element_allocator_->free, DataSampleElement);
  ++data_delivered_count_;

  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);
    if ((--pending_write_count_) == 0) {
      empty_condition_.broadcast();
    }
  }
}

void
ReplayerImpl::control_delivered(ACE_Message_Block* sample)
{
  ACE_UNUSED_ARG(sample);
}

void
ReplayerImpl::data_dropped(const DataSampleElement* sample,
                           bool                         dropped_by_transport)
{
  DBG_ENTRY_LVL("ReplayerImpl","data_dropped",6);
  // this->data_container_->data_dropped(element, dropped_by_transport);
  ACE_UNUSED_ARG(dropped_by_transport);
  DataSampleElement* elem = const_cast<DataSampleElement*>(sample);
  ACE_DES_FREE(elem, sample_list_element_allocator_->free, DataSampleElement);
  ++data_dropped_count_;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);
    if ((--pending_write_count_) == 0) {
      empty_condition_.broadcast();
    }
  }
}

void
ReplayerImpl::control_dropped(ACE_Message_Block* sample,
                              bool /* dropped_by_transport */)
{
  ACE_UNUSED_ARG(sample);
}

void
ReplayerImpl::notify_publication_disconnected(const ReaderIdSeq& subids)
{
  ACE_UNUSED_ARG(subids);
}

void
ReplayerImpl::notify_publication_reconnected(const ReaderIdSeq& subids)
{
  ACE_UNUSED_ARG(subids);
}

void
ReplayerImpl::notify_publication_lost(const ReaderIdSeq& subids)
{
  ACE_UNUSED_ARG(subids);
}

void
ReplayerImpl::notify_publication_lost(const DDS::InstanceHandleSeq& handles)
{
  ACE_UNUSED_ARG(handles);
}

void
ReplayerImpl::notify_connection_deleted(const RepoId&)
{
}

void
ReplayerImpl::retrieve_inline_qos_data(TransportSendListener::InlineQosData& qos_data) const
{
  qos_data.pub_qos = this->publisher_qos_;
  qos_data.dw_qos = this->qos_;
  qos_data.topic_name = this->topic_name_.in();
}

DDS::ReturnCode_t
ReplayerImpl::write (const RawDataSample*   samples,
                     int                    num_samples,
                     DDS::InstanceHandle_t* reader_ih_ptr)
{
  DBG_ENTRY_LVL("ReplayerImpl","write",6);

  OpenDDS::DCPS::RepoId repo_id;
  if (reader_ih_ptr) {
    repo_id = this->participant_servant_->get_repoid(*reader_ih_ptr);
    if (repo_id == GUID_UNKNOWN) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ReplayerImpl::write: ")
                        ACE_TEXT("Invalid reader instance handle (%d)\n"), *reader_ih_ptr),
                       DDS::RETCODE_ERROR);
    }
  }

  SendStateDataSampleList list;

  for (int i = 0; i < num_samples; ++i) {
    DataSampleElement* element = 0;

    ACE_NEW_MALLOC_RETURN(
      element,
      static_cast<DataSampleElement*>(
        sample_list_element_allocator_->malloc(
          sizeof(DataSampleElement))),
      DataSampleElement(publication_id_,
                            this,
                            PublicationInstance_rch(),
                            transport_send_element_allocator_.get(),
                            transport_customized_element_allocator_.get()),
      DDS::RETCODE_ERROR);

    element->get_header().byte_order_ = samples[i].sample_byte_order_;
    element->get_header().publication_id_ = this->publication_id_;
    list.enqueue_tail(element);
    DataSample* temp;
    DDS::ReturnCode_t ret = create_sample_data_message(samples[i].sample_->duplicate(),
                                                       element->get_header(),
                                                       temp,
                                                       samples[i].source_timestamp_,
                                                       false);
    element->set_sample(temp);
    if (reader_ih_ptr) {
      element->set_num_subs(1);
      element->set_sub_id(0, repo_id);
    }

    if (ret != DDS::RETCODE_OK) {
      // we need to free the list
      while (list.dequeue(element)) {
        ACE_DES_FREE(element, sample_list_element_allocator_->free, DataSampleElement);
      }

      return ret;
    }
  }

  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, DDS::RETCODE_ERROR);
    ++pending_write_count_;
  }

  this->send(list);

  for (RepoIdToReaderInfoMap::iterator iter = reader_info_.begin(),
       end = reader_info_.end(); iter != end; ++iter) {
    iter->second.expected_sequence_ = sequence_number_;
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
ReplayerImpl::write(const RawDataSample& sample)
{
  return this->write(&sample, 1, 0);
}

DDS::ReturnCode_t
ReplayerImpl::create_sample_data_message(DataSample*         data,
                                         DataSampleHeader&   header_data,
                                         ACE_Message_Block*& message,
                                         const DDS::Time_t&  source_timestamp,
                                         bool                content_filter)
{
  header_data.message_id_ = SAMPLE_DATA;
  header_data.coherent_change_ = content_filter;

  header_data.content_filter_ = 0;
  header_data.cdr_encapsulation_ = this->cdr_encapsulation();
  header_data.message_length_ = static_cast<ACE_UINT32>(data->total_length());
  header_data.sequence_repair_ = need_sequence_repair();
  if (this->sequence_number_ == SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    this->sequence_number_ = SequenceNumber();
  } else {
    ++this->sequence_number_;
  }
  header_data.sequence_ = this->sequence_number_;
  header_data.source_timestamp_sec_ = source_timestamp.sec;
  header_data.source_timestamp_nanosec_ = source_timestamp.nanosec;

  if (qos_.lifespan.duration.sec != DDS::DURATION_INFINITE_SEC
      || qos_.lifespan.duration.nanosec != DDS::DURATION_INFINITE_NSEC) {
    header_data.lifespan_duration_ = true;
    header_data.lifespan_duration_sec_ = qos_.lifespan.duration.sec;
    header_data.lifespan_duration_nanosec_ = qos_.lifespan.duration.nanosec;
  }

  // header_data.publication_id_ = publication_id_;
  // header_data.publisher_id_ = this->publisher_servant_->publisher_id_;
  size_t max_marshaled_size = header_data.max_marshaled_size();

  ACE_NEW_MALLOC_RETURN(message,
                        static_cast<ACE_Message_Block*>(
                          mb_allocator_->malloc(sizeof(ACE_Message_Block))),
                        ACE_Message_Block(max_marshaled_size,
                                          ACE_Message_Block::MB_DATA,
                                          data,   //cont
                                          0,   //data
                                          header_allocator_.get(),   //alloc_strategy
                                          0,   //locking_strategy
                                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                          ACE_Time_Value::zero,
                                          ACE_Time_Value::max_time,
                                          db_allocator_.get(),
                                          mb_allocator_.get()),
                        DDS::RETCODE_ERROR);

  *message << header_data;
  return DDS::RETCODE_OK;
}

bool
ReplayerImpl::lookup_instance_handles(const ReaderIdSeq&       ids,
                                      DDS::InstanceHandleSeq & hdls)
{
  if (DCPS_debug_level > 9) {
    CORBA::ULong const size = ids.length();
    OPENDDS_STRING separator;
    OPENDDS_STRING buffer;

    for (unsigned long i = 0; i < size; ++i) {
      buffer += separator + OPENDDS_STRING(GuidConverter(ids[i]));
      separator = ", ";
    }

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataWriterImpl::lookup_instance_handles: ")
               ACE_TEXT("searching for handles for reader Ids: %C.\n"),
               buffer.c_str()));
  }

  CORBA::ULong const num_rds = ids.length();
  hdls.length(num_rds);

  for (CORBA::ULong i = 0; i < num_rds; ++i) {
    hdls[i] = this->participant_servant_->id_to_handle(ids[i]);
  }

  return true;
}

bool
ReplayerImpl::need_sequence_repair() const
{
  for (RepoIdToReaderInfoMap::const_iterator it = reader_info_.begin(),
       end = reader_info_.end(); it != end; ++it) {
    if (it->second.expected_sequence_ != sequence_number_) {
      return true;
    }
  }
  return false;
}

DDS::InstanceHandle_t
ReplayerImpl::get_instance_handle()
{
  return this->participant_servant_->id_to_handle(publication_id_);
}

DDS::ReturnCode_t
ReplayerImpl::write_to_reader (DDS::InstanceHandle_t subscription,
                               const RawDataSample&  sample )
{
  return write(&sample, 1, &subscription);
}

DDS::ReturnCode_t
ReplayerImpl::write_to_reader (DDS::InstanceHandle_t    subscription,
                               const RawDataSampleList& samples )
{
  if (samples.size())
    return write(&samples[0], static_cast<int>(samples.size()), &subscription);
  return DDS::RETCODE_ERROR;
}

} // namespace DCPS
} // namespace

OPENDDS_END_VERSIONED_NAMESPACE_DECL
