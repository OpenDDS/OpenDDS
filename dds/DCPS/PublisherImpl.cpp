/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "PublisherImpl.h"

#include "FeatureDisabledQosCheck.h"
#include "DataWriterImpl.h"
#include "DomainParticipantImpl.h"
#include "DataWriterImpl.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "GuidConverter.h"
#include "Marked_Default_Qos.h"
#include "TopicImpl.h"
#include "MonitorFactory.h"
#include "transport/framework/ReceivedDataSample.h"
#include "transport/framework/DataLinkSet.h"
#include "transport/framework/TransportImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

PublisherImpl::PublisherImpl(DDS::InstanceHandle_t      handle,
    GUID_t                     id,
    const DDS::PublisherQos&   qos,
    DDS::PublisherListener_ptr a_listener,
    const DDS::StatusMask&     mask,
    DomainParticipantImpl*     participant)
: handle_(handle),
  qos_(qos),
  default_datawriter_qos_(TheServiceParticipant->initial_DataWriterQos()),
  listener_mask_(mask),
  listener_(DDS::PublisherListener::_duplicate(a_listener)),
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  change_depth_(0),
#endif
  domain_id_(participant->get_domain_id()),
  participant_(*participant),
  suspend_depth_count_(0),
  sequence_number_(),
  reverse_pi_lock_(pi_lock_),
  publisher_id_(id)
{
  monitor_.reset(TheServiceParticipant->monitor_factory_->create_publisher_monitor(this));
}

PublisherImpl::~PublisherImpl()
{
  const RcHandle<DomainParticipantImpl> participant = participant_.lock();
  if (participant) {
    participant->return_handle(handle_);
  }

  // The datawriters should be deleted already before calling delete
  // publisher.
  String leftover_entities;
  if (!is_clean(&leftover_entities)) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: PublisherImpl::~PublisherImpl: "
                 "%C still exist\n", leftover_entities.c_str()));
    }
  }
}

DDS::InstanceHandle_t
PublisherImpl::get_instance_handle()
{
  return handle_;
}

bool
PublisherImpl::contains_writer(DDS::InstanceHandle_t a_handle)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      guard,
      this->pi_lock_,
      DDS::RETCODE_ERROR);

  for (DataWriterMap::iterator it(datawriter_map_.begin());
      it != datawriter_map_.end(); ++it) {
    if (a_handle == it->second->get_instance_handle()) {
      return true;
    }
  }

  return false;
}

DDS::DataWriter_ptr
PublisherImpl::create_datawriter(
    DDS::Topic_ptr              a_topic,
    const DDS::DataWriterQos &  qos,
    DDS::DataWriterListener_ptr a_listener,
    DDS::StatusMask             mask)
{
  if (!a_topic) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
                 "(%P|%t) NOTICE: PublisherImpl::create_datawriter: "
                 "topic is nil\n"));
    }
    return 0;
  }

  DDS::DomainParticipant_var my_participant = get_participant();
  DDS::DomainParticipant_var topic_participant = a_topic->get_participant();

  if (my_participant != topic_participant) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
                 "(%P|%t) NOTICE: PublisherImpl::create_datawriter: "
                 "topic does not belong to same participant\n"));
    }
    return 0;
  }

  DDS::DataWriterQos dw_qos;

  if (!validate_datawriter_qos(qos, default_datawriter_qos_, a_topic, dw_qos)) {
    return DDS::DataWriter::_nil();
  }

  TopicImpl* topic_servant = dynamic_cast<TopicImpl*>(a_topic);

  if (!topic_servant) {
    if (DCPS_debug_level > 0) {
      CORBA::String_var name = a_topic->get_name();
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("PublisherImpl::create_datawriter, ")
        ACE_TEXT("topic_servant(topic_name=%C) is nil.\n"),
        name.in()));
    }
    return 0;
  }

  OpenDDS::DCPS::TypeSupport_ptr typesupport =
      topic_servant->get_type_support();

  if (typesupport == 0) {
    if (DCPS_debug_level > 0) {
      CORBA::String_var name = topic_servant->get_name();
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("PublisherImpl::create_datawriter, ")
          ACE_TEXT("typesupport(topic_name=%C) is nil.\n"),
          name.in()));
    }
    return DDS::DataWriter::_nil();
  }

  DDS::DataWriter_var dw_obj = typesupport->create_datawriter();

  DataWriterImpl* dw_servant =
      dynamic_cast <DataWriterImpl*>(dw_obj.in());

  if (dw_servant == 0) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("PublisherImpl::create_datawriter, ")
          ACE_TEXT("servant is nil.\n")));
    }
    return DDS::DataWriter::_nil();
  }

  dw_servant->init(
      topic_servant,
      dw_qos,
      a_listener,
      mask,
      participant_,
      this);

  if (enabled_ && qos_.entity_factory.autoenable_created_entities) {
    const DDS::ReturnCode_t ret = dw_servant->enable();

    if (ret != DDS::RETCODE_OK) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_WARNING,
            ACE_TEXT("(%P|%t) WARNING: ")
            ACE_TEXT("PublisherImpl::create_datawriter, ")
            ACE_TEXT("enable failed.\n")));
      }
      return DDS::DataWriter::_nil();
    }
  } else {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, pi_lock_, 0);
    writers_not_enabled_.insert(rchandle_from(dw_servant));
  }

  return DDS::DataWriter::_duplicate(dw_obj.in());
}

