/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicDataImpl.h"

#  include "DynamicTypeMemberImpl.h"
#  include "Utils.h"

#  include <dds/DCPS/DisjointSequence.h>
#  include <dds/DCPS/DCPS_Utils.h>

#  include <dds/DdsDynamicDataSeqTypeSupportImpl.h>
#  include <dds/DdsDcpsCoreTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

using DCPS::LogLevel;
using DCPS::log_level;
using DCPS::retcode_to_string;

DynamicDataImpl::DynamicDataImpl(DDS::DynamicType_ptr type,
                                 DDS::DynamicData_ptr backing_store)
  : DynamicDataBase(type)
  , container_(type_, this)
  , backing_store_(DDS::DynamicData::_duplicate(backing_store))
{
}

DynamicDataImpl::DynamicDataImpl(const DynamicDataImpl& other)
  : CORBA::Object()
  , DynamicData()
  , CORBA::LocalObject()
  , DCPS::RcObject()
  , DynamicDataBase(other.type_)
  , container_(other.container_, this)
  , backing_store_(other.backing_store_)
{
}

DDS::ReturnCode_t DynamicDataImpl::set_descriptor(MemberId, DDS::MemberDescriptor*)
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::MemberId DynamicDataImpl::get_member_id_at_index(ACE_CDR::ULong index)
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
#endif
  case TK_ENUM:
    // Value of enum or primitive types can be indicated by Id MEMBER_ID_INVALID
    // or by index 0 (Section 7.5.2.11.1).
    if (index != 0 && log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_member_id_at_index:"
                 " Received invalid index (%u) for type %C\n", index, typekind_to_string(tk)));
    }
    return MEMBER_ID_INVALID;
  case TK_BITMASK:
    // TODO: Bitmask type needs improvement. See comments in set_single_value method.
    return MEMBER_ID_INVALID;
  case TK_STRING8:
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
#endif
  case TK_SEQUENCE: {
    const CORBA::ULong bound = type_desc_->bound()[0];
    if (bound > 0 && index >= bound) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_member_id_at_index:"
                   " Input index (%u) is out-of-bound (bound is %u)\n", index, bound));
      }
      return MEMBER_ID_INVALID;
    }
    return index;
  }
  case TK_ARRAY: {
    const DDS::UInt32 length = bound_total(type_desc_);
    if (index >= length) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_member_id_at_index:"
                   " Input index (%u) is out-of-bound (array length is %u)\n", index, length));
      }
      return MEMBER_ID_INVALID;
    }
    return index;
  }
  case TK_STRUCTURE: {
    // Use the member order defined in the type since it's the order used for serialization.
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member_by_index(dtm, index) != DDS::RETCODE_OK) {
      return MEMBER_ID_INVALID;
    }
    return dtm->get_id();
  }
  case TK_UNION: {
    if (index == 0) {
      return DISCRIMINATOR_ID;
    }

    bool has_selected_branch;
    DDS::MemberDescriptor_var selected_md;
    const DDS::ReturnCode_t rc = get_selected_union_branch(has_selected_branch, selected_md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_member_id_at_index:"
                   " get_selected_union_branch failed: %C\n", retcode_to_string(rc)));
      }
      return MEMBER_ID_INVALID;
    }
    if (index == 1 && has_selected_branch) {
      return selected_md->id();
    }
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_member_id_at_index:"
                 " invalid index: %u\n", index));
    }
    return MEMBER_ID_INVALID;
  }
  }

  if (log_level >= LogLevel::Warning) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_member_id_at_index:"
               " Called on an unexpected type %C\n", typekind_to_string(tk)));
  }
  return MEMBER_ID_INVALID;
}

void DynamicDataImpl::erase_member(DDS::MemberId id)
{
  if (container_.single_map_.erase(id) == 0) {
    if (container_.sequence_map_.erase(id) == 0) {
      container_.complex_map_.erase(id);
    }
  }
}

CORBA::ULong DynamicDataImpl::get_string_item_count() const
{
  CORBA::ULong bs_item_count = 0;
  if (backing_store_) {
    bs_item_count = backing_store_->get_item_count();
  }

  CORBA::ULong container_item_count = 0;
  if (!container_.single_map_.empty() || !container_.complex_map_.empty()) {
    CORBA::ULong largest_index;
    if (!container_.get_largest_index_basic(largest_index)) {
      return 0;
    }
    container_item_count = largest_index + 1;
  }
  return std::max(bs_item_count, container_item_count);
}

CORBA::ULong DynamicDataImpl::get_sequence_item_count() const
{
  CORBA::ULong bs_item_count = 0;
  if (backing_store_) {
    bs_item_count = backing_store_->get_item_count();
  }

  CORBA::ULong container_item_count = 0;
  if (!container_.single_map_.empty()
      || !container_.sequence_map_.empty()
      || !container_.complex_map_.empty()) {
    CORBA::ULong largest_single_index = 0;
    if (!container_.single_map_.empty() &&
        !container_.get_largest_single_index(largest_single_index)) {
      return 0;
    }
    CORBA::ULong largest_sequence_index = 0;
    if (!container_.sequence_map_.empty() &&
        !container_.get_largest_sequence_index(largest_sequence_index)) {
      return 0;
    }
    CORBA::ULong largest_complex_index = 0;
    if (!container_.complex_map_.empty() &&
        !container_.get_largest_complex_index(largest_complex_index)) {
      return 0;
    }
    container_item_count = std::max(largest_single_index,
                                    std::max(largest_sequence_index,
                                             largest_complex_index)) + 1;
  }

  return std::max(bs_item_count, container_item_count);
}

bool DynamicDataImpl::has_member(DDS::MemberId id) const
{
  if (container_.single_map_.find(id) != container_.single_map_.end() ||
      container_.sequence_map_.find(id) != container_.sequence_map_.end() ||
      container_.complex_map_.find(id) != container_.complex_map_.end()) {
    return true;
  }

  DDS::DynamicData_var member_dd;
  return backing_store_ && backing_store_->get_complex_value(member_dd, id) == DDS::RETCODE_OK;
}

ACE_CDR::ULong DynamicDataImpl::get_item_count()
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_CHAR8:
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
#endif
  case TK_ENUM:
    return 1;
  case TK_STRING8:
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
#endif
    return get_string_item_count();
  case TK_SEQUENCE:
    return get_sequence_item_count();
  case TK_BITMASK:
    return static_cast<ACE_CDR::ULong>(container_.single_map_.size() +
                                       container_.complex_map_.size());
  case TK_ARRAY:
    return bound_total(type_desc_);
  case TK_STRUCTURE: {
    const CORBA::ULong member_count = type_->get_member_count();
    CORBA::ULong count = member_count;
    // An optional member that hasn't been set is not counted.
    // All non-optional members are counted since they either are set directly
    // or hold default values (XTypes spec 7.5.2.11.6).
    for (CORBA::ULong i = 0; i < member_count; ++i) {
      DDS::DynamicTypeMember_var dtm;
      if (type_->get_member_by_index(dtm, i) != DDS::RETCODE_OK) {
        return 0;
      }
      DDS::MemberDescriptor_var md;
      if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
        return 0;
      }
      if (md->is_optional() && !has_member(md->id())) {
        --count;
      }
    }
    return count;
  }
  case TK_UNION: {
    bool has_selected_branch;
    DDS::MemberDescriptor_var selected_md;
    const DDS::ReturnCode_t rc = get_selected_union_branch(has_selected_branch, selected_md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_item_count:"
                   " get_selected_union_branch returned %C\n", retcode_to_string(rc)));
      }
      return 0;
    }
    if (has_selected_branch) {
      DDS::UInt32 count = 2;
      if (selected_md->is_optional() && !has_member(selected_md->id())) {
        --count;
      }
      return count;
    }
    return 1;
  }
  case TK_MAP:
  case TK_BITSET:
  case TK_ALIAS:
  case TK_ANNOTATION:
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_item_count:"
                 " Encounter unexpected type kind %C\n", typekind_to_string(tk)));
    }
    return 0;
  }
}

DDS::ReturnCode_t DynamicDataImpl::clear_all_values()
{
  const TypeKind tk = type_->get_kind();
  if (is_primitive(tk) || tk == TK_ENUM) {
    return clear_value_i(MEMBER_ID_INVALID, type_);
  }

  switch (tk) {
  case TK_BITMASK:
  case TK_ARRAY:
  case TK_STRING8:
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
#endif
  case TK_SEQUENCE:
  case TK_STRUCTURE:
  case TK_UNION:
    clear_container();
    set_backing_store(0);
    break;
  case TK_MAP:
  case TK_BITSET:
  case TK_ALIAS:
  case TK_ANNOTATION:
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::clear_all_values:"
                 " Encounter unexpected type kind %C\n", typekind_to_string(tk)));
    }
    return DDS::RETCODE_ERROR;
  }
  return DDS::RETCODE_OK;
}

void DynamicDataImpl::clear_container()
{
  container_.clear();
}

