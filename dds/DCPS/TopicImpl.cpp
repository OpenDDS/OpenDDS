/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TopicImpl.h"

#include "Qos_Helper.h"
#include "FeatureDisabledQosCheck.h"
#include "Definitions.h"
#include "Service_Participant.h"
#include "DomainParticipantImpl.h"
#include "MonitorFactory.h"
#include "DCPS_Utils.h"
#include "transport/framework/TransportExceptions.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TopicImpl::TopicImpl(const char*                    topic_name,
                     const char*                    type_name,
                     OpenDDS::DCPS::TypeSupport_ptr type_support,
                     const DDS::TopicQos &          qos,
                     DDS::TopicListener_ptr         a_listener,
                     const DDS::StatusMask &        mask,
                     DomainParticipantImpl*         participant)
  : TopicDescriptionImpl(topic_name,
                         type_name,
                         type_support,
                         participant),
    qos_(qos),
    listener_mask_(mask),
    listener_(DDS::TopicListener::_duplicate(a_listener)),
    id_(GUID_UNKNOWN)
{
  inconsistent_topic_status_.total_count = 0;
  inconsistent_topic_status_.total_count_change = 0;
  monitor_.reset(TheServiceParticipant->monitor_factory_->create_topic_monitor(this));
}

TopicImpl::~TopicImpl()
{
}

DDS::ReturnCode_t TopicImpl::set_qos(const DDS::TopicQos& qos_arg)
{
  DDS::TopicQos qos = qos_arg;

  OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_DURABILITY_SERVICE_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);
  OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT_COMPATIBILITY_CHECK(qos, DDS::RETCODE_UNSUPPORTED);

  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (qos_ == qos)
      return DDS::RETCODE_OK;

    // for the not changeable qos, it can be changed before enable
    if (!Qos_Helper::changeable(qos_, qos) && enabled_ == true) {
      return DDS::RETCODE_IMMUTABLE_POLICY;

    } else {
      qos_ = qos;

      Discovery_rch disco =
        TheServiceParticipant->get_discovery(participant_->get_domain_id());
      const bool status =
        disco->update_topic_qos(this->id_, participant_->get_domain_id(),
                               participant_->get_id(), qos_);

      if (!status) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TopicImpl::set_qos, ")
                          ACE_TEXT("failed on compatibility check.\n")),
                         DDS::RETCODE_ERROR);
      }
    }

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
TopicImpl::get_qos(DDS::TopicQos& qos)
{
  qos = qos_;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
TopicImpl::set_listener(DDS::TopicListener_ptr a_listener, DDS::StatusMask mask)
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  listener_mask_ = mask;
  //note: OK to duplicate  a nil object ref
  listener_ = DDS::TopicListener::_duplicate(a_listener);
  return DDS::RETCODE_OK;
}

DDS::TopicListener_ptr
TopicImpl::get_listener()
{
  ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
  return DDS::TopicListener::_duplicate(listener_.in());
}

DDS::ReturnCode_t
TopicImpl::get_inconsistent_topic_status(DDS::InconsistentTopicStatus& a_status)
{
  set_status_changed_flag(DDS::INCONSISTENT_TOPIC_STATUS, false);
  a_status = inconsistent_topic_status_;
  inconsistent_topic_status_.total_count_change = 0;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
TopicImpl::enable()
{
  //According spec:
  // - Calling enable on an already enabled Entity returns OK and has no
  // effect.
  // - Calling enable on an Entity whose factory is not enabled will fail
  // and return PRECONDITION_NOT_MET.

  if (this->is_enabled()) {
    return DDS::RETCODE_OK;
  }

  if (!this->participant_->is_enabled()) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  if (id_ == GUID_UNKNOWN) {
    const DDS::DomainId_t dom_id = participant_->get_domain_id();
    Discovery_rch disco = TheServiceParticipant->get_discovery(dom_id);
    TopicStatus status = disco->assert_topic(id_,
                                             dom_id,
                                             participant_->get_id(),
                                             topic_name_.c_str(),
                                             type_name_.c_str(),
                                             qos_,
                                             type_support_ ? type_support_->has_dcps_key() : false,
                                             this);
    if (status != CREATED && status != FOUND) {
      if (DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: TopicImpl::enable, ")
                   ACE_TEXT("assert_topic failed with return value <%C>.\n"),
                   topicstatus_to_string(status)));
      }
      return DDS::RETCODE_ERROR;
    }
  }

  if (this->monitor_) {
    monitor_->report();
  }
  return this->set_enabled();
}

RepoId
TopicImpl::get_id() const
{
  return id_;
}

DDS::InstanceHandle_t
TopicImpl::get_instance_handle()
{
  return get_entity_instance_handle(id_, participant_);
}

const char*
TopicImpl::type_name() const
{
  return this->type_name_.c_str();
}

const char*
TopicImpl::topic_name() const
{
  return this->topic_name_.c_str();
}


void
TopicImpl::transport_config(const TransportConfig_rch&)
{
  throw Transport::MiscProblem();
}

void
TopicImpl::inconsistent_topic(int count)
{
  inconsistent_topic_status_.total_count_change += count - inconsistent_topic_status_.total_count;
  inconsistent_topic_status_.total_count = count;

  set_status_changed_flag(DDS::INCONSISTENT_TOPIC_STATUS, true);

  DDS::TopicListener_var listener;
  {
    ACE_Guard<ACE_Thread_Mutex> g(listener_mutex_);
    listener = listener_;
    if (!listener || !(listener_mask_ & DDS::INCONSISTENT_TOPIC_STATUS)) {
      g.release();
      listener = participant_->listener_for(DDS::INCONSISTENT_TOPIC_STATUS);
    }
  }
  if (listener) {
    listener->on_inconsistent_topic(this, inconsistent_topic_status_);
    inconsistent_topic_status_.total_count_change = 0;
  }

  notify_status_condition();
}

bool TopicImpl::check_data_representation(const DDS::DataRepresentationIdSeq& qos_ids, bool is_data_writer)
{
  if (!type_support_) {
    return true;
  }
  DDS::DataRepresentationIdSeq type_allowed_reprs;
  type_support_->representations_allowed_by_type(type_allowed_reprs);
  //default for blank annotation is to allow all types of data representation
  if (type_allowed_reprs.length() == 0) {
    return true;
  }
  if (qos_ids.length() == 0) {
    return false;
  }
  //Data Writer will only use the 1st QoS declared
  if (is_data_writer) {
    DDS::DataRepresentationId_t id = qos_ids[0];
    for (CORBA::ULong j = 0; j < type_allowed_reprs.length(); ++j) {
      if (id == type_allowed_reprs[j]) {
        return true;
      }
    }
  } else { // if data reader compare both lists for a compatible QoS
    for (CORBA::ULong i = 0; i < qos_ids.length(); ++i) {
      for (CORBA::ULong j = 0; j < type_allowed_reprs.length(); ++j) {
        if (qos_ids[i] == type_allowed_reprs[j]) {
          return true;
        }
      }
    }
  }
  return false;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
