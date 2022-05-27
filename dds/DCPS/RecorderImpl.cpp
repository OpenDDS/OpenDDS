/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "RecorderImpl.h"

#include "SubscriptionInstance.h"
#include "ReceivedDataElementList.h"
#include "DomainParticipantImpl.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "FeatureDisabledQosCheck.h"
#include "GuidConverter.h"
#include "Serializer.h"
#include "SubscriberImpl.h"
#include "Transient_Kludge.h"
#include "Util.h"
#include "QueryConditionImpl.h"
#include "ReadConditionImpl.h"
#include "MonitorFactory.h"
#include "SafetyProfileStreams.h"
#include "TypeSupportImpl.h"
#include "PoolAllocator.h"
#include "DCPS_Utils.h"
#ifndef DDS_HAS_MINIMUM_BIT
#  include "BuiltInTopicUtils.h"
#endif
#include "transport/framework/EntryExit.h"
#include "transport/framework/TransportExceptions.h"

#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsDcpsGuidTypeSupportImpl.h>
#ifndef DDS_HAS_MINIMUM_BIT
#  include <dds/DdsDcpsCoreTypeSupportC.h>
#endif

#include <tao/ORB_Core.h>

#include <ace/Reactor.h>

#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RecorderImpl::RecorderImpl()
  : qos_(TheServiceParticipant->initial_DataReaderQos()),
  participant_servant_(0),
  topic_servant_(0),
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  is_exclusive_ownership_ (false),
#endif
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  owner_manager_ (0),
#endif
  subqos_ (TheServiceParticipant->initial_SubscriberQos()),
  topic_desc_(0),
  listener_mask_(DEFAULT_STATUS_MASK),
  domain_id_(0),
  is_bit_(false),
  check_encap_(true)
{
  requested_incompatible_qos_status_.total_count = 0;
  requested_incompatible_qos_status_.total_count_change = 0;
  requested_incompatible_qos_status_.last_policy_id = 0;
  requested_incompatible_qos_status_.policies.length(0);

  subscription_match_status_.total_count = 0;
  subscription_match_status_.total_count_change = 0;
  subscription_match_status_.current_count = 0;
  subscription_match_status_.current_count_change = 0;
  subscription_match_status_.last_publication_handle = DDS::HANDLE_NIL;
}

// This method is called when there are no longer any reference to the
// the servant.
RecorderImpl::~RecorderImpl()
{
  DBG_ENTRY_LVL("RecorderImpl","~RecorderImpl",6);
}


DDS::ReturnCode_t
RecorderImpl::cleanup()
{

  Discovery_rch disco = TheServiceParticipant->get_discovery(this->domain_id_);
  if (!disco->remove_subscription(this->domain_id_,
                                  participant_servant_->get_id(),
                                  this->subscription_id_)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: RecorderImpl::cleanup: "
        "could not remove subscription from discovery\n"));
    }
    return DDS::RETCODE_ERROR;
  }

  // Call remove association before unregistering the datareader from the transport,
  // otherwise some callbacks resulted from remove_association may lost.

  this->remove_all_associations();

  return DDS::RETCODE_OK;
}

void RecorderImpl::init(
  TopicDescriptionImpl*      a_topic_desc,
  const DDS::DataReaderQos & qos,
  RecorderListener_rch       a_listener,
  const DDS::StatusMask &    mask,
  DomainParticipantImpl*     participant,
  DDS::SubscriberQos         subqos)
{
  if (DCPS_debug_level >= 2) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RecorderImpl::init\n"));
  }


  topic_desc_ = DDS::TopicDescription::_duplicate(a_topic_desc);
  if (TopicImpl* a_topic = dynamic_cast<TopicImpl*>(a_topic_desc)) {
    topic_servant_ = a_topic;
  }

  CORBA::String_var topic_name = a_topic_desc->get_name();
  qos_ = qos;
  passed_qos_ = qos;

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  is_exclusive_ownership_ = this->qos_.ownership.kind == ::DDS::EXCLUSIVE_OWNERSHIP_QOS;
#endif

  listener_ = a_listener;
  listener_mask_ = mask;

  // Only store the participant pointer, since it is our "grand"
  // parent, we will exist as long as it does
  participant_servant_ = participant;

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if (is_exclusive_ownership_) {
    owner_manager_ = participant_servant_->ownership_manager ();
  }
