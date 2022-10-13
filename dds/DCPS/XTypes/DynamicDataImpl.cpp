/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#include "DynamicDataImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicDataImpl::DynamicDataImpl(DDS::DynamicType_ptr type)
  : type_(get_base_type(type))
  , container_(type_)
{}

DDS::DynamicType_ptr DynamicDataImpl::type()
{
  return type_.in();
}

DDS::ReturnCode_t DynamicDataImpl::get_descriptor(DDS::MemberDescriptor*& value, MemberId id)
{
  // TODO: Can use the same implementation as the reading class.
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::set_descriptor(MemberId, DDS::MemberDescriptor*)
{
  return DDS::RETCODE_UNSUPPORTED;
}

CORBA::Boolean DynamicDataImpl::equals(DDS::DynamicData_ptr other)
{
  // FUTURE: Implement this.
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataImpl::equals: Not implemented\n"));
  return false;
}

MemberId DynamicDataImpl::get_member_id_by_name(const char* name)
{
  // TODO: Can use the same implementation as the reading class.
  return 0;
}

MemberId DynamicDataImpl::get_member_id_at_index(ACE_CDR::ULong index)
{
  // TODO: Not sure if this should be supported for the writing direction.
  // The index is counted from a complete data sample which we may not have
  // yet when this is called.
  return 0;
}

ACE_CDR::ULong DynamicDataImpl::get_item_count()
{
  // TODO: Return the number of items from the internal data container.
  return 0;
}

DDS::ReturnCode_t DynamicDataImpl::clear_all_values()
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::clear_nonkey_values()
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::clear_value(DDS::MemberId /*id*/)
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::DynamicData_ptr DynamicDataImpl::loan_value(DDS::MemberId /*id*/)
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataImpl::loan_value: Not implemented\n"));
  return 0;
}

DDS::ReturnCode_t DynamicDataImpl::return_loaned_value(DDS::DynamicData_ptr /*value*/)
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataImpl::return_loaned_value: Not implemented\n"));
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::DynamicData_ptr DynamicDataImpl::clone()
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataImpl::clone: Not implemented\n"));
  return 0;
}

template<typename SingleType>
bool DynamicDataImpl::insert_single(DDS::MemberId id, const SingleType& value)
{
  // The same member might be already written to complex_map_.
  // Make sure there is only one entry for each member.
  if (container_.complex_map_.count(id) > 0) {
    container_.complex_map_.erase(id);
  }
  return container_.single_map_.insert(make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_complex(DDS::MemberId id, const DDS::DynamicData_var& value)
{
  if (container_.single_map_.count(id) > 0) {
    container_.single_map_.erase(id);
  } else if (container_.sequence_map_.count(id) > 0) {
    container_.sequence_map_.erase(id);
  }
  return container_.complex_map_.insert(make_pair(id, value)).second;
}

// Set a member with the given ID in a struct. The member must have type MemberTypeKind or
// enum/bitmask. In the latter case, its bit bound must be in the range [lower, upper].
template<TypeKind MemberTypeKind, typename MemberType>
bool DynamicDataImpl::set_value_to_struct(DDS::MemberId id, const MemberType& value,
                                               TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, id) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::MemberDescriptor_var md;
  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var member_type = get_base_type(md->type());
  const TypeKind member_tk = member_type->get_kind();

  if (member_tk != MemberTypeKind && member_tk != enum_or_bitmask) {
    return false;
  }

  if (member_tk == enum_or_bitmask) {
    DDS::TypeDescriptor_var member_td;
    if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::ULong bit_bound = member_td->bound()[0];
    if (bit_bound < lower || bit_bound > upper) {
      return false;
    }
  }

  return insert_single(id, value);
}

bool DynamicDataImpl::is_discriminator_type(TypeKind tk) const
{
  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_CHAR8:
  case TK_CHAR16:
  case TK_INT8:
  case TK_UINT8:
  case TK_INT16:
  case TK_UINT16:
  case TK_INT32:
  case TK_UINT32:
  case TK_INT64:
  case TK_UINT64:
  case TK_ENUM:
    return true;
  default:
    return false;
  }
}

// Return true if a discriminator value selects the default member of a union.
bool DynamicDataImpl::is_default_member_selected(CORBA::ULong disc_val, DDS::MemberId default_id) const
{
  if (type_->get_kind() != TK_UNION) {
    return false;
  }

  DDS::DynamicTypeMembersById_var members_var;
  if (type_->get_all_members(members_var) != DDS::RETCODE_OK) {
    return false;
  }
  DynamicTypeMembersByIdImpl* members = dynamic_cast<DynamicTypeMembersByIdImpl*>(members_var.in());

  for (DynamicTypeMembersByIdImpl::const_iterator it = members->begin(); it != members->end(); ++it) {
    if (it->first == default_id) continue;

    DDS::MemberDescriptor_var md;
    if (it->second->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }
    const DDS::UnionCaseLabelSeq& labels = md->label();
    for (CORBA::ULong i = 0; i < labels.length(); ++i) {
      if (disc_val == labels[i]) {
        return false;
      }
    }
  }
  return true;
}

// Read a discriminator value from a DynamicData that represents it.
bool DynamicDataImpl::read_discriminator(CORBA::Long& disc_val) const
{
  if (!is_discriminator_type(type_->get_kind())) {
    return false;
  }

  DataContainer::const_single_iterator found = container_.single_map_.find(MEMBER_ID_INVALID);
  if (found == container_.single_map_.end()) {
    return false;
  }

  disc_val = static_cast<CORBA::Long>(found->second.get());
  return true;
}

// If a selected member of a union is already written, return its ID.
// Should only be called for union.
DDS::MemberId DynamicDataImpl::find_selected_member() const
{
  // There can be at most 2 entries in total in all three maps,
  // one for the discriminator, one for a selected member.
  for (DataContainer::const_single_iterator single_it = container_.single_map_.begin();
       single_it != container_.single_map_.end(); ++single_it) {
    if (single_it->first != DISCRIMINATOR_ID) {
      return single_it->first;
    }
  }

  // If there is any entry in sequence_map_, that must be for a selected member
  // since discriminator cannot be sequence.
  if (container_.sequence_map_.size() > 0) {
    OPENDDS_ASSERT(container_.sequence_map_.size() == 1);
    return container_.sequence_map_.begin()->first;
  }

  for (DataContainer::const_complex_iterator cmpl_it = container_.complex_map_.begin();
       cmpl_it != container_.complex_map_.end(); ++cmpl_it) {
    if (cmpl_it->first != DISCRIMINATOR_ID) {
      return cmpl_it->first;
    }
  }

  // There was no selected member written.
  return MEMBER_ID_INVALID;
}

