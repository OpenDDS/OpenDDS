#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#include "DynamicTypeImpl.h"

#include "DynamicTypeMemberImpl.h"

#include <dds/DCPS/debug.h>

#include <dds/DdsDcpsInfrastructureC.h>

#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicTypeImpl::DynamicTypeImpl()
  : preset_type_info_set_(false)
{}

DynamicTypeImpl::~DynamicTypeImpl()
{}

void DynamicTypeImpl::set_descriptor(DDS::TypeDescriptor* descriptor)
{
  // ValueTypes don't have a duplicate so manually add_ref.
  CORBA::add_ref(descriptor);
  descriptor_ = descriptor;
}

DDS::ReturnCode_t DynamicTypeImpl::get_descriptor(DDS::TypeDescriptor*& descriptor)
{
  DDS::TypeDescriptor_var descriptor_v(descriptor);
  descriptor = descriptor_;
  CORBA::add_ref(descriptor);
  return DDS::RETCODE_OK;
}

char* DynamicTypeImpl::get_name()
{
  CORBA::String_var str = descriptor_->name();
  return str._retn();
}

TypeKind DynamicTypeImpl::get_kind()
{
  return descriptor_->kind();
}

DDS::ReturnCode_t DynamicTypeImpl::get_member_by_name(DDS::DynamicTypeMember_ptr& member, const char* name)
{
  const DynamicTypeMembersByName::const_iterator pos = members_by_name_.find(name);
  if (pos != members_by_name_.end()) {
    DDS::DynamicTypeMember_var member_v(member);
    member = DDS::DynamicTypeMember::_duplicate(pos->second);
    return DDS::RETCODE_OK;
  }
  return DDS::RETCODE_ERROR;
}

#if OPENDDS_CONFIG_IDL_MAP
DDS::ReturnCode_t DynamicTypeImpl::get_all_members_by_name(DDS::DynamicTypeMembersByName& members)
{
  members = members_by_name_;
  return DDS::RETCODE_OK;
}
#else
DDS::ReturnCode_t DynamicTypeImpl::get_all_members_by_name(DDS::DynamicTypeMembersByName*&)
{
  return DDS::RETCODE_UNSUPPORTED;
}
#endif

DDS::ReturnCode_t DynamicTypeImpl::get_member(DDS::DynamicTypeMember_ptr& member, MemberId id)
{
  const DynamicTypeMembersById::const_iterator pos = members_by_id_.find(id);
  if (pos == members_by_id_.end()) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      CORBA::String_var name = descriptor_->name();
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicTypeImpl::get_member: "
        "type %C doesn't have a member with id %u\n", name.in(), id));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::DynamicTypeMember_var member_v(member);
  member = DDS::DynamicTypeMember::_duplicate(pos->second);
  return DDS::RETCODE_OK;
}

#if OPENDDS_CONFIG_IDL_MAP
DDS::ReturnCode_t DynamicTypeImpl::get_all_members(DDS::DynamicTypeMembersById& members)
{
  members = members_by_id_;
  return DDS::RETCODE_OK;
}
#else
DDS::ReturnCode_t DynamicTypeImpl::get_all_members(DDS::DynamicTypeMembersById*&)
{
  return DDS::RETCODE_UNSUPPORTED;
}
#endif

ACE_CDR::ULong DynamicTypeImpl::get_member_count()
{
  return ACE_CDR::ULong(members_by_index_.size());
}

DDS::ReturnCode_t DynamicTypeImpl::get_member_by_index(DDS::DynamicTypeMember_ptr& member, ACE_CDR::ULong index)
{
  if (index >= members_by_index_.size()) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      CORBA::String_var name = descriptor_->name();
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicTypeImpl::get_member_by_index: "
        "type %C doesn't have a member with index %u\n", name.in(), index));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::DynamicTypeMember_var member_v(member);
  member = DDS::DynamicTypeMember::_duplicate(members_by_index_[index]);
  return DDS::RETCODE_OK;
}

CORBA::ULong DynamicTypeImpl::get_annotation_count()
{
  // FUTURE
  return 0;
}

DDS::ReturnCode_t DynamicTypeImpl::get_annotation(DDS::AnnotationDescriptor*&, CORBA::ULong)
{
  // FUTURE
  return DDS::RETCODE_UNSUPPORTED;
}

CORBA::ULong DynamicTypeImpl::get_verbatim_text_count()
{
  // FUTURE
  return 0;
}

DDS::ReturnCode_t DynamicTypeImpl::get_verbatim_text(DDS::VerbatimTextDescriptor*&, CORBA::ULong)
{
  // FUTURE
  return DDS::RETCODE_UNSUPPORTED;
}

bool DynamicTypeImpl::equals(DDS::DynamicType_ptr other)
{
  //7.5.2.8.4 Operation: equals
  //Two types shall be considered equal if and only if all of their respective properties, as identified
  //in Table 54 above, are equal.

  //In addition to what the spec says to compare, we will be comparing the TypeDescriptors of both
  //DynamicTypes. The spec seems to assume this is the case, despite not listing the TypeDescriptor
  //as a property in table 54. This is done to allow the recursive comparison required by the changes
  //to DynamicTypeMember::equals.
  DynamicTypePtrPairSeen dt_ptr_pair;
  return test_equality(this, other, dt_ptr_pair);
}