DDS::ReturnCode_t
PublisherImpl::delete_datawriter(DDS::DataWriter_ptr a_datawriter)
{
  DataWriterImpl* dw_servant = dynamic_cast<DataWriterImpl*>(a_datawriter);
  if (!dw_servant) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                "(%P|%t) PublisherImpl::delete_datawriter - dynamic cast to DataWriterImpl failed\n"));
    }
    return DDS::RETCODE_ERROR;
  }

  {
    DDS::Publisher_var dw_publisher(dw_servant->get_publisher());

    if (dw_publisher.in() != this) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) PublisherImpl::delete_datawriter: ")
            ACE_TEXT("the data writer %C doesn't ")
            ACE_TEXT("belong to this subscriber\n"),
            LogGuid(dw_servant->get_guid()).c_str()));
      }
      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }
  }

  if (!dw_servant->get_deleted()) {
    dw_servant->prepare_to_delete();
    dw_servant->set_wait_pending_deadline(TheServiceParticipant->new_pending_timeout_deadline());
  }

  // Wait for any data and control messages to be transported during
  // unregistering of instances.
  dw_servant->wait_pending();

  GUID_t publication_id  = GUID_UNKNOWN;
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
        guard,
        this->pi_lock_,
        DDS::RETCODE_ERROR);

    publication_id = dw_servant->get_guid();

    PublicationMap::iterator it = publication_map_.find(publication_id);

    if (it == publication_map_.end()) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: ")
            ACE_TEXT("PublisherImpl::delete_datawriter, ")
            ACE_TEXT("datawriter %C not found.\n"),
            LogGuid(publication_id).c_str()));
      }
      return DDS::RETCODE_ERROR;
    }

    // We can not erase the datawriter from datawriter map by the topic name
    // because the map might have multiple datawriters with the same topic
    // name.
    // Find the iterator to the datawriter in the datawriter map and erase
    // by the iterator.
    DataWriterMap::iterator writ;
    DataWriterMap::iterator the_writ = datawriter_map_.end();

    for (writ = datawriter_map_.begin();
        writ != datawriter_map_.end();
        ++writ) {
      if (writ->second == it->second) {
        the_writ = writ;
        break;
      }
    }

    if (the_writ != datawriter_map_.end()) {
      datawriter_map_.erase(the_writ);
    }

    publication_map_.erase(it);

    // not just unregister but remove any pending writes/sends.
    dw_servant->unregister_all();

    // Release pi_lock_ before making call to transport layer to avoid
    // some deadlock situations that threads acquire locks(PublisherImpl
    // pi_lock_, TransportClient reservation_lock and TransportImpl
    // lock_) in reverse order.
    ACE_GUARD_RETURN(reverse_lock_type, reverse_monitor, this->reverse_pi_lock_,
        DDS::RETCODE_ERROR);
    // Wait for pending samples to drain prior to removing associations
    // and unregistering the publication.
    dw_servant->wait_pending();

    // Call remove association before unregistering the datawriter
    // with the transport, otherwise some callbacks resulted from
    // remove_association may lost.
    dw_servant->remove_all_associations();
    dw_servant->cleanup();
  }

  if (this->monitor_) {
    this->monitor_->report();
  }

  RcHandle<DomainParticipantImpl> participant = this->participant_.lock();

  Discovery_rch disco = TheServiceParticipant->get_discovery(this->domain_id_);
  if (!disco->remove_publication(
      this->domain_id_,
      participant->get_id(),
      publication_id)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("PublisherImpl::delete_datawriter, ")
          ACE_TEXT("publication not removed from discovery.\n")));
    }
    return DDS::RETCODE_ERROR;
  }

  participant->remove_adjust_liveliness_timers();

  return DDS::RETCODE_OK;
}

