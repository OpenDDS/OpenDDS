/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicDataWriteImpl::DynamicDataWriteImpl(DDS::DynamicType_ptr type)
  : type_(get_base_type(type))
{}

DDS::DynamicType_ptr DynamicDataWriteImpl::type()
{
  return type_.in();
}

DDS::ReturnCode_t DynamicDataWriteImpl::get_descriptor(DDS::MemberDescriptor*& value, MemberId id)
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_descriptor(MemberId id, DDS::MemberDescriptor* value)
{
  // TODO
}

CORBA::Boolean DynamicDataWriteImpl::equals(DDS::DynamicData_ptr other)
{
  // TODO
}

MemberId DynamicDataWriteImpl::get_member_id_by_name(const char* name)
{
  // TODO
}

MemberId DynamicDataWriteImpl::get_member_id_at_index(ACE_CDR::ULong index)
{
  // TODO
}

ACE_CDR::ULong DynamicDataWriteImpl::get_item_count()
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::clear_all_values()
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::clear_nonkey_values()
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::clear_value(DDS::MemberId /*id*/)
{
  // TODO
}

DDS::DynamicData_ptr DynamicDataWriteImpl::loan_value(DDS::MemberId /*id*/)
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::return_loaned_value(DDS::DynamicData_ptr /*value*/)
{
  // TODO
}

DDS::DynamicData_ptr DynamicDataWriteImpl::clone()
{
  // TODO
}

// Set a member with the given ID in a struct. The member must have type MemberTypeKind or
// enum/bitmask. In the latter case, its bit bound must be in the range [lower, upper].
template<TypeKind MemberTypeKind, typename MemberType>
bool DynamicDataWriteImpl::set_value_to_struct(DDS::MemberId id, const MemberType& value,
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
  if (member_tk == MemberTypeKind) {
    return container_.single_map_.insert(make_pair(id, value)).second;
  }

  if (member_tk == enum_or_bitmask) {
    DDS::TypeDescriptor_var member_td;
    if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::ULong bit_bound = member_td->bound()[0];
    return bit_bound >= lower && bit_bound <= upper &&
      container_.single_map_.insert(make_pair(id, value)).second;
  }

  return false;
}

bool DynamicDataWriteImpl::is_discriminator_type(TypeKind tk) const
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

// Check if a discriminator value selects the default member. Only works for union.
bool DynamicDataWriteImpl::select_default_member(CORBA::ULong disc_val, DDS::MemberId default_id) const
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