#endif

  domain_id_ = participant_servant_->get_domain_id();
  subqos_ = subqos;
}

bool RecorderImpl::check_transport_qos(const TransportInst& ti)
{
  if (this->qos_.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS) {
    return ti.is_reliable();
  }
  return true;
}

RepoId RecorderImpl::get_repo_id() const
{
  return this->subscription_id_;
}

CORBA::Long RecorderImpl::get_priority_value(const AssociationData& data) const
{
  return data.publication_transport_priority_;
}


void RecorderImpl::data_received(const ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("RecorderImpl","data_received",6);

  // Ensure some other thread is not changing the sample container
  // or statuses related to samples.
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);

  if (DCPS_debug_level >= 8) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) RecorderImpl::data_received: "
               "%C received sample: %C\n",
               LogGuid(subscription_id_).c_str(),
               to_string(sample.header_).c_str()));
  }

  // we only support SAMPLE_DATA messages
  if (sample.header_.message_id_ == SAMPLE_DATA && listener_.in()) {
    Encoding::Kind kind = Encoding::KIND_UNALIGNED_CDR;
    if (sample.header_.cdr_encapsulation_ && check_encap_) {
      Encoding enc;
      Serializer ser(sample.sample_.get(), enc);
      EncapsulationHeader encap;
      if (ser >> encap && encap.to_any_encoding(enc)) {
        kind = enc.kind();
      }
    }
    RawDataSample rawSample(sample.header_,
                            static_cast<MessageId> (sample.header_.message_id_),
                            sample.header_.source_timestamp_sec_,
                            sample.header_.source_timestamp_nanosec_,
                            sample.header_.publication_id_,
                            sample.header_.byte_order_,
                            sample.sample_.get(),
                            kind);
    listener_->on_sample_data_received(this, rawSample);
  }
}

void RecorderImpl::notify_subscription_disconnected(const WriterIdSeq&)
{
}

void RecorderImpl::notify_subscription_reconnected(const WriterIdSeq&)
{
}

void
RecorderImpl::notify_subscription_lost(const DDS::InstanceHandleSeq&)
{
}

void RecorderImpl::notify_subscription_lost(const WriterIdSeq&)
{
}

#ifndef OPENDDS_SAFETY_PROFILE
void
RecorderImpl::add_to_dynamic_type_map(const PublicationId& pub_id, const XTypes::TypeIdentifier& ti)
{
  XTypes::TypeLookupService_rch tls = participant_servant_->get_type_lookup_service();
  DDS::DynamicType_var dt = tls->type_identifier_to_dynamic(ti, pub_id);
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) RecorderImpl::add_association: "
               "DynamicType added to map with guid: %C\n", LogGuid(pub_id).c_str()));
  }
  dt_map_.insert(std::make_pair(pub_id, dt));
}
#endif