// Check if a discriminator value would select a member with the given descriptor in a union.
bool DynamicDataImpl::validate_discriminator(CORBA::Long disc_val,
                                                  const DDS::MemberDescriptor_var& md) const
{
  // If the selected member is not default, the discriminator value must equal one of its
  // labels. If the selected member is default, the discriminator value must not equal
  // any label of the non-default members.
  if (!md->is_default_label()) {
    const DDS::UnionCaseLabelSeq& labels = md->label();
    bool found = false;
    for (CORBA::ULong i = 0; !found && i < labels.length(); ++i) {
      if (disc_val == labels[i]) {
        found = true;
      }
    }
    if (!found) {
      return false;
    }
  } else if (!is_default_member_selected(disc_val, md->id())) {
    return false;
  }
  return true;
}

bool DynamicDataImpl::find_selected_member_and_discriminator(DDS::MemberId& selected_id,
                                                                  bool& has_disc,
                                                                  CORBA::Long& disc_val) const
{
  for (DataContainer::const_single_iterator single_it = container_.single_map_.begin();
       single_it != container_.single_map_.end(); ++single_it) {
    if (single_it->first == DISCRIMINATOR_ID) {
      has_disc = true;
      disc_val = static_cast<CORBA::Long>(single_it->second.get());
    } else {
      selected_id = single_it->first;
    }
  }

  if (selected_id == MEMBER_ID_INVALID && container_.sequence_map_.size() > 0) {
    OPENDDS_ASSERT(container_.sequence_map_.size() == 1);
    selected_id = container_.sequence_map_.begin()->first;
  }

  if (selected_id == MEMBER_ID_INVALID || !has_disc) {
    for (DataContainer::const_complex_iterator cmpl_it = container_.complex_map_.begin();
         cmpl_it != container_.complex_map_.end(); ++cmpl_it) {
      if (cmpl_it->first == DISCRIMINATOR_ID) {
        has_disc = true;
        if (!cmpl_it->second->read_discriminator(disc_val)) {
          return false;
        }
      } else {
        selected_id = cmpl_it->first;
      }
    }
  }
  return true;
}

template<TypeKind MemberTypeKind, typename MemberType>
bool DynamicDataImpl::set_value_to_union(DDS::MemberId id, const MemberType& value,
                                              TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::DynamicType_var member_type;
  if (id == DISCRIMINATOR_ID) {
    // Discriminator can only be of certain types (XTypes spec, 7.2.2.4.4.3)
    if (!is_discriminator_type(MemberTypeKind)) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DynamicDataImpl::set_value_to_union: Type %C cannot be used for union discriminator\n",
                 typekind_to_string(MemberTypeKind)));
      return false;
    }

    member_type = get_base_type(descriptor->discriminator_type());

    const TypeKind member_tk = member_type->get_kind();
    if (member_tk != MemberTypeKind && member_tk != enum_or_bitmask) {
      return false;
    }

    if (member_tk == enum_or_bitmask) {
      DDS::TypeDescriptor_var member_td;
      if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
        return false;
      }
      const CORBA::ULong bit_bound = member_td->bound()[0];
      if (bit_bound < lower || bit_bound > upper) {
        return false;
      }
    }

    const DDS::MemberId selected_id = find_selected_member();

    if (selected_id != MEMBER_ID_INVALID) {
      DDS::DynamicTypeMember_var selected_member;
      if (type_->get_member(selected_member, selected_id) != DDS::RETCODE_OK) {
        return false;
      }
      DDS::MemberDescriptor_var selected_md;
      if (selected_member->get_descriptor(selected_md) != DDS::RETCODE_OK) {
        return false;
      }

      if (!validate_discriminator(static_cast<CORBA::Long>(value), selected_md)) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_value_to_union:"
                   " Discriminator value %d does not select existing selected member (ID %d)",
                   static_cast<CORBA::Long>(value), selected_id));
        return false;
      }
    }

  } else { // Writing a selected member
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }
    member_type = get_base_type(md->type());

    const TypeKind member_tk = member_type->get_kind();
    if (member_tk != MemberTypeKind && member_tk != enum_or_bitmask) {
      return false;
    }

    if (member_tk == enum_or_bitmask) {
      DDS::TypeDescriptor_var member_td;
      if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
        return false;
      }
      const CORBA::ULong bit_bound = member_td->bound()[0];
      if (bit_bound < lower || bit_bound > upper) {
        return false;
      }
    }

    DDS::MemberId selected_id = MEMBER_ID_INVALID;
    bool has_disc = false;
    CORBA::Long disc_val;
    if (!find_selected_member_and_discriminator(selected_id, has_disc, disc_val)) {
      return false;
    }

    // Prohibit writing another member if a member was already written.
    // Overwrite the same member is allowed.
    if (selected_id != MEMBER_ID_INVALID && selected_id != id) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_value_to_union:"
                   " Already had an active member (%d)\n", selected_id));
      }
      return false;
    }

    if (selected_id == MEMBER_ID_INVALID && has_disc && !validate_discriminator(disc_val, md)) {
      return false;
    }
  }

  return insert_single(id, value);
}

// Check if a given member ID is valid for a given type with maximum number of elements.
bool DynamicDataImpl::check_index_from_id(TypeKind tk, DDS::MemberId id, CORBA::ULong bound) const
{
  // The given ID is implicitly treated as index.
  switch (tk) {
  case TK_STRING8:
  case TK_STRING16:
  case TK_SEQUENCE:
  case TK_MAP:
    // Bound of 0 means unbounded.
    if (bound == 0 || id < bound) {
      return true;
    }
    break;
  case TK_BITMASK:
  case TK_ARRAY:
    if (id < bound) {
      return true;
    }
    break;
  }

  return false;
}

