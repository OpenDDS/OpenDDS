/*
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "EntityImpl.h"

#include "DomainParticipantImpl.h"
#include "StatusConditionImpl.h"
#include "transport/framework/TransportConfig.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

EntityImpl::EntityImpl()
  : enabled_(false)
  , entity_deleted_(false)
  , status_changes_(0)
  , status_condition_(new StatusConditionImpl(this))
  , observer_()
  , events_(Observer::e_NONE)
  , instance_handle_(DDS::HANDLE_NIL)
{
}

EntityImpl::~EntityImpl()
{
  const RcHandle<DomainParticipantImpl> participant = participant_for_instance_handle_.lock();
  if (participant) {
    participant->return_handle(instance_handle_);
  }
}

DDS::ReturnCode_t
EntityImpl::set_enabled()
{
  enabled_ = true;

  return DDS::RETCODE_OK;
}

bool
EntityImpl::is_enabled() const
{
#ifdef ACE_HAS_CPP11
  return this->enabled_;
#else
  return this->enabled_.value();
#endif
}

DDS::StatusCondition_ptr
EntityImpl::get_statuscondition()
{
  return DDS::StatusCondition::_duplicate(status_condition_);
}

DDS::StatusMask
EntityImpl::get_status_changes()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
  return status_changes_;
}

void
EntityImpl::set_status_changed_flag(
  DDS::StatusKind status,
  bool status_changed_flag)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (status_changed_flag) {
    status_changes_ |= status;

  } else {
    status_changes_ &= ~status;
  }
}

void
EntityImpl::set_deleted(bool state)
{
  entity_deleted_ = state;
}

bool
EntityImpl::get_deleted() const
{
#ifdef ACE_HAS_CPP11
  return this->entity_deleted_;
#else
  return this->entity_deleted_.value();
#endif
}

Observer_rch EntityImpl::get_observer(Observer::Event e)
{
  return (observer_ && (events_ & e)) ? observer_ :
         parent() ? parent()->get_observer(e) : Observer_rch();
}

void
EntityImpl::notify_status_condition()
{
  StatusConditionImpl* sci =
    dynamic_cast<StatusConditionImpl*>(status_condition_.in());
  if (sci) {
    sci->signal_all();
  } else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("EntityImpl::notify_status_condition: ")
      ACE_TEXT("failed to obtain the StatusConditionImpl.\n")));
  }
}

void
EntityImpl::transport_config(const TransportConfig_rch& cfg)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  transport_config_ = cfg;
}

TransportConfig_rch
EntityImpl::transport_config() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, TransportConfig_rch());
  return transport_config_;
}

void EntityImpl::set_observer(Observer_rch observer, Observer::Event e)
{
  observer_ = observer;
  events_ = e;
}

DDS::InstanceHandle_t EntityImpl::get_entity_instance_handle(const GUID_t& id,
                                                             DomainParticipantImpl* participant)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DDS::HANDLE_NIL);

  if (instance_handle_ != DDS::HANDLE_NIL) {
    return instance_handle_;
  }

  if (!participant) {
    return DDS::HANDLE_NIL;
  }

  participant_for_instance_handle_ = *participant;
  return instance_handle_ = participant->assign_handle(id);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
