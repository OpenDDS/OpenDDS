#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#include "TypeDescriptorImpl.h"

#include "DynamicTypeImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

TypeDescriptorImpl::TypeDescriptorImpl()
{
  kind(TK_NONE);
  extensibility_kind(DDS::FINAL);
  is_nested(false);
}

TypeDescriptorImpl::~TypeDescriptorImpl()
{}

bool TypeDescriptorImpl::equals(DDS::TypeDescriptor* other)
{
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality(this, other, dt_ptr_pair);
}

CORBA::ValueBase* TypeDescriptorImpl::_copy_value()
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TypeDescriptorImpl::_copy_value is not implemented\n"));
  return 0;
}

CORBA::TypeCode_ptr TypeDescriptorImpl::_tao_type() const
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TypeDescriptorImpl::_tao_type is not implemented\n"));
  return 0;
}

DDS::ReturnCode_t TypeDescriptorImpl::copy_from(DDS::TypeDescriptor*)
{
  // FUTURE
  return DDS::RETCODE_UNSUPPORTED;
}

CORBA::Boolean TypeDescriptorImpl::is_consistent()
{
  // FUTURE
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TypeDescriptorImpl::is_consistent is not implemented\n"));
  return false;
}

CORBA::Boolean TypeDescriptorImpl::_tao_marshal__DDS_TypeDescriptor(TAO_OutputCDR&, TAO_ChunkInfo&) const
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TypeDescriptorImpl::_tao_marshal__DDS_TypeDescriptor is not implemented\n"));
  return false;
}

CORBA::Boolean TypeDescriptorImpl::_tao_unmarshal__DDS_TypeDescriptor(TAO_InputCDR&, TAO_ChunkInfo&)
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TypeDescriptorImpl::_tao_unmarshal__DDS_TypeDescriptor is not implemented\n"));
  return false;
}

inline bool operator==(const DDS::BoundSeq& lhs, const DDS::BoundSeq& rhs)
{
  if (lhs.length() == rhs.length()) {
    for (ACE_CDR::ULong i = 0 ; i < lhs.length() ; ++i) {
      if (lhs[i] != rhs[i]) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool test_equality(DDS::TypeDescriptor* lhs, DDS::TypeDescriptor* rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs == rhs) {
    return true;
  }

  if (!lhs || !rhs) {
    return false;
  }

  return
    lhs->kind() == rhs->kind() &&
    std::strcmp(lhs->name(), rhs->name()) == 0 &&
    test_equality(lhs->base_type(), rhs->base_type(), dt_ptr_pair) &&
    test_equality(lhs->discriminator_type(), rhs->discriminator_type(), dt_ptr_pair) &&
    lhs->bound() == rhs->bound() &&
    test_equality(lhs->element_type(), rhs->element_type(), dt_ptr_pair) &&
    test_equality(lhs->key_element_type(), rhs->key_element_type(), dt_ptr_pair) &&
    lhs->extensibility_kind() == rhs->extensibility_kind() &&
    lhs->is_nested() == rhs->is_nested();
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