template<TypeKind ElementTypeKind, typename ElementType>
bool DynamicDataImpl::set_value_to_collection(DDS::MemberId id, const ElementType& value,
                                                   TypeKind collection_tk, TypeKind enum_or_bitmask,
                                                   LBound lower, LBound upper)
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const TypeKind elem_tk = elem_type->get_kind();

  if (elem_tk != ElementTypeKind && elem_tk != enum_or_bitmask) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::set_value_to_collection:")
                 ACE_TEXT(" Could not write a value of type %C to %C with element type %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(collection_tk),
                 typekind_to_string(elem_tk)));
    }
    return false;
  }

  if (elem_tk == enum_or_bitmask) {
    DDS::TypeDescriptor_var elem_td;
    if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::ULong bit_bound = elem_td->bound()[0];
    if (bit_bound < lower || bit_bound > upper) {
      return false;
    }
  }

  return validate_member_id_collection(descriptor, id, collection_tk) && insert_single(id, value);
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::set_single_value(DDS::MemberId id, const ValueType& value,
                                                         TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (!is_type_supported(ValueTypeKind, "set_single_value")) {
    return DDS::RETCODE_ERROR;
  }

  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  const TypeKind tk = type_->get_kind();
  bool good = true;

  if (tk == enum_or_bitmask) {
    const CORBA::ULong bit_bound = descriptor->bound()[0];
    good = id == MEMBER_ID_INVALID && bit_bound >= lower && bit_bound <= upper &&
      container_.single_map_.insert(make_pair(id, value)).second;
  } else {
    switch (tk) {
    case ValueTypeKind:
      good = is_primitive(tk) && id == MEMBER_ID_INVALID &&
        container_.single_map_.insert(make_pair(id, value)).second;
      break;
    case TK_STRUCTURE:
      good = set_value_to_struct<ValueTypeKind>(id, value, enum_or_bitmask, lower, upper);
      break;
    case TK_UNION:
      good = set_value_to_union<ValueTypeKind>(id, value, enum_or_bitmask, lower, upper);
      break;
    case TK_SEQUENCE:
    case TK_ARRAY:
    case TK_MAP:
      good = set_value_to_collection<ValueTypeKind>(id, value, tk, enum_or_bitmask, lower, upper);
      break;
    default:
      good = false;
      break;
    }
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataImpl::set_single_value: "
               "Failed to write a value of %C to DynamicData object of type %C\n",
               typekind_to_string(ValueTypeKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::set_int32_value(DDS::MemberId id, CORBA::Long value)
{
  return set_single_value<TK_INT32>(id, value, TK_ENUM, 17, 32);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint32_value(DDS::MemberId id, CORBA::ULong value)
{
  return set_single_value<TK_UINT32>(id, value, TK_BITMASK, 17, 32);
}

DDS::ReturnCode_t DynamicDataImpl::set_int8_value(DDS::MemberId id, CORBA::Int8 value)
{
  return set_single_value<TK_INT8>(id, value, TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint8_value(DDS::MemberId id, CORBA::UInt8 value)
{
  return set_single_value<TK_UINT8>(id, value, TK_BITMASK, 1, 8);
}

DDS::ReturnCode_t DynamicDataImpl::set_int16_value(DDS::MemberId id, CORBA::Short value)
{
  return set_single_value<TK_INT16>(id, value, TK_ENUM, 9, 16);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint16_value(DDS::MemberId id, CORBA::UShort value)
{
  return set_single_value<TK_UINT16>(id, value, TK_BITMASK, 9, 16);
}

DDS::ReturnCode_t DynamicDataImpl::set_int64_value(DDS::MemberId id, CORBA::LongLong value)
{
  return set_single_value<TK_INT64>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint64_value(DDS::MemberId id, CORBA::ULongLong value)
{
  return set_single_value<TK_UINT64>(id, value, TK_BITMASK, 33, 64);
}

DDS::ReturnCode_t DynamicDataImpl::set_float32_value(DDS::MemberId id, CORBA::Float value)
{
  return set_single_value<TK_FLOAT32>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_float64_value(DDS::MemberId id, CORBA::Double value)
{
  return set_single_value<TK_FLOAT64>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_float128_value(DDS::MemberId id, CORBA::LongDouble value)
{
  return set_single_value<TK_FLOAT128>(id, value);
}

template<TypeKind CharKind, TypeKind StringKind, typename CharT>
DDS::ReturnCode_t DynamicDataImpl::set_char_common(DDS::MemberId id, CharT& value)
{
  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case CharKind:
    good = id == MEMBER_ID_INVALID &&
      container_.single_map_.insert(make_pair(id, value)).second;
    break;
  case StringKind:
    {
      DDS::TypeDescriptor_var descriptor;
      if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
        good = false;
        break;
      }
      const CORBA::ULong bound = descriptor->bound()[0];
      if (!check_index_from_id(tk, id, bound)) {
        good = false;
      } else {
        good = container_.single_map_.insert(make_pair(id, value)).second;
      }
      break;
    }
  case TK_STRUCTURE:
    good = set_value_to_struct<CharKind>(id, value);
    break;
  case TK_UNION:
    good = set_value_to_union<CharKind>(id, value);
    break;
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    good = set_value_to_collection<CharKind>(id, value, tk);
    break;
  default:
    good = false;
    break;
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataImpl::set_char_common:"
               " Failed to write DynamicData object of type %C\n", typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::set_char8_value(DDS::MemberId id, CORBA::Char value)
{
  return set_char_common<TK_CHAR8, TK_STRING8>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_char16_value(DDS::MemberId id, CORBA::WChar value)
{
#ifdef DDS_HAS_WCHAR
  return set_char_common<TK_CHAR16, TK_STRING16>(id, value);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataImpl::set_byte_value(DDS::MemberId id, CORBA::Octet value)
{
  return set_single_value<TK_BYTE>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_boolean_value(DDS::MemberId id, CORBA::Boolean value)
{
  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case TK_BOOLEAN:
    good = id == MEMBER_ID_INVALID && container_.single_map_.insert(make_pair(id, value)).second;
    break;
  case TK_BITMASK:
    {
      DDS::TypeDescriptor_var descriptor;
      if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
        good = false;
        break;
      }
      const CORBA::ULong bit_bound = descriptor->bound()[0];
      if (!check_index_from_id(tk, id, bit_bound)) {
        good = false;
      } else {
        good = container_.single_map_.insert(make_pair(id, value)).second;
      }
      break;
    }
  case TK_STRUCTURE:
    good = set_value_to_struct<TK_BOOLEAN>(id, value);
    break;
  case TK_UNION:
    good = set_value_to_union<TK_BOOLEAN>(id, value);
    break;
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    good = set_value_to_collection<TK_BOOLEAN>(id, value, tk);
    break;
  default:
    good = false;
    break;
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataImpl::set_boolean_value:"
               " Failed to write boolean to DynamicData object of type %C\n", typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::set_string_value(DDS::MemberId id, const char* value)
{
  return set_single_value<TK_STRING8>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_wstring_value(DDS::MemberId id, const CORBA::WChar* value)
{
#ifdef DDS_HAS_WCHAR
  return set_single_value<TK_STRING16>(id, value);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

bool DynamicDataImpl::set_complex_to_struct(DDS::MemberId id, DDS::DynamicData_ptr value)
{
  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, id) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var md;
  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var member_type = get_base_type(md->type());
  const DDS::DynamicType_ptr value_type = value->type();
  if (!member_type || !value_type || !member_type->equals(value_type)) {
    return false;
  }
  return insert_complex(id, value);
}

bool DynamicDataImpl::set_complex_to_union(DDS::MemberId id, DDS::DynamicData_ptr value)
{
  if (id == DISCRIMINATOR_ID) {
    DDS::DynamicType_var disc_type = get_base_type(descriptor->discriminator_type());
    if (!disc_type->equals(value->type())) {
      return false;
    }

    const DDS::MemberId selected_id = find_selected_member();
    DDS::DynamicTypeMember_var selected_member;
    if (type_->get_member(selected_member, selected_id) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var selected_md;
    if (member->get_descriptor(selected_md) != DDS::RETCODE_OK) {
      return false;
    }

    CORBA::ULong disc_val;
    if (!value->read_discriminator(disc_val)) {
      return false;
    }

    if (selected_id != MEMBER_ID_INVALID && !validate_discriminator(disc_val, selected_md)) {
      return false;
    }
  } else { // Writing a selected member
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }
    if (get_base_type(md->type())->equals(value->type())) {
      return false;
    }

    DDS::MemberId selected_id = MEMBER_ID_INVALID;
    bool has_disc = false;
    CORBA::Long disc_val;
    if (!find_selected_member_and_discriminator(selected_id, has_disc, disc_val)) {
      return false;
    }
    if (selected_id != MEMBER_ID_INVALID && selected_id != id) {
      return false;
    }

    if (selected_id == MEMBER_ID_INVALID && has_disc && !validate_discriminator(disc_val, md)) {
      return false;
    }
  }
  return insert_complex(id, value);
}

bool DynamicDataImpl::validate_member_id_collection(const DDS::TypeDescriptor_var& descriptor,
                                                         DDS::MemberId id, TypeKind tk) const
{
  switch (tk) {
  case TK_SEQUENCE:
    {
      const CORBA::ULong bound = descriptor->bound()[0];
      return check_index_from_id(tk, id, bound);
    }
  case TK_ARRAY:
    {
      CORBA::ULong bound = 1;
      for (CORBA::ULong i = 0; i < descriptor->bound().length(); ++i) {
        bound *= descriptor->bound()[i];
      }
      return check_index_from_id(tk, id, bound);
    }
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::validate_member_id_collection::"
                 " Map is currently not supported\n"));
    }
  }
  return false;
}

bool DynamicDataImpl::set_complex_to_collection(DDS::MemberId id, DDS::DynamicData_ptr value, TypeKind collection_tk)
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  if (!elem_type->equals(value->type())) {
    return false;
  }

  return validate_member_id_collection(descriptor, id, collection_tk) && insert_complex(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_complex_value(DDS::MemberId id, DDS::DynamicData_ptr value)
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  const TypeKind tk = type_->get_kind();
  bool good = false;

  switch (tk) {
  case TK_STRUCTURE:
    good = set_complex_to_struct(id, value);
    break;
  case TK_UNION:
    good = set_complex_to_union(id, value);
    break;
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    good = set_complex_to_collection(id, value, tk);
    break;
  default:
    good = false;
    break;
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataImpl::set_complex_value:"
               " Failed to write complex value for member with ID %d\n", id));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

template<typename SequenceType>
bool DynamicDataImpl::insert_sequence(DDS::MemberId id, const SequenceType& value)
{
  if (container_.complex_map_.count(id) > 0) {
    container_.complex_map_.erase(id);
  }
  return container_.sequence_map_.insert(make_pair(id, value)).second;
}

template<TypeKind ElementTypeKind>
bool DynamicDataImpl::check_seqmem_in_struct_union(DDS::MemberId id, TypeKind enum_or_bitmask,
                                                        LBound lower, LBound upper) const
{
  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, id)) {
    return false;
  }
  DDS::MemberDescriptor_var md;
  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var member_type = get_base_type(md->type());
  const TypeKind member_tk = member_type->get_kind();
  if (member_tk != TK_SEQUENCE) {
    return false;
  }

  DDS::TypeDescriptor_var member_td;
  if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var elem_type = get_base_type(member_td->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  if (elem_tk != ElementTypeKind && elem_tk != enum_or_bitmask) {
    return false;
  }

  if (elem_tk == enum_or_bitmask) {
    DDS::TypeDescriptor_var elem_td;
    if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::ULong bit_bound = elem_td->bound()[0];
    if (bit_bound < lower || bit_bound > upper) {
      return false;
    }
  }
  return true;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::set_values_to_struct(DDS::MemberId id, const SequenceType& value,
                                                TypeKind enum_or_bitmask,
                                                LBound lower, LBound upper)
{
  return check_seqmem_in_struct_union<ElementTypeKind>(id, enum_or_bitmask, lower, upper) &&
    insert_sequence(id, value);
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::set_values_to_union(DDS::MemberId id, const SequenceType& value,
                                               TypeKind enum_or_bitmask,
                                               LBound lower, LBound upper)
{
  if (id == DISCRIMINATOR_ID) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_values_to_union:"
                 " Union discriminator cannot be a sequence\n"));
    }
    return false;
  }

  // Check the member type against the input type parameters.
  if (!check_seqmem_in_struct_union<ElementTypeKind>(id, enum_or_bitmask, lower, upper)) {
    return false;
  }

  // If discriminator was already written, make sure it selects the target member.
  DDS::MemberId selected_id = MEMBER_ID_INVALID;
  bool has_disc = false;
  CORBA::Long disc_val;
  if (!find_selected_member_and_discriminator(selected_id, has_disc, disc_val)) {
    return false;
  }

  if (selected_id != MEMBER_ID_INVALID && selected_id != id) {
    return false;
  }

  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, id) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var md;
  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }

  if (selected_id == MEMBER_ID_INVALID && has_disc && !validate_discriminator(disc_val, md)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_values_to_union:"
                 " Already had an active member (%d)\n", selected_id));
    }
    return false;
  }
  return insert_sequence(id, value);
}

bool DynamicDataImpl::check_seqmem_in_sequence_array(DDS::MemberId id, TypeKind enum_or_bitmask,
                                                          LBound lower, LBound upper) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const TypeKind elem_tk = elem_type->get_kind();

  if (elem_tk != TK_SEQUENCE) {
    return false;
  }

  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var nested_elem_type = get_base_type(elem_td->element_type());
  const TypeKind nested_elem_tk = nested_elem_type->get_kind();
  if (nested_elem_tk != ElementTypeKind && nested_elem_tk != enum_or_bitmask) {
    return false;
  }

  if (nested_elem_tk == enum_or_bitmask) {
    DDS::TypeDescriptor_var nested_elem_td;
    if (nested_elem_type->get_descriptor(nested_elem_td) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::ULong bit_bound = nested_elem_td->bound()[0];
    if (bit_bound < lower || bit_bound > upper) {
      return false;
    }
  }
  return true;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::set_values_to_sequence(DDS::MemberId id, const SequenceType& value,
                                                  TypeKind enum_or_bitmask,
                                                  LBound lower, LBound upper)
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  return check_seqmem_in_sequence_array(id, enum_or_bitmask, lower, upper) &&
    validate_member_id_collection(descriptor, id, TK_SEQUENCE) &&
    insert_sequence(id, value);
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::set_values_to_array(DDS::MemberId id, const SequenceType& value,
                                               TypeKind enum_or_bitmask,
                                               LBound lower, LBound upper)
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  return check_seqmem_in_sequence_array(id, enum_or_bitmask, lower, upper) &&
    validate_member_id_collection(descriptor, id, TK_ARRAY) &&
    insert_sequence(id, value);
}

template<TypeKind ElementTypeKind, typename SequenceType>
DDS::ReturnCode_t DynamicDataImpl::set_sequence_values(DDS::MemberId id, const SequenceType& value,
                                                            TypeKind enum_or_bitmask,
                                                            LBound lower, LBound upper)
{
  if (!is_type_supported(ElementTypeKind, "set_sequence_values")) {
    return DDS::RETCODE_ERROR;
  }

  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case TK_STRUCTURE:
    good = set_values_to_struct<ElementTypeKind>(id, value, enum_or_bitmask, lower, upper);
    break;
  case TK_UNION:
    good = set_values_to_union<ElementTypeKind>(id, value, enum_or_bitmask, lower, upper);
    break;
  case TK_SEQUENCE:
    good = set_values_to_sequence<ElementTypeKind>(id, value, enum_or_bitmaks, lower, upper);
    break;
  case TK_ARRAY:
    good = set_values_to_array<ElementTypeKind>(id, value, enum_or_bitmask, lower, upper);
    break;
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_sequence_values:"
                 " Map is currently not supported\n"));
    }
    return DDS::RETCODE_ERROR;
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_sequence_values:"
                 " Write to unsupported type (%C)\n", typekind_to_string(tk)));
    }
    return DDS::RETCODE_ERROR;
  }

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_sequence_values:"
               " Failed to write sequence of %C to member with ID %d\n",
               typekind_to_string(ElementTypeKind), id));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::set_int32_values(DDS::MemberId id, const DDS::Int32Seq& value)
{
  return set_sequence_values<TK_INT32>(id, value, TK_ENUM, 17, 32);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint32_values(DDS::MemberId id, const DDS::UInt32Seq& value)
{
  return set_sequence_values<TK_UINT32>(id, value, TK_BITMASK, 17, 32);
}

DDS::ReturnCode_t DynamicDataImpl::set_int8_values(DDS::MemberId id, const DDS::Int8Seq& value)
{
  return set_sequence_values<TK_INT8>(id, value, TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint8_values(DDS::MemberId id, const DDS::UInt8Seq& value)
{
  return set_sequence_values<TK_UINT8>(id, value, TK_BITMASK, 1, 8);
}

DDS::ReturnCode_t DynamicDataImpl::set_int16_values(DDS::MemberId id, const DDS::Int16Seq& value)
{
  return set_sequence_values<TK_INT16>(id, value, TK_ENUM, 9, 16);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint16_values(DDS::MemberId id, const DDS::UInt16Seq& value)
{
  return set_sequence_values<TK_UINT16>(id, value, TK_BITMASK, 9, 16);
}

DDS::ReturnCode_t DynamicDataImpl::set_int64_values(DDS::MemberId id, const DDS::Int64Seq& value)
{
  return set_sequence_values<TK_INT64>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint64_values(DDS::MemberId id, const DDS::UInt64Seq& value)
{
  return set_sequence_values<TK_UINT64>(id, value, TK_BITMASK, 33, 64);
}

DDS::ReturnCode_t DynamicDataImpl::set_float32_values(DDS::MemberId id, const DDS::Float32Seq& value)
{
  return set_sequence_values<TK_FLOAT32>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_float64_values(DDS::MemberId id, const DDS::Float64Seq& value)
{
  return set_sequence_values<TK_FLOAT64>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_float128_values(DDS::MemberId id, const DDS::Float128Seq& value)
{
  return set_sequence_values<TK_FLOAT128>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_char8_values(DDS::MemberId id, const DDS::CharSeq& value)
{
  return set_sequence_values<TK_CHAR8>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_char16_values(DDS::MemberId id, const DDS::WcharSeq& value)
{
#ifdef DDS_HAS_WCHAR
  return set_sequence_values<TK_CHAR16>(id, value);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataImpl::set_byte_values(DDS::MemberId id, const DDS::ByteSeq& value)
{
  return set_sequence_values<TK_BYTE>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_boolean_values(DDS::MemberId id, const DDS::BooleanSeq& value)
{
  return set_sequence_values<TK_BOOLEAN>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_string_values(DDS::MemberId id, const DDS::StringSeq& value)
{
  return set_sequence_values<TK_STRING8>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_wstring_values(DDS::MemberId id, const DDS::WstringSeq& value)
{
#ifdef DDS_HAS_WCHAR
  return set_sequence_values<TK_STRING16>(id, value);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

bool DynamicDataImpl::is_basic_type(TypeKind tk) const
{
  return is_primitive(tk) || tk == TK_STRING8 || tk == TK_STRING16;
}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Long int32)
  : kind_(TK_INT32), int32_(int32)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::ULong uint32)
  : kind_(TK_UINT32), uint32_(uint32)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Int8 int8)
  : kind_(TK_INT8), int8_(int8)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::UInt8 uint8)
  : kind_(TK_UINT8), uint8_(uint8)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Short int16)
  : kind_(TK_INT16), int16_(int16)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::UShort uint16)
  : kind_(TK_UINT16), uint16_(uint16)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::LongLong int64)
  : kind_(TK_INT64), int64_(int64)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::ULongLong uint64)
  : kind_(TK_UINT64), uint64_(uint64)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Float float32)
  : kind_(TK_FLOAT32), float32_(float32)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Double float64)
  : kind_(TK_FLOAT64), float64_(float64)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::LongDouble float128)
  : kind_(TK_FLOAT128), float128_(float128)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Char char8)
  : kind_(TK_CHAR8), char8_(char8)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Octet byte)
  : kind_(TK_BYTE), byte_(byte)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Boolean boolean)
  : kind_(TK_BOOLEAN), boolean_(boolean)
{}