DDS::DataWriter_ptr
PublisherImpl::lookup_datawriter(const char* topic_name)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      guard,
      this->pi_lock_,
      DDS::DataWriter::_nil());

  // If multiple entries whose key is "topic_name" then which one is
  // returned ? Spec does not limit which one should give.
  DataWriterMap::iterator it = datawriter_map_.find(topic_name);

  if (it == datawriter_map_.end()) {
    if (DCPS_debug_level >= 2) {
      ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) ")
          ACE_TEXT("PublisherImpl::lookup_datawriter, ")
          ACE_TEXT("The datawriter(topic_name=%C) is not found\n"),
          topic_name));
    }

    return DDS::DataWriter::_nil();

  } else {
    return DDS::DataWriter::_duplicate(it->second.in());
  }
}

bool PublisherImpl::prepare_to_delete_datawriters()
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, pi_lock_, false);
  bool result = true;
  const DataWriterMap::iterator end = datawriter_map_.end();
  for (DataWriterMap::iterator i = datawriter_map_.begin(); i != end; ++i) {
    DataWriterImpl* const writer = dynamic_cast<DataWriterImpl*>(i->second.in());
    if (writer) {
      if (!writer->get_deleted()) {
        writer->prepare_to_delete();
      }
    } else {
      result = false;
    }
  }

  return result;
}

bool PublisherImpl::set_wait_pending_deadline(const MonotonicTimePoint& deadline)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, pi_lock_, false);
  bool result = true;
  const DataWriterMap::iterator end = datawriter_map_.end();
  for (DataWriterMap::iterator i = datawriter_map_.begin(); i != end; ++i) {
    DataWriterImpl* const writer = dynamic_cast<DataWriterImpl*>(i->second.in());
    if (writer) {
      writer->set_wait_pending_deadline(deadline);
    } else {
      result = false;
    }
  }
  return result;
}