template<TypeKind MemberTypeKind, typename MemberType>
bool DynamicDataWriteImpl::set_value_to_union(DDS::MemberId id, const MemberType& value,
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
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DynamicDataWriteImpl::set_value_to_union: Type %C cannot be used for union discriminator\n",
                 typekind_to_string(MemberTypeKind)));
      return false;
    }

    member_type = get_base_type(descriptor->discriminator_type());

    DDS::MemberId selected_id = MEMBER_ID_INVALID;
    if (container_.single_map_.size() > 0) {
      OPENDDS_MAP(DDS::MemberId, SingleValue)::const_iterator it = container_.single_map_.begin();
      for (; it != container_.single_map_.end(); ++it) {
        // Discriminator might already be written.
        if (it->first != DISCRIMINATOR_ID) {
          selected_id = it->first;
        }
      }
    } else if (container_.sequence_map_.size() > 0) {
      selected_id = container_.sequence_map_.begin()->first;
    } else if (container_.complex_map_.size() > 0) {
      selected_id = container_.complex_map_.begin()->first;
    }

    if (selected_id != MEMBER_ID_INVALID) {
      DDS::DynamicTypeMember_var selected_member;
      if (type_->get_member(selected_member, selected_id) != DDS::RETCODE_OK) {
        return false;
      }
      DDS::MemberDescriptor_var selected_md;
      if (selected_member->get_descriptor(selected_md) != DDS::RETCODE_OK) {
        return false;
      }

      // If the selected member is not default, the discriminator value must equal one of its
      // labels. If the selected member is default, the discriminator value must not equal
      // any label of the non-default members.
      const bool is_default = selected_md->is_default_label();
      if (!is_default) {
        const DDS::UnionCaseLabelSeq& labels = selected_md->label();
        bool found = false;
        for (CORBA::ULong i = 0; i < labels.length(); ++i) {
          if (static_cast<CORBA::Long>(value) == labels[i]) {
            found = true;
            break;
          }
        }
        if (!found) {
          ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_value_to_union:"
                     " Discriminator value (%d) does not match any label of an existing"
                     " selected member (ID %d)\n", static_cast<CORBA::Long>(value), selected_id));
          return false;
        }
      } else if (!select_default_member(static_cast<CORBA::Long>(value), selected_id)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImp::set_value_to_union:"
                   " Discriminator value (%d) matches a non-default member (ID %d),"
                   " but the default member (ID %d) was already written\n",
                   static_cast<CORBA::Long>(value), it->first, selected_id));
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

    DDS::MemberId selected_id = MEMBER_ID_INVALID;
    bool has_disc = false;
    CORBA::Long disc_val;
    if (container_.single_map_.size() > 0) {
      OPENDDS_MAP(DDS::MemberId, SingleValue)::const_iterator it = container_.single_map_.begin();
      for (; it != container_.single_map_.end(); ++it) {
        if (it->first == DISCRIMINATOR_ID) {
          has_disc = true;
          disc_val = static_cast<CORBA::Long>(it->second.get());
        } else {
          selected_it = it->first;
        }
      }
    } else if (container_.sequence_map_.size() > 0) {
      selected_id = container_.sequence_map_.begin()->first;
    } else if (container_.complex_map_.size() > 0) {
      selected_id = container_.complex_map_.begin()->first;
    }

    if (selected_id != MEMBER_ID_INVALID && selected_id != id) {
      ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_value_to_union:"
                 " A member (ID %d) was already written\n", selected_id));
      return false;
    }

    if (selected_id == MEMBER_ID_INVALID && has_disc) {
      // If id is a non-default member, disc value must equal to one of its labels.
      // If id a default member, disc value must not equal to any label of the non-default members.
      const bool is_default = md->is_default_label();
      if (!is_default) {
        const DDS::UnionCaseLabelSeq& labels = md->label();
        bool found = false;
        for (CORBA::ULong i = 0; i < labels.length(); ++i) {
          if (labels[i] == disc_val) {
            found = true;
            break;
          }
        }
        if (!found) {
          ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_value_to_union:"
                     " Non-default member (ID %d) does not match the existing discriminator value %d\n",
                     id, disc_val));
          return false;
        }
      } else if (!select_default_member(disc_val, id)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_value_to_union:"
                   " Default member (ID %d) does not match the existing discriminator value %d\n",
                   id, disc_val));
        return false;
      }
    }
  }

  const TypeKind member_tk = member_type->get_kind();
  if (member_tk == MemberTypeKind) {
    return container_.single_map_.insert(make_pair(id, value)).second;
  }

  if (member_tk == enum_or_bitmask) {
    DDS::TypeDescriptor_var member_td;
    if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::ULong bit_bound = member_td->bound()[0];
    return bit_bound >= lower && bit_bound <= upper &&
      container_.single_map_.insert(make_pair(id, value)).second;
  }

  return false;
}