DDS::ReturnCode_t DynamicDataImpl::clear_nonkey_values()
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::clear_value(DDS::MemberId id)
{
  const TypeKind this_tk = type_->get_kind();
  if (is_primitive(this_tk) || this_tk == TK_ENUM) {
    if (id != MEMBER_ID_INVALID) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return clear_value_i(id, type_);
  }

  switch (this_tk) {
  case TK_BITMASK:
    return set_boolean_value(id, false);
  case TK_ARRAY: {
    const DDS::UInt32 bound = bound_total(type_desc_);
    if (!check_index_from_id(this_tk, id, bound)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
    return clear_value_i(id, elem_type);
  }
  case TK_STRING8:
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
#endif
  case TK_SEQUENCE: {
    // Shift subsequent elements to the left (XTypes spec 7.5.2.11.3).
    const CORBA::ULong size = get_sequence_item_count();
    if (id >= size) {
      return DDS::RETCODE_BAD_PARAMETER;
    }

    DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
    erase_member(id);
    for (CORBA::ULong i = id_to_index(id) + 1; i < size; ++i) {
      const DDS::MemberId curr_id = index_to_id(i);
      const DDS::MemberId prev_id = index_to_id(i - 1);
      const_single_iterator single_it = container_.single_map_.find(curr_id);
      if (single_it != container_.single_map_.end()) {
        container_.single_map_.insert(std::make_pair(prev_id, single_it->second));
        container_.single_map_.erase(curr_id);
        continue;
      }
      const_sequence_iterator seq_it = container_.sequence_map_.find(curr_id);
      if (seq_it != container_.sequence_map_.end()) {
        container_.sequence_map_.insert(std::make_pair(prev_id, seq_it->second));
        container_.sequence_map_.erase(curr_id);
        continue;
      }
      const_complex_iterator complex_it = container_.complex_map_.find(curr_id);
      if (complex_it != container_.complex_map_.end()) {
        container_.complex_map_.insert(std::make_pair(prev_id, complex_it->second));
        container_.complex_map_.erase(curr_id);
        continue;
      }
      // Since the backing store is read-only, we can't shift its elements. Instead,
      // copy (and shift) the elements that are missing in the container from the backing store.
      if (backing_store_) {
        DynamicDataImpl* elem_ddi = new DynamicDataImpl(elem_type);
        DDS::DynamicData_var elem_dd = elem_ddi;
        const DDS::ReturnCode_t rc = set_member_backing_store(elem_ddi, curr_id);
        if (rc != DDS::RETCODE_OK && rc != DDS::RETCODE_NO_DATA) {
          return DDS::RETCODE_ERROR;
        }
        container_.complex_map_.insert(std::make_pair(prev_id, elem_dd));
      }
    }

    // Then disable the backing store.
    set_backing_store(0);
    break;
  }
  case TK_STRUCTURE:
  case TK_UNION: {
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }
    DDS::MemberDescriptor_var md;
    if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }
    if (md->is_optional()) {
      erase_member(id);
      DDS::DynamicData_var opt_val;
      if (backing_store_ && backing_store_->get_complex_value(opt_val, id) == DDS::RETCODE_OK) {
        // The backing store is read-only, so to remove the optional member we need to
        // invalidate the backing store. Save all the members that are in the backing store
        // but not in the container.
        DDS::DynamicTypeMembersById_var members_var;
        if (type_->get_all_members(members_var) != DDS::RETCODE_OK) {
          return DDS::RETCODE_ERROR;
        }
        DynamicTypeMembersByIdImpl* members = dynamic_cast<DynamicTypeMembersByIdImpl*>(members_var.in());
        if (!members) {
          return DDS::RETCODE_ERROR;
        }
        for (DynamicTypeMembersByIdImpl::const_iterator it = members->begin(); it != members->end(); ++it) {
          const DDS::MemberId mid = it->first;
          if (mid == id) {
            continue;
          }
          const bool container_has_it =
            container_.single_map_.find(mid) != container_.single_map_.end() ||
            container_.sequence_map_.find(mid) != container_.sequence_map_.end() ||
            container_.complex_map_.find(mid) != container_.complex_map_.end();
          if (!container_has_it) {
            DDS::MemberDescriptor_var mem_desc;
            if (it->second->get_descriptor(mem_desc) != DDS::RETCODE_OK) {
              return DDS::RETCODE_ERROR;
            }
            DDS::DynamicType_var mem_dt = get_base_type(mem_desc->type());
            DynamicDataImpl* mem_ddi = new DynamicDataImpl(mem_dt);
            DDS::DynamicData_var mem_dd = mem_ddi;
            const DDS::ReturnCode_t rc = set_member_backing_store(mem_ddi, mid);
            if (rc != DDS::RETCODE_OK && rc != DDS::RETCODE_NO_DATA) {
              return DDS::RETCODE_ERROR;
            }
            container_.complex_map_.insert(std::make_pair(mid, mem_dd));
          }
        }
        set_backing_store(0);
      }
      break;
    }
    DDS::DynamicType_var member_type = get_base_type(md->type());
    return clear_value_i(id, member_type);
  }
  case TK_MAP:
  case TK_BITSET:
  case TK_ALIAS:
  case TK_ANNOTATION:
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::clear_value:"
                 " Encounter unexpected type kind %C\n", typekind_to_string(this_tk)));
    }
    return DDS::RETCODE_ERROR;
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::clear_value_i(DDS::MemberId id, const DDS::DynamicType_var& member_type)
{
  const TypeKind tk = member_type->get_kind();
  switch (tk) {
  case TK_BOOLEAN: {
    ACE_OutputCDR::from_boolean val(false);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_BYTE: {
    ACE_OutputCDR::from_octet val(0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_UINT8: {
    ACE_OutputCDR::from_uint8 val(0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_UINT16: {
    CORBA::UInt16 val(0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_UINT32: {
    CORBA::UInt32 val(0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_UINT64: {
    CORBA::UInt64 val(0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_INT8: {
    ACE_OutputCDR::from_int8 val(0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_INT16: {
    CORBA::Int16 val(0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_INT32: {
    CORBA::Int32 val(0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_INT64: {
    CORBA::Int64 val(0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_FLOAT32: {
    CORBA::Float val(0.0f);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_FLOAT64: {
    CORBA::Double val(0.0);
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_FLOAT128: {
    CORBA::LongDouble val;
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_CHAR8: {
    ACE_OutputCDR::from_char val('\0');
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_STRING8: {
    const char* val = 0;
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    ACE_OutputCDR::from_wchar val(L'\0');
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
  case TK_STRING16: {
    const CORBA::WChar* val = 0;
    set_default_basic_value(val);
    insert_single(id, val);
    break;
  }
#endif
  case TK_ENUM: {
    // Set to first enumerator
    CORBA::Long value;
    if (!set_default_enum_value(member_type, value)) {
      return DDS::RETCODE_ERROR;
    }
    TypeKind treat_as = tk;
    if (enum_bound(member_type, treat_as) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }
    if (treat_as == TK_INT8) {
      ACE_OutputCDR::from_int8 val(static_cast<CORBA::Int8>(value));
      insert_single(id, val);
    } else if (treat_as == TK_INT16) {
      insert_single(id, static_cast<CORBA::Short>(value));
    } else {
      insert_single(id, value);
    }
    break;
  }
  case TK_BITMASK: {
    // Set to default bitmask value
    TypeKind treat_as = tk;
    if (bitmask_bound(member_type, treat_as) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }
    if (treat_as == TK_UINT8) {
      ACE_OutputCDR::from_uint8 val(0);
      set_default_bitmask_value(val);
      insert_single(id, val);
    } else if (treat_as == TK_UINT16) {
      CORBA::UShort val;
      set_default_bitmask_value(val);
      insert_single(id, val);
    } else if (treat_as == TK_UINT32) {
      CORBA::ULong val;
      set_default_bitmask_value(val);
      insert_single(id, val);
    } else {
      CORBA::ULongLong val;
      set_default_bitmask_value(val);
      insert_single(id, val);
    }
    break;
  }
  case TK_ARRAY:
  case TK_SEQUENCE:
  case TK_STRUCTURE:
  case TK_UNION: {
    DDS::DynamicData_var dd = new DynamicDataImpl(member_type);
    insert_complex(id, dd);
    break;
  }
  case TK_MAP:
  case TK_BITSET:
  case TK_ALIAS:
  case TK_ANNOTATION:
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::clear_value_i:"
                 " Member %u has unexpected type kind %C\n", id, typekind_to_string(tk)));
    }
    return DDS::RETCODE_ERROR;
  }
  return DDS::RETCODE_OK;
}

DDS::DynamicData_ptr DynamicDataImpl::clone()
{
  return new DynamicDataImpl(*this);
}

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_int8& value)
{
  // The same member might be already written to complex_map_.
  // Make sure there is only one entry for each member.
  if (container_.complex_map_.erase(id) == 0) {
    container_.single_map_.erase(id);
  }
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_uint8& value)
{
  if (container_.complex_map_.erase(id) == 0) {
    container_.single_map_.erase(id);
  }
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_char& value)
{
  if (container_.complex_map_.erase(id) == 0) {
    container_.single_map_.erase(id);
  }
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_octet& value)
{
  if (container_.complex_map_.erase(id) == 0) {
    container_.single_map_.erase(id);
  }
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_boolean& value)
{
  if (container_.complex_map_.erase(id) == 0) {
    container_.single_map_.erase(id);
  }
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

#ifdef DDS_HAS_WCHAR
bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_wchar& value)
{
  if (container_.complex_map_.erase(id) == 0) {
    container_.single_map_.erase(id);
  }
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}
#endif

template<typename SingleType>
bool DynamicDataImpl::insert_single(DDS::MemberId id, const SingleType& value)
{
  if (container_.complex_map_.erase(id) == 0) {
    container_.single_map_.erase(id);
  }
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_complex(DDS::MemberId id, const DDS::DynamicData_var& value)
{
  if (container_.single_map_.erase(id) == 0) {
    if (container_.sequence_map_.erase(id) == 0) {
      container_.complex_map_.erase(id);
    }
  }
  return container_.complex_map_.insert(std::make_pair(id, value)).second;
}

// Set a member with the given ID in a struct. The member must have type MemberTypeKind or
// enum/bitmask. In the latter case, its bit bound must be in the range [lower, upper].
template<TypeKind MemberTypeKind, typename MemberType>
DDS::ReturnCode_t DynamicDataImpl::set_value_to_struct(DDS::MemberId id, const MemberType& value)
{
  DDS::MemberDescriptor_var md;
  DDS::DynamicType_var member_type;
  const DDS::ReturnCode_t rc = check_member(
    md, member_type, "DynamicDataImpl::set_value_to_struct", "set", id, MemberTypeKind);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  insert_single(id, value);
  return DDS::RETCODE_OK;
}

bool DynamicDataImpl::is_valid_discriminator_type(TypeKind tk)
{
  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_CHAR8:
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
#endif
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
bool DynamicDataImpl::is_default_member_selected(CORBA::Long disc_val, DDS::MemberId default_id) const
{
  if (type_->get_kind() != TK_UNION) {
    return false;
  }

  DDS::DynamicTypeMembersById_var members_var;
  if (type_->get_all_members(members_var) != DDS::RETCODE_OK) {
    return false;
  }
  DynamicTypeMembersByIdImpl* members = dynamic_cast<DynamicTypeMembersByIdImpl*>(members_var.in());
  if (!members) {
    return false;
  }

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

DynamicDataImpl::SingleValue::SingleValue()
  : kind_(TK_NONE), active_(0)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Long int32)
  : kind_(TK_INT32), active_(0), int32_(int32)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::ULong uint32)
  : kind_(TK_UINT32), active_(0), uint32_(uint32)
{}

DynamicDataImpl::SingleValue::SingleValue(ACE_OutputCDR::from_int8 value)
  : kind_(TK_INT8), active_(new(int8_) ACE_OutputCDR::from_int8(value.val_))
{}

DynamicDataImpl::SingleValue::SingleValue(ACE_OutputCDR::from_uint8 value)
  : kind_(TK_UINT8), active_(new(uint8_) ACE_OutputCDR::from_uint8(value.val_))
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Short int16)
  : kind_(TK_INT16), active_(0), int16_(int16)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::UShort uint16)
  : kind_(TK_UINT16), active_(0), uint16_(uint16)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::LongLong int64)
  : kind_(TK_INT64), active_(0), int64_(int64)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::ULongLong uint64)
  : kind_(TK_UINT64), active_(0), uint64_(uint64)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Float float32)
  : kind_(TK_FLOAT32), active_(0), float32_(float32)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::Double float64)
  : kind_(TK_FLOAT64), active_(0), float64_(float64)
{}

DynamicDataImpl::SingleValue::SingleValue(CORBA::LongDouble float128)
  : kind_(TK_FLOAT128), active_(0), float128_(float128)
{}

DynamicDataImpl::SingleValue::SingleValue(ACE_OutputCDR::from_char value)
  : kind_(TK_CHAR8), active_(new(char8_) ACE_OutputCDR::from_char(value.val_))
{}

DynamicDataImpl::SingleValue::SingleValue(ACE_OutputCDR::from_octet value)
  : kind_(TK_BYTE), active_(new(byte_) ACE_OutputCDR::from_octet(value.val_))
{}

DynamicDataImpl::SingleValue::SingleValue(ACE_OutputCDR::from_boolean value)
  : kind_(TK_BOOLEAN), active_(new(boolean_) ACE_OutputCDR::from_boolean(value.val_))
{}

DynamicDataImpl::SingleValue::SingleValue(const char* str)
  : kind_(TK_STRING8), active_(0), str_(CORBA::string_dup(str))
{}

#ifdef DDS_HAS_WCHAR
DynamicDataImpl::SingleValue::SingleValue(ACE_OutputCDR::from_wchar value)
  : kind_(TK_CHAR16), active_(new(char16_) ACE_OutputCDR::from_wchar(value.val_))
{}

DynamicDataImpl::SingleValue::SingleValue(const CORBA::WChar* wstr)
  : kind_(TK_STRING16), active_(0), wstr_(CORBA::wstring_dup(wstr))
{}
#endif

DynamicDataImpl::SingleValue::~SingleValue()
{
#define SINGLE_VALUE_DESTRUCT(T) static_cast<ACE_OutputCDR::T*>(active_)->~T(); break
  switch (kind_) {
  case TK_INT8:
    SINGLE_VALUE_DESTRUCT(from_int8);
  case TK_UINT8:
    SINGLE_VALUE_DESTRUCT(from_uint8);
  case TK_CHAR8:
    SINGLE_VALUE_DESTRUCT(from_char);
  case TK_BYTE:
    SINGLE_VALUE_DESTRUCT(from_octet);
  case TK_BOOLEAN:
    SINGLE_VALUE_DESTRUCT(from_boolean);
  case TK_STRING8:
    CORBA::string_free((char*)str_);
    break;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    SINGLE_VALUE_DESTRUCT(from_wchar);
  case TK_STRING16:
    CORBA::wstring_free((CORBA::WChar*)wstr_);
    break;
#endif
  }
#undef SINGLE_VALUE_DESTRUCT
}

template<> const CORBA::Long& DynamicDataImpl::SingleValue::get() const { return int32_; }
template<> const CORBA::ULong& DynamicDataImpl::SingleValue::get() const { return uint32_; }

template<> const ACE_OutputCDR::from_int8& DynamicDataImpl::SingleValue::get() const
{
  return *static_cast<ACE_OutputCDR::from_int8*>(active_);
}

template<> const ACE_OutputCDR::from_uint8& DynamicDataImpl::SingleValue::get() const
{
  return *static_cast<ACE_OutputCDR::from_uint8*>(active_);
}

template<> const CORBA::Short& DynamicDataImpl::SingleValue::get() const { return int16_; }
template<> const CORBA::UShort& DynamicDataImpl::SingleValue::get() const { return uint16_; }
template<> const CORBA::LongLong& DynamicDataImpl::SingleValue::get() const { return int64_; }
template<> const CORBA::ULongLong& DynamicDataImpl::SingleValue::get() const { return uint64_; }
template<> const CORBA::Float& DynamicDataImpl::SingleValue::get() const { return float32_; }
template<> const CORBA::Double& DynamicDataImpl::SingleValue::get() const { return float64_; }
template<> const CORBA::LongDouble& DynamicDataImpl::SingleValue::get() const { return float128_; }

template<> const ACE_OutputCDR::from_char& DynamicDataImpl::SingleValue::get() const
{
  return *static_cast<ACE_OutputCDR::from_char*>(active_);
}

template<> const ACE_OutputCDR::from_octet& DynamicDataImpl::SingleValue::get() const
{
  return *static_cast<ACE_OutputCDR::from_octet*>(active_);
}

template<> const ACE_OutputCDR::from_boolean& DynamicDataImpl::SingleValue::get() const
{
  return *static_cast<ACE_OutputCDR::from_boolean*>(active_);
}

template<> const char* const& DynamicDataImpl::SingleValue::get() const { return str_; }

#ifdef DDS_HAS_WCHAR
template<> const ACE_OutputCDR::from_wchar& DynamicDataImpl::SingleValue::get() const
{
  return *static_cast<ACE_OutputCDR::from_wchar*>(active_);
}

template<> const CORBA::WChar* const& DynamicDataImpl::SingleValue::get() const { return wstr_; }
#endif

char* DynamicDataImpl::SingleValue::get_string() const { return CORBA::string_dup(str_); }
CORBA::WChar* DynamicDataImpl::SingleValue::get_wstring() const { return CORBA::wstring_dup(wstr_); }

void DynamicDataImpl::SingleValue::copy(const SingleValue& other)
{
  switch (kind_) {
  case TK_INT8:
    active_ = new(int8_) ACE_OutputCDR::from_int8(other.get<ACE_OutputCDR::from_int8>());
    break;
  case TK_UINT8:
    active_ = new(uint8_) ACE_OutputCDR::from_uint8(other.get<ACE_OutputCDR::from_uint8>());
    break;
  case TK_INT16:
    int16_ = other.int16_;
    break;
  case TK_UINT16:
    uint16_ = other.uint16_;
    break;
  case TK_INT32:
    int32_ = other.int32_;
    break;
  case TK_UINT32:
    uint32_ = other.uint32_;
    break;
  case TK_INT64:
    int64_ = other.int64_;
    break;
  case TK_UINT64:
    uint64_ = other.uint64_;
    break;
  case TK_FLOAT32:
    float32_ = other.float32_;
    break;
  case TK_FLOAT64:
    float64_ = other.float64_;
    break;
  case TK_FLOAT128:
    float128_ = other.float128_;
    break;
  case TK_BOOLEAN:
    active_ = new(boolean_) ACE_OutputCDR::from_boolean(other.get<ACE_OutputCDR::from_boolean>());
    break;
  case TK_BYTE:
    active_ = new(byte_) ACE_OutputCDR::from_octet(other.get<ACE_OutputCDR::from_octet>());
    break;
  case TK_CHAR8:
    active_ = new(char8_) ACE_OutputCDR::from_char(other.get<ACE_OutputCDR::from_char>());
    break;
  case TK_STRING8:
    str_ = CORBA::string_dup(other.str_);
    break;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    active_ = new(char16_) ACE_OutputCDR::from_wchar(other.get<ACE_OutputCDR::from_wchar>());
    break;
  case TK_STRING16:
    wstr_ = CORBA::wstring_dup(other.wstr_);
    break;
#endif
  }
}

// Has to be below the get methods, or else there's a template specialization issue.
DynamicDataImpl::SingleValue::SingleValue(const SingleValue& other)
  : kind_(other.kind_)
  , active_(0)
{
  copy(other);
}

DynamicDataImpl::SingleValue& DynamicDataImpl::SingleValue::operator=(const SingleValue& other)
{
  kind_ = other.kind_;
  active_ = 0;
  copy(other);
  return *this;
}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Int32Seq& int32_seq)
  : elem_kind_(TK_INT32), active_(new(int32_seq_) DDS::Int32Seq(int32_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::UInt32Seq& uint32_seq)
  : elem_kind_(TK_UINT32), active_(new(uint32_seq_) DDS::UInt32Seq(uint32_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Int8Seq& int8_seq)
  : elem_kind_(TK_INT8), active_(new(int8_seq_) DDS::Int8Seq(int8_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::UInt8Seq& uint8_seq)
  : elem_kind_(TK_UINT8), active_(new(uint8_seq_) DDS::UInt8Seq(uint8_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Int16Seq& int16_seq)
  : elem_kind_(TK_INT16), active_(new(int16_seq_) DDS::Int16Seq(int16_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::UInt16Seq& uint16_seq)
  : elem_kind_(TK_UINT16), active_(new(uint16_seq_) DDS::UInt16Seq(uint16_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Int64Seq& int64_seq)
  : elem_kind_(TK_INT64), active_(new(int64_seq_) DDS::Int64Seq(int64_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::UInt64Seq& uint64_seq)
  : elem_kind_(TK_UINT64), active_(new(uint64_seq_) DDS::UInt64Seq(uint64_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Float32Seq& float32_seq)
  : elem_kind_(TK_FLOAT32), active_(new(float32_seq_) DDS::Float32Seq(float32_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Float64Seq& float64_seq)
  : elem_kind_(TK_FLOAT64), active_(new(float64_seq_) DDS::Float64Seq(float64_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::Float128Seq& float128_seq)
  : elem_kind_(TK_FLOAT128), active_(new(float128_seq_) DDS::Float128Seq(float128_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::CharSeq& char8_seq)
  : elem_kind_(TK_CHAR8), active_(new(char8_seq_) DDS::CharSeq(char8_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::ByteSeq& byte_seq)
  : elem_kind_(TK_BYTE), active_(new(byte_seq_) DDS::ByteSeq(byte_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::BooleanSeq& boolean_seq)
  : elem_kind_(TK_BOOLEAN), active_(new(boolean_seq_) DDS::BooleanSeq(boolean_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::StringSeq& str_seq)
  : elem_kind_(TK_STRING8), active_(new(string_seq_) DDS::StringSeq(str_seq))
{}

#ifdef DDS_HAS_WCHAR
DynamicDataImpl::SequenceValue::SequenceValue(const DDS::WcharSeq& char16_seq)
  : elem_kind_(TK_CHAR16), active_(new(char16_seq_) DDS::WcharSeq(char16_seq))
{}

DynamicDataImpl::SequenceValue::SequenceValue(const DDS::WstringSeq& wstr_seq)
  : elem_kind_(TK_STRING16), active_(new(wstring_seq_) DDS::WstringSeq(wstr_seq))
{}
#endif

DynamicDataImpl::SequenceValue::SequenceValue(const SequenceValue& rhs)
  : elem_kind_(rhs.elem_kind_), active_(0)
{
#define SEQUENCE_VALUE_PLACEMENT_NEW(T, N)  active_ = new(N) DDS::T(reinterpret_cast<const DDS::T&>(rhs.N)); break;
  switch (elem_kind_) {
  case TK_INT32:
    SEQUENCE_VALUE_PLACEMENT_NEW(Int32Seq, int32_seq_);
  case TK_UINT32:
    SEQUENCE_VALUE_PLACEMENT_NEW(UInt32Seq, uint32_seq_);
  case TK_INT8:
    SEQUENCE_VALUE_PLACEMENT_NEW(Int8Seq, int8_seq_);
  case TK_UINT8:
    SEQUENCE_VALUE_PLACEMENT_NEW(UInt8Seq, uint8_seq_);
  case TK_INT16:
    SEQUENCE_VALUE_PLACEMENT_NEW(Int16Seq, int16_seq_);
  case TK_UINT16:
    SEQUENCE_VALUE_PLACEMENT_NEW(UInt16Seq, uint16_seq_);
  case TK_INT64:
    SEQUENCE_VALUE_PLACEMENT_NEW(Int64Seq, int64_seq_);
  case TK_UINT64:
    SEQUENCE_VALUE_PLACEMENT_NEW(UInt64Seq, uint64_seq_);
  case TK_FLOAT32:
    SEQUENCE_VALUE_PLACEMENT_NEW(Float32Seq, float32_seq_);
  case TK_FLOAT64:
    SEQUENCE_VALUE_PLACEMENT_NEW(Float64Seq, float64_seq_);
  case TK_FLOAT128:
    SEQUENCE_VALUE_PLACEMENT_NEW(Float128Seq, float128_seq_);
  case TK_CHAR8:
    SEQUENCE_VALUE_PLACEMENT_NEW(CharSeq, char8_seq_);
  case TK_BYTE:
    SEQUENCE_VALUE_PLACEMENT_NEW(ByteSeq, byte_seq_);
  case TK_BOOLEAN:
    SEQUENCE_VALUE_PLACEMENT_NEW(BooleanSeq, boolean_seq_);
  case TK_STRING8:
    SEQUENCE_VALUE_PLACEMENT_NEW(StringSeq, string_seq_);
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    SEQUENCE_VALUE_PLACEMENT_NEW(WcharSeq, char16_seq_);
  case TK_STRING16:
    SEQUENCE_VALUE_PLACEMENT_NEW(WstringSeq, wstring_seq_);
#endif
  }
#undef SEQUENCE_VALUE_PLACEMENT_NEW
}

DynamicDataImpl::SequenceValue::~SequenceValue()
{
#define SEQUENCE_VALUE_DESTRUCT(T) static_cast<DDS::T*>(active_)->~T(); break
  switch (elem_kind_) {
  case TK_INT32:
    SEQUENCE_VALUE_DESTRUCT(Int32Seq);
  case TK_UINT32:
    SEQUENCE_VALUE_DESTRUCT(UInt32Seq);
  case TK_INT8:
    SEQUENCE_VALUE_DESTRUCT(Int8Seq);
  case TK_UINT8:
    SEQUENCE_VALUE_DESTRUCT(UInt8Seq);
  case TK_INT16:
    SEQUENCE_VALUE_DESTRUCT(Int16Seq);
  case TK_UINT16:
    SEQUENCE_VALUE_DESTRUCT(UInt16Seq);
  case TK_INT64:
    SEQUENCE_VALUE_DESTRUCT(Int64Seq);
  case TK_UINT64:
    SEQUENCE_VALUE_DESTRUCT(UInt64Seq);
  case TK_FLOAT32:
    SEQUENCE_VALUE_DESTRUCT(Float32Seq);
  case TK_FLOAT64:
    SEQUENCE_VALUE_DESTRUCT(Float64Seq);
  case TK_FLOAT128:
    SEQUENCE_VALUE_DESTRUCT(Float128Seq);
  case TK_CHAR8:
    SEQUENCE_VALUE_DESTRUCT(CharSeq);
  case TK_BYTE:
    SEQUENCE_VALUE_DESTRUCT(ByteSeq);
  case TK_BOOLEAN:
    SEQUENCE_VALUE_DESTRUCT(BooleanSeq);
  case TK_STRING8:
    SEQUENCE_VALUE_DESTRUCT(StringSeq);
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    SEQUENCE_VALUE_DESTRUCT(WcharSeq);
  case TK_STRING16:
    SEQUENCE_VALUE_DESTRUCT(WstringSeq);
#endif
  }
  #undef SEQUENCE_VALUE_DESTRUCT
}

#define SEQUENCE_VALUE_GETTERS(T) return *static_cast<DDS::T*>(active_)
template<> const DDS::Int32Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(Int32Seq); }
template<> const DDS::UInt32Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(UInt32Seq); }
template<> const DDS::Int8Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(Int8Seq); }
template<> const DDS::UInt8Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(UInt8Seq); }
template<> const DDS::Int16Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(Int16Seq); }
template<> const DDS::UInt16Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(UInt16Seq); }
template<> const DDS::Int64Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(Int64Seq); }
template<> const DDS::UInt64Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(UInt64Seq); }
template<> const DDS::Float32Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(Float32Seq); }
template<> const DDS::Float64Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(Float64Seq); }
template<> const DDS::Float128Seq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(Float128Seq); }
template<> const DDS::CharSeq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(CharSeq); }
template<> const DDS::ByteSeq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(ByteSeq); }
template<> const DDS::BooleanSeq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(BooleanSeq); }
template<> const DDS::StringSeq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(StringSeq); }
#ifdef DDS_HAS_WCHAR
template<> const DDS::WcharSeq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(WcharSeq); }
template<> const DDS::WstringSeq& DynamicDataImpl::SequenceValue::get() const
{ SEQUENCE_VALUE_GETTERS(WstringSeq); }
#endif
#undef SEQUENCE_VALUE_GETTERS

bool DynamicDataImpl::read_disc_from_single_map(CORBA::Long& disc_val,
                                                const DDS::DynamicType_var& disc_type,
                                                const const_single_iterator& it) const
{
  const TypeKind disc_tk = disc_type->get_kind();
  TypeKind treat_as_tk = disc_tk;
  if (disc_tk == TK_ENUM && enum_bound(disc_type, treat_as_tk) != DDS::RETCODE_OK) {
    return false;
  }

  switch (treat_as_tk) {
  case TK_BOOLEAN: {
    const ACE_OutputCDR::from_boolean& value = it->second.get<ACE_OutputCDR::from_boolean>();
    disc_val = static_cast<CORBA::Long>(value.val_);
    return true;
  }
  case TK_BYTE: {
    const ACE_OutputCDR::from_octet& value = it->second.get<ACE_OutputCDR::from_octet>();
    disc_val = static_cast<CORBA::Long>(value.val_);
    return true;
  }
  case TK_CHAR8: {
    const ACE_OutputCDR::from_char& value = it->second.get<ACE_OutputCDR::from_char>();
    disc_val = static_cast<CORBA::Long>(value.val_);
    return true;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    const ACE_OutputCDR::from_wchar& value = it->second.get<ACE_OutputCDR::from_wchar>();
    disc_val = static_cast<CORBA::Long>(value.val_);
    return true;
  }
#endif
  case TK_INT8: {
    const ACE_OutputCDR::from_int8& value = it->second.get<ACE_OutputCDR::from_int8>();
    disc_val = static_cast<CORBA::Long>(value.val_);
    return true;
  }
  case TK_UINT8: {
    const ACE_OutputCDR::from_uint8& value = it->second.get<ACE_OutputCDR::from_uint8>();
    disc_val = static_cast<CORBA::Long>(value.val_);
    return true;
  }
  case TK_INT16: {
    CORBA::Short value = it->second.get<CORBA::Short>();
    disc_val = value;
    return true;
  }
  case TK_UINT16: {
    CORBA::UShort value = it->second.get<CORBA::UShort>();
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT32: {
    disc_val = it->second.get<CORBA::Long>();
    return true;
  }
  case TK_UINT32: {
    CORBA::ULong value = it->second.get<CORBA::ULong>();
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT64: {
    CORBA::LongLong value = it->second.get<CORBA::LongLong>();
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_UINT64: {
    CORBA::ULongLong value = it->second.get<CORBA::ULongLong>();
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  }
  return false;
}

// Read discriminator, identified by a given id, from the backing store.
bool DynamicDataImpl::read_disc_from_backing_store(CORBA::Long& disc_val,
                                                   DDS::MemberId id,
                                                   const DDS::DynamicType_var& disc_type)
{
  const TypeKind disc_tk = disc_type->get_kind();
  TypeKind treat_as_tk = disc_tk;
  if (disc_tk == TK_ENUM && enum_bound(disc_type, treat_as_tk) != DDS::RETCODE_OK) {
    return false;
  }

  switch (treat_as_tk) {
  case TK_BOOLEAN: {
    ACE_OutputCDR::from_boolean val(false);
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val.val_);
    return true;
  }
  case TK_BYTE: {
    ACE_OutputCDR::from_octet val(0x00);
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val.val_);
    return true;
  }
  case TK_CHAR8: {
    ACE_OutputCDR::from_char val('\0');
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val.val_);
    return true;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    ACE_OutputCDR::from_wchar val(0);
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val.val_);
    return true;
  }
#endif
  case TK_INT8: {
    ACE_OutputCDR::from_int8 val(0);
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val.val_);
    return true;
  }
  case TK_UINT8: {
    ACE_OutputCDR::from_uint8 val(0);
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val.val_);
    return true;
  }
  case TK_INT16: {
    CORBA::Short val;
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = val;
    return true;
  }
  case TK_UINT16: {
    CORBA::UShort val;
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    return true;
  }
  case TK_INT32:
    return get_value_from_backing_store(disc_val, id);
  case TK_UINT32: {
    CORBA::ULong val;
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    return true;
  }
  case TK_INT64: {
    CORBA::LongLong val;
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    return true;
  }
  case TK_UINT64: {
    CORBA::ULongLong val;
    if (!get_value_from_backing_store(val, id)) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    return true;
  }
  }
  return false;
}

// Read a discriminator value from a DynamicData that represents it.
bool DynamicDataImpl::read_discriminator(CORBA::Long& disc_val)
{
  if (!is_valid_discriminator_type(type_->get_kind())) {
    return false;
  }
  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    return read_disc_from_single_map(disc_val, type_, it);
  }
  if (read_disc_from_backing_store(disc_val, MEMBER_ID_INVALID, type_)) {
    return true;
  }
  return set_default_discriminator_value(disc_val, type_);
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

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const ACE_OutputCDR::from_boolean& value) const
{
  disc_value = static_cast<CORBA::Long>(value.val_);
  return true;
}

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const ACE_OutputCDR::from_octet& value) const
{
  disc_value = static_cast<CORBA::Long>(value.val_);
  return true;
}

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const ACE_OutputCDR::from_char& value) const
{
  disc_value = static_cast<CORBA::Long>(value.val_);
  return true;
}

#ifdef DDS_HAS_WCHAR
bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const ACE_OutputCDR::from_wchar& value) const
{
  disc_value = static_cast<CORBA::Long>(value.val_);
  return true;
}
#endif

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const ACE_OutputCDR::from_int8& value) const
{
  disc_value = static_cast<CORBA::Long>(value.val_);
  return true;
}

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const ACE_OutputCDR::from_uint8& value) const
{
  disc_value = static_cast<CORBA::Long>(value.val_);
  return true;
}

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const CORBA::Short& value) const
{
  disc_value = static_cast<CORBA::Long>(value);
  return true;
}

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const CORBA::UShort& value) const
{
  disc_value = static_cast<CORBA::Long>(value);
  return true;
}

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const CORBA::Long& value) const
{
  disc_value = value;
  return true;
}

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const CORBA::ULong& value) const
{
  disc_value = static_cast<CORBA::Long>(value);
  return true;
}

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const CORBA::LongLong& value) const
{
  disc_value = static_cast<CORBA::Long>(value);
  return true;
}

bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& disc_value,
                                                  const CORBA::ULongLong& value) const
{
  disc_value = static_cast<CORBA::Long>(value);
  return true;
}

template<typename MemberType>
bool DynamicDataImpl::cast_to_discriminator_value(CORBA::Long& /*disc_value*/,
                                                  const MemberType& /*value*/) const
{
  return false;
}

// Return true if the DynamicData instance contains a value for the discriminator.
bool DynamicDataImpl::has_discriminator_value(const_single_iterator& single_it,
                                              const_complex_iterator& complex_it) const
{
  single_it = container_.single_map_.find(DISCRIMINATOR_ID);
  complex_it = container_.complex_map_.find(DISCRIMINATOR_ID);

  // A backing store, if present, must have valid data (for union in this case),
  // meaning it must have at least data for discriminator.
  return single_it != container_.single_map_.end()
    || complex_it != container_.complex_map_.end()
    || backing_store_;
}

// Get discriminator value from the data container or the backing store.
// Call only when the instance has data for discriminator.
bool DynamicDataImpl::get_discriminator_value(CORBA::Long& value,
                                              const const_single_iterator& single_it,
                                              const const_complex_iterator& complex_it,
                                              const DDS::DynamicType_var& disc_type)
{
  if (single_it != container_.single_map_.end()) {
    read_disc_from_single_map(value, disc_type, single_it);
  } else if (complex_it != container_.complex_map_.end()) {
    const DynamicDataImpl* dd_impl = dynamic_cast<const DynamicDataImpl*>(complex_it->second.in());
    if (!dd_impl) {
      return false;
    }
    const_single_iterator it = dd_impl->container_.single_map_.find(MEMBER_ID_INVALID);
    if (it != dd_impl->container_.single_map_.end()) {
      read_disc_from_single_map(value, disc_type, it);
    } else {
      return set_default_discriminator_value(value, disc_type);
    }
  } else {
    return read_disc_from_backing_store(value, DISCRIMINATOR_ID, disc_type);
  }
  return true;
}

// The following cases will succeed:
// (1) If the current state of the union has a branch, including the implicit default branch,
//     the new discriminator value must selects the same branch.
// (2) When the union is empty, the discriminator has its default value and if it
//     selects a branch, that branch will also have the default value. In this case, if
//     the new discriminator selects the same branch, the operation succeeds.
// (3) The new discriminator selects the implicit default member, if there is one. This
//     is equivalent to the _default() method of the IDL-to-C++ mappings.
//
// For all other cases, the operation returns PRECONDITION_NOT_MET.
DDS::ReturnCode_t DynamicDataImpl::set_union_discriminator_helper(CORBA::Long new_disc_value,
                                                                  const char* func_name)
{
  bool has_existing_branch;
  DDS::MemberDescriptor_var existing_md;
  const DDS::ReturnCode_t rc = get_selected_union_branch(has_existing_branch, existing_md);
  if (rc != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  // Always allow setting discriminator that selects the implicit default member.
  if (discriminator_selects_no_member(new_disc_value)) {
    return DDS::RETCODE_OK;
  }

  // The new discriminator selects a different branch than the current one.
  if (!has_existing_branch || !validate_discriminator(new_disc_value, existing_md)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::%C:"
                 " Discriminator value %d does not select the same activated branch!\n",
                 func_name, new_disc_value));
    }
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }
  return DDS::RETCODE_OK;
}

bool DynamicDataImpl::get_union_member_type(DDS::MemberId id, DDS::DynamicType_var& member_type,
                                            DDS::MemberDescriptor_var& md) const
{
  if (id == DISCRIMINATOR_ID) {
    member_type = get_base_type(type_desc_->discriminator_type());
  } else {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return false;
    }
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }
    member_type = get_base_type(md->type());
  }
  return true;
}

// Only call on union.
// Check if there is a selected branch and return its information if so.
bool DynamicDataImpl::find_selected_union_branch(bool& has_existing_branch,
                                                 DDS::MemberDescriptor_var& existing_md)
{
  const_single_iterator single_it;
  const_complex_iterator complex_it;
  has_existing_branch = false;

  if (has_discriminator_value(single_it, complex_it)) {
    DDS::DynamicType_var disc_type = get_base_type(type_desc_->discriminator_type());
    CORBA::Long existing_disc;
    if (!get_discriminator_value(existing_disc, single_it, complex_it, disc_type)) {
      return false;
    }
    if (XTypes::get_selected_union_branch(type_, existing_disc, has_existing_branch, existing_md) != DDS::RETCODE_OK) {
      return false;
    }
  }
  return true;
}

// With backing store, data for union (the discriminator and a selected branch) can
// scatter across the container and the backing store. E.g., the discriminator can be
// in the container but a branch selected by it is in the backing store, and vice versa.
// In any case, the container and backing store as a whole must represent a valid state
// of the union data. The union data can be in the following states:
// (a) It contains no data at all.
// (b) It contains a value for the discriminator and a value for the selected branch,
//     including the implicit default branch, if exists.
//
// In the case of (b), the data for the discriminator and the selected branch can be
// found at either the container or the backing store, if present:
// (i) Both the discriminator and the selected branch are in the container
//     (the backing store contains obsolete data)
// (ii) Both the discriminator and the selected branch are in the backing store
//      (the container is empty)
// (iii) The discriminator is in the container, the selected branch is in the backing store
// (iv) The discrininator is in the backing store, the selected branch is in the container
//
// Note also that the container have priority over the backing store when reading data.
template<TypeKind ValueTypeKind, typename MemberType>
DDS::ReturnCode_t DynamicDataImpl::set_value_to_union(DDS::MemberId id, const MemberType& value)
{
  DDS::DynamicType_var member_type;
  DDS::MemberDescriptor_var md;
  if (!get_union_member_type(id, member_type, md)) {
    return DDS::RETCODE_ERROR;
  }

  const TypeKind member_tk = member_type->get_kind();
  TypeKind treat_member_as = member_tk;
  if (member_tk == TK_ENUM && enum_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }
  if (member_tk == TK_BITMASK && bitmask_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  if (treat_member_as != ValueTypeKind) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  // This follows the IDL-to-C++ mapping for union.
  if (id == DISCRIMINATOR_ID) {
    CORBA::Long disc_value;
    if (!cast_to_discriminator_value(disc_value, value)) {
      return DDS::RETCODE_ERROR;
    }
    const DDS::ReturnCode_t rc = set_union_discriminator_helper(disc_value, "set_value_to_union");
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    insert_single(id, value);
    return DDS::RETCODE_OK;
  }

  bool has_existing_branch = false;
  DDS::MemberDescriptor_var existing_md;
  if (!find_selected_union_branch(has_existing_branch, existing_md)) {
    return DDS::RETCODE_ERROR;
  }

  // Update the value of the branch, but keep the existing discriminator value.
  if (has_existing_branch && existing_md->id() == id) {
    insert_single(id, value);
    return DDS::RETCODE_OK;
  }

  // Store both the selected branch and a matching discriminator to the container.
  clear_container();

  if (!insert_valid_discriminator(md) || !insert_single(id, value)) {
    return DDS::RETCODE_ERROR;
  }
  return DDS::RETCODE_OK;
}

bool DynamicDataImpl::insert_valid_discriminator(DDS::MemberDescriptor* memberSelected)
{
  if (memberSelected->is_default_label()) {
    DCPS::DisjointSequence::OrderedRanges<ACE_CDR::Long> used;
    const ACE_CDR::ULong members = type_->get_member_count();
    for (ACE_CDR::ULong i = 0; i < members; ++i) {
      DDS::DynamicTypeMember_var member;
      if (type_->get_member_by_index(member, i) != DDS::RETCODE_OK) {
        return false;
      }
      if (member->get_id() == DISCRIMINATOR_ID || member->get_id() == memberSelected->id()) {
        continue;
      }
      DDS::MemberDescriptor_var mdesc;
      if (member->get_descriptor(mdesc) != DDS::RETCODE_OK) {
        return false;
      }
      const DDS::UnionCaseLabelSeq& lseq = mdesc->label();
      for (ACE_CDR::ULong lbl = 0; lbl < lseq.length(); ++lbl) {
        used.add(lseq[lbl]);
      }
    }
    const ACE_CDR::Long disc = used.empty() ? 0 : used.begin()->second + 1;
    return insert_discriminator(disc);
  }
  const DDS::UnionCaseLabelSeq& lseq = memberSelected->label();
  return lseq.length() && insert_discriminator(lseq[0]);
}

bool DynamicDataImpl::insert_discriminator(ACE_CDR::Long value)
{
  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, DISCRIMINATOR_ID) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var descriptor;
  if (member->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::DynamicType_var discType = get_base_type(descriptor->type());
  switch (discType ? discType->get_kind() : TK_NONE) {
  case TK_BOOLEAN:
    return insert_single(DISCRIMINATOR_ID, ACE_OutputCDR::from_boolean(value));
  case TK_BYTE:
    return insert_single(DISCRIMINATOR_ID, ACE_OutputCDR::from_octet(value));
  case TK_CHAR8:
    return insert_single(DISCRIMINATOR_ID, ACE_OutputCDR::from_char(value));
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return insert_single(DISCRIMINATOR_ID, ACE_OutputCDR::from_wchar(value));
#endif
  case TK_INT8:
    return insert_single(DISCRIMINATOR_ID, ACE_OutputCDR::from_int8(value));
  case TK_UINT8:
    return insert_single(DISCRIMINATOR_ID, ACE_OutputCDR::from_uint8(value));
  case TK_INT16:
    return insert_single(DISCRIMINATOR_ID, static_cast<ACE_CDR::Short>(value));
  case TK_UINT16:
    return insert_single(DISCRIMINATOR_ID, static_cast<ACE_CDR::UShort>(value));
  case TK_ENUM:
  case TK_INT32:
    return insert_single(DISCRIMINATOR_ID, value);
  case TK_UINT32:
    return insert_single(DISCRIMINATOR_ID, static_cast<ACE_CDR::ULong>(value));
  case TK_INT64:
    return insert_single(DISCRIMINATOR_ID, static_cast<ACE_CDR::LongLong>(value));
  case TK_UINT64:
    return insert_single(DISCRIMINATOR_ID, static_cast<ACE_CDR::ULongLong>(value));
  default:
    return false;
  }
}

// Check if a given member ID is valid for a given type with a maximum number of elements.
bool DynamicDataImpl::check_index_from_id(TypeKind tk, DDS::MemberId id, CORBA::ULong bound) const
{
  const CORBA::ULong index = id_to_index(id);
  switch (tk) {
  case TK_STRING8:
  case TK_STRING16:
  case TK_SEQUENCE:
  case TK_MAP:
    // Bound of 0 means unbounded.
    if (bound == 0 || index < bound) {
      return true;
    }
    break;
  case TK_BITMASK:
  case TK_ARRAY:
    if (index < bound) {
      return true;
    }
    break;
  }
  return false;
}

bool DynamicDataImpl::check_out_of_bound_write(DDS::MemberId id)
{
  const TypeKind tk = type_->get_kind();
  CORBA::ULong bound = bound_total(type_desc_);
  const CORBA::ULong item_count = get_item_count();
  if (tk == TK_SEQUENCE || tk == TK_STRING8 || tk == TK_STRING16) {
    // Allow setting an existing element or appending a new one
    if (bound == 0) { // The sequence has no hard bound
      bound = item_count + 1;
    } else {
      bound = bound > item_count ? item_count + 1 : bound;
    }
  }
  if (!check_index_from_id(tk, id, bound)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) DynamicDataImpl::check_out_of_bound_write:"
                 " Invalid ID: %u. Total number of elements: %u\n", id, item_count));
    }
    return false;
  }
  return true;
}