DDS::ReturnCode_t PublisherImpl::delete_contained_entities()
{
  // If the call isn't part of another delete, prepare the datawriters to be
  // deleted and set the pending deadline on all the writers.
  if (!get_deleted()) {
    // mark that the entity is being deleted
    set_deleted(true);

    if (!prepare_to_delete_datawriters()) {
      return DDS::RETCODE_ERROR;
    }
    if (!set_wait_pending_deadline(TheServiceParticipant->new_pending_timeout_deadline())) {
      return DDS::RETCODE_ERROR;
    }
  }

  while (true) {
    GUID_t pub_id = GUID_UNKNOWN;
    DataWriterImpl_rch a_datawriter;

    {
      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
          guard,
          this->pi_lock_,
          DDS::RETCODE_ERROR);

      if (datawriter_map_.empty()) {
        break;
      } else {
        a_datawriter = datawriter_map_.begin()->second;
        pub_id = a_datawriter->get_guid();
      }
    }

    const DDS::ReturnCode_t ret = delete_datawriter(a_datawriter.in());

    if (ret != DDS::RETCODE_OK) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: ")
            ACE_TEXT("PublisherImpl::")
            ACE_TEXT("delete_contained_entities: ")
            ACE_TEXT("failed to delete ")
            ACE_TEXT("datawriter %C.\n"),
            LogGuid(pub_id).c_str()));
      }
      return ret;
    }
  }

  // the publisher can now start creating new publications
  set_deleted(false);

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::set_qos(const DDS::PublisherQos & qos)
{

  OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);

  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (qos_ == qos)
      return DDS::RETCODE_OK;

    // for the not changeable qos, it can be changed before enable
    if (!Qos_Helper::changeable(qos_, qos) && enabled_) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      qos_ = qos;

      DwIdToQosMap idToQosMap;
      {
        ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
            guard,
            this->pi_lock_,
            DDS::RETCODE_ERROR);

        for (PublicationMap::iterator iter = publication_map_.begin();
            iter != publication_map_.end();
            ++iter) {
          DDS::DataWriterQos qos = iter->second->qos_;
          GUID_t id = iter->second->get_guid();
          std::pair<DwIdToQosMap::iterator, bool> pair =
              idToQosMap.insert(DwIdToQosMap::value_type(id, qos));

          if (!pair.second) {
            if (DCPS_debug_level > 0) {
              ACE_ERROR((LM_ERROR,
                  ACE_TEXT("(%P|%t) ")
                  ACE_TEXT("PublisherImpl::set_qos: ")
                  ACE_TEXT("insert id %C to DwIdToQosMap ")
                  ACE_TEXT("failed.\n"),
                  LogGuid(id).c_str()));
            }
            return DDS::RETCODE_ERROR;
          }
        }
      }

      DwIdToQosMap::iterator iter = idToQosMap.begin();

      while (iter != idToQosMap.end()) {
        Discovery_rch disco = TheServiceParticipant->get_discovery(this->domain_id_);
        bool status = false;

        RcHandle<DomainParticipantImpl> participant = this->participant_.lock();
        if (participant)
          status = disco->update_publication_qos(
              participant->get_domain_id(),
              participant->get_id(),
              iter->first,
              iter->second,
              this->qos_);

        if (!status) {
          if (DCPS_debug_level > 0) {
            ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) PublisherImpl::set_qos, ")
                ACE_TEXT("failed.\n")));
          }
          return DDS::RETCODE_ERROR;
        }

        ++iter;
      }
    }

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
PublisherImpl::get_qos(DDS::PublisherQos & qos)
{
  qos = qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::set_listener(DDS::PublisherListener_ptr a_listener,
    DDS::StatusMask            mask)
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = DDS::PublisherListener::_duplicate(a_listener);
  return DDS::RETCODE_OK;
}

DDS::PublisherListener_ptr
PublisherImpl::get_listener()
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  return DDS::PublisherListener::_duplicate(listener_.in());
}

DDS::ReturnCode_t
PublisherImpl::suspend_publications()
{
  if (!enabled_) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("PublisherImpl::suspend_publications, ")
          ACE_TEXT(" Entity is not enabled.\n")));
    }
    return DDS::RETCODE_NOT_ENABLED;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      suspend_guard,
      this->pi_suspended_lock_,
      DDS::RETCODE_ERROR);
  ++suspend_depth_count_;
  return DDS::RETCODE_OK;
}

bool
PublisherImpl::is_suspended() const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      suspend_guard,
      this->pi_suspended_lock_,
      false);
  return suspend_depth_count_;
}

DDS::ReturnCode_t
PublisherImpl::resume_publications()
{
  if (!enabled_) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("PublisherImpl::resume_publications, ")
          ACE_TEXT(" Entity is not enabled.\n")));
    }
    return DDS::RETCODE_NOT_ENABLED;
  }

  PublicationMap publication_map_copy;
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
        suspend_guard,
        this->pi_suspended_lock_,
        DDS::RETCODE_ERROR);
    --suspend_depth_count_;

    if (suspend_depth_count_ < 0) {
      suspend_depth_count_ = 0;
      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }
    if (suspend_depth_count_ == 0) {
      suspend_guard.release();
      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
          guard,
          this->pi_lock_,
          DDS::RETCODE_ERROR);

      publication_map_copy = publication_map_;
    }
  }

  for (PublicationMap::const_iterator it = publication_map_copy.begin();
      it != publication_map_copy.end(); ++it) {
    it->second->send_suspended_data();
  }

  return DDS::RETCODE_OK;
}

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE

