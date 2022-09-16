#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#include "MemberDescriptorImpl.h"

#include "DynamicTypeImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

MemberDescriptorImpl::MemberDescriptorImpl()
{
  id(MEMBER_ID_INVALID);
  index(0);
  try_construct_kind(DDS::DISCARD);
  is_key(false);
  is_optional(false);
  is_must_understand(false);
  is_shared(false);
  is_default_label(false);
}

MemberDescriptorImpl::~MemberDescriptorImpl()
{}

bool MemberDescriptorImpl::equals(DDS::MemberDescriptor* other)
{
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality(this, other, dt_ptr_pair);
}

CORBA::ValueBase* MemberDescriptorImpl::_copy_value()
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MemberDescriptorImpl::_copy_value is not implemented\n"));
  return 0;
}

CORBA::TypeCode_ptr MemberDescriptorImpl::_tao_type() const
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MemberDescriptorImpl::_tao_type is not implemented\n"));
  return 0;
}

DDS::ReturnCode_t MemberDescriptorImpl::copy_from(DDS::MemberDescriptor*)
{
  // FUTURE
  return DDS::RETCODE_UNSUPPORTED;
}

::CORBA::Boolean MemberDescriptorImpl::is_consistent()
{
  // FUTURE
  ACE_ERROR((LM_ERROR, "(%P|%t) ERRROR: MemberDescriptorImpl::is_consistent is not implemented\n"));
  return false;
}

::CORBA::Boolean MemberDescriptorImpl::_tao_marshal__DDS_MemberDescriptor(TAO_OutputCDR &, TAO_ChunkInfo &) const
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MemberDescriptorImpl::_tao_marshal__DDS_MemberDescriptor is not implemented\n"));
  return false;
}

::CORBA::Boolean MemberDescriptorImpl::_tao_unmarshal__DDS_MemberDescriptor(TAO_InputCDR &, TAO_ChunkInfo &)
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MemberDescriptorImpl::_tao_unmarshal__DDS_MemberDescriptor is not implemented\n"));
  return false;
}

inline bool operator==(const DDS::UnionCaseLabelSeq& lhs, const DDS::UnionCaseLabelSeq& rhs)
{
  if (lhs.length() == rhs.length()) {
    for (ACE_CDR::ULong i = 0; i < lhs.length(); ++i) {
      if (lhs[i] != rhs[i]) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool test_equality(DDS::MemberDescriptor* lhs, DDS::MemberDescriptor* rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  const DDS::DynamicType_ptr lhs_type = lhs->type();
  const DDS::DynamicType_ptr rhs_type = rhs->type();
  return
    std::strcmp(lhs->name(), rhs->name()) == 0 &&
    lhs->id() == rhs->id() &&
    test_equality(lhs_type, rhs_type, dt_ptr_pair) &&
    lhs->default_value() == rhs->default_value() &&
    lhs->index() == rhs->index() &&
    lhs->label() == rhs->label() &&
    lhs->try_construct_kind() == rhs->try_construct_kind() &&
    lhs->is_key() == rhs->is_key() &&
    lhs->is_optional() == rhs->is_optional() &&
    lhs->is_must_understand() == rhs->is_must_understand() &&
    lhs->is_shared() == rhs->is_shared() &&
    lhs->is_default_label() == rhs->is_default_label();
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