DynamicDataImpl::SingleValue::SingleValue(const char* str)
  : kind_(TK_STRING8), str_(ACE_OS::strdup(str))
{}

#ifdef DDS_HAS_WCHAR
DynamicDataImpl::SingleValue::SingleValue(CORBA::WChar char16)
  : kind_(TK_CHAR16), char16_(char16)
{}

DynamicDataImpl::SingleValue::SingleValue(const CORBA::WChar* wstr)
  : kind_(TK_STRING16), wstr_(ACE_OS::strdup(wstr))
{}
#endif

DynamicDataImpl::SingleValue::~SingleValue()
{
  if (kind_ == TK_STRING8) {
    ACE_OS::free((void*)str_);
  } else if (kind == TK_STRING16) {
    ACE_OS::free((void*)wstr_);
  }
}

template<> CORBA::Long& DynamicDataImpl::SingleValue::get() const { return int32_; }
template<> CORBA::ULong& DynamicDataImpl::SingleValue::get() const { return uint32_; }
template<> CORBA::Int8& DynamicDataImpl::SingleValue::get() const { return int8_; }
template<> CORBA::UInt8& DynamicDataImpl::SingleValue::get() const { return uint8_; }
template<> CORBA::Short& DynamicDataImpl::SingleValue::get() const { return int16_; }
template<> CORBA::UShort& DynamicDataImpl::SingleValue::get() const { return uint16_; }
template<> CORBA::LongLong& DynamicDataImpl::SingleValue::get() const { return int64_; }
template<> CORBA::ULongLong& DynamicDataImpl::SingleValue::get() const { return uint64_; }
template<> CORBA::Float& DynamicDataImpl::SingleValue::get() const { return float32_; }
template<> CORBA::Double& DynamicDataImpl::SingleValue::get() const { return float64_; }
template<> CORBA::LongDouble& DynamicDataImpl::SingleValue::get() const { return float128_; }
template<> CORBA::Char& DynamicDataImpl::SingleValue::get() const { return char8_; }
template<> CORBA::Octet& DynamicDataImpl::SingleValue::get() const { return byte_; }
template<> CORBA::Boolean& DynamicDataImpl::SingleValue::get() const { return boolean_; }
template<> const char*& DynamicDataImpl::SingleValue::get() const { return str_; }
#ifdef DDS_HAS_WCHAR
template<> CORBA::WChar& DynamicDataImpl::SingleValue::get() const { return char16_; }
template<> const CORBA::WChar*& DynamicDataImpl::SingleValue::get() const { return wstr_; }
#endif

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Int32Seq& int32_seq)
  : elem_kind_(TK_INT32), int32_seq_(int32_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::UInt32Seq& uint32_seq)
  : elem_kind_(TK_UINT32), uint32_seq_(uint32_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Int8Seq& int8_seq)
  : elem_kind_(TK_INT8), int8_seq_(int8_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::UInt8Seq& uint8_seq)
  : elem_kind_(TK_UINT8), uint8_seq_(uint8_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Int16Seq& int16_seq)
  : elem_kind_(TK_INT16), int16_seq_(int16_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::UInt16Seq& uint16_seq)
  : elem_kind_(TK_UINT16), uint16_seq_(uint16_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Int64Seq& int64_seq)
  : elem_kind_(TK_INT64), int64_seq_(int64_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::UInt64Seq& uint64_seq)
  : elem_kind_(TK_UINT64), uint64_seq_(uint64_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Float32Seq& float32_seq)
  : elem_kind_(TK_FLOAT32), float32_seq_(float32_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::DoubleSeq& float64_seq)
  : elem_kind_(TK_FLOAT64), float64_seq_(float64_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::LongDoubleSeq& float128_seq)
  : elem_kind_(TK_FLOAT128), float128_seq_(float128_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::CharSeq& char8_seq)
  : elem_kind_(TK_CHAR8), char8_seq_(char8_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::OctetSeq& byte_seq)
  : elem_kind_(TK_BYTE), byte_seq_(byte_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::BooleanSeq& boolean_seq)
  : elem_kind_(TK_BOOLEAN), boolean_seq_(boolean_seq)
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::StringSeq& str_seq)
  : elem_kind_(TK_STRING8), string_seq_(str_seq)
{}