DDS::ReturnCode_t
PublisherImpl::begin_coherent_changes()
{
  if (!enabled_) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: PublisherImpl::begin_coherent_changes:")
          ACE_TEXT(" Publisher is not enabled!\n")));
    }
    return DDS::RETCODE_NOT_ENABLED;
  }

  if (!qos_.presentation.coherent_access) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: PublisherImpl::begin_coherent_changes:")
          ACE_TEXT(" QoS policy does not support coherent access!\n")));
    }
    return DDS::RETCODE_ERROR;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      guard,
      this->pi_lock_,
      DDS::RETCODE_ERROR);

  ++this->change_depth_;

  if (qos_.presentation.access_scope == DDS::INSTANCE_PRESENTATION_QOS) {
    // INSTANCE access scope essentially behaves
    // as a no-op. (see: 7.1.3.6)
    return DDS::RETCODE_OK;
  }

  // We should only notify publications on the first
  // and last change to the current change set:
  if (this->change_depth_ == 1) {
    for (PublicationMap::iterator it = this->publication_map_.begin();
        it != this->publication_map_.end(); ++it) {
      it->second->begin_coherent_changes();
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::end_coherent_changes()
{
  if (!enabled_) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: PublisherImpl::end_coherent_changes:")
          ACE_TEXT(" Publisher is not enabled!\n")));
    }
    return DDS::RETCODE_NOT_ENABLED;
  }

  if (!qos_.presentation.coherent_access) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: PublisherImpl::end_coherent_changes:")
          ACE_TEXT(" QoS policy does not support coherent access!\n")));
    }
    return DDS::RETCODE_ERROR;
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      guard,
      this->pi_lock_,
      DDS::RETCODE_ERROR);

  if (this->change_depth_ == 0) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: PublisherImpl::end_coherent_changes:")
          ACE_TEXT(" No matching call to begin_coherent_changes!\n")));
    }
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  --this->change_depth_;

  if (qos_.presentation.access_scope == DDS::INSTANCE_PRESENTATION_QOS) {
    // INSTANCE access scope essentially behaves
    // as a no-op. (see: 7.1.3.6)
    return DDS::RETCODE_OK;
  }

  // We should only notify publications on the first
  // and last change to the current change set:
  if (this->change_depth_ == 0) {
    GroupCoherentSamples group_samples;
    for (PublicationMap::iterator it = this->publication_map_.begin();
        it != this->publication_map_.end(); ++it) {

      if (it->second->coherent_samples_ == 0) {
        continue;
      }

      std::pair<GroupCoherentSamples::iterator, bool> pair =
          group_samples.insert(GroupCoherentSamples::value_type(
              it->second->get_guid(),
              WriterCoherentSample(it->second->coherent_samples_,
                  it->second->sequence_number_)));

      if (!pair.second) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: PublisherImpl::end_coherent_changes: ")
              ACE_TEXT("failed to insert to GroupCoherentSamples.\n")));
        }
        return DDS::RETCODE_ERROR;
      }
    }

    for (PublicationMap::iterator it = this->publication_map_.begin();
        it != this->publication_map_.end(); ++it) {
      if (it->second->coherent_samples_ == 0) {
        continue;
      }

      it->second->end_coherent_changes(group_samples);
    }
  }

  return DDS::RETCODE_OK;
}

#endif // OPENDDS_NO_OBJECT_MODEL_PROFILE

DDS::ReturnCode_t
PublisherImpl::wait_for_acknowledgments(
    const DDS::Duration_t& max_wait)
{
  if (!enabled_) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: PublisherImpl::wait_for_acknowledgments, ")
          ACE_TEXT("Entity is not enabled.\n")));
    }
    return DDS::RETCODE_NOT_ENABLED;
  }

  typedef OPENDDS_MAP(DataWriterImpl*, DataWriterImpl::AckToken) DataWriterAckMap;
  DataWriterAckMap ack_writers;
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
        guard,
        this->pi_lock_,
        DDS::RETCODE_ERROR);

    // Collect writers to request acks
    for (DataWriterMap::iterator it(this->datawriter_map_.begin());
        it != this->datawriter_map_.end(); ++it) {
      DataWriterImpl_rch writer = it->second;
      if (writer->qos_.reliability.kind != DDS::RELIABLE_RELIABILITY_QOS)
        continue;
      if (writer->should_ack()) {
        DataWriterImpl::AckToken token = writer->create_ack_token(max_wait);

        std::pair<DataWriterAckMap::iterator, bool> pair =
            ack_writers.insert(DataWriterAckMap::value_type(writer.in(), token));

        if (!pair.second) {
          if (DCPS_debug_level > 0) {
            ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: PublisherImpl::wait_for_acknowledgments, ")
                ACE_TEXT("Unable to insert AckToken into DataWriterAckMap!\n")));
          }
          return DDS::RETCODE_ERROR;
        }
      }
    }
  }

  if (ack_writers.empty()) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) PublisherImpl::wait_for_acknowledgments() - ")
          ACE_TEXT("not blocking due to no writers requiring acks.\n")));
    }

    return DDS::RETCODE_OK;
  }

  // Wait for ack responses from all associated readers
  for (DataWriterAckMap::iterator it(ack_writers.begin());
      it != ack_writers.end(); ++it) {
    DataWriterImpl::AckToken token = it->second;

    it->first->wait_for_specific_ack(token);
  }

  return DDS::RETCODE_OK;
}

