/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TypeSupportImpl.h"

#include "Registered_Data_Types.h"

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

char*
TypeSupportImpl::get_type_name()
{
  if (this->type_name_.in() == 0) {
    return CORBA::string_dup(this->_interface_repository_id());
  } else {
    return CORBA::string_dup(this->type_name_.in());
  }
}

}
}