void
RecorderImpl::add_association(const RepoId&            yourId,
                              const WriterAssociation& writer,
                              bool                     active)
{
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RecorderImpl::add_association: "
               "bit %d local %C remote %C\n",
               is_bit_,
               LogGuid(yourId).c_str(),
               LogGuid(writer.writerId).c_str()));
  }

  //
  // This block prevents adding associations to deleted readers.
  // Presumably this is a "good thing(tm)".
  //
  // if (entity_deleted_ == true) {
  //   if (DCPS_debug_level >= 1)
  //     ACE_DEBUG((LM_DEBUG,
  //                ACE_TEXT("(%P|%t) RecorderImpl::add_association")
  //                ACE_TEXT(" This is a deleted datareader, ignoring add.\n")));
  //
  //   return;
  // }

  //
  // We are being called back from the repository before we are done
  // processing after our call to the repository that caused this call
  // (from the repository) to be made.
  //
  if (GUID_UNKNOWN == subscription_id_) {
    // add_associations was invoked before DCSPInfoRepo::add_subscription() returned.
    subscription_id_ = yourId;
  }

  //
  // We do the following while holding the publication_handle_lock_.
  //
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

    //
    // For each writer in the list of writers to associate with, we
    // create a WriterInfo and a WriterStats object and store them in
    // our internal maps.
    //
    {
      ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, this->writers_lock_);

      const PublicationId& writer_id = writer.writerId;
      RcHandle<WriterInfo> info ( make_rch<WriterInfo>(static_cast<WriterInfoListener*>(this), writer_id, writer.writerQos));
      /*std::pair<WriterMapType::iterator, bool> bpair =*/
      this->writers_.insert(
        // This insertion is idempotent.
        WriterMapType::value_type(
          writer_id,
          info));
      // this->statistics_.insert(
      //   StatsMapType::value_type(
      //     writer_id,
      //     WriterStats(
      //       this->raw_latency_buffer_size_,
      //       this->raw_latency_buffer_type_)));

      // if (DCPS_debug_level > 4) {
      //   GuidConverter converter(writer_id);
      //   ACE_DEBUG((LM_DEBUG,
      //              "(%P|%t) RecorderImpl::add_association: "
      //              "inserted writer %C.return %d\n",
      //              OPENDDS_STRING(converter).c_str(), bpair.second));
      //
      //   WriterMapType::iterator iter = writers_.find(writer_id);
      //   if (iter != writers_.end()) {
      //     // This may not be an error since it could happen that the sample
      //     // is delivered to the datareader after the write is dis-associated
      //     // with this datareader.
      //     GuidConverter reader_converter(subscription_id_);
      //     GuidConverter writer_converter(writer_id);
      //     ACE_DEBUG((LM_DEBUG,
      //               ACE_TEXT("(%P|%t) RecorderImpl::add_association: ")
      //               ACE_TEXT("reader %C is associated with writer %C.\n"),
      //               OPENDDS_STRING(reader_converter).c_str(),
      //               OPENDDS_STRING(writer_converter).c_str()));
      //   }
      // }
    }

    //
    // Propagate the add_associations processing down into the Transport
    // layer here.  This will establish the transport support and reserve
    // usage of an existing connection or initiate creation of a new
    // connection if no suitable connection is available.
    //
    AssociationData data;
    data.remote_id_ = writer.writerId;
    data.remote_data_ = writer.writerTransInfo;
    data.discovery_locator_ = writer.writerDiscInfo;
    data.remote_transport_context_ = writer.transportContext;
    data.publication_transport_priority_ =
      writer.writerQos.transport_priority.value;
    data.remote_reliable_ =
      (writer.writerQos.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS);
    data.remote_durable_ =
      (writer.writerQos.durability.kind > DDS::VOLATILE_DURABILITY_QOS);

    if (!this->associate(data, active)) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: RecorderImpl::add_association: "
                   "transport layer failed to associate\n"));
      }
      return;
    }

    // Check if any publications have already sent a REQUEST_ACK message.
    // {
    //   ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);
    //
    //   WriterMapType::iterator where = this->writers_.find(writer.writerId);
    //
    //   if (where != this->writers_.end()) {
    //     const MonotonicTimePoint now = MonotonicTimePoint::now();
    //
    //     ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);
    //
    //     if (where->second->should_ack(now)) {
    //       const SequenceNumber sequence = where->second->ack_sequence();
    //       if (this->send_sample_ack(writer.writerId, sequence, now.to_dds_time())) {
    //         where->second->clear_acks(sequence);
    //       }
    //     }
    //   }
    // }

    //
    // LIVELINESS policy timers are managed here.
    //
    // if (liveliness_lease_duration_ != TimeDuration::zero) {
    //   // this call will start the timer if it is not already set
    //   const MonotonicTimePoint now = MonotonicTimePoint::now();
    //
    //   if (DCPS_debug_level >= 5) {
    //     GuidConverter converter(subscription_id_);
    //     ACE_DEBUG((LM_DEBUG,
    //                ACE_TEXT("(%P|%t) RecorderImpl::add_association: ")
    //                ACE_TEXT("starting/resetting liveliness timer for reader %C\n"),
    //                OPENDDS_STRING(converter).c_str()));
    //   }
    //
    //   this->handle_timeout(now, this);
    // }

    // else - no timer needed when LIVELINESS.lease_duration is INFINITE

  }
  //
  // We no longer hold the publication_handle_lock_.
  //

  //
  // We only do the following processing for readers that are *not*
  // readers of Builtin Topics.
  //
  if (!is_bit_) {

    const DDS::InstanceHandle_t handle = participant_servant_->assign_handle(writer.writerId);

    //
    // We acquire the publication_handle_lock_ for the remainder of our
    // processing.
    //
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

      // This insertion is idempotent.
      this->id_to_handle_map_.insert(
        RepoIdToHandleMap::value_type(writer.writerId, handle));

      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) RecorderImpl::add_association: ")
                   ACE_TEXT("id_to_handle_map_[ %C] = 0x%x.\n"),
                   LogGuid(writer.writerId).c_str(),
                   handle));
      }

      // We need to adjust these after the insertions have all completed
      // since insertions are not guaranteed to increase the number of
      // currently matched publications.
      int matchedPublications = static_cast<int>(this->id_to_handle_map_.size());
      this->subscription_match_status_.current_count_change
        = matchedPublications - this->subscription_match_status_.current_count;
      this->subscription_match_status_.current_count = matchedPublications;

      ++this->subscription_match_status_.total_count;
      ++this->subscription_match_status_.total_count_change;

      this->subscription_match_status_.last_publication_handle = handle;

      // set_status_changed_flag(DDS::SUBSCRIPTION_MATCHED_STATUS, true);


      if (listener_.in()) {
        listener_->on_recorder_matched(
          this,
          this->subscription_match_status_);

        // TBD - why does the spec say to change this but not change
        //       the ChangeFlagStatus after a listener call?

        // Client will look at it so next time it looks the change should be 0
        this->subscription_match_status_.total_count_change = 0;
        this->subscription_match_status_.current_count_change = 0;
      }

      // notify_status_condition();
    }

    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);
      ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, writers_lock_);

      if (!writers_.count(writer.writerId)) {
        return;
      }

      this->writers_[writer.writerId]->handle(handle);
    }
  }

  // if (this->monitor_) {
  //   this->monitor_->report();
  // }
}

void
RecorderImpl::remove_associations(const WriterIdSeq& writers,
                                  bool               notify_lost)
{
  DBG_ENTRY_LVL("RecorderImpl", "remove_associations", 6);
  if (writers.length() == 0) {
    return;
  }

  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) RecorderImpl::remove_associations: ")
               ACE_TEXT("bit %d local %C remote %C num remotes %d\n"),
               is_bit_,
               LogGuid(subscription_id_).c_str(),
               LogGuid(writers[0]).c_str(),
               writers.length()));
  }
  if (!get_deleted()) {
    // stop pending associations for these writer ids
    this->stop_associating(writers.get_buffer(), writers.length());
  }

  remove_associations_i(writers, notify_lost);
}

void
RecorderImpl::remove_associations_i(const WriterIdSeq& writers,
    bool notify_lost)
{
  DBG_ENTRY_LVL("RecorderImpl", "remove_associations_i", 6);

  if (writers.length() == 0) {
    return;
  }

  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) RecorderImpl::remove_associations_i: ")
               ACE_TEXT("bit %d local %C remote %C num remotes %d\n"),
               is_bit_,
               LogGuid(subscription_id_).c_str(),
               LogGuid(writers[0]).c_str(),
               writers.length()));
  }
  DDS::InstanceHandleSeq handles;

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

  // This is used to hold the list of writers which were actually
  // removed, which is a proper subset of the writers which were
  // requested to be removed.
  WriterIdSeq updated_writers;

  CORBA::ULong wr_len;

  //Remove the writers from writer list. If the supplied writer
  //is not in the cached writers list then it is already removed.
  //We just need remove the writers in the list that have not been
  //removed.
  {
    ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, this->writers_lock_);

    wr_len = writers.length();

    for (CORBA::ULong i = 0; i < wr_len; i++) {
      PublicationId writer_id = writers[i];

#ifndef OPENDDS_SAFETY_PROFILE
      if (dt_map_.erase(writer_id) == 0) {
        if (DCPS_debug_level >= 4) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) RecorderImpl::remove_associations_i: -"
            "failed to find writer_id in the DynamicTypeByPubId map.\n"));
        }
      }