DDS::DomainParticipant_ptr
PublisherImpl::get_participant()
{
  return participant_.lock()._retn();
}

DDS::ReturnCode_t
PublisherImpl::set_default_datawriter_qos(const DDS::DataWriterQos & qos)
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    default_datawriter_qos_ = qos;
    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
PublisherImpl::get_default_datawriter_qos(DDS::DataWriterQos & qos)
{
  qos = default_datawriter_qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
PublisherImpl::copy_from_topic_qos(DDS::DataWriterQos &  a_datawriter_qos,
    const DDS::TopicQos & a_topic_qos)
{
  if (Qos_Helper::copy_from_topic_qos(a_datawriter_qos, a_topic_qos)) {
    return DDS::RETCODE_OK;
  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
PublisherImpl::enable()
{
  //According spec:
  // - Calling enable on an already enabled Entity returns OK and has no
  // effect.
  // - Calling enable on an Entity whose factory is not enabled will fail
  // and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

  RcHandle<DomainParticipantImpl> participant = this->participant_.lock();
  if (!participant || !participant->is_enabled()) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  if (this->monitor_) {
    this->monitor_->report();
  }

  this->set_enabled();

  if (qos_.entity_factory.autoenable_created_entities) {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, pi_lock_, DDS::RETCODE_ERROR);
    DataWriterSet writers;
    writers_not_enabled_.swap(writers);
    for (DataWriterSet::iterator it = writers.begin(); it != writers.end(); ++it) {
      (*it)->enable();
    }
  }

  return DDS::RETCODE_OK;
}

bool PublisherImpl::is_clean(String* leftover_entities) const
{
  if (leftover_entities) {
    leftover_entities->clear();
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, pi_lock_, false);

  const size_t writer_count = datawriter_map_.size();
  if (leftover_entities && writer_count) {
    *leftover_entities += to_dds_string(writer_count) + " writer(s)";
  }

  const size_t publication_count = publication_map_.size();
  if (leftover_entities && publication_count) {
    if (leftover_entities->size()) {
      *leftover_entities += ", ";
    }
    *leftover_entities += to_dds_string(publication_count) + " publication(s)";
  }

  return writer_count == 0 && publication_count == 0;
}

DDS::ReturnCode_t
PublisherImpl::writer_enabled(const char*     topic_name,
    DataWriterImpl* writer_ptr)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      guard,
      this->pi_lock_,
      DDS::RETCODE_ERROR);
  DataWriterImpl_rch writer = rchandle_from(writer_ptr);
  writers_not_enabled_.erase(writer);

  datawriter_map_.insert(DataWriterMap::value_type(topic_name, writer));

  const GUID_t publication_id = writer->get_guid();

  std::pair<PublicationMap::iterator, bool> pair =
      publication_map_.insert(PublicationMap::value_type(publication_id, writer));

  if (!pair.second) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("PublisherImpl::writer_enabled: ")
          ACE_TEXT("insert publication %C failed.\n"),
          LogGuid(publication_id).c_str()));
    }
    return DDS::RETCODE_ERROR;
  }

  if (this->monitor_) {
    this->monitor_->report();
  }

  return DDS::RETCODE_OK;
}