#ifdef DDS_HAS_WCHAR
DynamicDataImpl::SequenceValue::SequenceValue(const DDS::WCharSeq& char16_seq)
  : elem_kind_(TK_CHAR16), char16_seq_(char16_seq)
{}

// TODO: strdup each wide string?
DynamicDataImpl::SequenceValue::SequenceValue(const DDS::WStringSeq& wstr_seq)
  : elem_kind_(TK_STRING16), wstring_seq_(wstr_seq)
{}
#endif

DynamicDataImpl::SequenceValue::~SequenceValue()
{
  // TODO
}

bool DynamicDataImpl::DataContainer::get_largest_single_index(CORBA::ULong& largest_index) const
{
  // TODO: Make sure all applicable types are here.
  const TypeKind tk = type_->get_kind();
  if (tk != TK_STRING8 && tk != TK_STRING16 && tk != TK_SEQUENCE && tk != TK_ARRAY) {
    return false;
  }

  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const CORBA::ULong bound = descriptor->bound()[0];

  // Since ID is used as index, the end element is used.
  // In a different ID-to-index mapping, it may need to loop
  // through the map to find the largest index.
  return get_index_from_id(single_map_.end()->first, largest_index, bound);
}

bool DynamicDataImpl::DataContainer::get_largest_sequence_index(CORBA::ULong& largest_index) const
{
  const TypeKind tk = type_->get_kind();
  if (tk != TK_SEQUENCE && tk != TK_ARRAY) {
    return false;
  }

  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const CORBA::ULong bound = descriptor->bound()[0];
  return get_index_from_id(sequence_map_.end()->first, largest_index, bound)
}