#endif

      WriterMapType::iterator it = this->writers_.find(writer_id);
      if (it != this->writers_.end()) {
        it->second->removed();
      }

      if (this->writers_.erase(writer_id) == 0) {
        if (DCPS_debug_level >= 4) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) RecorderImpl::remove_associations_i: ")
                     ACE_TEXT("the writer local %C was already removed.\n"),
                     LogGuid(writer_id).c_str()));
        }

      } else {
        push_back(updated_writers, writer_id);
      }
    }
  }

  wr_len = updated_writers.length();

  // Return now if the supplied writers have been removed already.
  if (wr_len == 0) {
    return;
  }

  if (!is_bit_) {
    // The writer should be in the id_to_handle map at this time.  Note
    // it if it not there.
    this->lookup_instance_handles(updated_writers, handles);

    for (CORBA::ULong i = 0; i < wr_len; ++i) {
      id_to_handle_map_.erase(updated_writers[i]);
    }
  }
  for (CORBA::ULong i = 0; i < updated_writers.length(); ++i) {
    this->disassociate(updated_writers[i]);
  }

  // Mirror the add_associations SUBSCRIPTION_MATCHED_STATUS processing.
  if (!this->is_bit_) {
    // Derive the change in the number of publications writing to this reader.
    int matchedPublications = static_cast<int>(this->id_to_handle_map_.size());
    this->subscription_match_status_.current_count_change
      = matchedPublications - this->subscription_match_status_.current_count;

    // Only process status if the number of publications has changed.
    if (this->subscription_match_status_.current_count_change != 0) {
      this->subscription_match_status_.current_count = matchedPublications;
      /// Section 7.1.4.1: total_count will not decrement.

      /// @TODO: Reconcile this with the verbiage in section 7.1.4.1
      this->subscription_match_status_.last_publication_handle
        = handles[ wr_len - 1];

      // set_status_changed_flag(DDS::SUBSCRIPTION_MATCHED_STATUS, true);

      // DDS::DataReaderListener_var listener
      // = listener_for(DDS::SUBSCRIPTION_MATCHED_STATUS);

      if (listener_.in()) {
        listener_->on_recorder_matched(
          this,
          this->subscription_match_status_);

        // Client will look at it so next time it looks the change should be 0
        this->subscription_match_status_.total_count_change = 0;
        this->subscription_match_status_.current_count_change = 0;
      }

      // notify_status_condition();
    }
  }

  // If this remove_association is invoked when the InfoRepo
  // detects a lost writer then make a callback to notify
  // subscription lost.
  if (notify_lost) {
    this->notify_subscription_lost(handles);
  }

  // if (this->monitor_) {
  //   this->monitor_->report();
  // }

  for (unsigned int i = 0; i < handles.length(); ++i) {
    participant_servant_->return_handle(handles[i]);
  }
}

void
RecorderImpl::remove_all_associations()
{
  DBG_ENTRY_LVL("RecorderImpl","remove_all_associations",6);

  OpenDDS::DCPS::WriterIdSeq writers;
  int size;

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->publication_handle_lock_);

  {
    ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);

    size = static_cast<int>(writers_.size());
    writers.length(size);

    WriterMapType::iterator curr_writer = writers_.begin();
    WriterMapType::iterator end_writer = writers_.end();

    int i = 0;

    while (curr_writer != end_writer) {
      writers[i++] = curr_writer->first;
      ++curr_writer;
    }
  }

  try {
    CORBA::Boolean dont_notify_lost = false;

    if (0 < size) {
      remove_associations(writers, dont_notify_lost);
    }

  } catch (const CORBA::Exception&) {
  }

  transport_stop();
}

void
RecorderImpl::update_incompatible_qos(const IncompatibleQosStatus& status)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex,
            guard,
            this->publication_handle_lock_);

  if (this->requested_incompatible_qos_status_.total_count == status.total_count) {
    // This test should make the method idempotent.
    return;
  }

  // set_status_changed_flag(DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS,
  //                         true);

  // copy status and increment change
  requested_incompatible_qos_status_.total_count = status.total_count;
  requested_incompatible_qos_status_.total_count_change +=
    status.count_since_last_send;
  requested_incompatible_qos_status_.last_policy_id =
    status.last_policy_id;
  requested_incompatible_qos_status_.policies = status.policies;

  // if (!CORBA::is_nil(listener.in())) {
  //   listener->on_requested_incompatible_qos(this,
  //                                           requested_incompatible_qos_status_);
  //
  //   // TBD - why does the spec say to change total_count_change but not
  //   // change the ChangeFlagStatus after a listener call?
  //
  //   // client just looked at it so next time it looks the
  //   // change should be 0
  //   requested_incompatible_qos_status_.total_count_change = 0;
  // }
  //
  // notify_status_condition();
}