DDS::PublisherListener_ptr
PublisherImpl::listener_for(DDS::StatusKind kind)
{
  // per 2.1.4.3.1 Listener Access to Plain Communication Status
  // use this entities factory if listener is mask not enabled
  // for this kind.
  RcHandle<DomainParticipantImpl> participant = this->participant_.lock();

  if (!participant)
    return 0;

  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  if (CORBA::is_nil(listener_.in()) || (listener_mask_ & kind) == 0) {
    g.release();
    return participant->listener_for(kind);

  } else {
    return DDS::PublisherListener::_duplicate(listener_.in());
  }
}

DDS::ReturnCode_t
PublisherImpl::assert_liveliness_by_participant()
{
  DDS::ReturnCode_t ret = DDS::RETCODE_OK;

  for (DataWriterMap::iterator it(datawriter_map_.begin());
      it != datawriter_map_.end(); ++it) {
    const DDS::ReturnCode_t dw_ret = it->second->assert_liveliness_by_participant();

    if (dw_ret != DDS::RETCODE_OK) {
      ret = dw_ret;
    }
  }

  return ret;
}

TimeDuration
PublisherImpl::liveliness_check_interval(DDS::LivelinessQosPolicyKind kind)
{
  TimeDuration tv = TimeDuration::max_value;
  for (DataWriterMap::iterator it(datawriter_map_.begin());
      it != datawriter_map_.end(); ++it) {
    tv = std::min(tv, it->second->liveliness_check_interval(kind));
  }
  return tv;
}

bool
PublisherImpl::participant_liveliness_activity_after(const MonotonicTimePoint& tv)
{
  for (DataWriterMap::iterator it(datawriter_map_.begin());
      it != datawriter_map_.end(); ++it) {
    if (it->second->participant_liveliness_activity_after(tv)) {
      return true;
    }
  }
  return false;
}

void
PublisherImpl::get_publication_ids(PublicationIdVec& pubs)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
      guard,
      this->pi_lock_,
  );

  pubs.reserve(publication_map_.size());
  for (PublicationMap::iterator iter = publication_map_.begin();
      iter != publication_map_.end();
      ++iter) {
    pubs.push_back(iter->first);
  }
}

RcHandle<EntityImpl>
PublisherImpl::parent() const
{
  return this->participant_.lock();
}

bool
PublisherImpl::validate_datawriter_qos(const DDS::DataWriterQos& qos,
    const DDS::DataWriterQos& default_qos,
    DDS::Topic_ptr            a_topic,
    DDS::DataWriterQos&       dw_qos)
{
  if (CORBA::is_nil(a_topic)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("PublisherImpl::create_datawriter, ")
          ACE_TEXT("topic is nil.\n")));
    }
    return DDS::DataWriter::_nil();
  }

  if (qos == DATAWRITER_QOS_DEFAULT) {
    dw_qos = default_qos;

  } else if (qos == DATAWRITER_QOS_USE_TOPIC_QOS) {
    DDS::TopicQos topic_qos;
    a_topic->get_qos(topic_qos);
    dw_qos = default_qos;

    Qos_Helper::copy_from_topic_qos(dw_qos, topic_qos);

  } else {
    dw_qos = qos;
  }

  OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(dw_qos, DDS::DataWriter::_nil());
  OPENDDS_NO_OWNERSHIP_STRENGTH_COMPATIBILITY_CHECK(dw_qos, DDS::DataWriter::_nil());
  OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(dw_qos, DDS::DataWriter::_nil());
  OPENDDS_NO_DURABILITY_SERVICE_COMPATIBILITY_CHECK(dw_qos, DDS::DataWriter::_nil());
  OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT_COMPATIBILITY_CHECK(dw_qos, DDS::DataWriter::_nil());

  if (!Qos_Helper::valid(dw_qos)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("PublisherImpl::create_datawriter, ")
          ACE_TEXT("invalid qos.\n")));
    }
    return DDS::DataWriter::_nil();
  }

  if (!Qos_Helper::consistent(dw_qos)) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("PublisherImpl::create_datawriter, ")
          ACE_TEXT("inconsistent qos.\n")));
    }
    return DDS::DataWriter::_nil();
  }
  return true;
}

} // namespace DCPS
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL
