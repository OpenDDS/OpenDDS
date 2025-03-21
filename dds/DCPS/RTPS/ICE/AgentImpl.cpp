/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "AgentImpl.h"

#include "Task.h"
#include "EndpointManager.h"

#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/TimeTypes.h>

#include <dds/OpenDDSConfigWrapper.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace ICE {

#if OPENDDS_CONFIG_SECURITY

using DCPS::TimeDuration;
using DCPS::MonotonicTimePoint;

void AgentImpl::enqueue(const DCPS::MonotonicTimePoint& a_release_time,
                        WeakTaskPtr wtask)
{
  if (tasks_.empty() || a_release_time < tasks_.top().release_time_) {
    const MonotonicTimePoint release = std::max(last_execute_ + T_a_, a_release_time);
    task_task_->schedule(release - MonotonicTimePoint::now());
  }
  tasks_.push(Item(a_release_time, wtask));
}

bool AgentImpl::reactor_is_shut_down() const
{
  return TheServiceParticipant->is_shut_down();
}

void AgentImpl::process_tasks(const DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();

  if (tasks_.empty()) {
    return;
  }

  if (tasks_.top().release_time_ <= now) {
    Item item = tasks_.top();
    tasks_.pop();

    TaskPtr task = item.task_.lock();
    if (task) {
      task->execute(now);
      last_execute_ = now;
    }
  }
  process_deferred();
  check_invariants();

  if (!tasks_.empty()) {
    const MonotonicTimePoint release = std::max(last_execute_ + T_a_, tasks_.top().release_time_);
    task_task_->schedule(release - now);
  }
}

AgentImpl::AgentImpl()
  : DCPS::InternalDataReaderListener<DCPS::NetworkInterfaceAddress>(TheServiceParticipant->job_queue())
  , T_a_(ICE::Configuration::instance()->T_a())
  , connectivity_check_ttl_(ICE::Configuration::instance()->connectivity_check_ttl())
  , checklist_period_(ICE::Configuration::instance()->checklist_period())
  , indication_period_(ICE::Configuration::instance()->indication_period())
  , nominated_ttl_(ICE::Configuration::instance()->nominated_ttl())
  , server_reflexive_address_period_(ICE::Configuration::instance()->server_reflexive_address_period())
  , server_reflexive_indication_count_(ICE::Configuration::instance()->server_reflexive_indication_count())
  , deferred_triggered_check_ttl_(ICE::Configuration::instance()->deferred_triggered_check_ttl())
  , change_password_period_(ICE::Configuration::instance()->change_password_period())
  , unfreeze_(false)
  , reader_(DCPS::make_rch<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> >(DCPS::DataReaderQosBuilder().reliability_reliable().durability_transient_local(), DCPS::rchandle_from(this)))
  , reader_added_(false)
  , remote_peer_reflexive_counter_(0)
  , task_task_(DCPS::make_rch<DCPS::PmfSporadicTask<AgentImpl> >(TheServiceParticipant->time_source(), TheServiceParticipant->reactor_task(), DCPS::rchandle_from(this), &AgentImpl::process_tasks))
{
  // Bind the lifetime of this to the service participant.
  TheServiceParticipant->set_shutdown_listener(DCPS::static_rchandle_cast<ShutdownListener>(rchandle_from(this)));
}

void AgentImpl::add_endpoint(DCPS::WeakRcHandle<Endpoint> a_endpoint)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();

  if (endpoint_managers_.find(a_endpoint) == endpoint_managers_.end()) {
    EndpointManagerPtr em = DCPS::make_rch<EndpointManager>(this, a_endpoint);
    endpoint_managers_[a_endpoint] = em;
  }

  check_invariants();

  if (!endpoint_managers_.empty() && !reader_added_) {
    TheServiceParticipant->network_interface_address_topic()->connect(reader_);
    reader_added_ = true;
  }
}

void AgentImpl::remove_endpoint(DCPS::WeakRcHandle<Endpoint> a_endpoint)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();

  EndpointManagerMapType::iterator pos = endpoint_managers_.find(a_endpoint);

  if (pos != endpoint_managers_.end()) {
    EndpointManagerPtr em = pos->second;
    em->purge();
    endpoint_managers_.erase(pos);
  }

  check_invariants();

  if (endpoint_managers_.empty() && reader_added_) {
    TheServiceParticipant->network_interface_address_topic()->disconnect(reader_);
    reader_added_ = false;
  }
}

AgentInfo AgentImpl::get_local_agent_info(DCPS::WeakRcHandle<Endpoint> a_endpoint) const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, mutex, AgentInfo());
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  return pos->second->agent_info();
}

