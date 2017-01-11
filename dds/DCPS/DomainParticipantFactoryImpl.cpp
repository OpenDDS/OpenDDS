/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DomainParticipantFactoryImpl.h"
#include "DomainParticipantImpl.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "GuidConverter.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "Util.h"
#include "tao/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DomainParticipantFactoryImpl::DomainParticipantFactoryImpl()
  : qos_(TheServiceParticipant->initial_DomainParticipantFactoryQos()),
    default_participant_qos_(TheServiceParticipant->initial_DomainParticipantQos())
{
}

DomainParticipantFactoryImpl::~DomainParticipantFactoryImpl()
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               "%T (%P|%t) DomainParticipantFactoryImpl::"
               "~DomainParticipantFactoryImpl()\n"));
  }
}

DDS::DomainParticipant_ptr
DomainParticipantFactoryImpl::create_participant(
  DDS::DomainId_t domainId,
  const DDS::DomainParticipantQos & qos,
  DDS::DomainParticipantListener_ptr a_listener,
  DDS::StatusMask mask)
{
  if (!Qos_Helper::valid(qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
               ACE_TEXT("invalid qos.\n")));
    return DDS::DomainParticipant::_nil();
  }

  if (!Qos_Helper::consistent(qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
               ACE_TEXT("inconsistent qos.\n")));
    return DDS::DomainParticipant::_nil();
  }

  Discovery_rch disco = TheServiceParticipant->get_discovery(domainId);

  if (disco.is_nil()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
               ACE_TEXT("no repository found for domainId: %d.\n"), domainId));
    return DDS::DomainParticipant::_nil();
  }

  const AddDomainStatus value =
    disco->add_domain_participant(domainId, qos);

  if (value.id == GUID_UNKNOWN) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
               ACE_TEXT("add_domain_participant returned invalid id.\n")));
    return DDS::DomainParticipant::_nil();
  }

  DomainParticipantImpl* dp = 0;

  ACE_NEW_RETURN(dp,
                 DomainParticipantImpl(this, domainId, value.id, qos, a_listener,
                                       mask, value.federated),
                 DDS::DomainParticipant::_nil());

  DDS::DomainParticipant_ptr dp_obj(dp);

  if (CORBA::is_nil(dp_obj)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
               ACE_TEXT("nil DomainParticipant.\n")));
    return DDS::DomainParticipant::_nil();
  }

  // Set the participant object reference before enable since it's
  // needed for the built in topics during enable.
  dp->set_object_reference(dp_obj);  //xxx no change

  if (qos_.entity_factory.autoenable_created_entities) {
    dp->enable();
  }

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->participants_protector_,
                   DDS::DomainParticipant::_nil());

  // the Pair will also act as a guard against leaking the
  // new DomainParticipantImpl (NO_DUP, so this takes over mem)
  Participant_Pair pair(dp, dp_obj, NO_DUP);

  DPSet* entry = 0;

  if (find(participants_, domainId, entry) == -1) {
    DPSet set;

    if (OpenDDS::DCPS::insert(set, pair) == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                 ACE_TEXT(" %p.\n"),
                 ACE_TEXT("insert")));
      return DDS::DomainParticipant::_nil();
    }

    if (OpenDDS::DCPS::bind(participants_, domainId, set)  == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                 ACE_TEXT(" %p.\n"),
                 ACE_TEXT("bind")));
      return DDS::DomainParticipant::_nil();
    }

  } else {
    if (OpenDDS::DCPS::insert(*entry, pair) == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                 ACE_TEXT(" %p.\n"),
                 ACE_TEXT("insert")));
      return DDS::DomainParticipant::_nil();
    }
  }

//xxx still ref_count = 1

  return DDS::DomainParticipant::_duplicate(dp_obj); //xxx still 2  (obj 3->4)
} //xxx obj 4->3

