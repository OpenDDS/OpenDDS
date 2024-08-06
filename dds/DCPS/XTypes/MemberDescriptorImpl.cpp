#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#include "MemberDescriptorImpl.h"

#include "DynamicTypeImpl.h"
#include "IdlScanner.h"

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
  if (!other) {
    return false;
  }

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

DDS::ReturnCode_t MemberDescriptorImpl::copy_from(DDS::MemberDescriptor* other)
{
  if (!other) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  name(other->name());
  id(other->id());
  type(other->type());
  default_value(other->default_value());
  index(other->index());
  label(other->label());
  try_construct_kind(other->try_construct_kind());
  is_key(other->is_key());
  is_optional(other->is_optional());
  is_must_understand(other->is_must_understand());
  is_shared(other->is_shared());
  is_default_label(other->is_default_label());

  return DDS::RETCODE_OK;
}

::CORBA::Boolean MemberDescriptorImpl::is_consistent()
{
  {
    CharacterScanner cs(name());
    IdlScanner is(cs);

    const IdlToken token = is.scan_token();
    if (token.is_identifier()) {
      return false;
    }
    if (!is.eoi()) {
      return false;
    }
  }

  // uniqueness of name must be checked by the enclosing type.

  // id must be checked by the enclosing type.

  DDS::DynamicType_ptr t = type();
  if (!t) {
    return false;
  }

  // type must be checked by the enclosing type.

  CharacterScanner cs(default_value());
  if (!cs.eoi()) {
    IdlScanner is(cs);
    const IdlToken token = is.scan_token(t);
    if (token.is_error()) {
      return false;
    }
    if (!is.eoi()) {
      return false;
    }
  }

  // index does not need to be checked.

  // label must be checked by the enclosing type.
  // try_construct_kind must be checked by the enclosing type.
  // is_key must be checked by the enclosing type.
  // is_optional must be checked by the enclosing type.
  // is_must_understand must be checked by the enclosing type.
  // is_shared must be checked by the enclosing type.
  // is_default_label must be checked by the enclosing type.

  return true;
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

namespace {

  bool default_value_equal(DDS::MemberDescriptor* lhs, DDS::MemberDescriptor* rhs)
  {
    if (std::strcmp(lhs->default_value(), rhs->default_value()) == 0) {
      return true;
    }

    IdlToken lhs_token;
    {
      DDS::DynamicType_ptr lhs_t = lhs->type();
      if (!lhs_t) {
        return false;
      }

      CharacterScanner cs(lhs->default_value());
      if (!cs.eoi()) {
        IdlScanner is(cs);
        lhs_token = is.scan_token(lhs_t);
      }
    }

    IdlToken rhs_token;
    {
      DDS::DynamicType_ptr rhs_t = rhs->type();
      if (!rhs_t) {
        return false;
      }

      CharacterScanner cs(rhs->default_value());
      if (!cs.eoi()) {
        IdlScanner is(cs);
        rhs_token = is.scan_token(rhs_t);
      }
    }

    return !lhs_token.is_error() && !rhs_token.is_error() && lhs_token == rhs_token;
  }
}

bool test_equality(DDS::MemberDescriptor* lhs, DDS::MemberDescriptor* rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  const DDS::DynamicType_ptr lhs_type = lhs->type();
  const DDS::DynamicType_ptr rhs_type = rhs->type();
  return
    std::strcmp(lhs->name(), rhs->name()) == 0 &&
    lhs->id() == rhs->id() &&
    test_equality(lhs_type, rhs_type, dt_ptr_pair) &&
    default_value_equal(lhs, rhs) &&
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