bool DynamicDataImpl::DataContainer::get_largest_index_basic(CORBA::ULong& largest_index) const
{
  largest_index = 0;
  if (!data.container_.single_map_.empty()) {
    CORBA::ULong max_index;
    if (!data.container_.get_largest_single_index(max_index)) {
      return false;
    }
    largest_index = max_index;
  }

  if (!data.container_.complex_map_.empty()) {
    CORBA::ULong max_index;
    if (!data.container_.get_largest_complex_index(max_index)) {
      return false;
    }
    largest_index = max(max_index, largest_index);
  }
  return true;
}

bool DynamicDataImpl::DataContainer::get_largest_index_basic_sequence(CORBA::ULong& largest_index) const
{
  // TODO
}

bool DynamicDataImpl::DataContainer::get_largest_index_complex(CORBA::ULong& largest_index) const
{
  const TypeKind tk = type_->get_kind();
  if (tk != TK_STRING8 && tk != TK_STRING16 && tk != TK_SEQUENCE && tk != TK_ARRAY) {
    return false;
  }

  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const CORBA::ULong bound = descriptor->bound()[0];
  return get_index_from_id(complex_map_.end()->first, largest_index, bound);
}

template<typename ValueType>
bool DynamicDataImpl::DataContainer::set_default_value_basic(ValueType& value, TypeKind kind) const
{
  // Set default value for a basic type according to Table 9.
  // When default_value in MemberDescriptor is fully supported, it would
  // have precedence over these default values.
  switch (kind) {
  case TK_INT32:
  case TK_UINT32:
  case TK_INT8:
  case TK_UINT8:
  case TK_INT16:
  case TK_UINT16:
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
    value = 0
    return true;
  case TK_CHAR8:
    value = '\0';
    return true;
  case TK_BYTE:
    value = 0x00;
    return true;
  case TK_STRING8:
    value = "";
    return true;
  case TK_BOOLEAN:
    value = false;
    return true;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    value = '\0';
    return true;
  case TK_STRING16:
    value = "";
    return true;
#endif
  }
  return false;
}