void
RecorderImpl::signal_liveliness(const RepoId& remote_participant)
{
  RepoId prefix = remote_participant;
  prefix.entityId = EntityId_t();

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_);

  typedef std::pair<RepoId, RcHandle<WriterInfo> > WriterSetElement;
  typedef OPENDDS_VECTOR(WriterSetElement) WriterSet;
  WriterSet writers;

  {
    ACE_READ_GUARD(ACE_RW_Thread_Mutex, read_guard, this->writers_lock_);
    for (WriterMapType::iterator pos = writers_.lower_bound(prefix),
           limit = writers_.end();
         pos != limit && equal_guid_prefixes(pos->first, prefix);
         ++pos) {
      writers.push_back(std::make_pair(pos->first, pos->second));
    }
  }

  const MonotonicTimePoint now = MonotonicTimePoint::now();
  for (WriterSet::iterator pos = writers.begin(), limit = writers.end();
       pos != limit;
       ++pos) {
    pos->second->received_activity(now);
  }
}

DDS::ReturnCode_t RecorderImpl::set_qos(
  const DDS::SubscriberQos & subscriber_qos,
  const DDS::DataReaderQos & qos)
{
  OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(subscriber_qos, DDS::RETCODE_UNSUPPORTED);

  if (Qos_Helper::valid(subscriber_qos) && Qos_Helper::consistent(subscriber_qos)) {
    if (subqos_ != subscriber_qos) {
      // for the not changeable qos, it can be changed before enable
      if (!Qos_Helper::changeable(subqos_, subscriber_qos) && enabled_ == true) {
        return DDS::RETCODE_IMMUTABLE_POLICY;

      } else {
        subqos_ = subscriber_qos;
      }
    }
  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }

  OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);

  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (qos_ == qos)
      return DDS::RETCODE_OK;

    if (!Qos_Helper::changeable(qos_, qos) && this->is_enabled()) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id_);
      const bool status =
        disco->update_subscription_qos(
          this->participant_servant_->get_domain_id(),
          this->participant_servant_->get_id(),
          this->subscription_id_,
          qos,
          subscriber_qos);
      if (!status) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: RecorderImpl::set_qos: qos not updated\n"));
        }
        return DDS::RETCODE_ERROR;
      }
    }

    qos_ = qos;
    subqos_ = subscriber_qos;

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
RecorderImpl::get_qos(
  DDS::SubscriberQos & subscriber_qos,
  DDS::DataReaderQos & qos)
{
  qos = passed_qos_;
  subscriber_qos = subqos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
RecorderImpl::set_listener(const RecorderListener_rch& a_listener,
                           DDS::StatusMask             mask)
{
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = a_listener;
  return DDS::RETCODE_OK;
}

RecorderListener_rch
RecorderImpl::get_listener()
{
  return listener_;
}

void
RecorderImpl::lookup_instance_handles(const WriterIdSeq&       ids,
                                      DDS::InstanceHandleSeq & hdls)
{
  CORBA::ULong const num_wrts = ids.length();

  if (DCPS_debug_level > 9) {
    OPENDDS_STRING separator = "";
    OPENDDS_STRING buffer;

    for (CORBA::ULong i = 0; i < num_wrts; ++i) {
      buffer += separator + LogGuid(ids[i]).conv_;
      separator = ", ";
    }

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) RecorderImpl::lookup_instance_handles: ")
               ACE_TEXT("searching for handles for writer Ids: %C.\n"),
               buffer.c_str()));
  }

  hdls.length(num_wrts);

  for (CORBA::ULong i = 0; i < num_wrts; ++i) {
    hdls[i] = participant_servant_->lookup_handle(ids[i]);
  }
}