DDS::ReturnCode_t
DomainParticipantFactoryImpl::delete_participant(
  DDS::DomainParticipant_ptr a_participant)
{

//xxx rc = 4
  if (CORBA::is_nil(a_participant)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                      ACE_TEXT("Nil participant.\n")),
                     DDS::RETCODE_BAD_PARAMETER);
  }

  // The servant's ref count should be 2 at this point, one referenced
  // by the poa and the other referenced by the map.
  DomainParticipantImpl* the_servant
  = dynamic_cast<DomainParticipantImpl*>(a_participant);

  //xxx servant rc = 4 (servant::DP::Entity::ServantBase::ref_count_
  if (the_servant->is_clean() == 0) {
    RepoId id = the_servant->get_id();
    GuidConverter converter(id);
    ACE_DEBUG((LM_DEBUG, // not an ERROR, tests may be doing this on purpose
               ACE_TEXT("(%P|%t) WARNING: ")
               ACE_TEXT("DomainParticipantFactoryImpl::delete_participant: ")
               ACE_TEXT("the participant %C is not empty.\n"),
               OPENDDS_STRING(converter).c_str()));

    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::DomainId_t domain_id = the_servant->get_domain_id();
  RepoId dp_id = the_servant->get_id();

  DPSet* entry = 0;

  if (find(participants_, domain_id, entry) == -1) {
    GuidConverter converter(dp_id);
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("DomainParticipantFactoryImpl::delete_participant: ")
                      ACE_TEXT("%p domain_id=%d dp_id=%s.\n"),
                      ACE_TEXT("find"),
                      domain_id,
                      OPENDDS_STRING(converter).c_str()), DDS::RETCODE_ERROR);

  } else {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                     tao_mon,
                     this->participants_protector_,
                     DDS::RETCODE_ERROR);

    DDS::ReturnCode_t result
    = the_servant->delete_contained_entities();

//xxx still rc=4
    if (result != DDS::RETCODE_OK) {
      return result;
    }

    Participant_Pair pair(the_servant, a_participant, DUP);

    if (OpenDDS::DCPS::remove(*entry, pair) == -1) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                        ACE_TEXT(" %p.\n"),
                        ACE_TEXT("remove")),
                       DDS::RETCODE_ERROR);
    }

//xxx now obj rc=5 and servant rc=4
    if (entry->empty()) {
      if (unbind(participants_, domain_id) == -1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                          ACE_TEXT(" %p.\n"),
                          ACE_TEXT("unbind")),
                         DDS::RETCODE_ERROR);
      }
    } //xxx now obj rc = 4
  }//xxx now obj rc = 3

  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id);
  if (!disco->remove_domain_participant(domain_id,
                                        dp_id)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("could not remove domain participant.\n")),
                     DDS::RETCODE_ERROR);
  }
  return DDS::RETCODE_OK;
}

DDS::DomainParticipant_ptr
DomainParticipantFactoryImpl::lookup_participant(
  DDS::DomainId_t domainId)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                   tao_mon,
                   this->participants_protector_,
                   DDS::DomainParticipant::_nil());

  DPSet* entry;

  if (find(participants_, domainId, entry) == -1) {
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) ")
                 ACE_TEXT("DomainParticipantFactoryImpl::lookup_participant, ")
                 ACE_TEXT(" not found for domain %d.\n"),
                 domainId));
    }

    return DDS::DomainParticipant::_nil();

  } else {
    // No specification about which participant will return. We just return the first
    // object.
    // Note: We are not duplicate the object ref, so a delete call is not needed.
    return DDS::DomainParticipant::_duplicate((*(entry->begin())).obj_.in());
  }
}

DDS::ReturnCode_t
DomainParticipantFactoryImpl::set_default_participant_qos(
  const DDS::DomainParticipantQos & qos)
{
  if (Qos_Helper::valid(qos)
      && Qos_Helper::consistent(qos)) {
    default_participant_qos_ = qos;
    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
DomainParticipantFactoryImpl::get_default_participant_qos(
  DDS::DomainParticipantQos & qos)
{
  qos = default_participant_qos_;
  return DDS::RETCODE_OK;
}

DDS::DomainParticipantFactory_ptr
DomainParticipantFactoryImpl::get_instance()
{
  return TheParticipantFactory;
}

DDS::ReturnCode_t
DomainParticipantFactoryImpl::set_qos(
  const DDS::DomainParticipantFactoryQos & qos)
{
  if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos)) {
    if (!(qos_ == qos) && Qos_Helper::changeable(qos_, qos))
      qos_ = qos;

    return DDS::RETCODE_OK;

  } else {
    return DDS::RETCODE_INCONSISTENT_POLICY;
  }
}

DDS::ReturnCode_t
DomainParticipantFactoryImpl::get_qos(
  DDS::DomainParticipantFactoryQos & qos)
{
  qos = this->qos_;
  return DDS::RETCODE_OK;
}

const DomainParticipantFactoryImpl::DPMap&
DomainParticipantFactoryImpl::participants() const
{
  return this->participants_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
