#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#include "DynamicTypeMemberImpl.h"

#include "DynamicTypeImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

DynamicTypeMemberImpl::DynamicTypeMemberImpl()
{}

DynamicTypeMemberImpl::~DynamicTypeMemberImpl()
{}

DDS::ReturnCode_t DynamicTypeMemberImpl::set_descriptor(DDS::MemberDescriptor* descriptor)
{
  // ValueTypes don't have a duplicate so manually add_ref.
  CORBA::add_ref(descriptor);
  descriptor_ = descriptor;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicTypeMemberImpl::get_descriptor(DDS::MemberDescriptor*& descriptor)
{
  DDS::MemberDescriptor_var descriptor_v(descriptor);
  descriptor = descriptor_;
  CORBA::add_ref(descriptor);
  return DDS::RETCODE_OK;
}

CORBA::ULong DynamicTypeMemberImpl::get_annotation_count()
{
  // FUTURE
  return 0;
}

DDS::ReturnCode_t DynamicTypeMemberImpl::get_annotation(DDS::AnnotationDescriptor*&, CORBA::ULong)
{
  // FUTURE
  return DDS::RETCODE_UNSUPPORTED;
}

CORBA::ULong DynamicTypeMemberImpl::get_verbatim_text_count()
{
  // FUTURE
  return 0;
}

DDS::ReturnCode_t DynamicTypeMemberImpl::get_verbatim_text(DDS::VerbatimTextDescriptor*&, CORBA::ULong)
{
  // FUTURE
  return DDS::RETCODE_UNSUPPORTED;
}

bool DynamicTypeMemberImpl::equals(DDS::DynamicTypeMember_ptr other)
{
  //7.5.2.6.3 Operation: equals
  //Two members shall be considered equal if and only if they belong to the same type and all of
  //their respective properties, as identified in Table 52 above, are equal.

  //In addition to what the spec says to compare, we will be comparing the MemberDescriptors of both
  //DynamicTypeMembers. The spec seems to assume this is the case, despite not listing the MemberDescriptor
  //as a property in table 52. If this were not the case, any two members within a type would be considered
  //equal, regardless of whether they are actually the same member.

  DDS::MemberDescriptor_var other_md;
  if (other->get_descriptor(other_md) != DDS::RETCODE_OK) {
    return false;
  }

  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality(descriptor_, other_md, dt_ptr_pair);
}

MemberId DynamicTypeMemberImpl::get_id()
{
  return descriptor_->id();
}

char* DynamicTypeMemberImpl::get_name()
{
  CORBA::String_var s = descriptor_->name();
  return s._retn();
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