template<typename SequenceType>
void DynamicDataImpl::DataContainer::set_default_values(SequenceType& seq, TypeKind elem_tk) const
{
  for (CORBA::ULong i = 0; i < seq.length(); ++i) {
    set_default_value_basic(seq[i], elem_tk);
  }
}

template<typename SequenceType>
bool DynamicDataImpl::DataContainer::reconstruct_basic_sequence(SequenceType& seq,
  TypeKind elem_tk, CORBA::ULong size, CORBA::ULong bound) const
{
  seq.length(size);
  set_default_values(seq, elem_tk);

  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    seq[index] = it->second.get();
  }

  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    seq[index] = it->second.container_.single_map_.at(MEMBER_ID_INVALID);
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialize_basic_sequence(DCPS::Serializer& ser,
  TypeKind elem_tk, CORBA::ULong size, CORBA::ULong bound) const
{
  switch (elem_tk) {
  case TK_INT32: {
    DDS::Int32Seq int32seq;
    return reconstruct_basic_sequence(int32seq, elem_tk, size, bound) && (ser << int32seq);
  }
  case TK_UINT32: {
    DDS::UInt32Seq uint32seq;
    return reconstruct_basic_sequence(uint32seq, elem_tk, size, bound) && (ser << uint32seq);
  }
  case TK_INT8: {
    DDS::Int8Seq int8seq;
    return reconstruct_basic_sequence(int8seq, elem_tk, size, bound) && (ser << int8seq);
  }
  case TK_UINT8: {
    DDS::UInt8Seq uint8seq;
    return reconstruct_basic_sequence(uint8seq, elem_tk, size, bound) && (ser << uint8seq);
  }
  case TK_INT16: {
    DDS::Int16Seq int16seq;
    return reconstruct_basic_sequence(int16seq, elem_tk, size, bound) && (ser << int16seq);
  }
  case TK_UINT16: {
    DDS::UInt16Seq uint16seq;
    return reconstruct_basic_sequence(uint16seq, elem_tk, size, bound) && (ser << uint16seq);
  }
  case TK_INT64: {
    DDS::Int64Seq int64seq;
    return reconstruct_basic_sequence(int64seq, elem_tk, size, bound) && (ser << int64seq);
  }
  case TK_UINT64: {
    DDS::UInt64Seq uint64seq;
    return reconstruct_basic_sequence(uint64seq, elem_tk, size, bound) && (ser << uint64seq);
  }
  case TK_FLOAT32: {
    DDS::Float32Seq float32seq;
    return reconstruct_basic_sequence(float32seq, elem_tk, size, bound) && (ser << float32seq);
  }
  case TK_FLOAT64: {
    DDS::Float64Seq float64seq;
    return reconstruct_basic_sequence(float64seq, elem_tk, size, bound) && (ser << float64seq);
  }
  case TK_FLOAT128: {
    DDS::Float128Seq float128seq;
    return reconstruct_basic_sequence(float128seq, elem_tk, size, bound) && (ser << float128seq);
  }
  case TK_CHAR8: {
    DDS::CharSeq charseq;
    return reconstruct_basic_sequence(charseq, elem_tk, size, bound) && (ser << charseq);
  }
  case TK_STRING8: {
    DDS::StringSeq strseq;
    return reconstruct_basic_sequence(strseq, elem_tk, size, bound) && (ser << strseq);
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    DDS::WCharSeq wcharseq;
    return reconstruct_basic_sequence(wcharseq, elem_tk, size, bound) && (ser << wcharseq);
  }
  case TK_STRING16: {
    DDS::WStringSeq wstrseq;
    return reconstruct_basic_sequence(wstrseq, elem_tk, size, bound) && (ser << wstrseq);
  }
#endif
  case TK_BYTE: {
    DDS::OctetSeq byteseq;
    return reconstruct_basic_sequence(byteseq, elem_tk, size, bound) && (ser << byteseq);
  }
  case TK_BOOLEAN: {
    DDS::BooleanSeq boolseq;
    return reconstruct_basic_sequence(boolseq, elem_tk, size, bound) && (ser << boolseq);
  }
  }
  return false;
}

template<SequenceType>
bool DynamicDataImpl::DataContainer::reconstruct_enum_sequence(SequenceType& seq,
  CORBA::ULong size, CORBA::ULong bound) const
{
  // TODO:
}

// Serialized size of a enum sequence represented as a sequence of Int8 or Int16 or Int32.

void DynamicDataImpl::DataContainer::serialized_size_enum_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::Int8Seq& seq) const
{
  // TODO:
}

void DynamicDataImpl::DataContainer::serialized_size_enum_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::Int16Seq& seq) const
{
  // TODO:
}

void DynamicDataImpl::DataContainer::serialized_size_enum_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::Int32Seq& seq) const
{
  // TODO:
}