template<TypeKind ElementTypeKind, typename ElementType>
DDS::ReturnCode_t DynamicDataImpl::set_value_to_collection(DDS::MemberId id, const ElementType& value)
{
  if (!check_out_of_bound_write(id)) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  // Check the compatibility of the element type.
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  TypeKind treat_elem_as = elem_tk;
  if (elem_tk == TK_ENUM && enum_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }
  if (elem_tk == TK_BITMASK && bitmask_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  if (treat_elem_as != ElementTypeKind) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_value_to_collection:"
                 " Can't write a value of type %C to a collection with element type %C\n",
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_tk)));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }
  insert_single(id, value);
  return DDS::RETCODE_OK;
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::set_single_value(DDS::MemberId id, const ValueType& value)
{
  if (!is_type_supported(ValueTypeKind, "set_single_value")) {
    return DDS::RETCODE_ERROR;
  }

  const TypeKind tk = type_->get_kind();
  TypeKind treat_as = tk;
  if (tk == TK_ENUM && enum_bound(type_, treat_as) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  // TODO: Bitmask can be stored as a whole as a unsigned integer using MEMBER_ID_INVALID
  // (this is an extension to the XTypes spec). Elements of the bitmask can also be set
  // using the set_boolean_value interface. The two copies of the bitmask value must be
  // made consistent. For example, when a bit in the bitmask is updated, either update
  // the unsigned integer representation or invalidate it. Similarly, when the unsigned
  // integer value is updated, either update the stored elements or invalidate them all.
  if (tk == TK_BITMASK && bitmask_bound(type_, treat_as) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  switch (treat_as) {
  case ValueTypeKind:
    if (!is_primitive(treat_as) || id != MEMBER_ID_INVALID) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    insert_single(id, value);
    return DDS::RETCODE_OK;
  case TK_STRUCTURE:
    return set_value_to_struct<ValueTypeKind>(id, value);
  case TK_UNION:
    return set_value_to_union<ValueTypeKind>(id, value);
  case TK_SEQUENCE:
  case TK_ARRAY:
    return set_value_to_collection<ValueTypeKind>(id, value);
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_single_value:"
                 " Called on unexpected type %C\n", typekind_to_string(tk)));
    }
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::set_int32_value(DDS::MemberId id, CORBA::Long value)
{
  return set_single_value<TK_INT32>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint32_value(DDS::MemberId id, CORBA::ULong value)
{
  return set_single_value<TK_UINT32>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_int8_value(DDS::MemberId id, CORBA::Int8 value)
{
  return set_single_value<TK_INT8>(id, ACE_OutputCDR::from_int8(value));
}

DDS::ReturnCode_t DynamicDataImpl::set_uint8_value(DDS::MemberId id, CORBA::UInt8 value)
{
  return set_single_value<TK_UINT8>(id, ACE_OutputCDR::from_uint8(value));
}

DDS::ReturnCode_t DynamicDataImpl::set_int16_value(DDS::MemberId id, CORBA::Short value)
{
  return set_single_value<TK_INT16>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint16_value(DDS::MemberId id, CORBA::UShort value)
{
  return set_single_value<TK_UINT16>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_int64_value(DDS::MemberId id, CORBA::LongLong value)
{
  return set_single_value<TK_INT64>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint64_value(DDS::MemberId id, CORBA::ULongLong value)
{
  return set_single_value<TK_UINT64>(id, value);
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

template<TypeKind CharKind, TypeKind StringKind, typename FromCharT>
DDS::ReturnCode_t DynamicDataImpl::set_char_common(DDS::MemberId id, const FromCharT& value)
{
  const TypeKind tk = type_->get_kind();

  switch (tk) {
  case CharKind:
    if (id != MEMBER_ID_INVALID) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    insert_single(id, value);
    return DDS::RETCODE_OK;
  case TK_STRUCTURE:
    return set_value_to_struct<CharKind>(id, value);
  case TK_UNION:
    return set_value_to_union<CharKind>(id, value);
  case StringKind:
  case TK_SEQUENCE:
  case TK_ARRAY:
    return set_value_to_collection<CharKind>(id, value);
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_char_common:"
                 " Called on unexpected type %C\n", typekind_to_string(tk)));
    }
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::set_char8_value(DDS::MemberId id, CORBA::Char value)
{
  return set_char_common<TK_CHAR8, TK_STRING8>(id, ACE_OutputCDR::from_char(value));
}

DDS::ReturnCode_t DynamicDataImpl::set_char16_value(DDS::MemberId id, CORBA::WChar value)
{
#ifdef DDS_HAS_WCHAR
  return set_char_common<TK_CHAR16, TK_STRING16>(id, ACE_OutputCDR::from_wchar(value));
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataImpl::set_byte_value(DDS::MemberId id, CORBA::Octet value)
{
  return set_single_value<TK_BYTE>(id, ACE_OutputCDR::from_octet(value));
}

DDS::ReturnCode_t DynamicDataImpl::set_boolean_value(DDS::MemberId id, CORBA::Boolean value)
{
  const TypeKind tk = type_->get_kind();

  switch (tk) {
  case TK_BOOLEAN:
    if (id != MEMBER_ID_INVALID) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    insert_single(id, ACE_OutputCDR::from_boolean(value));
    return DDS::RETCODE_OK;
  case TK_BITMASK: {
    const CORBA::ULong bit_bound = type_desc_->bound()[0];
    if (!check_index_from_id(tk, id, bit_bound)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    insert_single(id, ACE_OutputCDR::from_boolean(value));
    return DDS::RETCODE_OK;
  }
  case TK_STRUCTURE:
    return set_value_to_struct<TK_BOOLEAN>(id, ACE_OutputCDR::from_boolean(value));
  case TK_UNION:
    return set_value_to_union<TK_BOOLEAN>(id, ACE_OutputCDR::from_boolean(value));
  case TK_SEQUENCE:
  case TK_ARRAY:
    return set_value_to_collection<TK_BOOLEAN>(id, ACE_OutputCDR::from_boolean(value));
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_boolean_value:"
                 " Called on unexpected type %C\n", typekind_to_string(tk)));
    }
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::set_string_value(DDS::MemberId id, const char* value)
{
  if (!value) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_string_value: Input string is null!\n"));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::DynamicType_var mtype;
  DDS::ReturnCode_t rc = get_member_type(mtype, type_, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  if (mtype->get_kind() == TK_ENUM) {
    DDS::Int32 intValue;
    rc = get_enumerator_value(intValue, value, mtype);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    return set_enum_value(mtype, this, id, intValue);
  }
  return set_single_value<TK_STRING8>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_wstring_value(DDS::MemberId id, const CORBA::WChar* value)
{
#ifdef DDS_HAS_WCHAR
  if (!value) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_wstring_value: Input wstring is null!\n"));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }
  return set_single_value<TK_STRING16>(id, value);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

bool DynamicDataImpl::serialized_size(const DCPS::Encoding& enc, size_t& size, DCPS::Sample::Extent ext) const
{
  DynamicDataImpl* non_const_this = const_cast<DynamicDataImpl*>(this);
  if (ext == DCPS::Sample::Full) {
    return DCPS::serialized_size(enc, size, non_const_this);
  } else if (ext == DCPS::Sample::KeyOnly) {
    DDS::DynamicData_ptr tmp = non_const_this;
    return DCPS::serialized_size(enc, size, DCPS::KeyOnly<DDS::DynamicData_ptr>(tmp));
  }
  return false;
}

bool DynamicDataImpl::serialize(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const
{
  DynamicDataImpl* non_const_this = const_cast<DynamicDataImpl*>(this);
  if (ext == DCPS::Sample::Full) {
    return ser << non_const_this;
  } else if (ext == DCPS::Sample::KeyOnly) {
    DDS::DynamicData_ptr tmp = non_const_this;
    return ser << DCPS::KeyOnly<DDS::DynamicData_ptr>(tmp);
  }
  return false;
}

DDS::ReturnCode_t DynamicDataImpl::set_complex_to_struct(DDS::MemberId id, DDS::DynamicData_var value)
{
  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, id) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }
  DDS::MemberDescriptor_var md;
  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  const DDS::DynamicType_var member_type = get_base_type(md->type());
  const DDS::DynamicType_var value_type = value->type();
  if (!member_type || !value_type || !member_type->equals(value_type)) {
    return DDS::RETCODE_BAD_PARAMETER;
  }
  insert_complex(id, value);
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::set_complex_to_union(DDS::MemberId id, DDS::DynamicData_var value)
{
  DDS::DynamicType_var member_type;
  DDS::MemberDescriptor_var md;
  if (!get_union_member_type(id, member_type, md)) {
    return DDS::RETCODE_ERROR;
  }

  const DDS::DynamicType_var value_type = value->type();
  if (!member_type->equals(value_type)) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  if (id == DISCRIMINATOR_ID) {
    CORBA::Long disc_val;
    DynamicDataImpl* dd_impl = dynamic_cast<DynamicDataImpl*>(value.in());
    if (!dd_impl || !dd_impl->read_discriminator(disc_val)) {
      return DDS::RETCODE_ERROR;
    }
    const DDS::ReturnCode_t rc = set_union_discriminator_helper(disc_val, "set_complex_to_union");
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    insert_complex(id, value);
    return DDS::RETCODE_OK;
  }

  bool has_existing_branch = false;
  DDS::MemberDescriptor_var existing_md;
  if (!find_selected_union_branch(has_existing_branch, existing_md)) {
    return DDS::RETCODE_ERROR;
  }

  // Update the value for the existing branch.
  if (has_existing_branch && existing_md->id() == id) {
    insert_complex(id, value);
    return DDS::RETCODE_OK;
  }

  // Set the union to the new branch.
  clear_container();

  if (!insert_valid_discriminator(md) || !insert_complex(id, value)) {
    return DDS::RETCODE_ERROR;
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::set_complex_to_collection(DDS::MemberId id, DDS::DynamicData_var value)
{
  if (!check_out_of_bound_write(id)) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const DDS::DynamicType_var value_type = value->type();
  if (!elem_type->equals(value_type)) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  insert_complex(id, value);
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::set_complex_value(DDS::MemberId id, DDS::DynamicData_ptr value)
{
  DDS::DynamicData_var value_var = DDS::DynamicData::_duplicate(value);
  const TypeKind tk = type_->get_kind();

  switch (tk) {
  case TK_STRUCTURE:
    return set_complex_to_struct(id, value_var);
  case TK_UNION:
    return set_complex_to_union(id, value_var);
  case TK_SEQUENCE:
  case TK_ARRAY:
    return set_complex_to_collection(id, value_var);
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_complex_value:"
                 " Called on unexpected type %C\n", typekind_to_string(tk)));
    }
  }
  return DDS::RETCODE_ERROR;
}

template<typename SequenceType>
bool DynamicDataImpl::insert_sequence(DDS::MemberId id, const SequenceType& value)
{
  if (container_.complex_map_.erase(id) == 0) {
    container_.sequence_map_.erase(id);
  }
  return container_.sequence_map_.insert(std::make_pair(id, value)).second;
}

// Check that the member at the given Id has a compatible type.
template<TypeKind ElementTypeKind>
bool DynamicDataImpl::check_seqmem_in_struct_and_union(DDS::MemberId id, DDS::MemberDescriptor_var& md) const
{
  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, id) != DDS::RETCODE_OK) {
    return false;
  }
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
  TypeKind treat_elem_as = elem_tk;
  if (elem_tk == TK_ENUM && enum_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (elem_tk == TK_BITMASK && bitmask_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }

  return treat_elem_as == ElementTypeKind;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::set_values_to_struct(DDS::MemberId id, const SequenceType& value)
{
  DDS::MemberDescriptor_var md;
  return check_seqmem_in_struct_and_union<ElementTypeKind>(id, md)
    && insert_sequence(id, value);
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::set_values_to_union(DDS::MemberId id, const SequenceType& value)
{
  DDS::MemberDescriptor_var md;
  if (!check_seqmem_in_struct_and_union<ElementTypeKind>(id, md)) {
    return false;
  }

  bool has_existing_branch = false;
  DDS::MemberDescriptor_var existing_md;
  if (!find_selected_union_branch(has_existing_branch, existing_md)) {
    return false;
  }

  if (has_existing_branch && existing_md->id() == id) {
    return insert_sequence(id, value);
  }

  clear_container();

  return insert_valid_discriminator(md) && insert_sequence(id, value);
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::set_values_to_collection(DDS::MemberId id, const SequenceType& value)
{
  if (!check_out_of_bound_write(id)) {
    return false;
  }

  // Check that the element type is compatible with the input sequence.
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  if (elem_type->get_kind() != TK_SEQUENCE) {
    return false;
  }

  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var nested_elem_type = get_base_type(elem_td->element_type());
  const TypeKind nested_elem_tk = nested_elem_type->get_kind();
  TypeKind treat_nested_as = nested_elem_tk;
  if (nested_elem_tk == TK_ENUM && enum_bound(nested_elem_type, treat_nested_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (nested_elem_tk == TK_BITMASK && bitmask_bound(nested_elem_type, treat_nested_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (nested_elem_tk != ElementTypeKind) {
    return false;
  }

  // The length must be at most the bound of the element sequence type.
  const CORBA::ULong bound = bound_total(elem_td);
  if (bound > 0 && bound < value.length()) {
    return false;
  }

  return insert_sequence(id, value);
}

template<TypeKind ElementTypeKind, typename SequenceType>
DDS::ReturnCode_t DynamicDataImpl::set_sequence_values(DDS::MemberId id, const SequenceType& value)
{
  if (!is_type_supported(ElementTypeKind, "set_sequence_values")) {
    return DDS::RETCODE_ERROR;
  }

  const TypeKind tk = type_->get_kind();
  bool good = false;

  switch (tk) {
  case TK_STRUCTURE:
    good = set_values_to_struct<ElementTypeKind>(id, value);
    break;
  case TK_UNION:
    good = set_values_to_union<ElementTypeKind>(id, value);
    break;
  case TK_SEQUENCE:
  case TK_ARRAY:
    good = set_values_to_collection<ElementTypeKind>(id, value);
    break;
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_sequence_values:"
                 " Write to unsupported type (%C)\n", typekind_to_string(tk)));
    }
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
  return set_sequence_values<TK_INT32>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint32_values(DDS::MemberId id, const DDS::UInt32Seq& value)
{
  return set_sequence_values<TK_UINT32>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_int8_values(DDS::MemberId id, const DDS::Int8Seq& value)
{
  return set_sequence_values<TK_INT8>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint8_values(DDS::MemberId id, const DDS::UInt8Seq& value)
{
  return set_sequence_values<TK_UINT8>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_int16_values(DDS::MemberId id, const DDS::Int16Seq& value)
{
  return set_sequence_values<TK_INT16>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint16_values(DDS::MemberId id, const DDS::UInt16Seq& value)
{
  return set_sequence_values<TK_UINT16>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_int64_values(DDS::MemberId id, const DDS::Int64Seq& value)
{
  return set_sequence_values<TK_INT64>(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint64_values(DDS::MemberId id, const DDS::UInt64Seq& value)
{
  return set_sequence_values<TK_UINT64>(id, value);
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

bool DynamicDataImpl::read_basic_value(ACE_OutputCDR::from_int8& value)
{
  return DDS::RETCODE_OK == get_int8_value(value.val_, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(ACE_OutputCDR::from_uint8& value)
{
  return DDS::RETCODE_OK == get_uint8_value(value.val_, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(CORBA::Short& value)
{
  return DDS::RETCODE_OK == get_int16_value(value, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(CORBA::UShort& value)
{
  return DDS::RETCODE_OK == get_uint16_value(value, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(CORBA::Long& value)
{
  return DDS::RETCODE_OK == get_int32_value(value, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(CORBA::ULong& value)
{
  return DDS::RETCODE_OK == get_uint32_value(value, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(CORBA::LongLong& value)
{
  return DDS::RETCODE_OK == get_int64_value(value, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(CORBA::ULongLong& value)
{
  return DDS::RETCODE_OK == get_uint64_value(value, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(CORBA::Float& value)
{
  return DDS::RETCODE_OK == get_float32_value(value, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(CORBA::Double& value)
{
  return DDS::RETCODE_OK == get_float64_value(value, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(CORBA::LongDouble& value)
{
  return DDS::RETCODE_OK == get_float128_value(value, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(ACE_OutputCDR::from_char& value)
{
  return DDS::RETCODE_OK == get_char8_value(value.val_, MEMBER_ID_INVALID);
}

#ifdef DDS_HAS_WCHAR
bool DynamicDataImpl::read_basic_value(ACE_OutputCDR::from_wchar& value)
{
  return DDS::RETCODE_OK == get_char16_value(value.val_, MEMBER_ID_INVALID);
}
#endif

bool DynamicDataImpl::read_basic_value(ACE_OutputCDR::from_octet& value)
{
  return DDS::RETCODE_OK == get_byte_value(value.val_, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(ACE_OutputCDR::from_boolean& value)
{
  return DDS::RETCODE_OK == get_boolean_value(value.val_, MEMBER_ID_INVALID);
}

bool DynamicDataImpl::read_basic_value(char*& value) const
{
  const bool is_empty = container_.single_map_.empty() && container_.complex_map_.empty();
  if (!is_empty) {
    CORBA::ULong largest_index;
    if (!container_.get_largest_index_basic(largest_index)) {
      return false;
    }
    const CORBA::ULong length = largest_index + 2;
    CORBA::String_var str_var = CORBA::string_alloc(length);
    ACE_OS::memset(str_var.inout(), 0, length);
    if (!reconstruct_string_value(str_var.inout())) {
      return false;
    }
    CORBA::string_free(value);
    value = str_var._retn();
  } else {
    CORBA::string_free(value);
    value = CORBA::string_dup("");
  }
  return true;
}

#ifdef DDS_HAS_WCHAR
bool DynamicDataImpl::read_basic_value(CORBA::WChar*& value) const
{
  const bool is_empty = container_.single_map_.empty() && container_.complex_map_.empty();
  if (!is_empty) {
    CORBA::ULong largest_index;
    if (!container_.get_largest_index_basic(largest_index)) {
      return false;
    }
    const CORBA::ULong length = largest_index + 2;
    CORBA::WString_var wstr_var = CORBA::wstring_alloc(length);
    ACE_OS::memset(wstr_var.inout(), 0, length * sizeof(CORBA::WChar));
    if (!reconstruct_wstring_value(wstr_var.inout())) {
      return false;
    }
    CORBA::wstring_free(value);
    value = wstr_var._retn();
  } else {
    CORBA::wstring_free(value);
    value = CORBA::wstring_dup(L"");
  }
  return true;
}
#endif

template<typename ValueType>
bool DynamicDataImpl::read_basic_in_single_map(ValueType& value, DDS::MemberId id)
{
  const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get<ValueType>();
    return true;
  }
  return false;
}

template<>
bool DynamicDataImpl::read_basic_in_single_map(char*& value, DDS::MemberId id)
{
  const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get_string();
    return true;
  }
  return false;
}

template<>
bool DynamicDataImpl::read_basic_in_single_map(CORBA::WChar*& value, DDS::MemberId id)
{
  const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get_wstring();
    return true;
  }
  return false;
}

template<typename ValueType>
bool DynamicDataImpl::read_basic_in_complex_map(ValueType& value, DDS::MemberId id)
{
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    DynamicDataImpl* nested_dd = dynamic_cast<DynamicDataImpl*>(complex_it->second.in());
    return nested_dd && nested_dd->read_basic_value(value);
  }
  return false;
}

void DynamicDataImpl::set_backing_store(DDS::DynamicData_ptr backing_store)
{
  backing_store_ = backing_store;
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_int8& value,
                                                   DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_int8_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_uint8& value,
                                                   DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_uint8_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::Short& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_int16_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::UShort& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_uint16_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::Long& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_int32_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::ULong& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_uint32_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::LongLong& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_int64_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::ULongLong& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_uint64_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::Float& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_float32_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::Double& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_float64_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::LongDouble& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_float128_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_octet& value,
                                                   DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_byte_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(char*& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_string_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::WChar*& value, DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_wstring_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_char& value,
                                                   DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_char8_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_wchar& value,
                                                   DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_char16_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_boolean& value,
                                                   DDS::MemberId id)
{
  return backing_store_ && backing_store_->get_boolean_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

template<typename ValueType>
bool DynamicDataImpl::read_basic_member(ValueType& value, DDS::MemberId id)
{
  return read_basic_in_single_map(value, id)
    || read_basic_in_complex_map(value, id)
    || get_value_from_backing_store(value, id);
}

template<typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_self(ValueType& value, DDS::MemberId id)
{
  // Primitive or enum value can be read using MEMBER_ID_INVALID.
  if (!is_primitive(type_->get_kind())) {
    return DDS::RETCODE_ERROR;
  }
  if (id != MEMBER_ID_INVALID) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    value = it->second.get<ValueType>();
  } else if (!get_value_from_backing_store(value, id)) {
    set_default_basic_value(value);
  }
  return DDS::RETCODE_OK;
}

template<>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_self(char*&, DDS::MemberId)
{
  // Can't read a string from a DynamicData instance representing a string.
  return DDS::RETCODE_ERROR;
}

template<>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_self(CORBA::WChar*&, DDS::MemberId)
{
  // Can't read a wstring from a DynamicData instance representing a wstring.
  return DDS::RETCODE_ERROR;
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_enum(ValueType& value, DDS::MemberId id)
{
  TypeKind treat_as_tk;
  const DDS::ReturnCode_t rc = enum_bound(type_, treat_as_tk);
  if (rc != DDS::RETCODE_OK || treat_as_tk != ValueTypeKind) {
    return DDS::RETCODE_ERROR;
  }
  if (id != MEMBER_ID_INVALID) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    value = it->second.get<ValueType>();
  } else if (!get_value_from_backing_store(value, id)) {
    CORBA::Long enum_default_val;
    if (!set_default_enum_value(type_, enum_default_val)) {
      return DDS::RETCODE_ERROR;
    }
    cast_to_enum_value(value, enum_default_val);
  }

  return DDS::RETCODE_OK;
}

template<>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_enum<TK_STRING8>(char*&, DDS::MemberId)
{
  return DDS::RETCODE_ERROR;
}
template<>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_enum<TK_STRING16>(CORBA::WChar*&, DDS::MemberId)
{
  return DDS::RETCODE_ERROR;
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_bitmask(ValueType& value, DDS::MemberId id)
{
  // Allow bitmask to be read as an unsigned integer.
  TypeKind treat_as_tk;
  const DDS::ReturnCode_t rc = bitmask_bound(type_, treat_as_tk);
  if (rc != DDS::RETCODE_OK || treat_as_tk != ValueTypeKind) {
    return DDS::RETCODE_ERROR;
  }
  if (id != MEMBER_ID_INVALID) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    value = it->second.get<ValueType>();
  } else if (!get_value_from_backing_store(value, id)) {
    set_default_bitmask_value(value);
  }
  return DDS::RETCODE_OK;
}

template<>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_bitmask<TK_STRING8>(char*&, DDS::MemberId)
{
  return DDS::RETCODE_ERROR;
}
template<>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_bitmask<TK_STRING16>(CORBA::WChar*&, DDS::MemberId)
{
  return DDS::RETCODE_ERROR;
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_struct(ValueType& value, DDS::MemberId id)
{
  DDS::MemberDescriptor_var md;
  DDS::DynamicType_var member_type;
  DDS::ReturnCode_t rc = check_member(
    md, member_type, "DynamicDataImpl::get_value_from_struct", "get", id, ValueTypeKind);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  if (read_basic_member(value, id)) {
    return DDS::RETCODE_OK;
  }

  // Not returning a default value for a missing optional member.
  if (md->is_optional()) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_value_from_struct:"
                 " Optional member Id %u is not present\n", id));
    }
    return DDS::RETCODE_NO_DATA;
  }

  set_default_basic_value(value);
  return DDS::RETCODE_OK;
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_union(ValueType& value, DDS::MemberId id)
{
  DDS::MemberDescriptor_var md;
  DDS::DynamicType_var member_type;
  DDS::ReturnCode_t rc = check_member(
    md, member_type, "DynamicDataImpl::get_value_from_union", "get", id, ValueTypeKind);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  if (id == DISCRIMINATOR_ID) {
    if (!read_basic_member(value, id)) {
      set_default_basic_value(value);
    }
    return DDS::RETCODE_OK;
  }

  // Branch
  bool has_selected_branch;
  DDS::MemberDescriptor_var selected_md;
  if (get_selected_union_branch(has_selected_branch, selected_md) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  // Requesting a branch that is not selected.
  if (!has_selected_branch || selected_md->id() != id) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  if (read_basic_member(value, id)) {
    return DDS::RETCODE_OK;
  }

  // The selected branch has not been set explicitly, either because the union data
  // is completely empty, or the selected branch is optional and hasn't been set.
  if (selected_md->is_optional()) {
    return DDS::RETCODE_NO_DATA;
  }
  set_default_basic_value(value);
  return DDS::RETCODE_OK;
}

void DynamicDataImpl::cast_to_enum_value(ACE_OutputCDR::from_int8& dst, CORBA::Long src) const
{
  dst = ACE_OutputCDR::from_int8(static_cast<CORBA::Int8>(src));
}

void DynamicDataImpl::cast_to_enum_value(CORBA::Short& dst, CORBA::Long src) const
{
  dst = static_cast<CORBA::Short>(src);
}

void DynamicDataImpl::cast_to_enum_value(CORBA::Long& dst, CORBA::Long src) const
{
  dst = src;
}

template<typename ValueType>
void DynamicDataImpl::cast_to_enum_value(ValueType&, CORBA::Long) const
{}

// Check for out-of-bound read to a collection-like type (sequence, array, string)
bool DynamicDataImpl::check_out_of_bound_read(DDS::MemberId id)
{
  const TypeKind tk = type_->get_kind();
  CORBA::ULong bound = bound_total(type_desc_);
  const CORBA::ULong item_count = get_item_count();
  if (tk == TK_SEQUENCE || tk == TK_STRING8 || tk == TK_STRING16) {
    if (bound > 0) {
      bound = item_count;
    }
  }
  if (!check_index_from_id(tk, id, bound)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) DynamicDataImpl::check_out_of_bound_read:"
                 " Invalid ID: %u. Total number of elements: %u\n", id, item_count));
    }
    return false;
  }
  return true;
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_value_from_collection(ValueType& value, DDS::MemberId id)
{
  if (!check_out_of_bound_read(id)) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  // Check the element type
  DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  TypeKind treat_as_tk = elem_tk;
  switch (elem_tk) {
  case TK_ENUM:
    if (enum_bound(elem_type, treat_as_tk) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }
    break;
  case TK_BITMASK: {
    if (bitmask_bound(elem_type, treat_as_tk) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }
    break;
  }
  }
  if (treat_as_tk != ValueTypeKind) {
    return DDS::RETCODE_ERROR;
  }

  // For sequence or string, as long as there is no out-of-range access,
  // it'll read successfully from the container or the backing store.
  if (!read_basic_member(value, id)) {
    // This may only happen for array.
    set_default_basic_value(value);
  }
  return DDS::RETCODE_OK;
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_single_value(ValueType& value, DDS::MemberId id)
{
  if (!is_type_supported(ValueTypeKind, "get_single_value")) {
    return DDS::RETCODE_ERROR;
  }
  const TypeKind tk = type_->get_kind();

  switch (tk) {
  case ValueTypeKind:
    return get_value_from_self(value, id);
  case TK_ENUM:
    return get_value_from_enum<ValueTypeKind>(value, id);
  case TK_BITMASK:
    return get_value_from_bitmask<ValueTypeKind>(value, id);
  case TK_STRUCTURE:
    return get_value_from_struct<ValueTypeKind>(value, id);
  case TK_UNION:
    return get_value_from_union<ValueTypeKind>(value, id);
  case TK_SEQUENCE:
  case TK_ARRAY:
    return get_value_from_collection<ValueTypeKind>(value, id);
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_single_value:"
                 " Called on unexpected type %C\n", typekind_to_string(tk)));
    }
    break;
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_int8_value(CORBA::Int8& value, DDS::MemberId id)
{
  ACE_OutputCDR::from_int8 from_int8(value);
  const DDS::ReturnCode_t rc = get_single_value<TK_INT8>(from_int8, id);
  value = from_int8.val_;
  return rc;
}

DDS::ReturnCode_t DynamicDataImpl::get_uint8_value(CORBA::UInt8& value, DDS::MemberId id)
{
  ACE_OutputCDR::from_uint8 from_uint8(value);
  const DDS::ReturnCode_t rc = get_single_value<TK_UINT8>(from_uint8, id);
  value = from_uint8.val_;
  return rc;
}

DDS::ReturnCode_t DynamicDataImpl::get_int16_value(CORBA::Short& value, DDS::MemberId id)
{
  return get_single_value<TK_INT16>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint16_value(CORBA::UShort& value, DDS::MemberId id)
{
  return get_single_value<TK_UINT16>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_int32_value(CORBA::Long& value, DDS::MemberId id)
{
  return get_single_value<TK_INT32>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint32_value(CORBA::ULong& value, DDS::MemberId id)
{
  return get_single_value<TK_UINT32>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_int64_value_impl(CORBA::LongLong& value, DDS::MemberId id)
{
  return get_single_value<TK_INT64>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint64_value_impl(CORBA::ULongLong& value, DDS::MemberId id)
{
  return get_single_value<TK_UINT64>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_float32_value(CORBA::Float& value, DDS::MemberId id)
{
  return get_single_value<TK_FLOAT32>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_float64_value(CORBA::Double& value, DDS::MemberId id)
{
  return get_single_value<TK_FLOAT64>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_float128_value(CORBA::LongDouble& value, DDS::MemberId id)
{
  return get_single_value<TK_FLOAT128>(value, id);
}

template<TypeKind CharKind, TypeKind StringKind, typename FromCharT, typename CharT>
DDS::ReturnCode_t DynamicDataImpl::get_char_common(CharT& value, DDS::MemberId id)
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case CharKind: {
    FromCharT from_char(value);
    const DDS::ReturnCode_t rc = get_value_from_self(from_char, id);
    value = from_char.val_;
    return rc;
  }
  case TK_STRUCTURE: {
    FromCharT from_char(value);
    const DDS::ReturnCode_t rc = get_value_from_struct<CharKind>(from_char, id);
    value = from_char.val_;
    return rc;
  }
  case TK_UNION: {
    FromCharT from_char(value);
    const DDS::ReturnCode_t rc = get_value_from_union<CharKind>(from_char, id);
    value = from_char.val_;
    return rc;
  }
  case StringKind:
  case TK_SEQUENCE:
  case TK_ARRAY: {
    FromCharT from_char(value);
    const DDS::ReturnCode_t rc = get_value_from_collection<CharKind>(from_char, id);
    value = from_char.val_;
    return rc;
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_char_common:"
                 " Called on unexpected type %C\n", typekind_to_string(tk)));
    }
    break;
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_char8_value(CORBA::Char& value, DDS::MemberId id)
{
  return get_char_common<TK_CHAR8, TK_STRING8, ACE_OutputCDR::from_char>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_char16_value(CORBA::WChar& value, DDS::MemberId id)
{
#ifdef DDS_HAS_WCHAR
  return get_char_common<TK_CHAR16, TK_STRING16, ACE_OutputCDR::from_wchar>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataImpl::get_byte_value(CORBA::Octet& value, DDS::MemberId id)
{
  ACE_OutputCDR::from_octet from_octet(value);
  const DDS::ReturnCode_t rc = get_single_value<TK_BYTE>(from_octet, id);
  value = from_octet.val_;
  return rc;
}

template<typename UIntType>
DDS::ReturnCode_t DynamicDataImpl::get_boolean_from_bitmask(CORBA::ULong index, CORBA::Boolean& value)
{
  UIntType bitmask;
  if (!read_basic_value(bitmask)) {
    return DDS::RETCODE_ERROR;
  }
  value = (1ULL << index) & bitmask;
  return DDS::RETCODE_OK;
}

template<>
DDS::ReturnCode_t DynamicDataImpl::get_boolean_from_bitmask<CORBA::UInt8>(CORBA::ULong index, CORBA::Boolean& value)
{
  ACE_OutputCDR::from_uint8 bitmask(0);
  if (!read_basic_value(bitmask)) {
    return DDS::RETCODE_ERROR;
  }
  value = ((1 << index) & bitmask.val_) ? true : false;
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::get_boolean_value(CORBA::Boolean& value, DDS::MemberId id)
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case TK_BOOLEAN: {
    ACE_OutputCDR::from_boolean from_bool(value);
    const DDS::ReturnCode_t rc = get_value_from_self(from_bool, id);
    value = from_bool.val_;
    return rc;
  }
  case TK_BITMASK: {
    const LBound bitbound = type_desc_->bound()[0];
    ACE_CDR::ULong index;
    if (!get_index_from_id(id, index, bitbound)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    if (bitbound >= 1 && bitbound <= 8) {
      return get_boolean_from_bitmask<CORBA::UInt8>(index, value);
    } else if (bitbound >= 9 && bitbound <= 16) {
      return get_boolean_from_bitmask<CORBA::UShort>(index, value);
    } else if (bitbound >= 17 && bitbound <= 32) {
      return get_boolean_from_bitmask<CORBA::ULong>(index, value);
    }
    return get_boolean_from_bitmask<CORBA::ULongLong>(index, value);
  }
  case TK_STRUCTURE: {
    ACE_OutputCDR::from_boolean from_bool(value);
    const DDS::ReturnCode_t rc = get_value_from_struct<TK_BOOLEAN>(from_bool, id);
    value = from_bool.val_;
    return rc;
  }
  case TK_UNION: {
    ACE_OutputCDR::from_boolean from_bool(value);
    const DDS::ReturnCode_t rc = get_value_from_union<TK_BOOLEAN>(from_bool, id);
    value = from_bool.val_;
    return rc;
  }
  case TK_SEQUENCE:
  case TK_ARRAY: {
    ACE_OutputCDR::from_boolean from_bool(value);
    const DDS::ReturnCode_t rc = get_value_from_collection<TK_BOOLEAN>(from_bool, id);
    value = from_bool.val_;
    return rc;
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_boolean_value:"
                 " Called on unexpected type %C\n", typekind_to_string(tk)));
    }
    break;
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_string_value(char*& value, DDS::MemberId id)
{
  if (enum_string_helper(value, id)) {
    return DDS::RETCODE_OK;
  }

  CORBA::string_free(value);
  // Set to nil since get_string_value of the backing store may be called which
  // also releases the memory pointed to by value, causing double-free error.
  value = 0;
  return get_single_value<TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_wstring_value(CORBA::WChar*& value, DDS::MemberId id)
{
#ifdef DDS_HAS_WCHAR
  CORBA::wstring_free(value);
  value = 0;
  return get_single_value<TK_STRING16>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

bool DynamicDataImpl::move_single_to_complex(const const_single_iterator& it, DynamicDataImpl* data)
{
  DDS::DynamicType_var member_type = data->type();
  const TypeKind member_tk = member_type->get_kind();
  TypeKind treat_as = member_tk;
  if (member_tk == TK_ENUM) {
    if (enum_bound(member_type, treat_as) != DDS::RETCODE_OK) {
      return false;
    }
  }
  return move_single_to_complex_i(it, data, treat_as);
}

bool DynamicDataImpl::move_single_to_complex_i(const const_single_iterator& it,
                                               DynamicDataImpl* data, const TypeKind treat_as)
{
  switch (treat_as) {
  case TK_INT8: {
    const ACE_OutputCDR::from_int8& value = it->second.get<ACE_OutputCDR::from_int8>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_UINT8: {
    const ACE_OutputCDR::from_uint8& value = it->second.get<ACE_OutputCDR::from_uint8>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_INT16: {
    const CORBA::Short value = it->second.get<CORBA::Short>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_UINT16: {
    const CORBA::UShort value = it->second.get<CORBA::UShort>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_INT32: {
    const CORBA::Long value = it->second.get<CORBA::Long>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_UINT32: {
    const CORBA::ULong value = it->second.get<CORBA::ULong>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_INT64: {
    const CORBA::LongLong value = it->second.get<CORBA::LongLong>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_UINT64: {
    const CORBA::ULongLong value = it->second.get<CORBA::ULongLong>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_FLOAT32: {
    const CORBA::Float value = it->second.get<CORBA::Float>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_FLOAT64: {
    const CORBA::Double value = it->second.get<CORBA::Double>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_FLOAT128: {
    const CORBA::LongDouble value = it->second.get<CORBA::LongDouble>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_CHAR8: {
    const ACE_OutputCDR::from_char& value = it->second.get<ACE_OutputCDR::from_char>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    const ACE_OutputCDR::from_wchar& value = it->second.get<ACE_OutputCDR::from_wchar>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
#endif
  case TK_BYTE: {
    const ACE_OutputCDR::from_octet& value = it->second.get<ACE_OutputCDR::from_octet>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_BOOLEAN: {
    const ACE_OutputCDR::from_boolean& value = it->second.get<ACE_OutputCDR::from_boolean>();
    data->insert_single(MEMBER_ID_INVALID, value);
    break;
  }
  case TK_STRING8: {
    const char* str = it->second.get<const char*>();
    const size_t len = ACE_OS::strlen(str);
    for (CORBA::ULong i = 0; i < len; ++i) {
      data->insert_single(i, ACE_OutputCDR::from_char(str[i]));
    }
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    const CORBA::WChar* wstr = it->second.get<const CORBA::WChar*>();
    const size_t len = ACE_OS::strlen(wstr);
    for (CORBA::ULong i = 0; i < len; ++i) {
      data->insert_single(i, ACE_OutputCDR::from_wchar(wstr[i]));
    }
    break;
  }
#endif
  default:
    return false;
  }
  return true;
}

template<typename SequenceType>
void DynamicDataImpl::move_sequence_helper(const const_sequence_iterator& it, DynamicDataImpl* data)
{
  const SequenceType& values = it->second.get<SequenceType>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(index_to_id(i), values[i]);
  }
}

// Get the inner C-string explicitly
template<>
void DynamicDataImpl::move_sequence_helper<DDS::StringSeq>(const const_sequence_iterator& it,
                                                           DynamicDataImpl* data)
{
  const DDS::StringSeq& values = it->second.get<DDS::StringSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(index_to_id(i), values[i].in());
  }
}

#ifdef DDS_HAS_WCHAR
template<>
void DynamicDataImpl::move_sequence_helper<DDS::WstringSeq>(const const_sequence_iterator& it,
                                                            DynamicDataImpl* data)
{
  const DDS::WstringSeq& values = it->second.get<DDS::WstringSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(index_to_id(i), values[i].in());
  }
}
#endif

template<>
void DynamicDataImpl::move_sequence_helper<DDS::Int8Seq>(const const_sequence_iterator& it,
                                                         DynamicDataImpl* data)
{
  const DDS::Int8Seq& values = it->second.get<DDS::Int8Seq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(index_to_id(i), ACE_OutputCDR::from_int8(values[i]));
  }
}

template<>
void DynamicDataImpl::move_sequence_helper<DDS::UInt8Seq>(const const_sequence_iterator& it,
                                                          DynamicDataImpl* data)
{
  const DDS::UInt8Seq& values = it->second.get<DDS::UInt8Seq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(index_to_id(i), ACE_OutputCDR::from_uint8(values[i]));
  }
}

template<>
void DynamicDataImpl::move_sequence_helper<DDS::CharSeq>(const const_sequence_iterator& it,
                                                         DynamicDataImpl* data)
{
  const DDS::CharSeq& values = it->second.get<DDS::CharSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(index_to_id(i), ACE_OutputCDR::from_char(values[i]));
  }
}

template<>
void DynamicDataImpl::move_sequence_helper<DDS::ByteSeq>(const const_sequence_iterator& it,
                                                         DynamicDataImpl* data)
{
  const DDS::ByteSeq& values = it->second.get<DDS::ByteSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(index_to_id(i), ACE_OutputCDR::from_octet(values[i]));
  }
}

template<>
void DynamicDataImpl::move_sequence_helper<DDS::BooleanSeq>(const const_sequence_iterator& it,
                                                            DynamicDataImpl* data)
{
  const DDS::BooleanSeq& values = it->second.get<DDS::BooleanSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(index_to_id(i), ACE_OutputCDR::from_boolean(values[i]));
  }
}

#ifdef DDS_HAS_WCHAR
template<>
void DynamicDataImpl::move_sequence_helper<DDS::WcharSeq>(const const_sequence_iterator& it,
                                                          DynamicDataImpl* data)
{
  const DDS::WcharSeq& values = it->second.get<DDS::WcharSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(index_to_id(i), ACE_OutputCDR::from_wchar(values[i]));
  }
}
#endif

bool DynamicDataImpl::move_sequence_to_complex(const const_sequence_iterator& it,
                                               DynamicDataImpl* data)
{
  DDS::DynamicType_var seq_type = data->type();
  DDS::TypeDescriptor_var seq_td;
  if (seq_type->get_descriptor(seq_td) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::DynamicType_var elem_type = get_base_type(seq_td->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  TypeKind treat_elem_as = elem_tk;
  if (elem_tk == TK_ENUM && enum_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (elem_tk == TK_BITMASK && bitmask_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }

  switch (treat_elem_as) {
  case TK_INT8: {
    move_sequence_helper<DDS::Int8Seq>(it, data);
    break;
  }
  case TK_UINT8: {
    move_sequence_helper<DDS::UInt8Seq>(it, data);
    break;
  }
  case TK_INT16: {
    move_sequence_helper<DDS::Int16Seq>(it, data);
    break;
  }
  case TK_UINT16: {
    move_sequence_helper<DDS::UInt16Seq>(it, data);
    break;
  }
  case TK_INT32: {
    move_sequence_helper<DDS::Int32Seq>(it, data);
    break;
  }
  case TK_UINT32: {
    move_sequence_helper<DDS::UInt32Seq>(it, data);
    break;
  }
  case TK_INT64: {
    move_sequence_helper<DDS::Int64Seq>(it, data);
    break;
  }
  case TK_UINT64: {
    move_sequence_helper<DDS::UInt64Seq>(it, data);
    break;
  }
  case TK_FLOAT32: {
    move_sequence_helper<DDS::Float32Seq>(it, data);
    break;
  }
  case TK_FLOAT64: {
    move_sequence_helper<DDS::Float64Seq>(it, data);
    break;
  }
  case TK_FLOAT128: {
    move_sequence_helper<DDS::Float128Seq>(it, data);
    break;
  }
  case TK_CHAR8: {
    move_sequence_helper<DDS::CharSeq>(it, data);
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    move_sequence_helper<DDS::WcharSeq>(it, data);
    break;
  }
#endif
  case TK_BYTE: {
    move_sequence_helper<DDS::ByteSeq>(it, data);
    break;
  }
  case TK_BOOLEAN: {
    move_sequence_helper<DDS::BooleanSeq>(it, data);
    break;
  }
  case TK_STRING8: {
    move_sequence_helper<DDS::StringSeq>(it, data);
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    move_sequence_helper<DDS::WstringSeq>(it, data);
    break;
  }
#endif
  default:
    return false;
  }
  return true;
}

bool DynamicDataImpl::get_complex_from_container(DDS::DynamicData_var& value, DDS::MemberId id,
                                                 FoundStatus& found_status)
{
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    value = complex_it->second;
    found_status = FOUND_IN_COMPLEX_MAP;
    return true;
  }

  DDS::DynamicTypeMember_var dtm;
  if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var md;
  if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::DynamicType_var member_type = get_base_type(md->type());
  DynamicDataImpl* dd_impl = new DynamicDataImpl(member_type);
  DDS::DynamicData_var dd_var = dd_impl;

  const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    if (!move_single_to_complex(single_it, dd_impl)) {
      return false;
    }
    found_status = FOUND_IN_NON_COMPLEX_MAP;
  } else {
    const_sequence_iterator sequence_it = container_.sequence_map_.find(id);
    if (sequence_it != container_.sequence_map_.end()) {
      if (!move_sequence_to_complex(sequence_it, dd_impl)) {
        return false;
      }
      found_status = FOUND_IN_NON_COMPLEX_MAP;
    } else {
      found_status = NOT_FOUND;
    }
  }
  value = dd_var;
  return true;
}

DDS::ReturnCode_t DynamicDataImpl::set_member_backing_store(DynamicDataImpl* member_ddi,
                                                            DDS::MemberId id)
{
  if (!backing_store_) {
    return DDS::RETCODE_NO_DATA;
  }

  DDS::DynamicData_var member_dd;
  const DDS::ReturnCode_t rc = backing_store_->get_complex_value(member_dd, id);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_member_backing_store:"
                 " Get complex value for member ID: %u failed: %C\n", id, retcode_to_string(rc)));
    }
    return DDS::RETCODE_NO_DATA;
  }

  member_ddi->set_backing_store(DDS::DynamicData::_duplicate(member_dd));
  return DDS::RETCODE_OK;
}

// Get complex value for a member of a struct or a branch of a union.
DDS::ReturnCode_t DynamicDataImpl::get_complex_from_aggregated(DDS::DynamicData_var& dd_var,
                                                               DDS::MemberId id)
{
  // Whether the member is found in the container.
  FoundStatus found_status = NOT_FOUND;
  if (!get_complex_from_container(dd_var, id, found_status)) {
    return DDS::RETCODE_ERROR;
  }

  if (found_status == NOT_FOUND) {
    DynamicDataImpl* ddi = dynamic_cast<DynamicDataImpl*>(dd_var.in());
    if (!ddi) {
      return DDS::RETCODE_ERROR;
    }
    if (set_member_backing_store(ddi, id) != DDS::RETCODE_OK) {
      DDS::DynamicTypeMember_var dtm;
      if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
        return DDS::RETCODE_ERROR;
      }
      DDS::MemberDescriptor_var md;
      if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
        return DDS::RETCODE_ERROR;
      }
      if (md->is_optional()) {
        return DDS::RETCODE_NO_DATA;
      }
    }
  }

  if (found_status == NOT_FOUND || found_status == FOUND_IN_NON_COMPLEX_MAP) {
    insert_complex(id, dd_var);
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::get_complex_from_struct(DDS::DynamicData_ptr& value, DDS::MemberId id)
{
  DDS::MemberDescriptor_var md;
  if (get_descriptor(md, id) != DDS::RETCODE_OK) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::DynamicData_var dd_var;
  const DDS::ReturnCode_t rc = get_complex_from_aggregated(dd_var, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  CORBA::release(value);
  value = DDS::DynamicData::_duplicate(dd_var);
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::get_complex_from_union(DDS::DynamicData_ptr& value, DDS::MemberId id)
{
  DDS::MemberDescriptor_var md;
  if (get_descriptor(md, id) != DDS::RETCODE_OK) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  // Whether it is found in the container.
  FoundStatus found_status = NOT_FOUND;
  DDS::DynamicData_var dd_var;

  if (id == DISCRIMINATOR_ID) {
    if (!get_complex_from_container(dd_var, id, found_status)) {
      return DDS::RETCODE_ERROR;
    }
    if (found_status == NOT_FOUND) {
      DynamicDataImpl* ddi = dynamic_cast<DynamicDataImpl*>(dd_var.in());
      if (!ddi) {
        return DDS::RETCODE_ERROR;
      }
      const DDS::ReturnCode_t rc = set_member_backing_store(ddi, id);
      if (rc != DDS::RETCODE_OK && rc != DDS::RETCODE_NO_DATA) {
        return DDS::RETCODE_ERROR;
      }
    }

    if (found_status == NOT_FOUND || found_status == FOUND_IN_NON_COMPLEX_MAP) {
      insert_complex(id, dd_var);
    }
  } else { // Branch
    bool has_selected_branch;
    DDS::MemberDescriptor_var selected_md;
    if (get_selected_union_branch(has_selected_branch, selected_md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    // Requesting a branch that is not selected.
    if (!has_selected_branch || selected_md->id() != id) {
      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }

    const DDS::ReturnCode_t rc = get_complex_from_aggregated(dd_var, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
  }

  CORBA::release(value);
  value = DDS::DynamicData::_duplicate(dd_var);
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::get_complex_from_collection(DDS::DynamicData_ptr& value, DDS::MemberId id)
{
  if (!check_out_of_bound_read(id)) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    CORBA::release(value);
    value = DDS::DynamicData::_duplicate(complex_it->second);
    return DDS::RETCODE_OK;
  }

  DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  DynamicDataImpl* dd_impl = new DynamicDataImpl(elem_type);
  DDS::DynamicData_var dd_var = dd_impl;

  const_single_iterator single_it = container_.single_map_.find(id);
  bool found_in_maps = false;
  if (single_it != container_.single_map_.end()) {
    if (!move_single_to_complex(single_it, dd_impl)) {
      return DDS::RETCODE_ERROR;
    }
    found_in_maps = true;
  } else {
    const_sequence_iterator sequence_it = container_.sequence_map_.find(id);
    if (sequence_it != container_.sequence_map_.end()) {
      if (!move_sequence_to_complex(sequence_it, dd_impl)) {
        return DDS::RETCODE_ERROR;
      }
      found_in_maps = true;
    }
  }

  if (!found_in_maps) {
    const DDS::ReturnCode_t rc = set_member_backing_store(dd_impl, id);
    if (rc != DDS::RETCODE_OK && rc != DDS::RETCODE_NO_DATA) {
      return DDS::RETCODE_ERROR;
    }
  }
  insert_complex(id, dd_var);

  CORBA::release(value);
  value = DDS::DynamicData::_duplicate(dd_var);
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::get_complex_value(DDS::DynamicData_ptr& value, DDS::MemberId id)
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case TK_STRUCTURE:
    return get_complex_from_struct(value, id);
  case TK_UNION:
    return get_complex_from_union(value, id);
  case TK_SEQUENCE:
  case TK_ARRAY:
    return get_complex_from_collection(value, id);
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_complex_value:"
                 " Called on unexpected type %C\n", typekind_to_string(tk)));
    }
    break;
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_int32_values(DDS::Int32Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_uint32_values(DDS::UInt32Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_int8_values(DDS::Int8Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_uint8_values(DDS::UInt8Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_int16_values(DDS::Int16Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_uint16_values(DDS::UInt16Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_int64_values(DDS::Int64Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_uint64_values(DDS::UInt64Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_float32_values(DDS::Float32Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_float64_values(DDS::Float64Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_float128_values(DDS::Float128Seq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_char8_values(DDS::CharSeq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_char16_values(DDS::WcharSeq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_byte_values(DDS::ByteSeq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_boolean_values(DDS::BooleanSeq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_string_values(DDS::StringSeq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataImpl::get_wstring_values(DDS::WstringSeq& value, DDS::MemberId id)
{
  ACE_UNUSED_ARG(value);
  ACE_UNUSED_ARG(id);
  return DDS::RETCODE_UNSUPPORTED;
}

void DynamicDataImpl::DataContainer::clear()
{
  single_map_.clear();
  complex_map_.clear();
  sequence_map_.clear();
}

// Get largest index among elements of a sequence-like type written to the single map.
bool DynamicDataImpl::DataContainer::get_largest_single_index(CORBA::ULong& largest_index) const
{
  OPENDDS_ASSERT(is_sequence_like(type_->get_kind()));
  const DDS::UInt32 bound = bound_total(type_desc_);

  // Since ID is used as index in this implementation, the last element has largest index.
  // A different implementation (ID-to-index mapping) may need to iterate through all
  // stored elements to find the one with the largest index.
  return data_->get_index_from_id(single_map_.rbegin()->first, largest_index, bound);
}

// Get largest index among elements of a nesting sequence type written to the sequence map.
bool DynamicDataImpl::DataContainer::get_largest_sequence_index(CORBA::ULong& largest_index) const
{
  OPENDDS_ASSERT(type_->get_kind() == TK_SEQUENCE);
  const CORBA::ULong bound = type_desc_->bound()[0];
  return data_->get_index_from_id(sequence_map_.rbegin()->first, largest_index, bound);
}

// Get largest index among elements of a sequence-like type written to the complex map.
bool DynamicDataImpl::DataContainer::get_largest_complex_index(CORBA::ULong& largest_index) const
{
  OPENDDS_ASSERT(is_sequence_like(type_->get_kind()));
  const DDS::UInt32 bound = bound_total(type_desc_);
  return data_->get_index_from_id(complex_map_.rbegin()->first, largest_index, bound);
}

bool DynamicDataImpl::DataContainer::get_largest_index_basic(CORBA::ULong& largest_index) const
{
  largest_index = 0;
  if (!single_map_.empty() && !get_largest_single_index(largest_index)) {
    return false;
  }
  if (!complex_map_.empty()) {
    CORBA::ULong index;
    if (!get_largest_complex_index(index)) {
      return false;
    }
    largest_index = std::max(index, largest_index);
  }
  return true;
}

bool DynamicDataImpl::DataContainer::get_largest_index_basic_sequence(CORBA::ULong& largest_index) const
{
  largest_index = 0;
  if (!sequence_map_.empty() && !get_largest_sequence_index(largest_index)) {
    return false;
  }
  if (!complex_map_.empty()) {
    CORBA::ULong index;
    if (!get_largest_complex_index(index)) {
      return false;
    }
    largest_index = std::max(index, largest_index);
  }
  return true;
}

bool DynamicDataImpl::reconstruct_string_value(CORBA::Char* str) const
{
  const CORBA::ULong bound = type_desc_->bound()[0];
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    str[index] = it->second.get<ACE_OutputCDR::from_char>().val_;
  }
  for (const_complex_iterator it = container_.complex_map_.begin(); it != container_.complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    // The DynamicData object for this character may not contain any data.
    // Use default value for character if it is the case.
    const DynamicDataImpl* elem_dd = dynamic_cast<const DynamicDataImpl*>(it->second.in());
    if (!elem_dd) {
      return false;
    }
    const_single_iterator elem_it = elem_dd->container_.single_map_.find(MEMBER_ID_INVALID);
    if (elem_it != elem_dd->container_.single_map_.end()) {
      str[index] = elem_it->second.get<ACE_OutputCDR::from_char>().val_;
    } else {
      ACE_OutputCDR::from_char from_char('\0');
      set_default_basic_value(from_char);
      str[index] = from_char.val_;
    }
  }
  return true;
}

#ifdef DDS_HAS_WCHAR
bool DynamicDataImpl::reconstruct_wstring_value(CORBA::WChar* wstr) const
{
  const CORBA::ULong bound = type_desc_->bound()[0];
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    wstr[index] = it->second.get<ACE_OutputCDR::from_wchar>().val_;
  }
  for (const_complex_iterator it = container_.complex_map_.begin(); it != container_.complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    const DynamicDataImpl* elem_dd = dynamic_cast<const DynamicDataImpl*>(it->second.in());
    if (!elem_dd) {
      return false;
    }
    const_single_iterator elem_it = elem_dd->container_.single_map_.find(MEMBER_ID_INVALID);
    if (elem_it != elem_dd->container_.single_map_.end()) {
      wstr[index] = elem_it->second.get<ACE_OutputCDR::from_wchar>().val_;
    } else {
      ACE_OutputCDR::from_wchar from_wchar('\0');
      set_default_basic_value(from_wchar);
      wstr[index] = from_wchar.val_;
    }
  }
  return true;
}
#endif

// Group of functions to set default value for a basic type (Table 9).
// When MemberDescriptor::default_value is fully supported, it would
// have precedence over these default values.
void DynamicDataImpl::set_default_basic_value(CORBA::Long& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_basic_value(CORBA::ULong& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_basic_value(ACE_OutputCDR::from_int8& value) const
{
  value.val_ = 0;
}

void DynamicDataImpl::set_default_basic_value(ACE_OutputCDR::from_uint8& value) const
{
  value.val_ = 0;
}

void DynamicDataImpl::set_default_basic_value(CORBA::Short& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_basic_value(CORBA::UShort& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_basic_value(CORBA::LongLong& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_basic_value(CORBA::ULongLong& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_basic_value(CORBA::Float& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_basic_value(CORBA::Double& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_basic_value(CORBA::LongDouble& value) const
{
#if ACE_SIZEOF_LONG_DOUBLE == 16
  value = 0;
#else
  ACE_OS::memset(value.ld, 0, 16);
#endif
}

void DynamicDataImpl::set_default_basic_value(ACE_OutputCDR::from_char& value) const
{
  value.val_ = '\0';
}

void DynamicDataImpl::set_default_basic_value(ACE_OutputCDR::from_octet& value) const
{
  value.val_ = 0x00;
}

void DynamicDataImpl::set_default_basic_value(const char*& value) const
{
  value = "";
}

void DynamicDataImpl::set_default_basic_value(char*& value) const
{
  CORBA::string_free(value);
  value = CORBA::string_dup("");
}

void DynamicDataImpl::set_default_basic_value(ACE_OutputCDR::from_boolean& value) const
{
  value.val_ = false;
}

#ifdef DDS_HAS_WCHAR
void DynamicDataImpl::set_default_basic_value(ACE_OutputCDR::from_wchar& value) const
{
  value.val_ = '\0';
}

void DynamicDataImpl::set_default_basic_value(const CORBA::WChar*& value) const
{
  value = L"";
}

void DynamicDataImpl::set_default_basic_value(CORBA::WChar*& value) const
{
  CORBA::wstring_free(value);
  value = CORBA::wstring_dup(L"");
}
#endif

bool DynamicDataImpl::set_default_enum_value(const DDS::DynamicType_var& enum_type,
                                             CORBA::Long& value) const
{
  // Default enum value is the first enumerator.
  DDS::DynamicTypeMember_var first_dtm;
  if (enum_type->get_member_by_index(first_dtm, 0) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var first_md;
  if (first_dtm->get_descriptor(first_md) != DDS::RETCODE_OK) {
    return false;
  }
  value = static_cast<CORBA::Long>(first_md->id());
  return true;
}

void DynamicDataImpl::set_default_bitmask_value(ACE_OutputCDR::from_uint8& value) const
{
  value.val_ = 0;
}

void DynamicDataImpl::set_default_bitmask_value(CORBA::UShort& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_bitmask_value(CORBA::ULong& value) const
{
  value = 0;
}

void DynamicDataImpl::set_default_bitmask_value(CORBA::ULongLong& value) const
{
  value = 0;
}

template<typename Type>
void DynamicDataImpl::set_default_bitmask_value(Type&) const
{
  // No-op. Should never be called.
}

void DynamicDataImpl::set_default_primitive_values(DDS::Int8Seq& collection) const
{
  ACE_OutputCDR::from_int8 value(0);
  set_default_basic_value(value);
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    collection[i] = value.val_;
  }
}

void DynamicDataImpl::set_default_primitive_values(DDS::UInt8Seq& collection) const
{
  ACE_OutputCDR::from_uint8 value(0);
  set_default_basic_value(value);
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    collection[i] = value.val_;
  }
}

void DynamicDataImpl::set_default_primitive_values(DDS::CharSeq& collection) const
{
  ACE_OutputCDR::from_char value('\0');
  set_default_basic_value(value);
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    collection[i] = value.val_;
  }
}

void DynamicDataImpl::set_default_primitive_values(DDS::ByteSeq& collection) const
{
  ACE_OutputCDR::from_octet value(0x00);
  set_default_basic_value(value);
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    collection[i] = value.val_;
  }
}

void DynamicDataImpl::set_default_primitive_values(DDS::BooleanSeq& collection) const
{
  ACE_OutputCDR::from_boolean value(false);
  set_default_basic_value(value);
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    collection[i] = value.val_;
  }
}

#ifdef DDS_HAS_WCHAR
void DynamicDataImpl::set_default_primitive_values(DDS::WcharSeq& collection) const
{
  ACE_OutputCDR::from_wchar value(0);
  set_default_basic_value(value);
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    collection[i] = value.val_;
  }
}
#endif

template<typename CollectionType>
void DynamicDataImpl::set_default_primitive_values(CollectionType& collection) const
{
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    set_default_basic_value(collection[i]);
  }
}

// Set discriminator to the default value of the corresponding type.
bool DynamicDataImpl::set_default_discriminator_value(CORBA::Long& value,
                                                      const DDS::DynamicType_var& disc_type) const
{
  const TypeKind disc_tk = disc_type->get_kind();
  switch (disc_tk) {
  case TK_BOOLEAN: {
    ACE_OutputCDR::from_boolean val(false);
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val.val_);
    return true;
  }
  case TK_BYTE: {
    ACE_OutputCDR::from_octet val(0x00);
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val.val_);
    return true;
  }
  case TK_CHAR8: {
    ACE_OutputCDR::from_char val('\0');
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val.val_);
    return true;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    ACE_OutputCDR::from_wchar val(0);
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val.val_);
    return true;
  }
#endif
  case TK_INT8: {
    ACE_OutputCDR::from_int8 val(0);
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val.val_);
    return true;
  }
  case TK_UINT8: {
    ACE_OutputCDR::from_uint8 val(0);
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val.val_);
    return true;
  }
  case TK_INT16: {
    CORBA::Short val;
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val);
    return true;
  }
  case TK_UINT16: {
    CORBA::UShort val;
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val);
    return true;
  }
  case TK_INT32: {
    set_default_basic_value(value);
    return true;
  }
  case TK_UINT32: {
    CORBA::ULong val;
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val);
    return true;
  }
  case TK_INT64: {
    CORBA::LongLong val;
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val);
    return true;
  }
  case TK_UINT64: {
    CORBA::ULongLong val;
    set_default_basic_value(val);
    value = static_cast<CORBA::Long>(val);
    return true;
  }
  case TK_ENUM: {
    return set_default_enum_value(disc_type, value);
  }
  }
  return false;
}

} // namespace XTypes


namespace DCPS {

// XCDR2 serialization using the DynamicData API (intended to work with any implementation).
// The get functions must already handle try-construct behavior (in case of reading from
// a XCDR backing store) or returning default value (in case the member data is missing
// from the internal container). So it's guaranteed that some data for each valid
// member is available for serialization.

bool serialized_size_i(
  const Encoding& encoding, size_t& size, DDS::DynamicData_ptr data, Sample::Extent ext);
bool serialize(Serializer& ser, DDS::DynamicData_ptr data, Sample::Extent ext);

bool get_type_descriptor(const DDS::DynamicType_var& type, DDS::TypeDescriptor_var& td)
{
  if (type->get_descriptor(td) != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      const CORBA::String_var type_name = type->get_name();
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: get_type_descriptor:"
                 " Failed to get type descriptor for type %C\n", type_name.in()));
    }
    return false;
  }
  return true;
}

void serialized_size_dynamic_member_header(
  const Encoding& encoding, size_t& size, size_t& mutable_running_total,
  DDS::ReturnCode_t rc, DDS::ExtensibilityKind extensibility, CORBA::Boolean optional)
{
  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    primitive_serialized_size_boolean(encoding, size);
    return;
  }
  if (extensibility == DDS::MUTABLE) {
    if (!optional || rc == DDS::RETCODE_OK) {
      serialized_size_parameter_id(encoding, size, mutable_running_total);
    }
  }
}

bool serialized_size_primitive_value(const Encoding& encoding, size_t& size, DDS::TypeKind member_tk)
{
  using namespace OpenDDS::XTypes;
  switch (member_tk) {
  case TK_INT32:
    return primitive_serialized_size(encoding, size, CORBA::Long());
  case TK_UINT32:
    return primitive_serialized_size(encoding, size, CORBA::ULong());
  case TK_INT8:
    primitive_serialized_size_int8(encoding, size);
    return true;
  case TK_UINT8:
    primitive_serialized_size_uint8(encoding, size);
    return true;
  case TK_INT16:
    return primitive_serialized_size(encoding, size, CORBA::Short());
  case TK_UINT16:
    return primitive_serialized_size(encoding, size, CORBA::UShort());
  case TK_INT64:
    return primitive_serialized_size(encoding, size, CORBA::LongLong());
  case TK_UINT64:
    return primitive_serialized_size(encoding, size, CORBA::ULongLong());
  case TK_FLOAT32:
    return primitive_serialized_size(encoding, size, CORBA::Float());
  case TK_FLOAT64:
    return primitive_serialized_size(encoding, size, CORBA::Double());
  case TK_FLOAT128:
    return primitive_serialized_size(encoding, size, CORBA::LongDouble());
  case TK_CHAR8:
    primitive_serialized_size_char(encoding, size);
    return true;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    primitive_serialized_size_wchar(encoding, size);
    return true;
#endif
  case TK_BYTE:
    primitive_serialized_size_octet(encoding, size);
    return true;
  case TK_BOOLEAN:
    primitive_serialized_size_boolean(encoding, size);
    return true;
  }
  return false;
}

void serialized_size_string_value(const Encoding& encoding, size_t& size, const char* str)
{
  primitive_serialized_size_ulong(encoding, size);
  if (str) {
    size += ACE_OS::strlen(str) + 1; // Include null termination
  }
}

#ifdef DDS_HAS_WCHAR
void serialized_size_wstring_value(const Encoding& encoding, size_t& size, const CORBA::WChar* wstr)
{
  primitive_serialized_size_ulong(encoding, size);
  if (wstr) {
    size += ACE_OS::strlen(wstr) * char16_cdr_size; // Not include null termination
  }
}
#endif

bool serialized_size_dynamic_member(DDS::DynamicData_ptr data, const Encoding& encoding,
  size_t& size, const DDS::MemberDescriptor_var& md, DDS::ExtensibilityKind extensibility,
  size_t& mutable_running_total, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  const DDS::MemberId member_id = md->id();
  const CORBA::Boolean optional = md->is_optional();
  const DDS::DynamicType_var member_type = get_base_type(md->type());
  const DDS::TypeKind member_tk = member_type->get_kind();
  DDS::TypeKind treat_member_as = member_tk;

  if (member_tk == TK_ENUM && enum_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (member_tk == TK_BITMASK && bitmask_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  if (is_primitive(treat_member_as)) {
    switch (treat_member_as) {
    case TK_INT8: {
      CORBA::Int8 val;
      rc = data->get_int8_value(val, member_id);
      break;
    }
    case TK_UINT8: {
      CORBA::UInt8 val;
      rc = data->get_uint8_value(val, member_id);
      break;
    }
    case TK_INT16: {
      CORBA::Short val;
      rc = data->get_int16_value(val, member_id);
      break;
    }
    case TK_UINT16: {
      CORBA::UShort val;
      rc = data->get_uint16_value(val, member_id);
      break;
    }
    case TK_INT32: {
      CORBA::Long val;
      rc = data->get_int32_value(val, member_id);
      break;
    }
    case TK_UINT32: {
      CORBA::ULong val;
      rc = data->get_uint32_value(val, member_id);
      break;
    }
    case TK_INT64: {
      CORBA::LongLong val;
      rc = data->get_int64_value(val, member_id);
      break;
    }
    case TK_UINT64: {
      CORBA::ULongLong val;
      rc = data->get_uint64_value(val, member_id);
      break;
    }
    case TK_FLOAT32: {
      CORBA::Float val;
      rc = data->get_float32_value(val, member_id);
      break;
    }
    case TK_FLOAT64: {
      CORBA::Double val;
      rc = data->get_float64_value(val, member_id);
      break;
    }
    case TK_FLOAT128: {
      CORBA::LongDouble val;
      rc = data->get_float128_value(val, member_id);
      break;
    }
    case TK_CHAR8: {
      CORBA::Char val;
      rc = data->get_char8_value(val, member_id);
      break;
    }
#ifdef DDS_HAS_WCHAR
    case TK_CHAR16: {
      CORBA::WChar val;
      rc = data->get_char16_value(val, member_id);
      break;
    }
#endif
    case TK_BYTE: {
      CORBA::Octet val;
      rc = data->get_byte_value(val, member_id);
      break;
    }
    case TK_BOOLEAN: {
      CORBA::Boolean val = false;
      rc = data->get_boolean_value(val, member_id);
      break;
    }
    }

    if (!XTypes::check_rc_from_get(rc, member_id, treat_member_as, "serialized_size_dynamic_member")) {
      return false;
    }
    serialized_size_dynamic_member_header(encoding, size, mutable_running_total,
                                          rc, extensibility, optional);
    if (optional && rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    return serialized_size_primitive_value(encoding, size, treat_member_as);
  }

  switch (treat_member_as) {
  case TK_STRING8: {
    CORBA::String_var val;
    rc = data->get_string_value(val, member_id);
    if (!XTypes::check_rc_from_get(rc, member_id, treat_member_as, "serialized_size_dynamic_member")) {
      return false;
    }
    serialized_size_dynamic_member_header(encoding, size, mutable_running_total,
                                          rc, extensibility, optional);
    if (optional && rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    serialized_size_string_value(encoding, size, val.in());
    return true;
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    CORBA::WString_var val;
    rc = data->get_wstring_value(val, member_id);
    if (!XTypes::check_rc_from_get(rc, member_id, treat_member_as, "serialized_size_dynamic_member")) {
      return false;
    }
    serialized_size_dynamic_member_header(encoding, size, mutable_running_total,
                                          rc, extensibility, optional);
    if (optional && rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    serialized_size_wstring_value(encoding, size, val.in());
    return true;
  }
#endif
  case TK_STRUCTURE:
  case TK_UNION:
  case TK_ARRAY:
  case TK_SEQUENCE: {
    DDS::DynamicData_var member_data;
    rc = data->get_complex_value(member_data, member_id);
    if (!XTypes::check_rc_from_get(rc, member_id, treat_member_as, "serialized_size_dynamic_member")) {
      return false;
    }
    serialized_size_dynamic_member_header(encoding, size, mutable_running_total,
                                          rc, extensibility, optional);
    if (optional && rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    return serialized_size_i(encoding, size, member_data, ext);
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: serialized_size_dynamic_member:"
                 " Unsupported member type %C at ID %u\n",
                 XTypes::typekind_to_string(member_tk), member_id));
    }
  }
  return false;
}

bool serialized_size_dynamic_struct(const Encoding& encoding, size_t& size,
                                    DDS::DynamicData_ptr struct_data, Sample::Extent ext)
{
  const DDS::DynamicType_var type = struct_data->type();
  const DDS::DynamicType_var base_type = XTypes::get_base_type(type);
  const bool struct_has_explicit_keys = XTypes::has_explicit_keys(base_type);
  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(base_type, td)) {
    return false;
  }

  const DDS::ExtensibilityKind extensibility = td->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    serialized_size_delimiter(encoding, size);
  }

  size_t mutable_running_total = 0;
  const CORBA::ULong member_count = base_type->get_member_count();
  for (CORBA::ULong i = 0; i < member_count; ++i) {
    DDS::DynamicTypeMember_var dtm;
    if (base_type->get_member_by_index(dtm, i) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var md;
    if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }

    if (XTypes::exclude_member(ext, md->is_key(), struct_has_explicit_keys)) {
      continue;
    }

    if (!serialized_size_dynamic_member(struct_data, encoding, size, md, extensibility,
                                        mutable_running_total, XTypes::nested(ext))) {
      if (log_level >= LogLevel:: Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: serialized_size_dynamic_struct:"
                   " Failed to compute serialized size for member ID %u\n", md->id()));
      }
      return false;
    }
  }

  if (extensibility == DDS::MUTABLE) {
    serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
  }
  return true;
}

bool get_discriminator_value(CORBA::Long& disc_val, DDS::DynamicData_ptr union_data,
                             const DDS::DynamicType_var& disc_type)
{
  using namespace OpenDDS::XTypes;
  const DDS::TypeKind disc_tk = disc_type->get_kind();
  DDS::TypeKind treat_as = disc_tk;
  if (disc_tk == TK_ENUM && enum_bound(disc_type, treat_as) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::MemberId id = DISCRIMINATOR_ID;
  switch (treat_as) {
  case TK_BOOLEAN: {
    CORBA::Boolean value = false;
    const DDS::ReturnCode_t rc = union_data->get_boolean_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_BYTE: {
    CORBA::Octet value;
    const DDS::ReturnCode_t rc = union_data->get_byte_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_CHAR8: {
    CORBA::Char value;
    const DDS::ReturnCode_t rc = union_data->get_char8_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_CHAR16: {
    CORBA::WChar value;
    const DDS::ReturnCode_t rc = union_data->get_char16_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT8: {
    CORBA::Int8 value;
    const DDS::ReturnCode_t rc = union_data->get_int8_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_UINT8: {
    CORBA::UInt8 value;
    const DDS::ReturnCode_t rc = union_data->get_uint8_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT16: {
    CORBA::Short value;
    const DDS::ReturnCode_t rc = union_data->get_int16_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = value;
    return true;
  }
  case TK_UINT16: {
    CORBA::UShort value;
    const DDS::ReturnCode_t rc = union_data->get_uint16_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT32: {
    const DDS::ReturnCode_t rc = union_data->get_int32_value(disc_val, id);
    return XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value");
  }
  case TK_UINT32: {
    CORBA::ULong value;
    const DDS::ReturnCode_t rc = union_data->get_uint32_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT64: {
    CORBA::LongLong value;
    const DDS::ReturnCode_t rc = union_data->get_int64_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_UINT64: {
    CORBA::ULongLong value;
    const DDS::ReturnCode_t rc = union_data->get_uint64_value(value, id);
    if (!XTypes::check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  }
  return false;
}

bool serialized_size_enum(const Encoding& encoding, size_t& size,
                          const DDS::DynamicType_var& enum_type)
{
  using namespace OpenDDS::XTypes;
  DDS::TypeKind equivalent_int_tk;
  if (enum_bound(enum_type, equivalent_int_tk) != DDS::RETCODE_OK) {
    return false;
  }
  switch (equivalent_int_tk) {
  case TK_INT8:
    primitive_serialized_size_int8(encoding, size);
    return true;
  case TK_INT16:
    return primitive_serialized_size(encoding, size, CORBA::Short());
  case TK_INT32:
    return primitive_serialized_size(encoding, size, CORBA::Long());
  }
  return false;
}

bool serialized_size_dynamic_discriminator(
  const Encoding& encoding, size_t& size, const DDS::DynamicType_var& disc_type,
  DDS::ExtensibilityKind extensibility, size_t& mutable_running_total)
{
  if (extensibility == DDS::MUTABLE) {
    serialized_size_parameter_id(encoding, size, mutable_running_total);
  }
  const DDS::TypeKind disc_tk = disc_type->get_kind();
  if (XTypes::is_primitive(disc_tk)) {
    return serialized_size_primitive_value(encoding, size, disc_tk);
  }
  return serialized_size_enum(encoding, size, disc_type);
}

bool serialized_size_dynamic_union(const Encoding& encoding, size_t& size,
                                   DDS::DynamicData_ptr union_data, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  const DDS::DynamicType_var type = union_data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);

  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(base_type, td)) {
    return false;
  }

  // Dheader
  const DDS::ExtensibilityKind extensibility = td->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    serialized_size_delimiter(encoding, size);
  }

  if (ext == Sample::KeyOnly && !has_explicit_keys(base_type)) {
    return true;
  }

  // Discriminator
  size_t mutable_running_total = 0;
  DDS::DynamicType_var disc_type = get_base_type(td->discriminator_type());
  if (!serialized_size_dynamic_discriminator(encoding, size, disc_type,
                                             extensibility, mutable_running_total)) {
    return false;
  }

  CORBA::Long disc_val;
  if (!get_discriminator_value(disc_val, union_data, disc_type)) {
    return false;
  }

  if (ext == Sample::Full) {
    // Selected branch
    bool has_branch = false;
    DDS::MemberDescriptor_var selected_md;
    if (get_selected_union_branch(base_type, disc_val, has_branch, selected_md) != DDS::RETCODE_OK) {
      return false;
    }

    if (has_branch &&
        !serialized_size_dynamic_member(union_data, encoding, size, selected_md,
                                        extensibility, mutable_running_total, nested(ext))) {
      return false;
    }
  }

  if (extensibility == DDS::MUTABLE) {
    serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
  }
  return true;
}

bool serialized_size_dynamic_element(DDS::DynamicData_ptr col_data, const Encoding& encoding,
  size_t& size, DDS::MemberId elem_id, DDS::TypeKind elem_tk, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (elem_tk) {
  case TK_STRING8: {
    CORBA::String_var val;
    rc = col_data->get_string_value(val, elem_id);
    if (!XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialized_size_dynamic_element")) {
      return false;
    }
    serialized_size_string_value(encoding, size, val.in());
    return true;
  }
  case TK_STRING16: {
    CORBA::WString_var val;
    rc = col_data->get_wstring_value(val, elem_id);
    if (!XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialized_size_dynamic_element")) {
      return false;
    }
    serialized_size_wstring_value(encoding, size, val.in());
    return true;
  }
  case TK_STRUCTURE:
  case TK_UNION:
  case TK_ARRAY:
  case TK_SEQUENCE: {
    DDS::DynamicData_var elem_data;
    rc = col_data->get_complex_value(elem_data, elem_id);
    if (!XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialized_size_dynamic_element")) {
      return false;
    }
    return serialized_size_i(encoding, size, elem_data, ext);
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: serialized_size_dynamic_element:"
                 " Unsupported element type %C at ID %u\n", typekind_to_string(elem_tk), elem_id));
    }
  }
  return false;
}

void serialized_size_primitive_elements(const Encoding& encoding, size_t& size,
                                        DDS::TypeKind elem_tk, CORBA::ULong length)
{
  using namespace OpenDDS::XTypes;
  switch (elem_tk) {
  case TK_INT32:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::Long(), length);
    }
    return;
  case TK_UINT32:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::ULong(), length);
    }
    return;
  case TK_INT8:
    if (length != 0) {
      primitive_serialized_size_int8(encoding, size, length);
    }
    return;
  case TK_UINT8:
    if (length != 0) {
      primitive_serialized_size_uint8(encoding, size, length);
    }
    return;
  case TK_INT16:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::Short(), length);
    }
    return;
  case TK_UINT16:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::UShort(), length);
    }
    return;
  case TK_INT64:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::LongLong(), length);
    }
    return;
  case TK_UINT64:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::ULongLong(), length);
    }
    return;
  case TK_FLOAT32:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::Float(), length);
    }
    return;
  case TK_FLOAT64:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::Double(), length);
    }
    return;
  case TK_FLOAT128:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::LongDouble(), length);
    }
    return;
  case TK_CHAR8:
    if (length != 0) {
      primitive_serialized_size_char(encoding, size, length);
    }
    return;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    if (length != 0) {
      primitive_serialized_size_wchar(encoding, size, length);
    }
    return;
#endif
  case TK_BYTE:
    if (length != 0) {
      primitive_serialized_size_octet(encoding, size, length);
    }
    return;
  case TK_BOOLEAN:
    if (length != 0) {
      primitive_serialized_size_boolean(encoding, size, length);
    }
    return;
  }
}

bool serialized_size_dynamic_collection(const Encoding& encoding, size_t& size,
                                        DDS::DynamicData_ptr col_data, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  const DDS::DynamicType_var type = col_data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(base_type, td)) {
    return false;
  }
  DDS::DynamicType_var elem_type = get_base_type(td->element_type());
  const DDS::TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeKind treat_elem_as = elem_tk;

  if (elem_tk == TK_ENUM && enum_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (elem_tk == TK_BITMASK && bitmask_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }

  // Dheader
  if (!is_primitive(elem_tk)) {
    serialized_size_delimiter(encoding, size);
  }

  if (base_type->get_kind() == TK_SEQUENCE) {
    // Sequence length.
    primitive_serialized_size_ulong(encoding, size);
  }

  const CORBA::ULong item_count = col_data->get_item_count();
  if (is_primitive(treat_elem_as)) {
    serialized_size_primitive_elements(encoding, size, treat_elem_as, item_count);
    return true;
  }

  // Non-primitive element types.
  for (CORBA::ULong i = 0; i < item_count; ++i) {
    const DDS::MemberId elem_id = col_data->get_member_id_at_index(i);
    if (elem_id == MEMBER_ID_INVALID ||
        !serialized_size_dynamic_element(col_data, encoding, size, elem_id,
                                         treat_elem_as, nested(ext))) {
      return false;
    }
  }
  return true;
}

bool serialized_size_i(const Encoding& encoding, size_t& size,
                       DDS::DynamicData_ptr data, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  switch (base_type->get_kind()) {
  case TK_STRUCTURE:
    return serialized_size_dynamic_struct(encoding, size, data, ext);
  case TK_UNION:
    return serialized_size_dynamic_union(encoding, size, data, ext);
  case TK_ARRAY:
  case TK_SEQUENCE:
    return serialized_size_dynamic_collection(encoding, size, data, ext);
  }
  return false;
}

bool serialized_size(const Encoding& encoding, size_t& size, DDS::DynamicData_ptr data)
{
  return serialized_size_i(encoding, size, data, Sample::Full);
}

bool serialized_size(const Encoding& encoding, size_t& size, const KeyOnly<DDS::DynamicData_ptr>& key)
{
  return serialized_size_i(encoding, size, key.value, Sample::KeyOnly);
}

// Serialize header for a basic member.
// The return code @rc must be either NO_DATA or OK.
bool serialize_dynamic_basic_member_header(Serializer& ser, void* value, DDS::ReturnCode_t rc,
  DDS::MemberId id, DDS::TypeKind tk, DDS::ExtensibilityKind extensibility,
  CORBA::Boolean optional, CORBA::Boolean must_understand)
{
  using namespace OpenDDS::XTypes;
  if (optional && rc == DDS::RETCODE_NO_DATA) {
    if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
      return ser << ACE_OutputCDR::from_boolean(false);
    }
    return true;
  }

  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    return ser << ACE_OutputCDR::from_boolean(true);
  } else if (extensibility == DDS::MUTABLE) {
    const Encoding& encoding = ser.encoding();
    size_t member_size = 0;
    if (is_primitive(tk)) {
      if (!serialized_size_primitive_value(encoding, member_size, tk)) {
        return false;
      }
    } else if (tk == TK_STRING8) {
      const char* str = (const char*)value;
      serialized_size_string_value(encoding, member_size, str);
    }
#ifdef DDS_HAS_WCHAR
    else if (tk == TK_STRING16) {
      const CORBA::WChar* wstr = (const CORBA::WChar*)value;
      serialized_size_wstring_value(encoding, member_size, wstr);
    }
#endif
    else {
      return false;
    }
    return ser.write_parameter_id(id, member_size, must_understand);
  }
  return true;
}

// Serialize header for a non-basic member.
// The return code @rc must be either NO_DATA or OK.
bool serialize_dynamic_complex_member_header(Serializer& ser, DDS::ReturnCode_t rc,
  const DDS::DynamicData_ptr member_data, DDS::ExtensibilityKind extensibility,
  CORBA::Boolean optional, DDS::MemberId id, CORBA::Boolean must_understand, Sample::Extent ext)
{
  if (optional && rc == DDS::RETCODE_NO_DATA) {
    if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
      return ser << ACE_OutputCDR::from_boolean(false);
    }
    return true;
  }

  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    return ser << ACE_OutputCDR::from_boolean(true);
  } else if (extensibility == DDS::MUTABLE) {
    const Encoding& encoding = ser.encoding();
    size_t member_size = 0;
    return serialized_size_i(encoding, member_size, member_data, ext)
      && ser.write_parameter_id(id, member_size, must_understand);
  }
  return true;
}

template <typename T>
bool serialize_dynamic_primitive_member(Serializer& ser, const T& value, DDS::ReturnCode_t rc,
  DDS::MemberId id, DDS::TypeKind type_kind, DDS::ExtensibilityKind extensibility,
  CORBA::Boolean optional, CORBA::Boolean must_understand)
{
  if (!XTypes::check_rc_from_get(rc, id, type_kind, "serialize_dynamic_primitive_member") ||
      !serialize_dynamic_basic_member_header(ser, 0, rc, id, type_kind, extensibility,
                                             optional, must_understand)) {
    return false;
  }
  if (optional && rc == DDS::RETCODE_NO_DATA) {
    return true;
  }
  return ser << value;
}

bool serialize_dynamic_member(Serializer& ser, DDS::DynamicData_ptr data,
  const DDS::MemberDescriptor_var md, DDS::ExtensibilityKind extensibility, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  const DDS::MemberId id = md->id();
  const CORBA::Boolean optional = md->is_optional();
  const CORBA::Boolean must_understand = md->is_must_understand() || md->is_key();
  const DDS::DynamicType_var member_type = get_base_type(md->type());
  const DDS::TypeKind member_tk = member_type->get_kind();
  DDS::TypeKind treat_member_as = member_tk;

  if (member_tk == TK_ENUM && enum_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (member_tk == TK_BITMASK && bitmask_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (treat_member_as) {
  case TK_INT8: {
    CORBA::Int8 val;
    rc = data->get_int8_value(val, id);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_int8(val), rc, id,
                                              treat_member_as, extensibility, optional, must_understand);
  }
  case TK_UINT8: {
    CORBA::UInt8 val;
    rc = data->get_uint8_value(val, id);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_uint8(val), rc, id,
                                              treat_member_as, extensibility, optional, must_understand);
  }
  case TK_INT16: {
    CORBA::Short val;
    rc = data->get_int16_value(val, id);
    return serialize_dynamic_primitive_member(ser, val, rc, id, treat_member_as,
                                              extensibility, optional, must_understand);
  }
  case TK_UINT16: {
    CORBA::UShort val;
    rc = data->get_uint16_value(val, id);
    return serialize_dynamic_primitive_member(ser, val, rc, id, treat_member_as,
                                              extensibility, optional, must_understand);
  }
  case TK_INT32: {
    CORBA::Long val;
    rc = data->get_int32_value(val, id);
    return serialize_dynamic_primitive_member(ser, val, rc, id, treat_member_as,
                                              extensibility, optional, must_understand);
  }
  case TK_UINT32: {
    CORBA::ULong val;
    rc = data->get_uint32_value(val, id);
    return serialize_dynamic_primitive_member(ser, val, rc, id, treat_member_as,
                                              extensibility, optional, must_understand);
  }
  case TK_INT64: {
    CORBA::LongLong val;
    rc = data->get_int64_value(val, id);
    return serialize_dynamic_primitive_member(ser, val, rc, id, treat_member_as,
                                              extensibility, optional, must_understand);
  }
  case TK_UINT64: {
    CORBA::ULongLong val;
    rc = data->get_uint64_value(val, id);
    return serialize_dynamic_primitive_member(ser, val, rc, id, treat_member_as,
                                              extensibility, optional, must_understand);
  }
  case TK_FLOAT32: {
    CORBA::Float val;
    rc = data->get_float32_value(val, id);
    return serialize_dynamic_primitive_member(ser, val, rc, id, treat_member_as,
                                              extensibility, optional, must_understand);
  }
  case TK_FLOAT64: {
    CORBA::Double val;
    rc = data->get_float64_value(val, id);
    return serialize_dynamic_primitive_member(ser, val, rc, id, treat_member_as,
                                              extensibility, optional, must_understand);
  }
  case TK_FLOAT128: {
    CORBA::LongDouble val;
    rc = data->get_float128_value(val, id);
    return serialize_dynamic_primitive_member(ser, val, rc, id, treat_member_as,
                                              extensibility, optional, must_understand);
  }
  case TK_CHAR8: {
    CORBA::Char val;
    rc = data->get_char8_value(val, id);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_char(val), rc, id,
                                              treat_member_as, extensibility, optional, must_understand);
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    CORBA::WChar val;
    rc = data->get_char16_value(val, id);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_wchar(val), rc, id,
                                              treat_member_as, extensibility, optional, must_understand);
  }
#endif
  case TK_BYTE: {
    CORBA::Octet val;
    rc = data->get_byte_value(val, id);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_octet(val), rc, id,
                                              treat_member_as, extensibility, optional, must_understand);
  }
  case TK_BOOLEAN: {
    CORBA::Boolean val = false;
    rc = data->get_boolean_value(val, id);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_boolean(val), rc, id,
                                              treat_member_as, extensibility, optional, must_understand);
  }
  case TK_STRING8: {
    CORBA::String_var val;
    rc = data->get_string_value(val, id);
    if (!XTypes::check_rc_from_get(rc, id, treat_member_as, "serialize_dynamic_member") ||
        !serialize_dynamic_basic_member_header(ser, (void*)val.in(), rc, id, TK_STRING8,
                                               extensibility, optional, must_understand)) {
      return false;
    }
    if (optional && rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    return ser << val.in();
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    CORBA::WString_var val;
    rc = data->get_wstring_value(val, id);
    if (!XTypes::check_rc_from_get(rc, id, treat_member_as, "serialize_dynamic_member") ||
        !serialize_dynamic_basic_member_header(ser, (void*)val.in(), rc, id, TK_STRING16,
                                               extensibility, optional, must_understand)) {
      return false;
    }
    if (optional && rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    return ser << val.in();
  }
#endif
  case TK_STRUCTURE:
  case TK_UNION:
  case TK_ARRAY:
  case TK_SEQUENCE: {
    DDS::DynamicData_var member_data;
    rc = data->get_complex_value(member_data, id);
    if (!XTypes::check_rc_from_get(rc, id, treat_member_as, "serialize_dynamic_member") ||
        !serialize_dynamic_complex_member_header(ser, rc, member_data, extensibility,
                                                 optional, id, must_understand, ext)) {
      return false;
    }
    if (optional && rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    return serialize(ser, member_data, ext);
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: serialize_dynamic_member:"
                 " Unsupported member type %C at ID %u\n", typekind_to_string(member_tk), id));
    }
  }
  return false;
}

bool serialize_dynamic_struct(Serializer& ser, DDS::DynamicData_ptr data, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  const bool struct_has_explicit_keys = has_explicit_keys(base_type);
  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(base_type, td)) {
    return false;
  }

  const Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  const DDS::ExtensibilityKind extensibility = td->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    if (!serialized_size_dynamic_struct(encoding, total_size, data, ext)
        || !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  const CORBA::ULong member_count = base_type->get_member_count();
  for (CORBA::ULong i = 0; i < member_count; ++i) {
    DDS::DynamicTypeMember_var dtm;
    if (base_type->get_member_by_index(dtm, i) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var md;
    if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }

    if (exclude_member(ext, md->is_key(), struct_has_explicit_keys)) {
      continue;
    }

    // The serialization function for individual member must account for any header it has.
    if (!serialize_dynamic_member(ser, data, md, extensibility, nested(ext))) {
      return false;
    }
  }
  return true;
}

bool serialize_dynamic_discriminator(Serializer& ser, DDS::DynamicData_ptr union_data,
  const DDS::MemberDescriptor_var disc_md, DDS::ExtensibilityKind extensibility, CORBA::Long& disc_val)
{
  using namespace OpenDDS::XTypes;
  const DDS::MemberId disc_id = DISCRIMINATOR_SERIALIZED_ID;
  const CORBA::Boolean optional = disc_md->is_optional(); // Discriminator must be non-optional.
  const CORBA::Boolean must_understand = disc_md->is_must_understand() || disc_md->is_key();
  const DDS::DynamicType_var disc_type = get_base_type(disc_md->type());
  const DDS::TypeKind disc_tk = disc_type->get_kind();
  DDS::TypeKind treat_disc_as = disc_tk;

  if (disc_tk == TK_ENUM && enum_bound(disc_type, treat_disc_as) != DDS::RETCODE_OK) {
    return false;
  }

  switch (treat_disc_as) {
  case TK_BOOLEAN: {
    CORBA::Boolean val = false;
    const DDS::ReturnCode_t rc = union_data->get_boolean_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_boolean(val), rc, disc_id,
                                              treat_disc_as, extensibility, optional, must_understand);
  }
  case TK_BYTE: {
    CORBA::Octet val;
    const DDS::ReturnCode_t rc = union_data->get_byte_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_octet(val), rc, disc_id,
                                              treat_disc_as, extensibility, optional, must_understand);
  }
  case TK_CHAR8: {
    CORBA::Char val;
    const DDS::ReturnCode_t rc = union_data->get_char8_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_char(val), rc, disc_id,
                                              treat_disc_as, extensibility, optional, must_understand);
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    CORBA::WChar val;
    const DDS::ReturnCode_t rc = union_data->get_char16_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_wchar(val), rc, disc_id,
                                              treat_disc_as, extensibility, optional, must_understand);
  }
#endif
  case TK_INT8: {
    CORBA::Int8 val;
    const DDS::ReturnCode_t rc = union_data->get_int8_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_int8(val), rc, disc_id,
                                              treat_disc_as, extensibility, optional, must_understand);
  }
  case TK_UINT8: {
    CORBA::UInt8 val;
    const DDS::ReturnCode_t rc = union_data->get_uint8_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, ACE_OutputCDR::from_uint8(val), rc, disc_id,
                                              treat_disc_as, extensibility, optional, must_understand);
  }
  case TK_INT16: {
    CORBA::Short val;
    const DDS::ReturnCode_t rc = union_data->get_int16_value(val, DISCRIMINATOR_ID);
    disc_val = val;
    return serialize_dynamic_primitive_member(ser, val, rc, disc_id, treat_disc_as,
                                              extensibility, optional, must_understand);
  }
  case TK_UINT16: {
    CORBA::UShort val;
    const DDS::ReturnCode_t rc = union_data->get_uint16_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, val, rc, disc_id, treat_disc_as,
                                              extensibility, optional, must_understand);
  }
  case TK_INT32: {
    CORBA::Long val;
    const DDS::ReturnCode_t rc = union_data->get_int32_value(val, DISCRIMINATOR_ID);
    disc_val = val;
    return serialize_dynamic_primitive_member(ser, val, rc, disc_id, treat_disc_as,
                                              extensibility, optional, must_understand);
  }
  case TK_UINT32: {
    CORBA::ULong val;
    const DDS::ReturnCode_t rc = union_data->get_uint32_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, val, rc, disc_id, treat_disc_as,
                                              extensibility, optional, must_understand);
  }
  case TK_INT64: {
    CORBA::LongLong val;
    const DDS::ReturnCode_t rc = union_data->get_int64_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, val, rc, disc_id, treat_disc_as,
                                              extensibility, optional, must_understand);
  }
  case TK_UINT64: {
    CORBA::ULongLong val;
    const DDS::ReturnCode_t rc = union_data->get_uint64_value(val, DISCRIMINATOR_ID);
    disc_val = static_cast<CORBA::Long>(val);
    return serialize_dynamic_primitive_member(ser, val, rc, disc_id, treat_disc_as,
                                              extensibility, optional, must_understand);
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: seialize_dynamic_discriminator:"
                 " Invalid discriminator type: %C\n", typekind_to_string(disc_tk)));
    }
  }
  return false;
}

bool serialize_dynamic_union(Serializer& ser, DDS::DynamicData_ptr data, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);

  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(base_type, td)) {
    return false;
  }

  // Dheader
  const Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  const DDS::ExtensibilityKind extensibility = td->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    if (!serialized_size_dynamic_union(encoding, total_size, data, ext)
        || !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  if (ext == Sample::KeyOnly && !has_explicit_keys(base_type)) {
    return true;
  }

  // Discriminator
  DDS::DynamicTypeMember_var dtm;
  if (base_type->get_member(dtm, DISCRIMINATOR_ID) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var disc_md;
  if (dtm->get_descriptor(disc_md) != DDS::RETCODE_OK) {
    return false;
  }
  CORBA::Long disc_val;
  if (!serialize_dynamic_discriminator(ser, data, disc_md, extensibility, disc_val)) {
    return false;
  }

  if (ext != Sample::Full) {
    return true;
  }

  // Selected branch
  bool has_branch = false;
  DDS::MemberDescriptor_var selected_md;
  if (get_selected_union_branch(base_type, disc_val, has_branch, selected_md) != DDS::RETCODE_OK) {
    return false;
  }
  return !has_branch || serialize_dynamic_member(ser, data, selected_md, extensibility, nested(ext));
}

bool serialize_dynamic_element(Serializer& ser, DDS::DynamicData_ptr col_data,
                               DDS::MemberId elem_id, DDS::TypeKind elem_tk, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (elem_tk) {
  case TK_INT8: {
    CORBA::Int8 val;
    rc = col_data->get_int8_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << ACE_OutputCDR::from_int8(val));
  }
  case TK_UINT8: {
    CORBA::UInt8 val;
    rc = col_data->get_uint8_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << ACE_OutputCDR::from_uint8(val));
  }
  case TK_INT16: {
    CORBA::Short val;
    rc = col_data->get_int16_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val);
  }
  case TK_UINT16: {
    CORBA::UShort val;
    rc = col_data->get_uint16_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val);
  }
  case TK_INT32: {
    CORBA::Long val;
    rc = col_data->get_int32_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val);
  }
  case TK_UINT32: {
    CORBA::ULong val;
    rc = col_data->get_uint32_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val);
  }
  case TK_INT64: {
    CORBA::LongLong val;
    rc = col_data->get_int64_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val);
  }
  case TK_UINT64: {
    CORBA::ULongLong val;
    rc = col_data->get_uint64_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val);
  }
  case TK_FLOAT32: {
    CORBA::Float val;
    rc = col_data->get_float32_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val);
  }
  case TK_FLOAT64: {
    CORBA::Double val;
    rc = col_data->get_float64_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val);
  }
  case TK_FLOAT128: {
    CORBA::LongDouble val;
    rc = col_data->get_float128_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val);
  }
  case TK_CHAR8: {
    CORBA::Char val;
    rc = col_data->get_char8_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << ACE_OutputCDR::from_char(val));
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    CORBA::WChar val;
    rc = col_data->get_char16_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << ACE_OutputCDR::from_wchar(val));
  }
#endif
  case TK_BYTE: {
    CORBA::Octet val;
    rc = col_data->get_byte_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << ACE_OutputCDR::from_octet(val));
  }
  case TK_BOOLEAN: {
    CORBA::Boolean val = false;
    rc = col_data->get_boolean_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << ACE_OutputCDR::from_boolean(val));
  }
  case TK_STRING8: {
    CORBA::String_var val;
    rc = col_data->get_string_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val.in());
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    CORBA::WString_var val;
    rc = col_data->get_wstring_value(val, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && (ser << val.in());
  }
#endif
  case TK_STRUCTURE:
  case TK_UNION:
  case TK_ARRAY:
  case TK_SEQUENCE: {
    DDS::DynamicData_var elem_data;
    rc = col_data->get_complex_value(elem_data, elem_id);
    return XTypes::check_rc_from_get(rc, elem_id, elem_tk, "serialize_dynamic_element")
      && serialize(ser, elem_data, ext);
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: serialize_dynamic_element:"
                 " Unsupported element type %C at ID %u\n", typekind_to_string(elem_tk), elem_id));
    }
  }
  return false;
}

bool serialize_dynamic_collection(Serializer& ser, DDS::DynamicData_ptr data, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(base_type, td)) {
    return false;
  }
  DDS::DynamicType_var elem_type = get_base_type(td->element_type());
  const DDS::TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeKind treat_elem_as = elem_tk;

  if (elem_tk == TK_ENUM && enum_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (elem_tk == TK_BITMASK && bitmask_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }

  // Dheader
  const Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (!is_primitive(elem_tk)) {
    if (!serialized_size_dynamic_collection(encoding, total_size, data, ext) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  const DDS::TypeKind tk = base_type->get_kind();
  const CORBA::ULong item_count = data->get_item_count();
  if (tk == TK_SEQUENCE && !(ser << item_count)) {
    // Sequence length
    return false;
  }

  // Use the get APIs for sequences when they are supported.
  // Then we can serialize the whole sequence (for basic element types).
  // For now, serialize elements one-by-one.
  for (CORBA::ULong i = 0; i < item_count; ++i) {
    const DDS::MemberId elem_id = data->get_member_id_at_index(i);
    if (elem_id == MEMBER_ID_INVALID ||
        !serialize_dynamic_element(ser, data, elem_id, treat_elem_as, nested(ext))) {
      return false;
    }
  }
  return true;
}

bool serialize(Serializer& ser, DDS::DynamicData_ptr data, Sample::Extent ext)
{
  using namespace OpenDDS::XTypes;
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  switch (base_type->get_kind()) {
  case TK_STRUCTURE:
    return serialize_dynamic_struct(ser, data, ext);
  case TK_UNION:
    return serialize_dynamic_union(ser, data, ext);
  case TK_ARRAY:
  case TK_SEQUENCE:
    return serialize_dynamic_collection(ser, data, ext);
  }
  return false;
}

bool operator<<(Serializer& ser, DDS::DynamicData_ptr data)
{
  return serialize(ser, data, Sample::Full);
}

bool operator<<(Serializer& ser, const KeyOnly<DDS::DynamicData_ptr>& key)
{
  return serialize(ser, key.value, Sample::KeyOnly);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