// Check if a given member ID is valid for a given type with maximum number of elements.
bool DynamicDataWriteImpl::check_index_from_id(TypeKind tk, DDS::MemberId id, CORBA::ULong bound) const
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
bool DynamicDataWriteImpl::set_value_to_collection(DDS::MemberId id, const ElementType& value,
                                                   TypeKind collection_tk, TypeKind enum_or_bitmask,
                                                   LBound lower, LBound upper)
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const TypeKind elem_tk = elem_type->get_kind();

  if (elem_tk != ElementTypeKind && elem_tk != enum_or_bitmask) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataWriteImpl::set_value_to_collection:")
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

  switch (collection_tk) {
  case TK_SEQUENCE:
    {
      const CORBA::ULong bound = descriptor->bound()[0];
      if (!check_index_from_id(id, bound)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_value_to_collection:"
                     " Failed to write a member (ID %d) to %C with bound %d\n",
                     id, typekind_to_string(collection_tk), bound));
        }
        return false;
      }
      break;
    }
  case TK_ARRAY:
    {
      CORBA::ULong bound = 1;
      for (CORBA::ULong i = 0; i < descriptor->bound().length(); ++i) {
        bound *= descriptor->bound()[i];
      }
      if (!check_index_from_id(id, bound)) {
        if (DCPS::DCPS_debug_leve >= 1) {
          ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_value_to_collection:"
                     " Failed to write a member (ID %d) to %C with bound %d\n",
                     id, typekind_to_string(collection_tk), bound));
        }
        return false;
      }
      break;
    }
  case TK_MAP:
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_value_to_collection:"
                 " Write to map is not supported\n"));
    }
    return false;
  default:
    return false;
  }

  container_.single_map_.insert(make_pair(id, value));
  return true;
}