void AgentImpl::add_local_agent_info_listener(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                              const DCPS::GUID_t& a_local_guid,
                                              DCPS::WeakRcHandle<AgentInfoListener> a_agent_info_listener)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->add_agent_info_listener(a_local_guid, a_agent_info_listener);
}

void AgentImpl::remove_local_agent_info_listener(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                                 const DCPS::GUID_t& a_local_guid)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->remove_agent_info_listener(a_local_guid);
}

void  AgentImpl::start_ice(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                           const DCPS::GUID_t& a_local_guid,
                           const DCPS::GUID_t& a_remote_guid,
                           const AgentInfo& a_remote_agent_info)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->start_ice(a_local_guid, a_remote_guid, a_remote_agent_info);
  check_invariants();
}

void AgentImpl::stop_ice(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                         const DCPS::GUID_t& a_local_guid,
                         const DCPS::GUID_t& a_remote_guid)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->stop_ice(a_local_guid, a_remote_guid);
  check_invariants();
}

ACE_INET_Addr  AgentImpl::get_address(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                                      const DCPS::GUID_t& a_local_guid,
                                      const DCPS::GUID_t& a_remote_guid) const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, mutex, ACE_INET_Addr());
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  return pos->second->get_address(a_local_guid, a_remote_guid);
}

// Receive a STUN message.
void  AgentImpl::receive(DCPS::WeakRcHandle<Endpoint> a_endpoint,
                         const ACE_INET_Addr& a_local_address,
                         const ACE_INET_Addr& a_remote_address,
                         const STUN::Message& a_message)
{
  if (a_local_address.is_any()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) AgentImpl::receive: ERROR local_address is empty, ICE will not work on this platform\n")));
    return;
  }

  if (a_remote_address.is_any()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) AgentImpl::receive: ERROR remote_address is empty, ICE will not work on this platform\n")));
    return;
  }

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  check_invariants();
  EndpointManagerMapType::const_iterator pos = endpoint_managers_.find(a_endpoint);
  OPENDDS_ASSERT(pos != endpoint_managers_.end());
  pos->second->receive(a_local_address, a_remote_address, a_message);
  process_deferred();
  check_invariants();
}

void AgentImpl::remove(const FoundationType& a_foundation)
{
  // Foundations that are completely removed from the set of active foundations may unfreeze a checklist.
  unfreeze_ = active_foundations_.remove(a_foundation) || unfreeze_;
}

void AgentImpl::unfreeze(const FoundationType& a_foundation)
{
  to_unfreeze_.push_back(a_foundation);
}

void AgentImpl::check_invariants() const
{
  ActiveFoundationSet expected;

  for (EndpointManagerMapType::const_iterator pos = endpoint_managers_.begin(),
       limit = endpoint_managers_.end(); pos != limit; ++pos) {
    pos->second->compute_active_foundations(expected);
    pos->second->check_invariants();
  }

  OPENDDS_ASSERT(expected == active_foundations_);
}

void AgentImpl::shutdown()
{
  task_task_->cancel();
}

void AgentImpl::notify_shutdown()
{
  shutdown();
}

void AgentImpl::on_data_available(DCPS::RcHandle<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> >)
{
  DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  reader_->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  // FUTURE: This polls the endpoints.  The endpoints should just publish the change.
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, mutex);
  for (EndpointManagerMapType::const_iterator pos = endpoint_managers_.begin(),
         limit = endpoint_managers_.end(); pos != limit; ++pos) {
    pos->second->network_change();
  }
}

void AgentImpl::process_deferred()
{
  // A successful connectivity check unfreezed a foundation.
  // Communicate this to all endpoints and checklists.
  for (FoundationList::const_iterator fpos = to_unfreeze_.begin(), flimit = to_unfreeze_.end(); fpos != flimit; ++fpos) {
    for (EndpointManagerMapType::const_iterator pos = endpoint_managers_.begin(),
           limit = endpoint_managers_.end(); pos != limit; ++pos) {
      pos->second->unfreeze(*fpos);
    }
  }
  to_unfreeze_.clear();

  // A foundation was completely removed.
  // Communicate this to all endpoints and checklists.
  if (unfreeze_) {
    for (EndpointManagerMapType::const_iterator pos = endpoint_managers_.begin(),
           limit = endpoint_managers_.end(); pos != limit; ++pos) {
      pos->second->unfreeze();
    }
    unfreeze_ = false;
  }
}

#endif

} // namespace ICE
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
