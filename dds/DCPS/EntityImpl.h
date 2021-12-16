/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ENTITY_IMPL_H
#define OPENDDS_DCPS_ENTITY_IMPL_H

#include "Observer.h"
#include "LocalObject.h"
#include "Definitions.h"
#include "transport/framework/TransportConfig_rch.h"

#include <dds/DdsDcpsInfrastructureC.h>

#ifndef ACE_HAS_CPP11
#  include <ace/Atomic_Op_T.h>
#endif

#ifdef ACE_HAS_CPP11
#  include <atomic>
#endif

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DomainParticipantImpl;

/**
* @class EntityImpl
*
* @brief Implements the OpenDDS::DCPS::Entity
*        interfaces.
*
* This class is the base class of other servant classes.
* e.g. DomainParticipantImpl, PublisherImpl ...
*/
class OpenDDS_Dcps_Export EntityImpl
  : public virtual LocalObject<DDS::Entity> {
public:
  EntityImpl();

  virtual ~EntityImpl();

  bool is_enabled() const;

  virtual DDS::StatusCondition_ptr get_statuscondition();

  virtual DDS::StatusMask get_status_changes();

  virtual DDS::InstanceHandle_t get_instance_handle() = 0;

  virtual DDS::DomainId_t get_domain_id() { return DOMAIN_UNKNOWN; }

  virtual RepoId get_id() const { return GUID_UNKNOWN; }

  void set_status_changed_flag(DDS::StatusKind status,
                               bool status_changed_flag);

  /// Call this *after* dispatching to listeners when the "changed status
  /// flag" is enabled so that any waiting waitsets can be unblocked.
  void notify_status_condition();

  virtual void transport_config(const TransportConfig_rch& cfg);
  TransportConfig_rch transport_config() const;

  virtual RcHandle<EntityImpl> parent() const { return RcHandle<EntityImpl>(); }

  void set_observer(Observer_rch observer, Observer::Event e);

  Observer_rch get_observer(Observer::Event e);

protected:
  DDS::ReturnCode_t set_enabled();

  void set_deleted(bool state);

  bool get_deleted() const;

  DDS::InstanceHandle_t get_entity_instance_handle(const GUID_t& id, DomainParticipantImpl* participant);

  typedef
#ifdef ACE_HAS_CPP11
    std::atomic<bool>
#else
    ACE_Atomic_Op<TAO_SYNCH_MUTEX, bool>
#endif
    AtomicBool;

  /// The flag indicates the entity is enabled.
  AtomicBool enabled_;

  /// The flag indicates the entity is being deleted.
  AtomicBool entity_deleted_;

private:
  /// The status_changes_ variable lists all status changed flag.
  /// The StatusChangedFlag becomes TRUE whenever the plain communication
  /// status changes and it is reset to FALSE each time the application
  /// accesses the plain communication status via the proper
  /// get_<plain communication status> operation on the Entity.
  DDS::StatusMask status_changes_;
  DDS::StatusCondition_var status_condition_;

  TransportConfig_rch transport_config_;

  Observer_rch observer_;
  Observer::Event events_;

  mutable ACE_Thread_Mutex lock_;

  DDS::InstanceHandle_t instance_handle_;
  WeakRcHandle<DomainParticipantImpl> participant_for_instance_handle_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_ENTITY_IMPL_H */
