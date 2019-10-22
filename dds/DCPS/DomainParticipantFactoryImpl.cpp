/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DomainParticipantFactoryImpl.h"
#include "DomainParticipantImpl.h"
#include "Marked_Default_Qos.h"
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
  DDS::DomainParticipantQos par_qos = qos;

  if (par_qos == PARTICIPANT_QOS_DEFAULT) {
    get_default_participant_qos(par_qos);
  }

  if (!Qos_Helper::valid(par_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
               ACE_TEXT("invalid qos.\n")));
    return DDS::DomainParticipant::_nil();
  }

  if (!Qos_Helper::consistent(par_qos)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
               ACE_TEXT("inconsistent qos.\n")));
    return DDS::DomainParticipant::_nil();
  }

  RcHandle<DomainParticipantImpl> dp =
    make_rch<DomainParticipantImpl>(this, domainId, par_qos, a_listener, mask);

  if (qos_.entity_factory.autoenable_created_entities) {
    if (dp->enable() != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                 ACE_TEXT("unable to enable DomainParticipant.\n")));
      return DDS::DomainParticipant::_nil();
    }
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex,
                   tao_mon,
                   participants_protector_,
                   DDS::DomainParticipant::_nil());

  participants_[domainId].insert(dp);
  return dp._retn();
}

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

  if (!the_servant) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("DomainParticipantFactoryImpl::delete_participant: ")
      ACE_TEXT("failed to obtain the DomainParticipantImpl.\n")));

    return DDS::RETCODE_ERROR;
  }

  RcHandle<DomainParticipantImpl> servant_rch = rchandle_from(the_servant);

  //xxx servant rc = 4 (servant::DP::Entity::ServantBase::ref_count_
  if (!the_servant->is_clean()) {
    RepoId id = the_servant->get_id();
    GuidConverter converter(id);
    ACE_DEBUG((LM_DEBUG, // not an ERROR, tests may be doing this on purpose
               ACE_TEXT("(%P|%t) WARNING: ")
               ACE_TEXT("DomainParticipantFactoryImpl::delete_participant: ")
               ACE_TEXT("the participant %C is not empty.\n"),
               OPENDDS_STRING(converter).c_str()));

    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  const DDS::DomainId_t domain_id = the_servant->get_domain_id();
  RepoId dp_id = the_servant->get_id();

  DPSet* entry = 0;

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex,
                     tao_mon,
                     participants_protector_,
                     DDS::RETCODE_ERROR);

    if (find(participants_, domain_id, entry) == -1) {
      GuidConverter converter(dp_id);
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("DomainParticipantFactoryImpl::delete_participant: ")
                        ACE_TEXT("%p domain_id=%d dp_id=%C.\n"),
                        ACE_TEXT("find"),
                        domain_id,
                        OPENDDS_STRING(converter).c_str()), DDS::RETCODE_ERROR);

    } else {
      DDS::ReturnCode_t result
      = the_servant->delete_contained_entities();

      if (result != DDS::RETCODE_OK) {
        return result;
      }

      if (OpenDDS::DCPS::remove(*entry, servant_rch) == -1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                          ACE_TEXT(" %p.\n"),
                          ACE_TEXT("remove")),
                         DDS::RETCODE_ERROR);
      }

      if (entry->empty()) {
        if (unbind(participants_, domain_id) == -1) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                            ACE_TEXT(" %p.\n"),
                            ACE_TEXT("unbind")),
                           DDS::RETCODE_ERROR);
        }
      }
    }
  }

  Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id);
  if (disco) {
    if (!disco->remove_domain_participant(domain_id,
                                          dp_id)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("could not remove domain participant.\n")),
                       DDS::RETCODE_ERROR);
    }
  }
  return DDS::RETCODE_OK;
}

DDS::DomainParticipant_ptr
DomainParticipantFactoryImpl::lookup_participant(
  DDS::DomainId_t domainId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex,
                   tao_mon,
                   participants_protector_,
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
    return DDS::DomainParticipant::_duplicate(entry->begin()->in());
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

DomainParticipantFactoryImpl::DPMap
DomainParticipantFactoryImpl::participants() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex,
                   tao_mon,
                   participants_protector_,
                   DomainParticipantFactoryImpl::DPMap());

  return participants_;
}

void DomainParticipantFactoryImpl::cleanup()
{
  ACE_GUARD(ACE_Thread_Mutex,
            tao_mon,
            participants_protector_);

  DPMap::iterator itr;
  for (itr = participants_.begin(); itr != participants_.end(); ++itr) {
    DPSet& dp_set = itr->second;
    DPSet::iterator dp_set_itr;
    for (dp_set_itr = dp_set.begin(); dp_set_itr != dp_set.end(); ++dp_set_itr) {
      (*dp_set_itr)->delete_contained_entities();
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
