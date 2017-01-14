/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Registered_Data_Types.h"

#include "dds/DCPS/Util.h"
#include "dds/DdsDcpsDomainC.h"

#include "ace/Singleton.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

Data_Types_Register::Data_Types_Register()
{
}

Data_Types_Register::~Data_Types_Register()
{
}

Data_Types_Register*
Data_Types_Register::instance()
{
  return ACE_Singleton<Data_Types_Register, ACE_SYNCH_MUTEX>::instance();
}

DDS::ReturnCode_t Data_Types_Register::register_type(
  DDS::DomainParticipant_ptr domain_participant,
  const char* type_name,
  TypeSupport_ptr the_type)
{
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, lock_, DDS::RETCODE_ERROR);

  TypeSupportMap& tsm = participants_[domain_participant];
  const TypeSupport_var typeSupport = TypeSupport::_duplicate(the_type);

  TypeSupportMap::iterator iter = tsm.find(type_name);
  if (iter == tsm.end()) {
    tsm[type_name] = typeSupport;
    return DDS::RETCODE_OK;
  }

  if (std::strcmp(typeSupport->_interface_repository_id(),
                  iter->second->_interface_repository_id()) == 0) {
    return DDS::RETCODE_OK;
  }

  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t Data_Types_Register::unregister_type(
    DDS::DomainParticipant_ptr domain_participant,
    const char* type_name,
    TypeSupport_ptr the_type)
{
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, lock_, DDS::RETCODE_ERROR);

  TypeSupportMap& tsm = participants_[domain_participant];

  TypeSupportMap::iterator iter = tsm.find(type_name);
  if (iter == tsm.end()) {
    // Not in the map, can't delete
    return DDS::RETCODE_ERROR;
  }
  else {
    if (std::strcmp(the_type->_interface_repository_id(), iter->second->_interface_repository_id()) == 0) {
      tsm.erase(iter);
      return DDS::RETCODE_OK;
    }
    else {
      return DDS::RETCODE_ERROR;
    }
  }
}

DDS::ReturnCode_t Data_Types_Register::unregister_participant(
  DDS::DomainParticipant_ptr domain_participant)
{
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, lock_, DDS::RETCODE_ERROR);
  participants_.erase(domain_participant);
  return DDS::RETCODE_OK;
}

TypeSupport_ptr Data_Types_Register::lookup(
  DDS::DomainParticipant_ptr domain_participant,
  const char* type_name) const
{
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, lock_, 0);

  ParticipantMap::const_iterator iter1 = participants_.find(domain_participant);
  if (iter1 == participants_.end()) {
    return 0;
  }

  TypeSupportMap::const_iterator iter2 = iter1->second.find(type_name);
  if (iter2 == iter1->second.end()) {
    return 0;
  }

  TypeSupport_var typeSupport = iter2->second;
  return typeSupport._retn();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