bool DynamicDataImpl::DataContainer::serialize_enum_sequence_as_int8s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::Int8Seq enumseq;
  if (!reconstruct_enum_sequence(enumseq, TK_INT8, size, bound)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    serialized_size(encoding, total_size, enumseq);
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  const CORBA::ULong length = enumseq.length();
  if (!(ser << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return ser.write_int8_array(enumseq.get_buffer(), length);
}

bool DynamicDataImpl::DataContainer::serialize_enum_sequence_as_int16s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::Int16Seq enumseq;
  if (!reconstruct_enum_sequence(enumseq, TK_INT16, size, bound)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    serialized_size(encoding, total_size, enumseq);
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  const CORBA::ULong length = enumseq.length();
  if (!(ser << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return ser.write_short_array(enumseq.get_buffer(), length);
}

bool DynamicDataImpl::DataContainer::serialize_enum_sequence_as_int32s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::Int32Seq enumseq;
  if (!reconstruct_enum_sequence(enumseq, TK_INT32, size, bound)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    serialized_size(encoding, total_size, enumseq);
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  const CORBA::ULong length = enumseq.length();
  if (!(ser << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return ser.write_long_array(enumseq.get_buffer(), length);
}

bool DynamicDataImpl::DataContainer::serialize_enum_sequence(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bitbound, CORBA::ULong seqbound) const
{
  if (bitbound >= 1 && bitbound <= 8) {
    return serialize_enum_sequence_as_int8s(ser, size, bitbound, seqbound);
  } else if (bitbound >= 9 && bitbound <= 16) {
    return serialize_enum_sequence_as_int16s(ser, size, bitbound, seqbound);
  } else if (bitbound >= 17 && bitbound <= 32) {
    return serialize_enum_sequence_as_int32s(ser, size, bitbound, seqbound);
  }
  return false;
}

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence(DCPS::Serializer& ser,
  CORBA::ULong largest_index, CORBA::ULong bitbound) const
{
    // TODO: use unsigned int type corresponding to the bitbound
}

} // namespace XTypes

namespace DCPS {

bool operator<<(Serializer& ser, const XTypes::DynamicDataImpl::SingleValue& value)
{
  switch (value.kind_) {
  case TK_INT32:
  case TK_UINT32:
  case TK_INT8:
  case TK_UINT8:
  case TK_INT16:
  case TK_UINT16:
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_CHAR8:
  case TK_STRING8:
  case TK_BYTE:
  case TK_BOOLEAN:
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
  case TK_STRING16:
#endif
    return ser << value.get();
  }
  return false;
}

bool operator<<(Serializer& ser, const XTypes::DynamicDataImpl& data)
{
  // TODO: Serialize depending on the type kind of dd's type.
  // 1. If dd is primitive, the value
  // is only contained in the single_map_ with key MEMBER_ID_INVALID.
  // 2. If dd is string/wstring, the values are characters which can be stored in the single_map_
  // and complex_map_. Traverse the 2 maps to serialize each member. In case a member is stored
  // as its own DynamicData, call operator<< with that DynamicData.
  // 3. If dd is a struct, if extensibility is appendable or final, serialize members in their order.
  // If extensibility is mutable, the order doesn't matter. If a member is stored in single_map,
  // serialize it is straightforward. If it is stored in sequence_map_, it's also straightforward
  // (refer rules for serialization of sequences). If it's stored in complex_map_, call operator<<
  // with the DynamicData of that object.
  // 4. If dd is a union, it's probably similar.
  // 5. If dd is a sequence, check the element type. If it's basic type, the elements can be stored in
  // the single_map_ and complex_map_. If it's sequence of basic type, it can be stored in
  // sequence_map_ and complex_map_. If it's a more complex type, it is stored in complex_map_.
  // Traverve the correponding maps depending on each case.
  // 6. If dd is an array, it's probably the same.
  //
  // Question: how does it handle an member which is not set yet? Should it return an error or
  // should it use a default value for the member's type?
  const TypeKind tk = data.type_->get_kind();
  switch (tk) {
  case TK_INT32:
  case TK_UINT32:
  case TK_INT8:
  case TK_UINT8:
  case TK_INT16:
  case TK_UINT16:
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_CHAR8:
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
#endif
  case TK_BYTE:
  case TK_BOOLEAN:
  case TK_ENUM:
  case TK_BITMASK:
    {
      DynamicDataImpl::DataContainer::const_single_iterator it =
        data.container_single_map_.find(MEMBER_ID_INVALID);
      if (it != data.container_.single_map_.end()) {
        return ser << it->second;
      }
    }
  case TK_STRING8:
    {
      const bool is_empty = data.container_.single_map_.empty() &&
        data.container_.complex_map_.empty();

      if (!is_empty) {
        CORBA::ULong largest_index;
        if (!data.container_.get_largest_index_basic(largest_index)) {
          return false;
        }
        CORBA::Char str[largest_index + 2] = ""; // Include NUL.
        for (DynamicDataImpl::DataContainer::const_single_iterator it = data.container_.single_map_.begin();
             it != data.container_.single_map_.end(); ++it) {
          CORBA::ULong index;
          if (!get_index_from_id(it->first, index, bound)) {
            return false;
          }
          str[index] = it->second.get();
        }
        for (DynamicDataImpl::DataContainer::const_complex_iterator it = data.container_.complex_map_.begin();
             it != data.container_.complex_map_.end(); ++it) {
          CORBA::ULong index;
          if (!get_index_from_id(it->first, index, bound)) {
            return false;
          }
          str[index] = it->second.container_.single_map_.at(MEMBER_ID_INVALID);
        }
        return ser << str;
      } else {
        return ser << (CORBA::Char*)0;
      }
    }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
    {
      const bool is_empty = data.container_.single_map_.empty() &&
        data.container_.complex_map_.empty();

      if (!is_empty) {
        CORBA::ULong largest_index;
        if (!data.container_.get_largest_index_basic(largest_index)) {
          return false;
        }
        CORBA::WChar wstr[largest_index + 1] = ""; // Not include NUL.
        for (DynamicDataImpl::DataContainer::const_single_iterator it = data.container_.single_map_.begin();
             it != data.container_.single_map_.end(); ++it) {
          CORBA::ULong index;
          if (!get_index_from_id(it->first, index, bound)) {
            return false;
          }
          wstr[index] = it->second.get();
        }
        for (DynamicDataImpl::DataContainer::const_complex_iterator it = data.container_.complex_map_.begin();
             it != data.container_.complex_map_.end(); ++it) {
          CORBA::ULong index;
          if (!get_index_from_id(it->first, index, bound)) {
            return false;
          }
          wstr[index] = it->second.container_.single_map_.at(MEMBER_ID_INVALID);
        }
        return ser << wstr;
      } else {
        return ser << (CORBA::Char*)0;
      }
    }
#endif
  case TK_STRUCTURE:
  case TK_UNION:
  case TK_SEQUENCE:
    {
      DDS::TypeDescriptor_var descriptor;
      if (data.type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
        return false;
      }
      const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
      const TypeKind elem_tk = elem_type->get_kind();
      const CORBA::ULong bound = descriptor->bound()[0];

      if (is_basic_type(elem_tk)) {
        const bool is_empty = data.container_.single_map_.empty() &&
          data.container_.complex_map_.empty();
        if (!is_empty) {
          CORBA::ULong largest_index;
          if (!data.container_.get_largest_index_basic(largest_index)) {
            return false;
          }
          return serialize_basic_sequence(ser, elem_tk, largest_index + 1, bound);
        } else {
          return serialize_basic_sequence(ser, elem_tk, 0, bound);
        }
      } else if (elem_tk == TK_ENUM) {
        // TODO
      } else if (elem_tk == TK_BITMASK) {
        // TODO
      } else if (elem_tk == TK_SEQUENCE) {
        DDS::TypeDescriptor_var elem_td;
        if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
          return false;
        }
        const TypeKind nested_elem_tk = get_base_type(elem_td->element_type())->get_kind();
        if (is_basic_type(nested_elem_tk)) {
          // TODO: Elements are stored in sequence_map_.
        }
      }

      // TODO: The remaining case, elements stored in complex_map_.
    }
  case TK_ARRAY:
  case TK_MAP:
  default:
  }
  return false;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