// If a DynamicData represents data of a primitive type or enum/bitmask, the data is stored
// in the single_map_ in the container_ where id is MEMBER_ID_INVALID as per the XTypes spec.
// If a DynamicData represents data of a string/wstring, the characters of the string/wstring
// are also stored in the single_map_ with id of each character translated from its index.
// Similarly, if a DynamicData represents data of a sequence of a basic type, its elements are
// also stored in the single_map_ with id tranlated from the element index.
// This template is probably common to all set_*_value methods except for char8, char16, boolean,
// complex.
template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataWriteImpl::set_single_value(DDS::MemberId id, const ValueType& value,
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
      // TODO: Only allow primitive types in this case?
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
    // TODO: Use correct logging level
    ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_single_value: "
               "Failed to write a value of %C to DynamicData object of type %C\n",
               typekind_to_string(ValueTypeKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int32_value(DDS::MemberId id, CORBA::Long value)
{
  return set_single_value<TK_INT32>(id, value, TK_ENUM, 17, 32);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_uint32_value(DDS::MemberId id, CORBA::ULong value)
{
  return set_single_value<TK_UINT32>(id, value, TK_BITMASK, 17, 32);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int8_value(DDS::MemberId id, CORBA::Int8 value)
{
  return set_single_value<TK_INT8>(id, value, TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_uint8_value(DDS::MemberId id, CORBA::UInt8 value)
{
  return set_single_value<TK_UINT8>(id, value, TK_BITMASK, 1, 8);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int16_value(DDS::MemberId id, CORBA::Short value)
{
  return set_single_value<TK_INT16>(id, value, TK_ENUM, 9, 16);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_uint16_value(DDS::MemberId id, CORBA::UShort value)
{
  return set_single_value<TK_UINT16>(id, value, TK_BITMASK, 9, 16);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int64_value(DDS::MemberId id, CORBA::LongLong value)
{
  return set_single_value<TK_INT64>(id, value);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_uint64_value(DDS::MemberId id, CORBA::ULongLong value)
{
  return set_single_value<TK_UINT64>(id, value, TK_BITMASK, 33, 64);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_float32_value(DDS::MemberId id, CORBA::Float value)
{
  return set_single_value<TK_FLOAT32>(id, value);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_float64_value(DDS::MemberId id, CORBA::Double value)
{
  return set_single_value<TK_FLOAT64>(id, value);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_float128_value(DDS::MemberId id, CORBA::LongDouble value)
{
  return set_single_value<TK_FLOAT128>(id, value);
}

template<TypeKind CharKind, TypeKind StringKind, typename CharT>
DDS::ReturnCode_t DynamicDataWriteImpl::set_char_common(DDS::MemberId id, CharT& value)
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
      if (!check_index_from_id(id, bound)) {
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
    ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_char_common:"
               " Failed to write DynamicData object of type %C\n", typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_char8_value(DDS::MemberId id, CORBA::Char value)
{
  return set_char_common<TK_CHAR8, TK_STRING8>(id, value);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_char16_value(DDS::MemberId id, CORBA::WChar value)
{
#ifdef DDS_HAS_WCHAR
  return set_char_common<TK_CHAR16, TK_STRING16>(id, value);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_byte_value(DDS::MemberId id, CORBA::Octet value)
{
  return set_single_value<TK_BYTE>(id, value);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_boolean_value(DDS::MemberId id, CORBA::Boolean value)
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
      if (!check_index_from_id(id, bit_bound)) {
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
    ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_boolean_value:"
               " Failed to write boolean to DynamicData object of type %C\n", typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_string_value(DDS::MemberId id, const char* value)
{
  return set_single_value<TK_STRING8>(id, value);
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_wstring_value(DDS::MemberId id, const CORBA::WChar* value)
{
#ifdef DDS_HAS_WCHAR
  return set_single_value<TK_STRING16>(id, value);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_complex_value(DDS::MemberId id, DDS::DynamicData_ptr value)
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  const TypeKind tk = type_->get_kind();
  bool good = false;

  switch (tk) {
  case TK_STRUCTURE:
    {
      DDS::DynamicTypeMember_var member;
      if (type_->get_member(member, id) != DDS::RETCODE_OK) {
        good = false;
        break;
      }
      DDS::MemberDescriptor_var md;
      if (member->get_descriptor(md) != DDS::RETCODE_OK) {
        good = false;
        break;
      }
      DDS::DynamicType_var member_type = get_base_type(md->type());
      if (!member_type || !value_type || !member_type->equals(value->type())) {
        good = false;
      } else {
        good = container_.complex_map_.insert(make_pair(id, value)).second;
      }
      break;
    }
  case TK_UNION:
    {
      if (id == DISCRIMINATOR_ID) {
        DDS::DynamicType_var disc_type = get_base_type(descriptor->discriminator_type());
        if (!disc_type->equals(value->type())) {
          good = false;
          break;
        }
        // TODO: If a selected member is already written, check that the input disc value matches.
      } else {
        // TODO: If discriminator is already written, check that it matches the member being written.
      }
    }
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
  default:
    good = false;
    break;
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int32_values(DDS::MemberId id, const DDS::Int32Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_uint32_values(DDS::MemberId id, const DDS::UInt32Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int8_values(DDS::MemberId id, const DDS::Int8Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_uint8_values(DDS::MemberId id, const DDS::UInt8Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int16_values(DDS::MemberId id, const DDS::Int16Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_uint16_values(DDS::MemberId id, const DDS::UInt16Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int64_values(DDS::MemberId id, const DDS::Int64Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_uint64_values(DDS::MemberId id, const DDS::UInt64Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_float32_values(DDS::MemberId id, const DDS::Float32Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_float64_values(DDS::MemberId id, const DDS::Float64Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_float128_values(DDS::MemberId id, const DDS::Float128Seq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_char8_values(DDS::MemberId id, const DDS::CharSeq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_char16_values(DDS::MemberId id, const DDS::WcharSeq& value)
{
#ifdef DDS_HAS_WCHAR
  //TODO
  return DDS::RETCODE_OK;
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_byte_values(DDS::MemberId id, const DDS::ByteSeq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_boolean_values(DDS::MemberId id, const DDS::BooleanSeq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_string_values(DDS::MemberId id, const DDS::StringSeq& value)
{
  // TODO
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_wstring_values(DDS::MemberId id, const DDS::WstringSeq& value)
{
#ifdef DDS_HAS_WCHAR
  // TODO
  return DDS::RETCODE_OK;
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DynamicDataWriteImmp::SingleValue::SingleValue(CORBA::Long i32)
  : kind_(TK_INT32), i32_(i32)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::ULong ui32)
  : kind_(TK_UINT32), ui32_(ui32)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::Int8 i8)
  : kind_(TK_INT8), i8_(i8)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::UInt8 ui8)
  : kind_(TK_UINT8), ui8_(ui8)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::Short i16)
  : kind_(TK_INT16), i16_(i16)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::UShort ui16)
  : kind_(TK_UINT16), ui16_(ui16)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::LongLong i64)
  : kind_(TK_INT64), i64_(i64)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::ULongLong ui64)
  : kind_(TK_UINT64), ui64_(ui64)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::Float f32)
  : kind_(TK_FLOAT32), f32_(f32)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::Double f64)
  : kind_(TK_FLOAT64), f64_(f64)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::LongDouble f128)
  : kind_(TK_FLOAT128), f128_(f128)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::Char c8)
  : kind_(TK_CHAR8), c8_(c8)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::Octet byte)
  : kind_(TK_BYTE), byte_(byte)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::Boolean boolean)
  : kind_(TK_BOOLEAN), boolean_(boolean)
{}

DynamicDataWriteImpl::SingleValue::SingleValue(const char* str)
  : kind_(TK_STRING8), str_(ACE_OS::strdup(str))
{}

#ifdef DDS_HAS_WCHAR
DynamicDataWriteImpl::SingleValue::SingleValue(CORBA::WChar c16)
  : kind_(TK_CHAR16), c16_(c16)
{}

// TODO: Does ACE_OS::strdup works with wide string?
DynamicDataWriteImpl::SingleValue::SingleValue(const CORBA::WChar* wstr)
  : kind_(TK_STRING16), wstr_(ACE_OS::strdup(wstr))
{}
#endif

DynamicDataWriteImpl::SingleValue::~SingleValue()
{
  if (kind_ == TK_STRING8 || kind_ == TK_STRING16) {
    // TODO: Does this work for wstring?
    ACE_OS::free((void*)str_);
  }
}

template<> CORBA::Long& DynamicDataWriteImpl::SingleValue::get() const { return i32_; }
template<> CORBA::ULong& DynamicDataWriteImpl::SingleValue::get() const { return ui32_; }
template<> CORBA::Int8& DynamicDataWriteImpl::SingleValue::get() const { return i8_; }
template<> CORBA::UInt8& DynamicDataWriteImpl::SingleValue::get() const { return ui8_; }
template<> CORBA::Short& DynamicDataWriteImpl::SingleValue::get() const { return i16_; }
template<> CORBA::UShort& DynamicDataWriteImpl::SingleValue::get() const { return ui16_; }
template<> CORBA::LongLong& DynamicDataWriteImpl::SingleValue::get() const { return i64_; }
template<> CORBA::ULongLong& DynamicDataWriteImpl::SingleValue::get() const { return ui64_; }
template<> CORBA::Float& DynamicDataWriteImpl::SingleValue::get() const { return f32_; }
template<> CORBA::Double& DynamicDataWriteImpl::SingleValue::get() const { return f64_; }
template<> CORBA::LongDouble& DynamicDataWriteImpl::SingleValue::get() const { return f128_; }
template<> CORBA::Char& DynamicDataWriteImpl::SingleValue::get() const { return c8_; }
template<> CORBA::Octet& DynamicDataWriteImpl::SingleValue::get() const { return byte_; }
template<> CORBA::Boolean& DynamicDataWriteImpl::SingleValue::get() const { return boolean_; }
template<> const char*& DynamicDataWriteImpl::SingleValue::get() const { return str_; }
#ifdef DDS_HAS_WCHAR
template<> CORBA::WChar& DynamicDataWriteImpl::SingleValue::get() const { return c16_; }
template<> const CORBA::WChar*& DynamicDataWriteImpl::SingleValue::get() const { return wstr_; }
#endif

namespace DCPS {

bool operator<<(Serializer& ser, const DynamicDataWriteImpl& dd)
{
  // TODO
  return true;
}

}

} // namespace XTypes
} // namespace OpenDDS


OPENDDS_END_VERSIONED_NAMESPACE_DECL
