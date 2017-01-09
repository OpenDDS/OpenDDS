/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TypeSupportImpl.h"

#include "Registered_Data_Types.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TypeSupportImpl::~TypeSupportImpl()
{}

DDS::ReturnCode_t
TypeSupportImpl::register_type(DDS::DomainParticipant_ptr participant,
                               const char* type_name)
{
  if (type_name == 0 || type_name[0] == '\0') {
    this->type_name_ = this->get_type_name();
  } else {
    this->type_name_ = type_name;
  }

  return Registered_Data_Types->register_type(participant,
                                              this->type_name_.in(), this);
}

DDS::ReturnCode_t
TypeSupportImpl::unregister_type(DDS::DomainParticipant_ptr participant,
    const char* type_name)
{
  if (this->type_name_ == 0 || this->type_name_[0] == '\0') {
    if (strcmp(this->type_name_.in(), this->get_type_name()) == 0) {
      return Registered_Data_Types->unregister_type(participant, this->type_name_.in(), this);
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }
  else {
    return Registered_Data_Types->unregister_type(participant, type_name, this);
  }
}

char*
TypeSupportImpl::get_type_name()
{
  if (type_name_.in() == 0) {
    return CORBA::string_dup(default_type_name());
  } else {
    return CORBA::string_dup(type_name_.in());
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