DDS::ReturnCode_t
RecorderImpl::enable()
{
  if (DCPS_debug_level >= 2) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) RecorderImpl::enable\n")));
  }
  //According spec:
  // - Calling enable on an already enabled Entity returns OK and has no
  // effect.
  // - Calling enable on an Entity whose factory is not enabled will fail
  // and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

  this->set_enabled();

  // if (topic_servant_ && !transport_disabled_) {
  if (topic_servant_) {
    if (DCPS_debug_level >= 2) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) RecorderImpl::enable: enable_transport\n"));
    }

    try {
      this->enable_transport(this->qos_.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS,
                             this->qos_.durability.kind > DDS::VOLATILE_DURABILITY_QOS);
    } catch (const Transport::Exception&) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: RecorderImpl::enable: Transport Exception\n"));
      }
      return DDS::RETCODE_ERROR;
    }

    const TransportLocatorSeq& trans_conf_info = this->connection_info();

    CORBA::String_var filterClassName = "";
    CORBA::String_var filterExpression = "";
    DDS::StringSeq exprParams;

    Discovery_rch disco =
      TheServiceParticipant->get_discovery(this->domain_id_);

    DCPS::set_reader_effective_data_rep_qos(this->qos_.representation.value);
    if (!topic_servant_->check_data_representation(this->qos_.representation.value, false)) {
      return DDS::RETCODE_ERROR;
    }

    if (DCPS_debug_level >= 2) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) RecorderImpl::enable: add_subscription\n"));
    }

    XTypes::TypeInformation type_info;

    this->subscription_id_ =
      disco->add_subscription(this->domain_id_,
                              this->participant_servant_->get_id(),
                              this->topic_servant_->get_id(),
                              rchandle_from(this),
                              this->qos_,
                              trans_conf_info,
                              this->subqos_,
                              filterClassName,
                              filterExpression,
                              exprParams,
                              type_info);

    if (this->subscription_id_ == OpenDDS::DCPS::GUID_UNKNOWN) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: RecorderImpl::enable: "
          "add_subscription returned invalid id\n"));
      }
      return DDS::RETCODE_ERROR;
    }
  }

  return DDS::RETCODE_OK;
}

DDS::InstanceHandle_t
RecorderImpl::get_instance_handle()
{
  return get_entity_instance_handle(subscription_id_, participant_servant_);
}

void
RecorderImpl::register_for_writer(const RepoId& participant,
                                  const RepoId& readerid,
                                  const RepoId& writerid,
                                  const TransportLocatorSeq& locators,
                                  DiscoveryListener* listener)
{
  TransportClient::register_for_writer(participant, readerid, writerid, locators, listener);
}

void
RecorderImpl::unregister_for_writer(const RepoId& participant,
                                    const RepoId& readerid,
                                    const RepoId& writerid)
{
  TransportClient::unregister_for_writer(participant, readerid, writerid);
}

#if !defined (DDS_HAS_MINIMUM_BIT)
DDS::ReturnCode_t
RecorderImpl::repoid_to_bit_key(const GUID_t& id,
                                DDS::BuiltinTopicKey_t& key)
{
  const DDS::InstanceHandle_t publication_handle = participant_servant_->lookup_handle(id);

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   guard,
                   this->publication_handle_lock_,
                   DDS::RETCODE_ERROR);

  DDS::PublicationBuiltinTopicDataSeq data;

  DDS::ReturnCode_t const ret = instance_handle_to_bit_data<DDS::PublicationBuiltinTopicDataDataReader_var>(
                            participant_servant_,
                            BUILT_IN_PUBLICATION_TOPIC,
                            publication_handle,
                            data);

  if (ret == DDS::RETCODE_OK) {
    key = data[0].key;
  }

  return ret;
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#ifndef OPENDDS_SAFETY_PROFILE
DDS::DynamicData_ptr RecorderImpl::get_dynamic_data(const RawDataSample& sample)
{
  const Encoding enc(sample.encoding_kind_, sample.header_.byte_order_ ? ENDIAN_LITTLE : ENDIAN_BIG);
  const DynamicTypeByPubId::const_iterator dt_found = dt_map_.find(sample.publication_id_);
  if (dt_found == dt_map_.end()) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RecorderImpl::get_dynamic_data: "
        "failed to find GUID: %C in DynamicTypeByPubId.\n", LogGuid(sample.publication_id_).c_str()));
    }
    return 0;
  }

  DDS::DynamicType_var dt = dt_found->second;
  XTypes::DynamicDataImpl* dd = new XTypes::DynamicDataImpl(sample.sample_.get(), enc, dt);
  DDS::DynamicData_var dd_var = dd;
  if (!dd->check_xcdr1_mutable(dt)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: RecorderImpl::get_dynamic_data: "
        "Encountered unsupported combination of XCDR1 encoding and mutable extensibility.\n"));
    }
    return 0;
  }
  return dd_var._retn();
}
#endif

} // namespace DCPS
} // namespace

OPENDDS_END_VERSIONED_NAMESPACE_DECL