void DynamicTypeImpl::insert_dynamic_member(DDS::DynamicTypeMember_ptr dtm)
{
  DDS::MemberDescriptor_var d;
  if (dtm->get_descriptor(d) != DDS::RETCODE_OK) {
    throw std::runtime_error("Could not get type descriptor of dynamic member");
  }

  members_by_index_.push_back(DDS::DynamicTypeMember::_duplicate(dtm));

  if (d->id() != MEMBER_ID_INVALID) {
    members_by_id_.insert(std::make_pair(d->id(), DDS::DynamicTypeMember::_duplicate(dtm)));
  }
  members_by_name_.insert(std::make_pair(d->name(), DDS::DynamicTypeMember::_duplicate(dtm)));
}

void DynamicTypeImpl::clear()
{
  members_by_name_.clear();
  members_by_id_.clear();
  members_by_index_.clear();
  descriptor_ = 0;
}

DDS::DynamicType_var get_base_type(DDS::DynamicType_ptr type)
{
  if (!type) {
    return 0;
  }

  DDS::DynamicType_var t = DDS::DynamicType::_duplicate(type);
  if (t->get_kind() != TK_ALIAS) {
    return t;
  }

  DDS::TypeDescriptor_var descriptor;
  if (t->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return 0;
  }

  return get_base_type(descriptor->base_type());
}

bool test_equality(DDS::DynamicType_ptr lhs, DDS::DynamicType_ptr rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs == rhs) {
    return true;
  }

  DynamicTypeImpl* lhs_i = dynamic_cast<DynamicTypeImpl*>(lhs);
  DynamicTypeImpl* rhs_i = dynamic_cast<DynamicTypeImpl*>(rhs);

  if (!lhs_i || !rhs_i) {
    return false;
  }

  const DynamicTypePtrPair this_pair = std::make_pair(lhs, rhs);
  DynamicTypePtrPairSeen::const_iterator have_seen = dt_ptr_pair.find(this_pair);
  if (have_seen == dt_ptr_pair.end()) {
    dt_ptr_pair.insert(this_pair);

    DDS::TypeDescriptor_var lhs_descriptor;
    DDS::TypeDescriptor_var rhs_descriptor;
    if (lhs->get_descriptor(lhs_descriptor) != DDS::RETCODE_OK ||
        rhs->get_descriptor(rhs_descriptor) != DDS::RETCODE_OK) {
      return false;
    }

    return
      test_equality(lhs_descriptor, rhs_descriptor, dt_ptr_pair) &&
      DynamicTypeImpl::test_equality_i(&lhs_i->members_by_name_, &rhs_i->members_by_name_, dt_ptr_pair) &&
      DynamicTypeImpl::test_equality_i(&lhs_i->members_by_id_, &rhs_i->members_by_id_, dt_ptr_pair);
  }

  return true;
}

bool DynamicTypeImpl::test_equality_i(DynamicTypeMembersByName* lhs, DynamicTypeMembersByName* rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs == rhs) {
    return true;
  }

  if (!lhs || !rhs) {
    return false;
  }

  if (lhs->size() != rhs->size()) {
    return false;
  }

  for (DynamicTypeMembersByName::const_iterator lhs_it = lhs->begin(); lhs_it != lhs->end(); ++lhs_it) {
    const DynamicTypeMembersByName::const_iterator rhs_it = rhs->find(lhs_it->first);
    if (rhs_it == rhs->end()) {
      return false;
    }
    DDS::MemberDescriptor_var lhs_md;
    DDS::MemberDescriptor_var rhs_md;
    if (lhs_it->second->get_descriptor(lhs_md) != DDS::RETCODE_OK ||
        rhs_it->second->get_descriptor(rhs_md) != DDS::RETCODE_OK) {
      return false;
    }
    if (!XTypes::test_equality(lhs_md, rhs_md, dt_ptr_pair)) {
      return false;
    }
  }
  return true;
}

bool DynamicTypeImpl::test_equality_i(DynamicTypeMembersById* lhs, DynamicTypeMembersById* rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
{
  if (lhs == rhs) {
    return true;
  }

  if (!lhs || !rhs) {
    return false;
  }

  if (lhs->size() != rhs->size()) {
    return false;
  }

  for (DynamicTypeMembersById::const_iterator lhs_it = lhs->begin(); lhs_it != lhs->end(); ++lhs_it) {
    const DynamicTypeMembersById::const_iterator rhs_it = rhs->find(lhs_it->first);
    if (rhs_it == rhs->end()) {
      return false;
    }
    DDS::MemberDescriptor_var lhs_md;
    DDS::MemberDescriptor_var rhs_md;
    if (lhs_it->second->get_descriptor(lhs_md) != DDS::RETCODE_OK ||
        rhs_it->second->get_descriptor(rhs_md) != DDS::RETCODE_OK) {
      return false;
    }
    if (!XTypes::test_equality(lhs_md, rhs_md, dt_ptr_pair)) {
      return false;
    }
  }
  return true;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
