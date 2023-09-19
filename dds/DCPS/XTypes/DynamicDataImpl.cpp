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
                                 ACE_Message_Block* chain,
                                 const DCPS::Encoding* encoding)
  : DynamicDataBase(type)
  , container_(type_, this)
  , backing_store_(0)
{
  if (chain) {
    backing_store_ = new DynamicDataXcdrReadImpl(chain, *encoding, type, DCPS::Sample::Full);
  }
}

DynamicDataImpl::DynamicDataImpl(const DynamicDataImpl& other)
  : CORBA::Object()
  , DynamicData()
  , CORBA::LocalObject()
  , DCPS::RcObject()
  , DynamicDataBase(other.type_)
  , container_(other.container_, this)
  , backing_store_(0)
{
  if (other.backing_store_) {
    backing_store_ = new DynamicDataXcdrReadImpl(*other.backing_store_);
  }
}

DynamicDataImpl::~DynamicDataImpl()
{
  CORBA::release(backing_store_);
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
    if (index != 0 && log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_member_id_at_index:"
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
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_member_id_at_index:"
                   " Input index (%u) is out-of-bound (bound is %u)\n", index, bound));
      }
      return MEMBER_ID_INVALID;
    }
    return index;
  }
  case TK_ARRAY: {
    const DDS::UInt32 length = bound_total(type_desc_);
    if (index >= length) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_member_id_at_index:"
                   " Input index (%u) is out-of-bound (array length is %u)\n", index, length));
      }
      return MEMBER_ID_INVALID;
    }
    return index;
  }
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_member_id_at_index:"
                 " Map is currently not supported\n"));
    }
    return MEMBER_ID_INVALID;
  case TK_STRUCTURE: {
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
    bool select_a_member;
    DDS::MemberDescriptor_var selected_md;
    const DDS::ReturnCode_t rc = get_selected_union_branch(select_a_member, selected_md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_member_id_at_index:"
                   " get_selected_union_branch failed: %C\n", retcode_to_string(rc)));
      }
      return MEMBER_ID_INVALID;
    }
    if (index == 1 && select_a_member) {
      return selected_md->id();
    }
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_member_id_at_index:"
                 " invalid index: %u\n", index));
    }
    return MEMBER_ID_INVALID;
  }
  }

  if (log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_member_id_at_index:"
               " Calling on an unexpected type %C\n", typekind_to_string(tk)));
  }
  return MEMBER_ID_INVALID;
}

CORBA::ULong DynamicDataImpl::get_sequence_size() const
{
  if (type_->get_kind() != TK_SEQUENCE) {
    return 0;
  }

  if (!container_.single_map_.empty() || !container_.complex_map_.empty()) {
    CORBA::ULong largest_index;
    if (!container_.get_largest_index_basic(largest_index)) {
      return 0;
    }
    if (!container_.sequence_map_.empty()) {
      CORBA::ULong largest_seq_index;
      if (!container_.get_largest_sequence_index(largest_seq_index)) {
        return 0;
      }
      largest_index = std::max(largest_index, largest_seq_index);
    }
    return largest_index + 1;
  } else if (!container_.sequence_map_.empty()) {
    CORBA::ULong largest_index;
    if (!container_.get_largest_sequence_index(largest_index)) {
      return 0;
    }
    return largest_index + 1;
  }
  return 0;
}

void DynamicDataImpl::erase_member(DDS::MemberId id)
{
  if (container_.single_map_.erase(id) == 0) {
    if (container_.sequence_map_.erase(id) == 0) {
      container_.complex_map_.erase(id);
    }
  }
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
    {
      if (!container_.single_map_.empty() || !container_.complex_map_.empty()) {
        CORBA::ULong largest_index;
        if (!container_.get_largest_index_basic(largest_index)) {
          return 0;
        }
        return largest_index + 1;
      }
      return 0;
    }
  case TK_SEQUENCE:
    return get_sequence_size();
  case TK_BITMASK:
    return static_cast<ACE_CDR::ULong>(container_.single_map_.size() +
                                       container_.complex_map_.size());
  case TK_ARRAY:
    return bound_total(type_desc_);
  case TK_STRUCTURE: {
    const CORBA::ULong member_count = type_->get_member_count();
    CORBA::ULong count = member_count;
    // An optional member that hasn't been set is considered missing.
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
      if (md->is_optional()) {
        const DDS::MemberId id = md->id();
        if (container_.single_map_.find(id) == container_.single_map_.end() &&
            container_.sequence_map_.find(id) == container_.sequence_map_.end() &&
            container_.complex_map_.find(id) == container_.complex_map_.end()) {
          --count;
        }
      }
    }
    return count;
  }
  case TK_UNION: {
    CORBA::ULong count = static_cast<CORBA::ULong>(container_.single_map_.size() +
                                                   container_.sequence_map_.size() +
                                                   container_.complex_map_.size());
    if (count > 0) {
      return count;
    }
    DDS::DynamicType_var disc_type = get_base_type(type_desc_->discriminator_type());
    CORBA::Long disc_val;
    if (!set_default_discriminator_value(disc_val, disc_type)) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_item_count:"
                   " set_default_discriminator_value failed\n"));
      }
      return 0;
    }
    bool select_a_member;
    DDS::MemberDescriptor_var selected_md;
    const DDS::ReturnCode_t rc = get_selected_union_branch(disc_val, select_a_member, selected_md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataImpl::get_item_count:"
                   " get_selected_union_branch failed: %C\n", retcode_to_string(rc)));
      }
      return 0;
    }
    return select_a_member ? 2 : 1;
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
    if (id >= bound) {
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
    const CORBA::ULong size = get_sequence_size();
    if (id >= size) {
      return DDS::RETCODE_ERROR;
    }

    // At the begin of each iterator, member with the current id is not present
    // in any of the maps. Copy over the next member to the current id.
    erase_member(id);
    for (CORBA::ULong i = id; i < size - 1; ++i) {
      const DDS::MemberId next_id = i + 1;
      const_single_iterator single_it = container_.single_map_.find(next_id);
      if (single_it != container_.single_map_.end()) {
        container_.single_map_.insert(std::make_pair(i, single_it->second));
        container_.single_map_.erase(next_id);
        continue;
      }
      const_sequence_iterator seq_it = container_.sequence_map_.find(next_id);
      if (seq_it != container_.sequence_map_.end()) {
        container_.sequence_map_.insert(std::make_pair(i, seq_it->second));
        container_.sequence_map_.erase(next_id);
        continue;
      }
      const_complex_iterator complex_it = container_.complex_map_.find(next_id);
      if (complex_it != container_.complex_map_.end()) {
        container_.complex_map_.insert(std::make_pair(i, complex_it->second));
        container_.complex_map_.erase(next_id);
        continue;
      }
    }
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
bool DynamicDataImpl::set_value_to_struct(DDS::MemberId id, const MemberType& value)
{
  DDS::MemberDescriptor_var md;
  DDS::DynamicType_var member_type;
  const DDS::ReturnCode_t rc = check_member(
    md, member_type, "DynamicDataImpl::set_value_to_struct", "set", id, MemberTypeKind);
  if (rc != DDS::RETCODE_OK) {
    return false;
  }
  return insert_single(id, value);
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

// Has to be below the get methods, or else there's a template specialization issue.
DynamicDataImpl::SingleValue::SingleValue(const SingleValue& other)
  : kind_(other.kind_)
  , active_(0)
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
                                                const_single_iterator it) const
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
                                                   const DDS::DynamicType_var& disc_type) const
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
bool DynamicDataImpl::read_discriminator(CORBA::Long& disc_val) const
{
  if (!is_valid_discriminator_type(type_->get_kind())) {
    return false;
  }
  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    return read_disc_from_single_map(disc_val, type_, it);
  }
  return read_disc_from_backing_store(disc_val, MEMBER_ID_INVALID, type_);
}

// Return the ID of a selected branch from the maps or backing store.
// Should only be called for union.
// TODO(sonndinh): Need to look at the backing store for existing member if not
// already cached in the maps.
DDS::MemberId DynamicDataImpl::find_selected_member() const
{
  // There can be at most 2 entries in total in all three maps,
  // one for the discriminator, one for a selected member.
  for (const_single_iterator single_it = container_.single_map_.begin();
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

  for (const_complex_iterator cmpl_it = container_.complex_map_.begin();
       cmpl_it != container_.complex_map_.end(); ++cmpl_it) {
    if (cmpl_it->first != DISCRIMINATOR_ID) {
      return cmpl_it->first;
    }
  }

  // There was no selected member.
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
  if (single_it != container_.single_map_.end() || complex_it != container_.complex_map_.end()) {
    return true;
  }
  // A backing store must have valid data (for union in this case), meaning
  // it must have at least data for discriminator.
  return backing_store_ ? true : false;
}

// Get discriminator value from the data container or the backing store.
// Call only when the instance has data for discriminator.
bool DynamicDataImpl::get_discriminator_value(const_single_iterator& single_it,
                                              const_complex_iterator& complex_it,
                                              CORBA::Long& value,
                                              const DDS::DynamicType_var& disc_type) const
{
  if (single_it != container_.single_map_.end() || complex_it != container_.complex_map_.end()) {
    return get_discriminator_value(value, single_it, complex_it, disc_type);
  }
  return read_disc_from_backing_store(value, DISCRIMINATOR_ID, mmeber_type);
}

bool DynamicDataImpl::set_union_discriminator_helper(DDS::DynamicType_var disc_type,
                                                     CORBA::Long new_disc_value,
                                                     const char* func_name) const
{
  const_single_iterator single_it;
  const_complex_iterator complex_it;
  const bool has_disc = has_discriminator_value(single_it, complex_it);
  bool has_existing_branch = false;
  if (has_disc) {
    CORBA::Long existing_disc;
    if (!get_discriminator_value(single_it, complex_it, existing_disc, disc_type)) {
      return false;
    }
    DDS::MemberDescriptor_var existing_md;
    if (get_selected_union_branch(existing_disc, has_existing_branch, existing_md) != DDS::RETCODE_OK) {
      return false;
    }
    if (has_existing_branch && !validate_discriminator(new_disc_value, existing_md)) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::%C:"
                   " Discriminator value %d does not select the activated branch (ID %u)\n",
                   func_name, new_disc_value, existing_md->id()));
      }
      return false;
    }
  }

  // In case the union has implicit default member and the input discriminator value
  // selects that implicit default member, store the discriminator value. The semantics
  // of this is similar to the _default() method of the IDL-to-C++ mapping for union.
  const set_disc_implicit_default = !has_disc || !has_existing_branch;
  if (set_disc_implicit_default && !discriminator_selects_no_member(new_disc_value)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::%C:"
                 " Can't directly set a discriminator that selects a member."
                 " Activate the member first!\n", func_name));
    }
    return false;
  }
  return true;
}

// With backing store, data for union (discriminator and selected branch) can
// scatter across the maps and the backing store. E.g., the discriminator can be
// in a map but a branch selected by it is in the backing store, and vice versa.
// In any case, the maps and backing store as a whole must represent a valid state
// of the union. That is, they represent an empty union, a union with a discriminator
// that selects no branch, or a union with a discriminator and a branch selected by it.
// Note also that the maps have priority over the backing store. So if that maps
// already have all data for the union, then the backing store won't be considered.
template<TypeKind MemberTypeKind, typename MemberType>
bool DynamicDataImpl::set_value_to_union(DDS::MemberId id, const MemberType& value,
                                         TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  // Discriminator can only be of certain types (XTypes spec, 7.2.2.4.4.3)
  if (id == DISCRIMINATOR_ID && !is_valid_discriminator_type(MemberTypeKind)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_value_to_union:"
                 " Type %C cannot be used for union discriminator\n",
                 typekind_to_string(MemberTypeKind)));
    }
    return false;
  }

  DDS::DynamicType_var member_type;
  DDS::MemberDescriptor_var md;
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

  // This follows the IDL-to-C++ mapping for union.
  if (id == DISCRIMINATOR_ID) {
    CORBA::Long disc_value;
    if (!cast_to_discriminator_value(disc_value, value)) {
      return false;
    }

    if (!set_union_discriminator_helper(member_type, disc_value, "set_value_to_union")) {
      return false;
    }
    return insert_single(id, value);
  }

  // Activate a member
  clear_container();

  return insert_valid_discriminator(md) && insert_single(id, value);
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
  // The given Id is treated as index.
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
  TypeKind collection_tk, TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const TypeKind elem_tk = elem_type->get_kind();

  if (elem_tk != ElementTypeKind && elem_tk != enum_or_bitmask) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_value_to_collection:"
                 " Could not write a value of type %C to %C with element type %C\n",
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

  return validate_member_id_collection(id, collection_tk) && insert_single(id, value);
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::set_single_value(DDS::MemberId id, const ValueType& value,
  TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (!is_type_supported(ValueTypeKind, "set_single_value")) {
    return DDS::RETCODE_ERROR;
  }

  const TypeKind tk = type_->get_kind();
  bool good = true;

  // TODO: Bitmask can be stored as a whole as a unsigned integer using MEMBER_ID_INVALID
  // (this is an extension to the XTypes spec). Elements of the bitmask can also be set
  // using the set_boolean_value interface. The two copies of the bitmask value must be
  // made consistent. For example, when a bit in the bitmask is updated, either update
  // the unsigned integer representation or invalidate it. Similarly, when the unsigned
  // integer value is updated, either update the stored elements or invalidate them all.
  if (tk == enum_or_bitmask) {
    const CORBA::ULong bit_bound = type_desc_->bound()[0];
    good = id == MEMBER_ID_INVALID && bit_bound >= lower && bit_bound <= upper &&
      insert_single(id, value);
  } else {
    switch (tk) {
    case ValueTypeKind:
      good = is_primitive(tk) && id == MEMBER_ID_INVALID && insert_single(id, value);
      break;
    case TK_STRUCTURE:
      good = set_value_to_struct<ValueTypeKind>(id, value);
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

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_single_value: "
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
  return set_single_value<TK_INT8>(id, ACE_OutputCDR::from_int8(value), TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicDataImpl::set_uint8_value(DDS::MemberId id, CORBA::UInt8 value)
{
  return set_single_value<TK_UINT8>(id, ACE_OutputCDR::from_uint8(value), TK_BITMASK, 1, 8);
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

template<TypeKind CharKind, TypeKind StringKind, typename FromCharT>
DDS::ReturnCode_t DynamicDataImpl::set_char_common(DDS::MemberId id, const FromCharT& value)
{
  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case CharKind:
    good = id == MEMBER_ID_INVALID && insert_single(id, value);
    break;
  case StringKind: {
    const CORBA::ULong bound = type_desc_->bound()[0];
    if (!check_index_from_id(tk, id, bound)) {
      good = false;
    } else {
      good = insert_single(id, value);
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

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_char_common:"
               " Failed to write DynamicData object of type %C\n", typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
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
  bool good = true;

  switch (tk) {
  case TK_BOOLEAN:
    good = id == MEMBER_ID_INVALID && insert_single(id, ACE_OutputCDR::from_boolean(value));
    break;
  case TK_BITMASK: {
    const CORBA::ULong bit_bound = type_desc_->bound()[0];
    if (!check_index_from_id(tk, id, bit_bound)) {
      good = false;
    } else {
      good = insert_single(id, ACE_OutputCDR::from_boolean(value));
    }
    break;
  }
  case TK_STRUCTURE:
    good = set_value_to_struct<TK_BOOLEAN>(id, ACE_OutputCDR::from_boolean(value));
    break;
  case TK_UNION:
    good = set_value_to_union<TK_BOOLEAN>(id, ACE_OutputCDR::from_boolean(value));
    break;
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    good = set_value_to_collection<TK_BOOLEAN>(id, ACE_OutputCDR::from_boolean(value), tk);
    break;
  default:
    good = false;
    break;
  }

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_boolean_value:"
               " Failed to write boolean to DynamicData object of type %C\n",
               typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::set_string_value(DDS::MemberId id, const char* value)
{
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
  return set_single_value<TK_STRING16>(id, value);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
DDS::ReturnCode_t DynamicDataImpl::get_simple_value_boolean(DCPS::Value& value,
                                                            DDS::MemberId id) const
{
  const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get<ACE_OutputCDR::from_boolean>().val_;
    return DDS::RETCODE_OK;
  }
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    const DynamicDataImpl* inner_dd = dynamic_cast<DynamicDataImpl*>(complex_it->second.in());
    if (!inner_dd) {
      return DDS::RETCODE_ERROR;
    }
    const_single_iterator inner_it = inner_dd->container_.single_map_.find(MEMBER_ID_INVALID);
    if (inner_it != inner_dd->container_.single_map_.end()) {
      value = inner_it->second.get<ACE_OutputCDR::from_boolean>().val_;
      return DDS::RETCODE_OK;
    }
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_simple_value_char(DCPS::Value& value,
                                                         DDS::MemberId id) const
{
  const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get<ACE_OutputCDR::from_char>().val_;
    return DDS::RETCODE_OK;
  }
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    const DynamicDataImpl* inner_dd = dynamic_cast<DynamicDataImpl*>(complex_it->second.in());
    if (!inner_dd) {
      return DDS::RETCODE_ERROR;
    }
    const_single_iterator inner_it = inner_dd->container_.single_map_.find(MEMBER_ID_INVALID);
    if (inner_it != inner_dd->container_.single_map_.end()) {
      value = inner_it->second.get<ACE_OutputCDR::from_char>().val_;
      return DDS::RETCODE_OK;
    }
  }
  return DDS::RETCODE_ERROR;
}

template<typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_simple_value_primitive(DCPS::Value& value,
                                                              DDS::MemberId id) const
{
  const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get<ValueType>();
    return DDS::RETCODE_OK;
  }
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    const DynamicDataImpl* inner_dd = dynamic_cast<DynamicDataImpl*>(complex_it->second.in());
    if (!inner_dd) {
      return DDS::RETCODE_ERROR;
    }
    const_single_iterator inner_it = inner_dd->container_.single_map_.find(MEMBER_ID_INVALID);
    if (inner_it != inner_dd->container_.single_map_.end()) {
      value = inner_it->second.get<ValueType>();
      return DDS::RETCODE_OK;
    }
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_simple_value_string(DCPS::Value& value,
                                                           DDS::MemberId id) const
{
  const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get<const char*>();
    return DDS::RETCODE_OK;
  }

  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    // The string member has its own DynamicData object.
    const DynamicDataImpl* str_dd = dynamic_cast<DynamicDataImpl*>(complex_it->second.in());
    char* str = 0;
    if (!str_dd || !str_dd->read_basic_value(str)) {
      return DDS::RETCODE_ERROR;
    }
    value = str;
    return DDS::RETCODE_OK;
  }
  return DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_simple_value_enum(DCPS::Value& value,
                                                         DDS::MemberId id) const
{
  DDS::DynamicType_var mtype;
  DDS::ReturnCode_t ret = get_member_type(mtype, type_, id);
  if (ret != DDS::RETCODE_OK) {
    return ret;
  }

  DDS::Int32 enumAsInteger;
  ret = get_enum_value(enumAsInteger, mtype, interface_from_this(), id);
  if (ret != DDS::RETCODE_OK) {
    return ret;
  }

  DDS::String8_var str;
  ret = get_enumerator_name(str, enumAsInteger, mtype);
  if (ret != DDS::RETCODE_OK) {
    return ret;
  }

  value = str.in();
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataImpl::get_simple_value(DCPS::Value& value, DDS::MemberId id)
{
  DDS::DynamicTypeMember_var dtm;
  if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }
  DDS::MemberDescriptor_var md;
  if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }
  DDS::DynamicType_var member_type = get_base_type(md->type());
  const TypeKind member_kind = member_type->get_kind();
  switch (member_kind) {
  case TK_BOOLEAN:
    return get_simple_value_boolean(value, id);
  case TK_INT32:
    return get_simple_value_primitive<CORBA::Long>(value, id);
  case TK_UINT32:
    return get_simple_value_primitive<CORBA::ULong>(value, id);
  case TK_INT64:
    return get_simple_value_primitive<CORBA::LongLong>(value, id);
  case TK_UINT64:
    return get_simple_value_primitive<CORBA::ULongLong>(value, id);
  case TK_CHAR8:
    return get_simple_value_char(value, id);
  case TK_FLOAT64:
    return get_simple_value_primitive<CORBA::Double>(value, id);
  case TK_FLOAT128:
    return get_simple_value_primitive<CORBA::LongDouble>(value, id);
  case TK_STRING8:
    return get_simple_value_string(value, id);
  case TK_ENUM:
    return get_simple_value_enum(value, id);
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_simple_value:"
                 " Member type %C is not supported by DCPS::Value\n",
                 typekind_to_string(member_kind)));
    }
  }
  return DDS::RETCODE_ERROR;
}
#endif

bool DynamicDataImpl::serialized_size(const DCPS::Encoding& enc, size_t& size, DCPS::Sample::Extent ext) const
{
  return serialized_size_i(enc, size, ext);
}

bool DynamicDataImpl::serialize(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const
{
  return serialize_i(ser, ext);
}

bool DynamicDataImpl::set_complex_to_struct(DDS::MemberId id, DDS::DynamicData_var value)
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
  const DDS::DynamicType_var value_type = value->type();
  if (!member_type || !value_type || !member_type->equals(value_type)) {
    return false;
  }
  return insert_complex(id, value);
}

bool DynamicDataImpl::set_complex_to_union(DDS::MemberId id, DDS::DynamicData_var value)
{
  if (id == DISCRIMINATOR_ID) {
    DDS::DynamicType_var disc_type = get_base_type(type_desc_->discriminator_type());
    const DDS::DynamicType_var value_type = value->type();
    if (!disc_type->equals(value_type)) {
      return false;
    }

    CORBA::Long disc_val;
    const DynamicDataImpl* dd_impl = dynamic_cast<const DynamicDataImpl*>(value.in());
    if (!dd_impl || !dd_impl->read_discriminator(disc_val)) {
      return false;
    }

    if (!set_union_discriminator_helper(disc_type, disc_val, "set_complex_to_union")) {
      return false;
    }
    return insert_complex(id, value);
  }

  // Activate a member
  clear_container();

  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, id) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var md;
  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var value_type = value->type();
  if (!get_base_type(md->type())->equals(value_type)) {
    return false;
  }

  return insert_valid_discriminator(md) && insert_complex(id, value);
}

bool DynamicDataImpl::validate_member_id_collection(DDS::MemberId id, TypeKind tk) const
{
  switch (tk) {
  case TK_SEQUENCE:
  case TK_ARRAY:
    return check_index_from_id(tk, id, bound_total(type_desc_));
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::validate_member_id_collection::"
                 " Map is currently not supported\n"));
    }
  }
  return false;
}

bool DynamicDataImpl::set_complex_to_collection(DDS::MemberId id, DDS::DynamicData_var value,
                                                TypeKind collection_tk)
{
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const DDS::DynamicType_var value_type = value->type();
  if (!elem_type->equals(value_type)) {
    return false;
  }

  return validate_member_id_collection(id, collection_tk) && insert_complex(id, value);
}

DDS::ReturnCode_t DynamicDataImpl::set_complex_value(DDS::MemberId id, DDS::DynamicData_ptr value)
{
  DDS::DynamicData_var value_var = DDS::DynamicData::_duplicate(value);
  const TypeKind tk = type_->get_kind();
  bool good = false;

  switch (tk) {
  case TK_STRUCTURE:
    good = set_complex_to_struct(id, value_var);
    break;
  case TK_UNION:
    good = set_complex_to_union(id, value_var);
    break;
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    good = set_complex_to_collection(id, value_var, tk);
    break;
  default:
    good = false;
    break;
  }

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_complex_value:"
               " Failed to write complex value for member with ID %d\n", id));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

template<typename SequenceType>
bool DynamicDataImpl::insert_sequence(DDS::MemberId id, const SequenceType& value)
{
  if (container_.complex_map_.erase(id) == 0) {
    container_.sequence_map_.erase(id);
  }
  return container_.sequence_map_.insert(std::make_pair(id, value)).second;
}

template<TypeKind ElementTypeKind>
bool DynamicDataImpl::check_seqmem_in_struct_and_union(DDS::MemberId id, TypeKind enum_or_bitmask,
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
  return check_seqmem_in_struct_and_union<ElementTypeKind>(id, enum_or_bitmask, lower, upper) &&
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
  if (!check_seqmem_in_struct_and_union<ElementTypeKind>(id, enum_or_bitmask, lower, upper)) {
    return false;
  }

  clear_container();

  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, id) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var md;
  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }
  return insert_valid_discriminator(md) && insert_sequence(id, value);
}

template<TypeKind ElementTypeKind>
bool DynamicDataImpl::check_seqmem_in_sequence_and_array(DDS::MemberId id, CORBA::ULong bound,
  TypeKind enum_or_bitmask, LBound lower, LBound upper) const
{
  if (!check_index_from_id(type_->get_kind(), id, bound)) {
    return false;
  }

  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
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
  const DDS::UInt32 bound = type_desc_->bound()[0];
  return
    check_seqmem_in_sequence_and_array<ElementTypeKind>(id, bound, enum_or_bitmask, lower, upper) &&
    validate_member_id_collection(id, TK_SEQUENCE) &&
    insert_sequence(id, value);
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::set_values_to_array(DDS::MemberId id, const SequenceType& value,
                                          TypeKind enum_or_bitmask,
                                          LBound lower, LBound upper)
{
  const DDS::UInt32 length = bound_total(type_desc_);
  return
    check_seqmem_in_sequence_and_array<ElementTypeKind>(id, length, enum_or_bitmask, lower, upper) &&
    validate_member_id_collection(id, TK_ARRAY) &&
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
    good = set_values_to_sequence<ElementTypeKind>(id, value, enum_or_bitmask, lower, upper);
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
    CORBA::string_free(value);
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
    CORBA::wstring_free(value);
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

template<typename ValueType>
bool DynamicDataImpl::read_basic_member(ValueType& value, DDS::MemberId id)
{
  return read_basic_in_single_map(value, id)
    || read_basic_in_complex_map(value, id)
    || get_value_from_backing_store(value, id);
}

void DynamicDataImpl::set_backing_store(DynamicDataXcdrReadImpl* xcdr_store)
{
  CORBA::release(backing_store_);
  backing_store_ = dynamic_cast<DynamicDataXcdrReadImpl*>(DDS::DynamicData::_duplicate(xcdr_store));
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_int8& value,
                                                   DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_int8_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_uint8& value,
                                                   DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_uint8_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::Short& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_int16_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::UShort& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_uint16_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::Long& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_int32_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::ULong& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_uint32_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::LongLong& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_int64_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::ULongLong& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_uint64_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::Float& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_float32_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::Double& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_float64_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::LongDouble& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_float128_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_octet& value,
                                                   DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_byte_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(char*& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_string_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(CORBA::WChar*& value, DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_wstring_value(value, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_char& value,
                                                   DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_char8_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_wchar& value,
                                                   DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_char16_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

bool DynamicDataImpl::get_value_from_backing_store(ACE_OutputCDR::from_boolean& value,
                                                   DDS::MemberId id) const
{
  return backing_store_ && backing_store_->get_boolean_value(value.val_, id) == DDS::RETCODE_OK
    && insert_single(id, value);
}

template<typename ValueType>
bool DynamicDataImpl::get_value_from_self(ValueType& value, DDS::MemberId id)
{
  // Primitive or enum value can be read using MEMBER_ID_INVALID.
  if (!is_primitive(type_->get_kind()) || id != MEMBER_ID_INVALID) {
    return false;
  }
  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    value = it->second.get<ValueType>();
  } else if (!get_value_from_backing_store(value, id)) {
    set_default_basic_value(value);
  }
  return true;
}

template<>
bool DynamicDataImpl::get_value_from_self(char*&, DDS::MemberId)
{
  // Can't read a string from a DynamicData instance representing a string.
  return false;
}

template<>
bool DynamicDataImpl::get_value_from_self(CORBA::WChar*&, DDS::MemberId)
{
  // Can't read a wstring from a DynamicData instance representing a wstring.
  return false;
}

template<TypeKind ValueTypeKind, typename ValueType>
bool DynamicDataImpl::get_value_from_enum(ValueType& value, DDS::MemberId id)
{
  TypeKind treat_as_tk;
  const DDS::ReturnCode_t rc = enum_bound(type_, treat_as_tk);
  if (rc != DDS::RETCODE_OK || treat_as_tk != ValueTypeKind || id != MEMBER_ID_INVALID) {
    return false;
  }
  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    value = it->second.get<ValueType>();
  } else if (!get_value_from_backing_store(value, id)) {
    CORBA::Long enum_default_val;
    if (!set_default_enum_value(type_, enum_default_val)) {
      return false;
    }
    cast_to_enum_value(value, enum_default_val);
  }
  return true;
}

template<>
bool DynamicDataImpl::get_value_from_enum<TK_STRING8>(char*&, DDS::MemberId)
{
  return false;
}
template<>
bool DynamicDataImpl::get_value_from_enum<TK_STRING16>(CORBA::WChar*&, DDS::MemberId)
{
  return false;
}

template<TypeKind ValueTypeKind, typename ValueType>
bool DynamicDataImpl::get_value_from_bitmask(ValueType& value, DDS::MemberId id)
{
  // Allow bitmask to be read as an unsigned integer.
  TypeKind treat_as_tk;
  const DDS::ReturnCode_t rc = bitmask_bound(type_, treat_as_tk);
  if (rc != DDS::RETCODE_OK || treat_as_tk != ValueTypeKind || id != MEMBER_ID_INVALID) {
    return false;
  }
  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    value = it->second.get<ValueType>();
  } else if (!get_value_from_backing_store(value, id)) {
    set_default_bitmask_value(value);
  }
  return true;
}

template<>
bool DynamicDataImpl::get_value_from_bitmask<TK_STRING8>(char*&, DDS::MemberId)
{
  return false;
}
template<>
bool DynamicDataImpl::get_value_from_bitmask<TK_STRING16>(CORBA::WChar*&, DDS::MemberId)
{
  return false;
}

template<TypeKind ValueTypeKind, typename ValueType>
bool DynamicDataImpl::get_value_from_struct(ValueType& value, DDS::MemberId id)
{
  DDS::MemberDescriptor_var md;
  DDS::DynamicType_var member_type;
  DDS::ReturnCode_t rc = check_member(
    md, member_type, "DynamicDataImpl::get_value_from_struct", "get", id, ValueTypeKind);
  if (rc != DDS::RETCODE_OK) {
    return false;
  }
  if (read_basic_member(value, id)) {
    return true;
  }

  // Not returning a default value for a missing optional member.
  if (md->is_optional()) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_value_from_struct:"
                 " Optional member Id %u is not present\n", id));
    }
    return false;
  }
  set_default_basic_value(value);
  return true;
}

template<TypeKind ValueTypeKind, typename ValueType>
bool DynamicDataImpl::get_value_from_union(ValueType& value, DDS::MemberId id)
{
  DDS::MemberDescriptor_var md;
  DDS::DynamicType_var member_type;
  DDS::ReturnCode_t rc = check_member(
    md, member_type, "DynamicDataImpl::get_value_from_union", "get", id, ValueTypeKind);
  if (rc != DDS::RETCODE_OK) {
    return false;
  }

  // Return the member if the container or the backing store has it.
  if (read_basic_member(value, id)) {
    return true;
  }

  if (id == DISCRIMINATOR_ID) {
    // Set the discriminator to default value.
    // If it selects a branch, set the branch to default value.
    set_default_basic_value(value);
    CORBA::Long disc_value;
    if (!cast_to_discriminator_value(disc_value, value)) {
      return false;
    }
    bool found_selected_member = false;
    DDS::MemberDescriptor_var selected_md;
    const DDS::ReturnCode_t rc =
      get_selected_union_branch(disc_value, found_selected_member, selected_md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_value_from_union:"
                   " get_selected_union_branch failed: %C\n", retcode_to_string(rc)));
      }
      return false;
    }
    insert_single(id, value);
    if (found_selected_member && !selected_md->is_optional()) {
      DDS::DynamicType_var selected_type = get_base_type(selected_md->type());
      if (clear_value_i(selected_md->id(), selected_type) != DDS::RETCODE_OK) {
        return false;
      }
    }
  } else {
    const_single_iterator single_it;
    const_complex_iterator complex_it;
    if (has_discriminator_value(single_it, complex_it)) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_value_from_union:"
                   " Branch Id %u is not the active branch in the union\n", id));
      }
      return false;
    }
    // Set the branch to default value and set the discriminator to a value that selects this branch.
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var md;
    if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::DynamicType_var dt = get_base_type(md->type());
    if (clear_value_i(id, dt) != DDS::RETCODE_OK) {
      return false;
    }
    if (!insert_valid_discriminator(md)) {
      return false;
    }
    OPENDDS_ASSERT(read_basic_in_single_map(value, id));
  }
  return true;
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

template<TypeKind ValueTypeKind, typename ValueType>
bool DynamicDataImpl::get_value_from_collection(ValueType& value, DDS::MemberId id)
{
  if (type_->get_kind() == TK_ARRAY && id >= bound_total(type_desc_)) {
    return false;
  }

  DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  TypeKind treat_as_tk = elem_tk;
  switch (elem_tk) {
  case TK_ENUM:
    if (enum_bound(elem_type, treat_as_tk) != DDS::RETCODE_OK) {
      return false;
    }
    break;
  case TK_BITMASK: {
    if (bitmask_bound(elem_type, treat_as_tk) != DDS::RETCODE_OK) {
      return false;
    }
    break;
  }
  }
  if (treat_as_tk != ValueTypeKind) {
    return false;
  }
  if (read_basic_member(value, id)) {
    return true;
  }
  set_default_basic_value(value);

  // Must insert this member in case it's index is larger than the current largest index,
  // so that all new members up to this member are serialized. Otherwise, we would be returning
  // a value that wouldn't be in the serialized data.
  insert_single(id, value);
  return true;
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_single_value(ValueType& value, DDS::MemberId id)
{
  if (!is_type_supported(ValueTypeKind, "get_single_value")) {
    return DDS::RETCODE_ERROR;
  }
  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case ValueTypeKind:
    good = get_value_from_self(value, id);
    break;
  case TK_ENUM:
    good = get_value_from_enum<ValueTypeKind>(value, id);
    break;
  case TK_BITMASK:
    good = get_value_from_bitmask<ValueTypeKind>(value, id);
    break;
  case TK_STRUCTURE:
    good = get_value_from_struct<ValueTypeKind>(value, id);
    break;
  case TK_UNION:
    good = get_value_from_union<ValueTypeKind>(value, id);
    break;
  case TK_SEQUENCE:
  case TK_ARRAY:
    good = get_value_from_collection<ValueTypeKind>(value, id);
    break;
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_single_value:"
                 " Map is currently not supported\n"));
    }
    good = false;
    break;
  default:
    good = false;
    break;
  }

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_single_value:"
               " Failed to read a value of type %C from a DynamicData object of type %C\n",
               typekind_to_string(ValueTypeKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_int8_value(CORBA::Int8& value, DDS::MemberId id)
{
  ACE_OutputCDR::from_int8 from_int8(0);
  const DDS::ReturnCode_t rc = get_single_value<TK_INT8>(from_int8, id);
  if (rc == DDS::RETCODE_OK) {
    value = from_int8.val_;
  }
  return rc;
}

DDS::ReturnCode_t DynamicDataImpl::get_uint8_value(CORBA::UInt8& value, DDS::MemberId id)
{
  ACE_OutputCDR::from_uint8 from_uint8(0);
  const DDS::ReturnCode_t rc = get_single_value<TK_UINT8>(from_uint8, id);
  if (rc == DDS::RETCODE_OK) {
    value = from_uint8.val_;
  }
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
  bool good = true;
  switch (tk) {
  case CharKind: {
    FromCharT from_char_t('\0');
    good = get_value_from_self(from_char_t, id);
    if (good) {
      value = from_char_t.val_;
    }
    break;
  }
  case StringKind: {
    FromCharT from_char('\0');
    good = read_basic_member(from_char, id);
    if (good) {
      value = from_char.val_;
    }
    break;
  }
  case TK_STRUCTURE: {
    FromCharT from_char('\0');
    good = get_value_from_struct<CharKind>(from_char, id);
    if (good) {
      value = from_char.val_;
    }
    break;
  }
  case TK_UNION: {
    FromCharT from_char('\0');
    good = get_value_from_union<CharKind>(from_char, id);
    if (good) {
      value = from_char.val_;
    }
    break;
  }
  case TK_SEQUENCE:
  case TK_ARRAY: {
    FromCharT from_char('\0');
    good = get_value_from_collection<CharKind>(from_char, id);
    if (good) {
      value = from_char.val_;
    }
    break;
  }
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_char_common:"
                 " Map is currently not supported\n"));
    }
    good = false;
    break;
  default:
    good = false;
    break;
  }

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_char_common::"
               " Failed to read a value of type %C from a DynamicData object of type %C\n",
               typekind_to_string(CharKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
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
  ACE_OutputCDR::from_octet from_octet(0);
  const DDS::ReturnCode_t rc = get_single_value<TK_BYTE>(from_octet, id);
  if (rc == DDS::RETCODE_OK) {
    value = from_octet.val_;
  }
  return rc;
}

template<typename UIntType>
bool DynamicDataImpl::get_boolean_from_bitmask(CORBA::ULong index, CORBA::Boolean& value)
{
  UIntType bitmask;
  if (!read_basic_value(bitmask)) {
    return false;
  }
  value = (1ULL << index) & bitmask;
  return true;
}

template<>
bool DynamicDataImpl::get_boolean_from_bitmask<CORBA::UInt8>(CORBA::ULong index, CORBA::Boolean& value)
{
  ACE_OutputCDR::from_uint8 bitmask(0);
  if (!read_basic_value(bitmask)) {
    return false;
  }
  value = ((1 << index) & bitmask.val_) ? true : false;
  return true;
}

DDS::ReturnCode_t DynamicDataImpl::get_boolean_value(CORBA::Boolean& value, DDS::MemberId id)
{
  const TypeKind tk = type_->get_kind();
  bool good = true;
  switch (tk) {
  case TK_BOOLEAN: {
    ACE_OutputCDR::from_boolean from_bool(false);
    good = get_value_from_self(from_bool, id);
    if (good) {
      value = from_bool.val_;
    }
    break;
  }
  case TK_BITMASK: {
    const LBound bitbound = type_desc_->bound()[0];
    ACE_CDR::ULong index;
    if (!get_index_from_id(id, index, bitbound)) {
      good = false;
      break;
    }
    if (bitbound >= 1 && bitbound <= 8) {
      good = get_boolean_from_bitmask<CORBA::UInt8>(index, value);
    } else if (bitbound >= 9 && bitbound <= 16) {
      good = get_boolean_from_bitmask<CORBA::UShort>(index, value);
    } else if (bitbound >= 17 && bitbound <= 32) {
      good = get_boolean_from_bitmask<CORBA::ULong>(index, value);
    } else {
      good = get_boolean_from_bitmask<CORBA::ULongLong>(index, value);
    }
    break;
  }
  case TK_STRUCTURE: {
    ACE_OutputCDR::from_boolean from_bool(false);
    good = get_value_from_struct<TK_BOOLEAN>(from_bool, id);
    if (good) {
      value = from_bool.val_;
    }
    break;
  }
  case TK_UNION: {
    ACE_OutputCDR::from_boolean from_bool(false);
    good = get_value_from_union<TK_BOOLEAN>(from_bool, id);
    if (good) {
      value = from_bool.val_;
    }
    break;
  }
  case TK_SEQUENCE:
  case TK_ARRAY: {
    ACE_OutputCDR::from_boolean from_bool(false);
    good = get_value_from_collection<TK_BOOLEAN>(from_bool, id);
    if (good) {
      value = from_bool.val_;
    }
    break;
  }
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_boolean_value:"
                 " Map is currently not supported\n"));
    }
    good = false;
    break;
  default:
    good = false;
    break;
  }

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_boolean_value:"
               " Failed to read a boolean value from a DynamicData object of type %C\n",
               typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_string_value(char*& value, DDS::MemberId id)
{
  if (enum_string_helper(value, id)) {
    return DDS::RETCODE_OK;
  }

  CORBA::string_free(value);
  return get_single_value<TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_wstring_value(CORBA::WChar*& value, DDS::MemberId id)
{
#ifdef DDS_HAS_WCHAR
  CORBA::wstring_free(value);
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
    data->insert_single(i, values[i]);
  }
}

// Get the inner C-string explicitly
template<>
void DynamicDataImpl::move_sequence_helper<DDS::StringSeq>(const const_sequence_iterator& it,
                                                           DynamicDataImpl* data)
{
  const DDS::StringSeq& values = it->second.get<DDS::StringSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(i, values[i].in());
  }
}

#ifdef DDS_HAS_WCHAR
template<>
void DynamicDataImpl::move_sequence_helper<DDS::WstringSeq>(const const_sequence_iterator& it,
                                                            DynamicDataImpl* data)
{
  const DDS::WstringSeq& values = it->second.get<DDS::WstringSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(i, values[i].in());
  }
}
#endif

template<>
void DynamicDataImpl::move_sequence_helper<DDS::Int8Seq>(const const_sequence_iterator& it,
                                                         DynamicDataImpl* data)
{
  const DDS::Int8Seq& values = it->second.get<DDS::Int8Seq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(i, ACE_OutputCDR::from_int8(values[i]));
  }
}

template<>
void DynamicDataImpl::move_sequence_helper<DDS::UInt8Seq>(const const_sequence_iterator& it,
                                                          DynamicDataImpl* data)
{
  const DDS::UInt8Seq& values = it->second.get<DDS::UInt8Seq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(i, ACE_OutputCDR::from_uint8(values[i]));
  }
}

template<>
void DynamicDataImpl::move_sequence_helper<DDS::CharSeq>(const const_sequence_iterator& it,
                                                         DynamicDataImpl* data)
{
  const DDS::CharSeq& values = it->second.get<DDS::CharSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(i, ACE_OutputCDR::from_char(values[i]));
  }
}

template<>
void DynamicDataImpl::move_sequence_helper<DDS::ByteSeq>(const const_sequence_iterator& it,
                                                         DynamicDataImpl* data)
{
  const DDS::ByteSeq& values = it->second.get<DDS::ByteSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(i, ACE_OutputCDR::from_octet(values[i]));
  }
}

template<>
void DynamicDataImpl::move_sequence_helper<DDS::BooleanSeq>(const const_sequence_iterator& it,
                                                            DynamicDataImpl* data)
{
  const DDS::BooleanSeq& values = it->second.get<DDS::BooleanSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(i, ACE_OutputCDR::from_boolean(values[i]));
  }
}

#ifdef DDS_HAS_WCHAR
template<>
void DynamicDataImpl::move_sequence_helper<DDS::WcharSeq>(const const_sequence_iterator& it,
                                                          DynamicDataImpl* data)
{
  const DDS::WcharSeq& values = it->second.get<DDS::WcharSeq>();
  for (CORBA::ULong i = 0; i < values.length(); ++i) {
    data->insert_single(i, ACE_OutputCDR::from_wchar(values[i]));
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

  switch (elem_type->get_kind()) {
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

bool DynamicDataImpl::get_complex_from_aggregated(DDS::DynamicData_var& value, DDS::MemberId id,
                                                  FoundStatus& found_status)
{
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    value = DDS::DynamicData::_duplicate(complex_it->second);
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

bool DynamicDataImpl::set_member_backing_store(DynamicDataImpl* member_ddi, DDS::MemberId id)
{
  DDS::DynamicData_var member_dd;
  const DDS::ReturnCode rc = backing_store_->get_complex_value(member_dd, id);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_member_backing_store:"
                 " Get complex value for member ID: %u failed: %C\n",
                 id, retcode_to_string(rc)));
    }
    return false;
  }
  DynamicDataXcdrReadImpl* member_store = dynamic_cast<DynamicDataXcdrReadImpl*>(member_dd.in());
  if (!member_store) {
    return false;
  }
  member_ddi->set_backing_store(member_store);
  return true;
}

bool DynamicDataImpl::get_complex_from_struct(DDS::DynamicData_ptr& value, DDS::MemberId id)
{
  FoundStatus found_status = NOT_FOUND;
  DDS::DynamicData_var dd_var;
  if (!get_complex_from_aggregated(dd_var, id, found_status)) {
    return false;
  }
  if (found_status == NOT_FOUND && backing_store_) {
    // The returned DynamicDataImpl object contains the data for the member
    // from the backing store, if available.
    DynamicDataImpl* ddi = dynamic_cast<DynamicDataImpl*>(dd_var.in());
    if (!ddi) {
      return false;
    }
    set_member_backing_store(ddi, id);
  }

  if (found_status == FOUND_IN_NON_COMPLEX_MAP || found_status == NOT_FOUND) {
    insert_complex(id, dd_var);
  }
  CORBA::release(value);
  value = DDS::DynamicData::_duplicate(dd_var);
  return true;
}

bool DynamicDataImpl::write_discriminator_helper(CORBA::Long value, TypeKind treat_as)
{
  switch (treat_as) {
  case TK_BOOLEAN:
    return insert_single(MEMBER_ID_INVALID, ACE_OutputCDR::from_boolean(value));
  case TK_BYTE:
    return insert_single(MEMBER_ID_INVALID, ACE_OutputCDR::from_octet(value));
  case TK_CHAR8:
    return insert_single(MEMBER_ID_INVALID, ACE_OutputCDR::from_char(value));
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return insert_single(MEMBER_ID_INVALID, ACE_OutputCDR::from_wchar(value));
#endif
  case TK_INT8:
    return insert_single(MEMBER_ID_INVALID, ACE_OutputCDR::from_int8(value));
  case TK_UINT8:
    return insert_single(MEMBER_ID_INVALID, ACE_OutputCDR::from_uint8(value));
  case TK_INT16:
    return insert_single(MEMBER_ID_INVALID, static_cast<CORBA::Short>(value));
  case TK_UINT16:
    return insert_single(MEMBER_ID_INVALID, static_cast<CORBA::UShort>(value));
  case TK_INT32:
    return insert_single(MEMBER_ID_INVALID, value);
  case TK_UINT32:
    return insert_single(MEMBER_ID_INVALID, static_cast<CORBA::ULong>(value));
  case TK_INT64:
    return insert_single(MEMBER_ID_INVALID, static_cast<CORBA::LongLong>(value));
  case TK_UINT64:
    return insert_single(MEMBER_ID_INVALID, static_cast<CORBA::ULongLong>(value));
  default:
    return false;
  }
}

// Write value to discriminator represented by a DynamicData instance.
bool DynamicDataImpl::write_discriminator(CORBA::Long value)
{
  TypeKind treat_as = type_->get_kind();
  if (treat_as == TK_ENUM) {
    if (enum_bound(type_, treat_as) != DDS::RETCODE_OK) {
      return false;
    }
  }
  return write_discriminator_helper(value, treat_as);
}

bool DynamicDataImpl::get_complex_from_union(DDS::DynamicData_ptr& value, DDS::MemberId id)
{
  FoundStatus found_status = NOT_FOUND;
  DDS::DynamicData_var dd_var;
  if (!get_complex_from_aggregated(dd_var, id, found_status)) {
    return false;
  }
  if (found_status != NOT_FOUND) {
    if (found_status == FOUND_IN_NON_COMPLEX_MAP) {
      insert_complex(id, dd_var);
    }
    CORBA::release(value);
    value = DDS::DynamicData::_duplicate(dd_var);
    return true;
  }

  // Cases where the requested member is not found in the maps.
  if (backing_store_) {
    DynamicDataImpl* ddi = dynamic_cast<DynamicDataImpl*>(dd_var.in());
    if (!ddi) {
      return false;
    }
    if (!set_member_backing_store(ddi, id)) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_complex_from_union:"
                   " Set backing store for member ID %u failed\n", id));
      }
      return false;
    }
    insert_complex(id, dd_var);
  } else {
    // Return default value for the requested member.
    if (id == DISCRIMINATOR_ID) {
      DDS::DynamicType_var disc_type = dd_var->type();
      CORBA::Long disc_value;
      if (!set_default_discriminator_value(disc_value, disc_type)) {
        return false;
      }
      bool found_selected_member = false;
      DDS::MemberDescriptor_var selected_md;
      const DDS::ReturnCode_t rc =
        get_selected_union_branch(disc_value, found_selected_member, selected_md);
      if (rc != DDS::RETCODE_OK) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_complex_from_union:"
                     " get_selected_union_branch failed: %C\n", retcode_to_string(rc)));
        }
        return false;
      }
      DynamicDataImpl* dd_impl = dynamic_cast<DynamicDataImpl*>(dd_var.in());
      dd_impl->write_discriminator(disc_value);
      insert_complex(DISCRIMINATOR_ID, dd_var);
      if (found_selected_member && !selected_md->is_optional()) {
        DDS::DynamicType_var selected_type = get_base_type(selected_md->type());
        if (clear_value_i(selected_md->id(), selected_type) != DDS::RETCODE_OK) {
          return false;
        }
      }
    } else {
      const_single_iterator single_it;
      const_complex_iterator complex_it;
      if (has_discriminator_value(single_it, complex_it)) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_complex_from_union:"
                     " Branch Id %u is not the active branch in the union\n", id));
        }
        return false;
      }
      DDS::DynamicTypeMember_var dtm;
      if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
        return false;
      }
      DDS::MemberDescriptor_var md;
      if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
        return false;
      }
      return insert_valid_discriminator(md) && insert_complex(id, dd_var);
    }
  }

  CORBA::release(value);
  value = DDS::DynamicData::_duplicate(dd_var);
  return true;
}

bool DynamicDataImpl::get_complex_from_collection(DDS::DynamicData_ptr& value, DDS::MemberId id)
{
  if (type_->get_kind() == TK_ARRAY && id >= bound_total(type_desc_)) {
    return false;
  }

  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    CORBA::release(value);
    value = DDS::DynamicData::_duplicate(complex_it->second);
    return true;
  }

  DynamicDataImpl* dd_impl = new DynamicDataImpl(type_desc_->element_type());
  DDS::DynamicData_var dd_var = dd_impl;

  const_single_iterator single_it = container_.single_map_.find(id);
  bool found_in_maps = false;
  if (single_it != container_.single_map_.end()) {
    if (!move_single_to_complex(single_it, dd_impl)) {
      return false;
    }
    found_in_maps = true;
  } else {
    const_sequence_iterator sequence_it = container_.sequence_map_.find(id);
    if (sequence_it != container_.sequence_map_.end()) {
      if (!move_sequence_to_complex(sequence_it, dd_impl)) {
        return false;
      }
      found_in_maps = true;
    }
  }
  if (!found_in_maps && backing_store_) {
    // Reading an out-of-range element from the backing store doesn't signify an error.
    set_member_backing_store(dd_impl, id);
  }

  insert_complex(id, dd_var);
  CORBA::release(value);
  value = DDS::DynamicData::_duplicate(dd_var);
  return true;
}

DDS::ReturnCode_t DynamicDataImpl::get_complex_value(DDS::DynamicData_ptr& value, DDS::MemberId id)
{
  const TypeKind tk = type_->get_kind();
  bool good = true;
  switch (tk) {
  case TK_STRUCTURE:
    good = get_complex_from_struct(value, id);
    break;
  case TK_UNION:
    good = get_complex_from_union(value, id);
    break;
  case TK_SEQUENCE:
  case TK_ARRAY:
    good = get_complex_from_collection(value, id);
    break;
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_complex_value:"
                 " Map is currently not supported\n"));
    }
    good = false;
    break;
  default:
    good = false;
    break;
  }

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_complex_value:"
               " Failed to read a complex value from a DynamicData object of type %C\n",
               typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
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

template<typename PrimitiveType>
bool DynamicDataImpl::serialize_primitive_value(DCPS::Serializer& ser,
                                                PrimitiveType default_value) const
{
  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    return serialize_single_value(ser, it->second);
  }

  // No data stored. Use default value.
  set_default_basic_value(default_value);
  return ser << default_value;
}

bool DynamicDataImpl::serialized_size_enum(const DCPS::Encoding& encoding,
  size_t& size, const DDS::DynamicType_var& enum_type) const
{
  DDS::TypeDescriptor_var enum_td;
  if (enum_type->get_descriptor(enum_td) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bit_bound = enum_td->bound()[0];
  if (bit_bound >= 1 && bit_bound <= 8) {
    primitive_serialized_size_int8(encoding, size);
    return true;
  } else if (bit_bound >= 9 && bit_bound <= 16) {
    return primitive_serialized_size(encoding, size, CORBA::Short());
  } else if (bit_bound >= 17 && bit_bound <= 32) {
    return primitive_serialized_size(encoding, size, CORBA::Long());
  }
  return false;
}

bool DynamicDataImpl::serialize_enum_default_value(DCPS::Serializer& ser,
  const DDS::DynamicType_var& enum_type) const
{
  // The first enumerator is used as the enum's default value (Table 9).
  DDS::DynamicTypeMember_var first_dtm;
  if (enum_type->get_member_by_index(first_dtm, 0) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var first_md;
  if (first_dtm->get_descriptor(first_md) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::TypeDescriptor_var descriptor;
  if (enum_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bit_bound = descriptor->bound()[0];

  if (bit_bound >= 1 && bit_bound <= 8) {
    return ser << static_cast<CORBA::Int8>(first_md->id());
  } else if (bit_bound >= 9 && bit_bound <= 16) {
    return ser << static_cast<CORBA::Short>(first_md->id());
  } else if (bit_bound >= 17 && bit_bound <= 32) {
    return ser << static_cast<CORBA::Long>(first_md->id());
  }
  return false;
}

bool DynamicDataImpl::serialize_enum_value(DCPS::Serializer& ser) const
{
  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    return serialize_single_value(ser, it->second);
  }
  return serialize_enum_default_value(ser, type_);
}

bool DynamicDataImpl::serialized_size_bitmask(const DCPS::Encoding& encoding,
  size_t& size, const DDS::DynamicType_var& bitmask_type) const
{
  DDS::TypeDescriptor_var bitmask_td;
  if (bitmask_type->get_descriptor(bitmask_td) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bit_bound = bitmask_td->bound()[0];
  if (bit_bound >= 1 && bit_bound <= 8) {
    primitive_serialized_size_uint8(encoding, size);
    return true;
  } else if (bit_bound >= 9 && bit_bound <= 16) {
    return primitive_serialized_size(encoding, size, CORBA::UShort());
  } else if (bit_bound >= 17 && bit_bound <= 32) {
    return primitive_serialized_size(encoding, size, CORBA::ULong());
  } else if (bit_bound >= 33 && bit_bound <= 64) {
    return primitive_serialized_size(encoding, size, CORBA::ULongLong());
  }
  return false;
}

bool DynamicDataImpl::serialize_bitmask_default_value(DCPS::Serializer& ser,
  const DDS::DynamicType_var& bitmask_type) const
{
  DDS::TypeDescriptor_var descriptor;
  if (bitmask_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bit_bound = descriptor->bound()[0];

  // Table 9 doesn't mention default value for bitmask. Use 0 as default here.
  if (bit_bound >= 1 && bit_bound <= 8) {
    return ser << static_cast<CORBA::UInt8>(0);
  } else if (bit_bound >= 9 && bit_bound <= 16) {
    return ser << static_cast<CORBA::UShort>(0);
  } else if (bit_bound >= 17 && bit_bound <= 32) {
    return ser << static_cast<CORBA::ULong>(0);
  } else if (bit_bound >= 33 && bit_bound <= 64) {
    return ser << static_cast<CORBA::ULongLong>(0);
  }
  return false;
}

bool DynamicDataImpl::serialize_bitmask_value(DCPS::Serializer& ser) const
{
  const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it != container_.single_map_.end()) {
    return serialize_single_value(ser, it->second);
  }
  return serialize_bitmask_default_value(ser, type_);
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

bool DynamicDataImpl::serialized_size_string(const DCPS::Encoding& encoding, size_t& size) const
{
  const bool is_empty = container_.single_map_.empty() && container_.complex_map_.empty();
  if (!is_empty) {
    CORBA::ULong largest_index;
    if (!container_.get_largest_index_basic(largest_index)) {
      return false;
    }
    primitive_serialized_size_ulong(encoding, size);
    size += largest_index + 2; // Include null
  } else {
    // Use default value for string, i.e., empty string.
    primitive_serialized_size_ulong(encoding, size);
    size += 1; // For the null termination
  }
  return true;
}

bool DynamicDataImpl::serialize_string_value(DCPS::Serializer& ser) const
{
  char* str = 0;
  return read_basic_value(str) && (ser << str);
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

bool DynamicDataImpl::serialized_size_wstring(const DCPS::Encoding& encoding, size_t& size) const
{
  const bool is_empty = container_.single_map_.empty() && container_.complex_map_.empty();
  if (!is_empty) {
    CORBA::ULong largest_index;
    if (!container_.get_largest_index_basic(largest_index)) {
      return false;
    }
    primitive_serialized_size_ulong(encoding, size);
    size += (largest_index + 1) * DCPS::char16_cdr_size; // Not include null termination
  } else {
    // Only need length
    primitive_serialized_size_ulong(encoding, size);
  }
  return true;
}

bool DynamicDataImpl::serialize_wstring_value(DCPS::Serializer& ser) const
{
  CORBA::WChar* wstr = 0;
  return read_basic_value(wstr) && (ser << wstr);
}
#endif

void DynamicDataImpl::serialized_size_primitive_sequence(const DCPS::Encoding& encoding,
  size_t& size, TypeKind elem_tk, CORBA::ULong length) const
{
  switch (elem_tk) {
  case TK_INT32:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size(encoding, size, CORBA::Long(), length);
    return;
  case TK_UINT32:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size(encoding, size, CORBA::ULong(), length);
    return;
  case TK_INT8:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size_int8(encoding, size, length);
    return;
  case TK_UINT8:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size_uint8(encoding, size, length);
    return;
  case TK_INT16:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size(encoding, size, CORBA::Short(), length);
    return;
  case TK_UINT16:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size(encoding, size, CORBA::UShort(), length);
    return;
  case TK_INT64:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size(encoding, size, CORBA::LongLong(), length);
    return;
  case TK_UINT64:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size(encoding, size, CORBA::ULongLong(), length);
    return;
  case TK_FLOAT32:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size(encoding, size, CORBA::Float(), length);
    return;
  case TK_FLOAT64:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size(encoding, size, CORBA::Double(), length);
    return;
  case TK_FLOAT128:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size(encoding, size, CORBA::LongDouble(), length);
    return;
  case TK_CHAR8:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size_char(encoding, size, length);
    return;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size_wchar(encoding, size, length);
    return;
#endif
  case TK_BYTE:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size_octet(encoding, size, length);
    return;
  case TK_BOOLEAN:
    primitive_serialized_size_ulong(encoding, size);
    if (length == 0) {
      return;
    }
    primitive_serialized_size_boolean(encoding, size, length);
    return;
  }
}

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

// Set elements for a sequence of primitive type
template<>
bool DynamicDataImpl::set_primitive_values(DDS::BooleanSeq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_boolean& /*elem_tag*/) const
{
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_boolean>().val_;
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
      collection[index] = elem_it->second.get<ACE_OutputCDR::from_boolean>().val_;
    }
  }
  return true;
}

template<>
bool DynamicDataImpl::set_primitive_values(DDS::ByteSeq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_octet& /*elem_tag*/) const
{
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_octet>().val_;
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
      collection[index] = elem_it->second.get<ACE_OutputCDR::from_octet>().val_;
    }
  }
  return true;
}

template<>
bool DynamicDataImpl::set_primitive_values(DDS::Int8Seq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_int8& /*elem_tag*/) const
{
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_int8>().val_;
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
      collection[index] = elem_it->second.get<ACE_OutputCDR::from_int8>().val_;
    }
  }
  return true;
}

template<>
bool DynamicDataImpl::set_primitive_values(DDS::UInt8Seq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_uint8& /*elem_tag*/) const
{
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_uint8>().val_;
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
      collection[index] = elem_it->second.get<ACE_OutputCDR::from_uint8>().val_;
    }
  }
  return true;
}

template<>
bool DynamicDataImpl::set_primitive_values(DDS::CharSeq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_char& /*elem_tag*/) const
{
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_char>().val_;
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
      collection[index] = elem_it->second.get<ACE_OutputCDR::from_char>().val_;
    }
  }
  return true;
}

#ifdef DDS_HAS_WCHAR
template<>
bool DynamicDataImpl::set_primitive_values(DDS::WcharSeq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_wchar& /*elem_tag*/) const
{
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_wchar>().val_;
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
      collection[index] = elem_it->second.get<ACE_OutputCDR::from_wchar>().val_;
    }
  }
  return true;
}
#endif

template<typename ElementType, typename CollectionType>
bool DynamicDataImpl::set_primitive_values(CollectionType& collection,
  CORBA::ULong bound, const ElementType& /*elem_tag*/) const
{
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ElementType>();
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
      collection[index] = elem_it->second.get<ElementType>();
    }
  }
  return true;
}

// Helper function to reconstruct a sequence or array of primitive type.
// For array, @a size is equal to @a bound.
template<typename ElementType, typename CollectionType>
bool DynamicDataImpl::reconstruct_primitive_collection(CollectionType& collection,
  CORBA::ULong size, CORBA::ULong bound, const ElementType& elem_tag) const
{
  collection.length(size);
  set_default_primitive_values(collection);
  return set_primitive_values(collection, bound, elem_tag);
}

// Reconstruct the primitive sequence written by the user (elements that are not
// explicitly written are set to default value of the corresponding type).
// Then serialize the constructed sequence.
bool DynamicDataImpl::serialize_primitive_sequence(DCPS::Serializer& ser,
  TypeKind elem_tk, CORBA::ULong size, CORBA::ULong bound) const
{
  switch (elem_tk) {
  case TK_INT32: {
    DDS::Int32Seq int32seq;
    return reconstruct_primitive_collection(int32seq, size, bound, CORBA::Long()) &&
      (ser << int32seq);
  }
  case TK_UINT32: {
    DDS::UInt32Seq uint32seq;
    return reconstruct_primitive_collection(uint32seq, size, bound, CORBA::ULong()) &&
      (ser << uint32seq);
  }
  case TK_INT8: {
    DDS::Int8Seq int8seq;
    return reconstruct_primitive_collection(int8seq, size, bound, ACE_OutputCDR::from_int8(0)) &&
      (ser << int8seq);
  }
  case TK_UINT8: {
    DDS::UInt8Seq uint8seq;
    return reconstruct_primitive_collection(uint8seq, size, bound, ACE_OutputCDR::from_uint8(0)) &&
      (ser << uint8seq);
  }
  case TK_INT16: {
    DDS::Int16Seq int16seq;
    return reconstruct_primitive_collection(int16seq, size, bound, CORBA::Short()) &&
      (ser << int16seq);
  }
  case TK_UINT16: {
    DDS::UInt16Seq uint16seq;
    return reconstruct_primitive_collection(uint16seq, size, bound, CORBA::UShort()) &&
      (ser << uint16seq);
  }
  case TK_INT64: {
    DDS::Int64Seq int64seq;
    return reconstruct_primitive_collection(int64seq, size, bound, CORBA::LongLong()) &&
      (ser << int64seq);
  }
  case TK_UINT64: {
    DDS::UInt64Seq uint64seq;
    return reconstruct_primitive_collection(uint64seq, size, bound, CORBA::ULongLong()) &&
      (ser << uint64seq);
  }
  case TK_FLOAT32: {
    DDS::Float32Seq float32seq;
    return reconstruct_primitive_collection(float32seq, size, bound, CORBA::Float()) &&
      (ser << float32seq);
  }
  case TK_FLOAT64: {
    DDS::Float64Seq float64seq;
    return reconstruct_primitive_collection(float64seq, size, bound, CORBA::Double()) &&
      (ser << float64seq);
  }
  case TK_FLOAT128: {
    DDS::Float128Seq float128seq;
    return reconstruct_primitive_collection(float128seq, size, bound, CORBA::LongDouble()) &&
      (ser << float128seq);
  }
  case TK_CHAR8: {
    DDS::CharSeq charseq;
    return reconstruct_primitive_collection(charseq, size, bound, ACE_OutputCDR::from_char('\0')) &&
      (ser << charseq);
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    DDS::WcharSeq wcharseq;
    return reconstruct_primitive_collection(wcharseq, size, bound, ACE_OutputCDR::from_wchar(0)) &&
      (ser << wcharseq);
  }
#endif
  case TK_BYTE: {
    DDS::ByteSeq byteseq;
    return reconstruct_primitive_collection(byteseq, size, bound, ACE_OutputCDR::from_octet(0x00)) &&
      (ser << byteseq);
  }
  case TK_BOOLEAN: {
    DDS::BooleanSeq boolseq;
    return reconstruct_primitive_collection(boolseq, size, bound, ACE_OutputCDR::from_boolean(false)) &&
      (ser << boolseq);
  }
  }
  return false;
}

// Unlike primitive types, string and wstring are not as easy to reconstruct since each
// element string may need to be reconstructed individually. Instead, we serialize
// the element strings one by one directly.
void DynamicDataImpl::serialized_size_string_common(const DCPS::Encoding& encoding,
  size_t& size, const char* str) const
{
  primitive_serialized_size_ulong(encoding, size);
  if (str) {
    size += ACE_OS::strlen(str) + 1; // Include null termination
  }
}

#ifdef DDS_HAS_WCHAR
void DynamicDataImpl::serialized_size_string_common(const DCPS::Encoding& encoding,
  size_t& size, const CORBA::WChar* wstr) const
{
  primitive_serialized_size_ulong(encoding, size);
  if (wstr) {
    size += ACE_OS::strlen(wstr) * DCPS::char16_cdr_size; // Not include null termination
  }
}
#endif

void DynamicDataImpl::serialized_size_string_common(const DCPS::Encoding& encoding,
  size_t& size, const SingleValue& sv) const
{
  if (sv.kind_ == TK_STRING8) {
    serialized_size_string_common(encoding, size, sv.str_);
  }
#ifdef DDS_HAS_WCHAR
  else if (sv.kind_ == TK_STRING16) {
    serialized_size_string_common(encoding, size, sv.wstr_);
  }
#endif
}

template<typename StringType>
bool DynamicDataImpl::serialized_size_generic_string_collection(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const DDS::MemberId id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_single_iterator single_it = container_.single_map_.find(id);
      if (single_it != container_.single_map_.end()) {
        serialized_size_string_common(encoding, size, single_it->second);
      } else if (!serialized_size_complex_member_i(encoding, size, id, DCPS::Sample::Full)) {
        return false;
      }
    } else {
      StringType default_value;
      set_default_basic_value(default_value);
      serialized_size_string_common(encoding, size, default_value);
    }
  }
  return true;
}

template<typename StringType>
bool DynamicDataImpl::serialized_size_generic_string_sequence(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (index_to_id.empty()) {
    return true;
  }
  return serialized_size_generic_string_collection<StringType>(encoding, size, index_to_id);
}

// Serialize the individual elements from a sequence or an array of string (or wstring).
template<typename StringType>
bool DynamicDataImpl::serialize_generic_string_collection(DCPS::Serializer& ser,
  const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const DDS::MemberId id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_single_iterator single_it = container_.single_map_.find(id);
      if (single_it != container_.single_map_.end()) {
        if (!serialize_single_value(ser, single_it->second)) {
          return false;
        }
      } else if (!serialize_complex_member_i(ser, id, DCPS::Sample::Full)) {
        return false;
      }
    } else { // Not set by the user. Use default value.
      StringType default_value;
      set_default_basic_value(default_value);
      if (!(ser << default_value)) {
        return false;
      }
    }
  }
  return true;
}

template<typename StringType>
bool DynamicDataImpl::serialize_generic_string_sequence(DCPS::Serializer& ser,
  CORBA::ULong length, CORBA::ULong bound) const
{
  IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, bound)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    size_t total_size = 0;
    if (!serialized_size_generic_string_sequence<StringType>(encoding, total_size, index_to_id) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }
  if (!(ser << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return serialize_generic_string_collection<StringType>(ser, index_to_id);
}

template<typename ElementType, typename CollectionType>
bool DynamicDataImpl::set_default_enum_values(CollectionType& collection,
                                              const DDS::DynamicType_var& enum_type) const
{
  CORBA::Long value;
  if (!set_default_enum_value(enum_type, value)) {
    return false;
  }
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    collection[i] = static_cast<ElementType>(value);
  }
  return true;
}

template<typename ElementType, typename WrapElementType, typename CollectionType>
bool DynamicDataImpl::reconstruct_enum_collection(CollectionType& collection,
  CORBA::ULong size, CORBA::ULong bound, const DDS::DynamicType_var& enum_type,
  const WrapElementType& elem_tag) const
{
  collection.length(size);
  if (!set_default_enum_values<ElementType>(collection, enum_type)) {
    return false;
  }
  return set_primitive_values(collection, bound, elem_tag);
}

// Serialize enum sequence represented as int8 sequence
void DynamicDataImpl::serialized_size_enum_sequence_as_int8s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size_int8(encoding, size, length);
}

void DynamicDataImpl::serialized_size_enum_sequence(
  const DCPS::Encoding& encoding, size_t& size, const DDS::Int8Seq& seq) const
{
  serialized_size_enum_sequence_as_int8s(encoding, size, seq.length());
}

bool DynamicDataImpl::serialize_enum_sequence_as_ints_i(DCPS::Serializer& ser,
  const DDS::Int8Seq& enumseq) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_enum_sequence_as_int8s(encoding, total_size, enumseq.length());
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

bool DynamicDataImpl::serialize_enum_sequence_as_int8s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int8Seq enumseq;
  return reconstruct_enum_collection<CORBA::Int8>(enumseq, size, bound, enum_type, ACE_OutputCDR::from_int8(0)) &&
    serialize_enum_sequence_as_ints_i(ser, enumseq);
}

// Serialize enum sequence represented as int16 sequence
void DynamicDataImpl::serialized_size_enum_sequence_as_int16s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::Short(), length);
}

void DynamicDataImpl::serialized_size_enum_sequence(
  const DCPS::Encoding& encoding, size_t& size, const DDS::Int16Seq& seq) const
{
  serialized_size_enum_sequence_as_int16s(encoding, size, seq.length());
}

bool DynamicDataImpl::serialize_enum_sequence_as_ints_i(DCPS::Serializer& ser,
  const DDS::Int16Seq& enumseq) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_enum_sequence_as_int16s(encoding, total_size, enumseq.length());
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

bool DynamicDataImpl::serialize_enum_sequence_as_int16s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int16Seq enumseq;
  return reconstruct_enum_collection<CORBA::Short>(enumseq, size, bound, enum_type, CORBA::Short()) &&
    serialize_enum_sequence_as_ints_i(ser, enumseq);
}

// Serialize enum sequence represented as int32 sequence
void DynamicDataImpl::serialized_size_enum_sequence_as_int32s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::Long(), length);
}

void DynamicDataImpl::serialized_size_enum_sequence(
  const DCPS::Encoding& encoding, size_t& size, const DDS::Int32Seq& seq) const
{
  serialized_size_enum_sequence_as_int32s(encoding, size, seq.length());
}

bool DynamicDataImpl::serialize_enum_sequence_as_ints_i(DCPS::Serializer& ser,
  const DDS::Int32Seq& enumseq) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_enum_sequence_as_int32s(encoding, total_size, enumseq.length());
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

bool DynamicDataImpl::serialize_enum_sequence_as_int32s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int32Seq enumseq;
  return reconstruct_enum_collection<CORBA::Long>(enumseq, size, bound, enum_type, CORBA::Long()) &&
    serialize_enum_sequence_as_ints_i(ser, enumseq);
}

void DynamicDataImpl::serialized_size_enum_sequence(const DCPS::Encoding& encoding,
  size_t& size, CORBA::ULong length, CORBA::ULong bitbound) const
{
  if (bitbound >= 1 && bitbound <= 8) {
    serialized_size_enum_sequence_as_int8s(encoding, size, length);
  } else if (bitbound >= 9 && bitbound <= 16) {
    serialized_size_enum_sequence_as_int16s(encoding, size, length);
  } else { // From 17 to 32
    serialized_size_enum_sequence_as_int32s(encoding, size, length);
  }
}

bool DynamicDataImpl::serialize_enum_sequence(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bitbound, CORBA::ULong seqbound,
  const DDS::DynamicType_var& enum_type) const
{
  if (bitbound >= 1 && bitbound <= 8) {
    return serialize_enum_sequence_as_int8s(ser, size, seqbound, enum_type);
  } else if (bitbound >= 9 && bitbound <= 16) {
    return serialize_enum_sequence_as_int16s(ser, size, seqbound, enum_type);
  } else if (bitbound >= 17 && bitbound <= 32) {
    return serialize_enum_sequence_as_int32s(ser, size, seqbound, enum_type);
  }
  return false;
}

template<typename CollectionType>
void DynamicDataImpl::set_default_bitmask_values(CollectionType& col) const
{
  // Table 9 doesn't mention default value for bitmask. Use 0 as default here.
  for (CORBA::ULong i = 0; i < col.length(); ++i) {
    col[i] = 0;
  }
}

template<typename WrapElementType, typename CollectionType>
bool DynamicDataImpl::reconstruct_bitmask_collection(CollectionType& collection,
  CORBA::ULong size, CORBA::ULong bound, const WrapElementType& elem_tag) const
{
  collection.length(size);
  set_default_bitmask_values(collection);
  return set_primitive_values(collection, bound, elem_tag);
}

// Bitmask sequence represented as uint8 sequence.
void DynamicDataImpl::serialized_size_bitmask_sequence_as_uint8s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size_uint8(encoding, size, length);
}

void DynamicDataImpl::serialized_size_bitmask_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::UInt8Seq& seq) const
{
  serialized_size_bitmask_sequence_as_uint8s(encoding, size, seq.length());
}

bool DynamicDataImpl::serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
  const DDS::UInt8Seq& bitmask_seq) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_bitmask_sequence(encoding, total_size, bitmask_seq);
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  const CORBA::ULong length = bitmask_seq.length();
  if (!(ser << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return ser.write_uint8_array(bitmask_seq.get_buffer(), length);
}

bool DynamicDataImpl::serialize_bitmask_sequence_as_uint8s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::UInt8Seq bitmask_seq;
  return reconstruct_bitmask_collection(bitmask_seq, size, bound, ACE_OutputCDR::from_uint8(0)) &&
    serialize_bitmask_sequence_as_uints_i(ser, bitmask_seq);
}

// Bitmask sequence represented as uint16 sequence
void DynamicDataImpl::serialized_size_bitmask_sequence_as_uint16s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::UShort(), length);
}

void DynamicDataImpl::serialized_size_bitmask_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::UInt16Seq& seq) const
{
  serialized_size_bitmask_sequence_as_uint16s(encoding, size, seq.length());
}

bool DynamicDataImpl::serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
  const DDS::UInt16Seq& bitmask_seq) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_bitmask_sequence(encoding, total_size, bitmask_seq);
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  const CORBA::ULong length = bitmask_seq.length();
  if (!(ser << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return ser.write_ushort_array(bitmask_seq.get_buffer(), length);
}

bool DynamicDataImpl::serialize_bitmask_sequence_as_uint16s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::UInt16Seq bitmask_seq;
  return reconstruct_bitmask_collection(bitmask_seq, size, bound, CORBA::UShort()) &&
    serialize_bitmask_sequence_as_uints_i(ser, bitmask_seq);
}

// Bitmask sequence represented as uint32 sequence
void DynamicDataImpl::serialized_size_bitmask_sequence_as_uint32s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::ULong(), length);
}

void DynamicDataImpl::serialized_size_bitmask_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::UInt32Seq& seq) const
{
  serialized_size_bitmask_sequence_as_uint32s(encoding, size, seq.length());
}

bool DynamicDataImpl::serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
  const DDS::UInt32Seq& bitmask_seq) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_bitmask_sequence(encoding, total_size, bitmask_seq);
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  const CORBA::ULong length = bitmask_seq.length();
  if (!(ser << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return ser.write_ulong_array(bitmask_seq.get_buffer(), length);
}

bool DynamicDataImpl::serialize_bitmask_sequence_as_uint32s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::UInt32Seq bitmask_seq;
  return reconstruct_bitmask_collection(bitmask_seq, size, bound, CORBA::ULong()) &&
    serialize_bitmask_sequence_as_uints_i(ser, bitmask_seq);
}

// Bitmask sequence represented as uint64 sequence
void DynamicDataImpl::serialized_size_bitmask_sequence_as_uint64s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::ULongLong(), length);
}

void DynamicDataImpl::serialized_size_bitmask_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::UInt64Seq& seq) const
{
  serialized_size_bitmask_sequence_as_uint64s(encoding, size, seq.length());
}

bool DynamicDataImpl::serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
  const DDS::UInt64Seq& bitmask_seq) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_bitmask_sequence(encoding, total_size, bitmask_seq);
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  const CORBA::ULong length = bitmask_seq.length();
  if (!(ser << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return ser.write_ulonglong_array(bitmask_seq.get_buffer(), length);
}

bool DynamicDataImpl::serialize_bitmask_sequence_as_uint64s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::UInt64Seq bitmask_seq;
  return reconstruct_bitmask_collection(bitmask_seq, size, bound, CORBA::ULongLong()) &&
    serialize_bitmask_sequence_as_uints_i(ser, bitmask_seq);
}

void DynamicDataImpl::serialized_size_bitmask_sequence(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length, CORBA::ULong bitbound) const
{
  if (bitbound >= 1 && bitbound <= 8) {
    serialized_size_bitmask_sequence_as_uint8s(encoding, size, length);
  } else if (bitbound >= 9 && bitbound <= 16) {
    serialized_size_bitmask_sequence_as_uint16s(encoding, size, length);
  } else if (bitbound >= 17 && bitbound <= 32) {
    serialized_size_bitmask_sequence_as_uint32s(encoding, size, length);
  } else { // from 33 to 64
    serialized_size_bitmask_sequence_as_uint64s(encoding, size, length);
  }
}

bool DynamicDataImpl::serialize_bitmask_sequence(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bitbound, CORBA::ULong seqbound) const
{
  if (bitbound >= 1 && bitbound <= 8) {
    return serialize_bitmask_sequence_as_uint8s(ser, size, seqbound);
  } else if (bitbound >= 9 && bitbound <= 16) {
    return serialize_bitmask_sequence_as_uint16s(ser, size, seqbound);
  } else if (bitbound >= 17 && bitbound <= 32) {
    return serialize_bitmask_sequence_as_uint32s(ser, size, seqbound);
  } else if (bitbound >= 33 && bitbound <= 64) {
    return serialize_bitmask_sequence_as_uint64s(ser, size, seqbound);
  }
  return false;
}

// Serialize a SequenceValue object
bool DynamicDataImpl::serialized_size_sequence_value(
  const DCPS::Encoding& encoding, size_t& size, const SequenceValue& sv) const
{
  switch (sv.elem_kind_) {
  case TK_INT32:
    DCPS::serialized_size(encoding, size, sv.get<DDS::Int32Seq>());
    return true;
  case TK_UINT32:
    DCPS::serialized_size(encoding, size, sv.get<DDS::UInt32Seq>());
    return true;
  case TK_INT8:
    DCPS::serialized_size(encoding, size, sv.get<DDS::Int8Seq>());
    return true;
  case TK_UINT8:
    DCPS::serialized_size(encoding, size, sv.get<DDS::UInt8Seq>());
    return true;
  case TK_INT16:
    DCPS::serialized_size(encoding, size, sv.get<DDS::Int16Seq>());
    return true;
  case TK_UINT16:
    DCPS::serialized_size(encoding, size, sv.get<DDS::UInt16Seq>());
    return true;
  case TK_INT64:
    DCPS::serialized_size(encoding, size, sv.get<DDS::Int64Seq>());
    return true;
  case TK_UINT64:
    DCPS::serialized_size(encoding, size, sv.get<DDS::UInt64Seq>());
    return true;
  case TK_FLOAT32:
    DCPS::serialized_size(encoding, size, sv.get<DDS::Float32Seq>());
    return true;
  case TK_FLOAT64:
    DCPS::serialized_size(encoding, size, sv.get<DDS::Float64Seq>());
    return true;
  case TK_FLOAT128:
    DCPS::serialized_size(encoding, size, sv.get<DDS::Float128Seq>());
    return true;
  case TK_CHAR8:
    DCPS::serialized_size(encoding, size, sv.get<DDS::CharSeq>());
    return true;
  case TK_BYTE:
    DCPS::serialized_size(encoding, size, sv.get<DDS::ByteSeq>());
    return true;
  case TK_BOOLEAN:
    DCPS::serialized_size(encoding, size, sv.get<DDS::BooleanSeq>());
    return true;
  case TK_STRING8:
    DCPS::serialized_size(encoding, size, sv.get<DDS::StringSeq>());
    return true;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    DCPS::serialized_size(encoding, size, sv.get<DDS::WcharSeq>());
    return true;
  case TK_STRING16:
    DCPS::serialized_size(encoding, size, sv.get<DDS::WstringSeq>());
    return true;
#endif
  default:
    return false;
  }
}

bool DynamicDataImpl::serialize_sequence_value(DCPS::Serializer& ser,
                                               const SequenceValue& sv) const
{
  switch (sv.elem_kind_) {
  case TK_INT32:
    return ser << sv.get<DDS::Int32Seq>();
  case TK_UINT32:
    return ser << sv.get<DDS::UInt32Seq>();
  case TK_INT8:
    return ser << sv.get<DDS::Int8Seq>();
  case TK_UINT8:
    return ser << sv.get<DDS::UInt8Seq>();
  case TK_INT16:
    return ser << sv.get<DDS::Int16Seq>();
  case TK_UINT16:
    return ser << sv.get<DDS::UInt16Seq>();
  case TK_INT64:
    return ser << sv.get<DDS::Int64Seq>();
  case TK_UINT64:
    return ser << sv.get<DDS::UInt64Seq>();
  case TK_FLOAT32:
    return ser << sv.get<DDS::Float32Seq>();
  case TK_FLOAT64:
    return ser << sv.get<DDS::Float64Seq>();
  case TK_FLOAT128:
    return ser << sv.get<DDS::Float128Seq>();
  case TK_CHAR8:
    return ser << sv.get<DDS::CharSeq>();
  case TK_BYTE:
    return ser << sv.get<DDS::ByteSeq>();
  case TK_BOOLEAN:
    return ser << sv.get<DDS::BooleanSeq>();
  case TK_STRING8:
    return ser << sv.get<DDS::StringSeq>();
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return ser << sv.get<DDS::WcharSeq>();
  case TK_STRING16:
    return ser << sv.get<DDS::WstringSeq>();
#endif
  default:
    return false;
  }
}

// Helper function for serializing sequences and arrays
bool DynamicDataImpl::get_index_to_id_map(IndexToIdMap& index_to_id,
                                          CORBA::ULong bound) const
{
  for (const_single_iterator it = container_.single_map_.begin(); it != container_.single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    index_to_id[index] = it->first;
  }
  for (const_sequence_iterator it = container_.sequence_map_.begin(); it != container_.sequence_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    index_to_id[index] = it->first;
  }
  for (const_complex_iterator it = container_.complex_map_.begin(); it != container_.complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    index_to_id[index] = it->first;
  }
  return true;
}

bool DynamicDataImpl::serialized_size_complex_member_i(
  const DCPS::Encoding& encoding, size_t& size, DDS::MemberId id, DCPS::Sample::Extent ext) const
{
  const DDS::DynamicData_var& dd_var = container_.complex_map_.at(id);
  const DynamicDataImpl* data_impl = dynamic_cast<const DynamicDataImpl*>(dd_var.in());
  if (!data_impl) {
    return false;
  }
  return data_impl->serialized_size_i(encoding, size, ext);
}

template<typename SequenceType>
bool DynamicDataImpl::serialized_size_nested_basic_sequences(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id,
  SequenceType protoseq) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = container_.sequence_map_.find(id);
      if (it != container_.sequence_map_.end()) {
        serialized_size_sequence_value(encoding, size, it->second);
      } else if (!serialized_size_complex_member_i(encoding, size, id, DCPS::Sample::Full)) {
        return false;
      }
    } else { // Empty sequence
      protoseq.length(0);
      DCPS::serialized_size(encoding, size, protoseq);
    }
  }
  return true;
}

template<typename SequenceType>
bool DynamicDataImpl::serialized_size_nesting_basic_sequence(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id,
  SequenceType protoseq) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (index_to_id.empty()) {
    return true;
  }
  return serialized_size_nested_basic_sequences(encoding, size, index_to_id, protoseq);
}

bool DynamicDataImpl::serialize_complex_member_i(DCPS::Serializer& ser,
  DDS::MemberId id, DCPS::Sample::Extent ext) const
{
  const DDS::DynamicData_var& dd_var = container_.complex_map_.at(id);
  const DynamicDataImpl* data_impl = dynamic_cast<const DynamicDataImpl*>(dd_var.in());
  if (!data_impl) {
    return false;
  }
  return data_impl->serialize_i(ser, ext);
}

template<typename SequenceType>
bool DynamicDataImpl::serialize_nested_basic_sequences(DCPS::Serializer& ser,
  const IndexToIdMap& index_to_id, SequenceType protoseq) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = container_.sequence_map_.find(id);
      if (it != container_.sequence_map_.end()) {
        if (!serialize_sequence_value(ser, it->second)) {
          return false;
        }
      } else if (!serialize_complex_member_i(ser, id, DCPS::Sample::Full)) {
        return false;
      }
    } else {
      // Table 9: Use zero-length sequence of the same element type.
      protoseq.length(0);
      if (!(ser << protoseq)) {
        return false;
      }
    }
  }
  return true;
}

template<typename SequenceType>
bool DynamicDataImpl::serialize_nesting_basic_sequence_i(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound, SequenceType protoseq) const
{
  // Map from index to ID. Use MEMBER_ID_INVALID to indicate there is
  // no data for an element at a given index.
  IndexToIdMap index_to_id(size, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, bound)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_nesting_basic_sequence(encoding, total_size, index_to_id, protoseq) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  // Serialize the top-level sequence's length
  if (!(ser << size)) {
    return false;
  }
  if (size == 0) {
    return true;
  }
  return serialize_nested_basic_sequences(ser, index_to_id, protoseq);
}

bool DynamicDataImpl::serialized_size_nesting_basic_sequence(
  const DCPS::Encoding& encoding, size_t& size, TypeKind nested_elem_tk,
  const IndexToIdMap& index_to_id) const
{
  switch (nested_elem_tk) {
  case TK_INT32:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::Int32Seq());
  case TK_UINT32:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::UInt32Seq());
  case TK_INT8:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::Int8Seq());
  case TK_UINT8:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::UInt8Seq());
  case TK_INT16:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::Int16Seq());
  case TK_UINT16:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::UInt16Seq());
  case TK_INT64:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::Int64Seq());
  case TK_UINT64:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::UInt64Seq());
  case TK_FLOAT32:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::Float32Seq());
  case TK_FLOAT64:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::Float64Seq());
  case TK_FLOAT128:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::Float128Seq());
  case TK_CHAR8:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::CharSeq());
  case TK_STRING8:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::StringSeq());
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::WcharSeq());
  case TK_STRING16:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::WstringSeq());
#endif
  case TK_BYTE:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::ByteSeq());
  case TK_BOOLEAN:
    return serialized_size_nesting_basic_sequence(encoding, size, index_to_id, DDS::BooleanSeq());
  }
  return false;
}

bool DynamicDataImpl::serialize_nesting_basic_sequence(DCPS::Serializer& ser,
  TypeKind nested_elem_tk, CORBA::ULong size, CORBA::ULong bound) const
{
  switch (nested_elem_tk) {
  case TK_INT32:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::Int32Seq());
  case TK_UINT32:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::UInt32Seq());
  case TK_INT8:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::Int8Seq());
  case TK_UINT8:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::UInt8Seq());
  case TK_INT16:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::Int16Seq());
  case TK_UINT16:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::UInt16Seq());
  case TK_INT64:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::Int64Seq());
  case TK_UINT64:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::UInt64Seq());
  case TK_FLOAT32:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::Float32Seq());
  case TK_FLOAT64:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::Float64Seq());
  case TK_FLOAT128:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::Float128Seq());
  case TK_CHAR8:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::CharSeq());
  case TK_STRING8:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::StringSeq());
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::WcharSeq());
  case TK_STRING16:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::WstringSeq());
#endif
  case TK_BYTE:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::ByteSeq());
  case TK_BOOLEAN:
    return serialize_nesting_basic_sequence_i(ser, size, bound, DDS::BooleanSeq());
  }
  return false;
}

bool DynamicDataImpl::serialized_size_nested_enum_sequences(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = container_.sequence_map_.find(id);
      if (it != container_.sequence_map_.end()) {
        serialized_size_enum_sequence(encoding, size, it);
      } else if (!serialized_size_complex_member_i(encoding, size, id, DCPS::Sample::Full)) {
        return false;
      }
    } else {
      serialized_size_delimiter(encoding, size);
      primitive_serialized_size_ulong(encoding, size);
    }
  }
  return true;
}

bool DynamicDataImpl::serialized_size_nesting_enum_sequence(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (index_to_id.empty()) {
    return true;
  }
  return serialized_size_nested_enum_sequences(encoding, size, index_to_id);
}

bool DynamicDataImpl::serialize_nested_enum_sequences(
  DCPS::Serializer& ser, const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = container_.sequence_map_.find(id);
      if (it != container_.sequence_map_.end()) {
        if (!serialize_enum_sequence(ser, it)) {
          return false;
        }
      } else if (!serialize_complex_member_i(ser, id, DCPS::Sample::Full)) {
        return false;
      }
    } else { // Empty sequence of enums
      if (ser.encoding().xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
        if (!ser.write_delimiter(2 * DCPS::uint32_cdr_size)) {
          return false;
        }
      }
      if (!(ser << static_cast<CORBA::ULong>(0))) {
        return false;
      }
    }
  }
  return true;
}

bool DynamicDataImpl::serialize_nesting_enum_sequence(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  IndexToIdMap index_to_id(size, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, bound)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_nesting_enum_sequence(encoding, total_size, index_to_id) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  if (!(ser << size)) {
    return false;
  }
  if (size == 0) {
    return true;
  }
  return serialize_nested_enum_sequences(ser, index_to_id);
}

bool DynamicDataImpl::serialized_size_nested_bitmask_sequences(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = container_.sequence_map_.find(id);
      if (it != container_.sequence_map_.end()) {
        serialized_size_bitmask_sequence(encoding, size, it);
      } else if (!serialized_size_complex_member_i(encoding, size, id, DCPS::Sample::Full)) {
        return false;
      }
    } else {
      serialized_size_delimiter(encoding, size);
      primitive_serialized_size_ulong(encoding, size);
    }
  }
  return true;
}

bool DynamicDataImpl::serialized_size_nesting_bitmask_sequence(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (index_to_id.empty()) {
    return true;
  }
  return serialized_size_nested_bitmask_sequences(encoding, size, index_to_id);
}

bool DynamicDataImpl::serialize_nested_bitmask_sequences(DCPS::Serializer& ser,
  const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = container_.sequence_map_.find(id);
      if (it != container_.sequence_map_.end()) {
        if (!serialize_bitmask_sequence(ser, it)) {
          return false;
        }
      } else if (!serialize_complex_member_i(ser, id, DCPS::Sample::Full)) {
        return false;
      }
    } else { // Empty sequence of bitmasks
      if (ser.encoding().xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
        if (!ser.write_delimiter(2 * DCPS::uint32_cdr_size)) {
          return false;
        }
      }
      if (!(ser << static_cast<CORBA::ULong>(0))) {
        return false;
      }
    }
  }
  return true;
}

bool DynamicDataImpl::serialize_nesting_bitmask_sequence(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  IndexToIdMap index_to_id(size, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, bound)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_nesting_bitmask_sequence(encoding, total_size, index_to_id) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  if (!(ser << size)) {
    return false;
  }
  if (size == 0) {
    return true;
  }
  return serialize_nested_bitmask_sequences(ser, index_to_id);
}

bool DynamicDataImpl::serialized_size_complex_member(const DCPS::Encoding& encoding,
  size_t& size, DDS::MemberId id, const DDS::DynamicType_var& elem_type, DCPS::Sample::Extent ext) const
{
  if (id != MEMBER_ID_INVALID) {
    return serialized_size_complex_member_i(encoding, size, id, ext);
  } else {
    return DynamicDataImpl(elem_type).serialized_size_i(encoding, size, ext);
  }
}

bool DynamicDataImpl::serialized_size_complex_sequence(const DCPS::Encoding& encoding,
  size_t& size, const IndexToIdMap& index_to_id,
  const DDS::DynamicType_var& elem_type, DCPS::Sample::Extent ext) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (index_to_id.empty()) {
    return true;
  }
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    if (!serialized_size_complex_member(encoding, size, index_to_id[i], elem_type, ext)) {
      return false;
    }
  }
  return true;
}

bool DynamicDataImpl::serialize_complex_sequence_i(DCPS::Serializer& ser,
  const IndexToIdMap& index_to_id, const DDS::DynamicType_var& elem_type, DCPS::Sample::Extent ext) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      if (!serialize_complex_member_i(ser, id, ext)) {
        return false;
      }
    } else {
      if (!DynamicDataImpl(elem_type).serialize_i(ser, ext)) {
        return false;
      }
    }
  }
  return true;
}

bool DynamicDataImpl::serialize_complex_sequence(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound,
  const DDS::DynamicType_var& elem_type, DCPS::Sample::Extent ext) const
{
  IndexToIdMap index_to_id(size, MEMBER_ID_INVALID);
  for (const_complex_iterator it = container_.complex_map_.begin(); it != container_.complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    index_to_id[index] = it->first;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_complex_sequence(encoding, total_size, index_to_id, elem_type, ext) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  if (!(ser << size)) {
    return false;
  }
  if (size == 0) {
    return true;
  }
  return serialize_complex_sequence_i(ser, index_to_id, elem_type, ext);
}

bool DynamicDataImpl::get_index_to_id_from_complex(IndexToIdMap& index_to_id,
                                                   CORBA::ULong bound) const
{
  CORBA::ULong length = 0;
  if (!container_.complex_map_.empty()) {
    CORBA::ULong largest_index;
    if (!container_.get_largest_complex_index(largest_index)) {
      return false;
    }
    length = largest_index + 1;
  }
  index_to_id.resize(length, MEMBER_ID_INVALID);
  for (const_complex_iterator it = container_.complex_map_.begin(); it != container_.complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, bound)) {
      return false;
    }
    index_to_id[index] = it->first;
  }
  return true;
}

bool DynamicDataImpl::serialized_size_sequence(const DCPS::Encoding& encoding,
                                               size_t& size, DCPS::Sample::Extent ext) const
{
  const CORBA::ULong bound = type_desc_->bound()[0];

  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }

  if (is_basic(elem_tk) || elem_tk == TK_ENUM || elem_tk == TK_BITMASK) {
    const bool is_empty = container_.single_map_.empty() && container_.complex_map_.empty();
    CORBA::ULong length = 0;
    if (!is_empty) {
      CORBA::ULong largest_index;
      if (!container_.get_largest_index_basic(largest_index)) {
        return false;
      }
      length = largest_index + 1;
    }
    if (is_primitive(elem_tk)) {
      serialized_size_primitive_sequence(encoding, size, elem_tk, length);
      return true;
    } else if (elem_tk == TK_STRING8) {
      IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
      if (!get_index_to_id_map(index_to_id, bound)) {
        return false;
      }
      return serialized_size_generic_string_sequence<const char*>(encoding, size, index_to_id);
    } else if (elem_tk == TK_STRING16) {
#ifdef DDS_HAS_WCHAR
      IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
      if (!get_index_to_id_map(index_to_id, bound)) {
        return false;
      }
      return serialized_size_generic_string_sequence<const CORBA::WChar*>(encoding, size, index_to_id);
#else
      return false;
#endif
    } else if (elem_tk == TK_ENUM) {
      const CORBA::ULong bit_bound = elem_td->bound()[0];
      serialized_size_enum_sequence(encoding, size, length, bit_bound);
      return true;
    } else if (elem_tk == TK_BITMASK) {
      const CORBA::ULong bit_bound = elem_td->bound()[0];
      serialized_size_bitmask_sequence(encoding, size, length, bit_bound);
    }
  } else if (elem_tk == TK_SEQUENCE) {
    const DDS::DynamicType_var nested_elem_type = get_base_type(elem_td->element_type());
    const TypeKind nested_elem_tk = nested_elem_type->get_kind();
    if (is_basic(nested_elem_tk) || nested_elem_tk == TK_ENUM ||
        nested_elem_tk == TK_BITMASK) {
      const bool is_empty = container_.sequence_map_.empty() && container_.complex_map_.empty();
      CORBA::ULong length = 0;
      if (!is_empty) {
        CORBA::ULong largest_index;
        if (!container_.get_largest_index_basic_sequence(largest_index)) {
          return false;
        }
        length = largest_index + 1;
      }
      IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
      if (!get_index_to_id_map(index_to_id, bound)) {
        return false;
      }
      if (is_basic(nested_elem_tk)) {
        return serialized_size_nesting_basic_sequence(encoding, size, nested_elem_tk, index_to_id);
      } else if (nested_elem_tk == TK_ENUM) {
        return serialized_size_nesting_enum_sequence(encoding, size, index_to_id);
      } else {
        return serialized_size_nesting_bitmask_sequence(encoding, size, index_to_id);
      }
    }
  }

  // Elements stored in complex map
  IndexToIdMap index_to_id;
  if (!get_index_to_id_from_complex(index_to_id, bound)) {
    return false;
  }
  return serialized_size_complex_sequence(encoding, size, index_to_id, elem_type, ext);
}

bool DynamicDataImpl::serialize_sequence(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const
{
  const CORBA::ULong bound = type_desc_->bound()[0];

  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }

  if (is_basic(elem_tk) || elem_tk == TK_ENUM || elem_tk == TK_BITMASK) {
    const bool is_empty = container_.single_map_.empty() && container_.complex_map_.empty();
    CORBA::ULong length = 0;
    if (!is_empty) {
      CORBA::ULong largest_index;
      if (!container_.get_largest_index_basic(largest_index)) {
        return false;
      }
      length = largest_index + 1;
    }
    if (is_primitive(elem_tk)) {
      return serialize_primitive_sequence(ser, elem_tk, length, bound);
    } else if (elem_tk == TK_STRING8) {
      return serialize_generic_string_sequence<const char*>(ser, length, bound);
    } else if (elem_tk == TK_STRING16) {
#ifdef DDS_HAS_WCHAR
      return serialize_generic_string_sequence<const CORBA::WChar*>(ser, length, bound);
#else
      return false;
#endif
    } else if (elem_tk == TK_ENUM) {
      const CORBA::ULong bit_bound = elem_td->bound()[0];
      return serialize_enum_sequence(ser, length, bit_bound, bound, elem_type);
    } else {
      const CORBA::ULong bit_bound = elem_td->bound()[0];
      return serialize_bitmask_sequence(ser, length, bit_bound, bound);
    }
  } else if (elem_tk == TK_SEQUENCE) {
    const DDS::DynamicType_var nested_elem_type = get_base_type(elem_td->element_type());
    const TypeKind nested_elem_tk = nested_elem_type->get_kind();
    if (is_basic(nested_elem_tk) || nested_elem_tk == TK_ENUM ||
        nested_elem_tk == TK_BITMASK) {
      const bool is_empty = container_.sequence_map_.empty() && container_.complex_map_.empty();
      CORBA::ULong length = 0;
      if (!is_empty) {
        CORBA::ULong largest_index;
        if (!container_.get_largest_index_basic_sequence(largest_index)) {
          return false;
        }
        length = largest_index + 1;
      }
      if (is_basic(nested_elem_tk)) {
        return serialize_nesting_basic_sequence(ser, nested_elem_tk, length, bound);
      } else if (nested_elem_tk == TK_ENUM) {
        return serialize_nesting_enum_sequence(ser, length, bound);
      } else {
        return serialize_nesting_bitmask_sequence(ser, length, bound);
      }
    }
  }

  // Elements with all the other types are stored in the complex map.
  CORBA::ULong length = 0;
  if (!container_.complex_map_.empty()) {
    CORBA::ULong largest_index;
    if (!container_.get_largest_complex_index(largest_index)) {
      return false;
    }
    length = largest_index + 1;
  }
  return serialize_complex_sequence(ser, length, bound, elem_type, ext);
}

void DynamicDataImpl::serialized_size_primitive_array(const DCPS::Encoding& encoding,
  size_t& size, TypeKind elem_tk, CORBA::ULong length) const
{
  switch (elem_tk) {
  case TK_INT32:
    primitive_serialized_size(encoding, size, CORBA::Long(), length);
    return;
  case TK_UINT32:
    primitive_serialized_size(encoding, size, CORBA::ULong(), length);
    return;
  case TK_INT8:
    primitive_serialized_size_int8(encoding, size, length);
    return;
  case TK_UINT8:
    primitive_serialized_size_uint8(encoding, size, length);
    return;
  case TK_INT16:
    primitive_serialized_size(encoding, size, CORBA::Short(), length);
    return;
  case TK_UINT16:
    primitive_serialized_size(encoding, size, CORBA::UShort(), length);
    return;
  case TK_INT64:
    primitive_serialized_size(encoding, size, CORBA::LongLong(), length);
    return;
  case TK_UINT64:
    primitive_serialized_size(encoding, size, CORBA::ULongLong(), length);
    return;
  case TK_FLOAT32:
    primitive_serialized_size(encoding, size, CORBA::Float(), length);
    return;
  case TK_FLOAT64:
    primitive_serialized_size(encoding, size, CORBA::Double(), length);
    return;
  case TK_FLOAT128:
    primitive_serialized_size(encoding, size, CORBA::LongDouble(), length);
    return;
  case TK_CHAR8:
    primitive_serialized_size_char(encoding, size, length);
    return;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    primitive_serialized_size_wchar(encoding, size, length);
    return;
#endif
  case TK_BYTE:
    primitive_serialized_size_octet(encoding, size, length);
    return;
  case TK_BOOLEAN:
    primitive_serialized_size_boolean(encoding, size, length);
    return;
  }
}

bool DynamicDataImpl::serialize_primitive_array(DCPS::Serializer& ser,
  TypeKind elem_tk, CORBA::ULong length) const
{
  switch (elem_tk) {
  case TK_INT32: {
    DDS::Int32Seq int32arr;
    return reconstruct_primitive_collection(int32arr, length, length, CORBA::Long()) &&
      ser.write_long_array(int32arr.get_buffer(), length);
  }
  case TK_UINT32: {
    DDS::UInt32Seq uint32arr;
    return reconstruct_primitive_collection(uint32arr, length, length, CORBA::ULong()) &&
      ser.write_ulong_array(uint32arr.get_buffer(), length);
  }
  case TK_INT8: {
    DDS::Int8Seq int8arr;
    return reconstruct_primitive_collection(int8arr, length, length, ACE_OutputCDR::from_int8(0)) &&
      ser.write_int8_array(int8arr.get_buffer(), length);
  }
  case TK_UINT8: {
    DDS::UInt8Seq uint8arr;
    return reconstruct_primitive_collection(uint8arr, length, length, ACE_OutputCDR::from_uint8(0)) &&
      ser.write_uint8_array(uint8arr.get_buffer(), length);
  }
  case TK_INT16: {
    DDS::Int16Seq int16arr;
    return reconstruct_primitive_collection(int16arr, length, length, CORBA::Short()) &&
      ser.write_short_array(int16arr.get_buffer(), length);
  }
  case TK_UINT16: {
    DDS::UInt16Seq uint16arr;
    return reconstruct_primitive_collection(uint16arr, length, length, CORBA::UShort()) &&
      ser.write_ushort_array(uint16arr.get_buffer(), length);
  }
  case TK_INT64: {
    DDS::Int64Seq int64arr;
    return reconstruct_primitive_collection(int64arr, length, length, CORBA::LongLong()) &&
      ser.write_longlong_array(int64arr.get_buffer(), length);
  }
  case TK_UINT64: {
    DDS::UInt64Seq uint64arr;
    return reconstruct_primitive_collection(uint64arr, length, length, CORBA::ULongLong()) &&
      ser.write_ulonglong_array(uint64arr.get_buffer(), length);
  }
  case TK_FLOAT32: {
    DDS::Float32Seq float32arr;
    return reconstruct_primitive_collection(float32arr, length, length, CORBA::Float()) &&
      ser.write_float_array(float32arr.get_buffer(), length);
  }
  case TK_FLOAT64: {
    DDS::Float64Seq float64arr;
    return reconstruct_primitive_collection(float64arr, length, length, CORBA::Double()) &&
      ser.write_double_array(float64arr.get_buffer(), length);
  }
  case TK_FLOAT128: {
    DDS::Float128Seq float128arr;
    return reconstruct_primitive_collection(float128arr, length, length, CORBA::LongDouble()) &&
      ser.write_longdouble_array(float128arr.get_buffer(), length);
  }
  case TK_CHAR8: {
    DDS::CharSeq chararr;
    return reconstruct_primitive_collection(chararr, length, length, ACE_OutputCDR::from_char('\0')) &&
      ser.write_char_array(chararr.get_buffer(), length);
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    DDS::WcharSeq wchararr;
    return reconstruct_primitive_collection(wchararr, length, length, ACE_OutputCDR::from_wchar(0)) &&
      ser.write_wchar_array(wchararr.get_buffer(), length);
  }
#endif
  case TK_BYTE: {
    DDS::ByteSeq bytearr;
    return reconstruct_primitive_collection(bytearr, length, length, ACE_OutputCDR::from_octet(0x00)) &&
      ser.write_octet_array(bytearr.get_buffer(), length);
  }
  case TK_BOOLEAN: {
    DDS::BooleanSeq boolarr;
    return reconstruct_primitive_collection(boolarr, length, length, ACE_OutputCDR::from_boolean(false)) &&
      ser.write_boolean_array(boolarr.get_buffer(), length);
  }
  }
  return false;
}

template<typename StringType>
bool DynamicDataImpl::serialized_size_generic_string_array(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  return serialized_size_generic_string_collection<StringType>(encoding, size, index_to_id);
}

template<typename StringType>
bool DynamicDataImpl::serialize_generic_string_array(DCPS::Serializer& ser,
                                                     CORBA::ULong length) const
{
  IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, length)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    size_t total_size = 0;
    if (!serialized_size_generic_string_array<StringType>(encoding, total_size, index_to_id) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return serialize_generic_string_collection<StringType>(ser, index_to_id);
}

// Serialize enum array represented as int8 array
void DynamicDataImpl::serialized_size_enum_array_as_int8s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_int8(encoding, size, length);
}

bool DynamicDataImpl::serialize_enum_array_as_ints_i(DCPS::Serializer& ser,
  const DDS::Int8Seq& enumarr) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_enum_array_as_int8s(encoding, total_size, enumarr.length());
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return ser.write_int8_array(enumarr.get_buffer(), enumarr.length());
}

bool DynamicDataImpl::serialize_enum_array_as_int8s(DCPS::Serializer& ser,
  CORBA::ULong length, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int8Seq enumarr;
  return reconstruct_enum_collection<CORBA::Int8>(enumarr, length, length, enum_type, ACE_OutputCDR::from_int8(0)) &&
    serialize_enum_array_as_ints_i(ser, enumarr);
}

// Serialize enum array represented as int16 array
void DynamicDataImpl::serialized_size_enum_array_as_int16s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::Short(), length);
}

bool DynamicDataImpl::serialize_enum_array_as_ints_i(DCPS::Serializer& ser,
  const DDS::Int16Seq& enumarr) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_enum_array_as_int16s(encoding, total_size, enumarr.length());
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return ser.write_short_array(enumarr.get_buffer(), enumarr.length());
}

bool DynamicDataImpl::serialize_enum_array_as_int16s(DCPS::Serializer& ser,
  CORBA::ULong length, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int16Seq enumarr;
  return reconstruct_enum_collection<CORBA::Short>(enumarr, length, length, enum_type, CORBA::Short()) &&
    serialize_enum_array_as_ints_i(ser, enumarr);
}

// Serialize enum array represented as int32 array
void DynamicDataImpl::serialized_size_enum_array_as_int32s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::Long(), length);
}

bool DynamicDataImpl::serialize_enum_array_as_ints_i(DCPS::Serializer& ser,
  const DDS::Int32Seq& enumarr) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_enum_array_as_int32s(encoding, total_size, enumarr.length());
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return ser.write_long_array(enumarr.get_buffer(), enumarr.length());
}

bool DynamicDataImpl::serialize_enum_array_as_int32s(DCPS::Serializer& ser,
  CORBA::ULong length, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int32Seq enumarr;
  return reconstruct_enum_collection<CORBA::Long>(enumarr, length, length, enum_type, CORBA::Long()) &&
    serialize_enum_array_as_ints_i(ser, enumarr);
}

void DynamicDataImpl::serialized_size_enum_array(const DCPS::Encoding& encoding,
  size_t& size, CORBA::ULong length, CORBA::ULong bitbound) const
{
  if (bitbound >= 1 && bitbound <= 8) {
    serialized_size_enum_array_as_int8s(encoding, size, length);
  } else if (bitbound >= 9 && bitbound <= 16) {
    serialized_size_enum_array_as_int16s(encoding, size, length);
  } else { // from 17 to 32
    serialized_size_enum_array_as_int32s(encoding, size, length);
  }
}

bool DynamicDataImpl::serialize_enum_array(DCPS::Serializer& ser,
  CORBA::ULong bitbound, CORBA::ULong length, const DDS::DynamicType_var& enum_type) const
{
  if (bitbound >= 1 && bitbound <= 8) {
    return serialize_enum_array_as_int8s(ser, length, enum_type);
  } else if (bitbound >= 9 && bitbound <= 16) {
    return serialize_enum_array_as_int16s(ser, length, enum_type);
  } else if (bitbound >= 17 && bitbound <= 32) {
    return serialize_enum_array_as_int32s(ser, length, enum_type);
  }
  return false;
}

// Bitmask array represented as uint8 array.
void DynamicDataImpl::serialized_size_bitmask_array_as_uint8s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_uint8(encoding, size, length);
}

bool DynamicDataImpl::serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
  const DDS::UInt8Seq& bitmask_arr) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_bitmask_array_as_uint8s(encoding, total_size, bitmask_arr.length());
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return ser.write_uint8_array(bitmask_arr.get_buffer(), bitmask_arr.length());
}

bool DynamicDataImpl::serialize_bitmask_array_as_uint8s(DCPS::Serializer& ser,
                                                        CORBA::ULong length) const
{
  DDS::UInt8Seq bitmask_arr;
  return reconstruct_bitmask_collection(bitmask_arr, length, length, ACE_OutputCDR::from_uint8(0)) &&
    serialize_bitmask_array_as_uints_i(ser, bitmask_arr);
}

// Bitmask array represented as uint16 array.
void DynamicDataImpl::serialized_size_bitmask_array_as_uint16s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::UShort(), length);
}

bool DynamicDataImpl::serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
  const DDS::UInt16Seq& bitmask_arr) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_bitmask_array_as_uint16s(encoding, total_size, bitmask_arr.length());
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return ser.write_ushort_array(bitmask_arr.get_buffer(), bitmask_arr.length());
}

bool DynamicDataImpl::serialize_bitmask_array_as_uint16s(DCPS::Serializer& ser,
                                                         CORBA::ULong length) const
{
  DDS::UInt16Seq bitmask_arr;
  return reconstruct_bitmask_collection(bitmask_arr, length, length, CORBA::UShort()) &&
    serialize_bitmask_array_as_uints_i(ser, bitmask_arr);
}

// Bitmask array represented as uint32 array.
void DynamicDataImpl::serialized_size_bitmask_array_as_uint32s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::ULong(), length);
}

bool DynamicDataImpl::serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
  const DDS::UInt32Seq& bitmask_arr) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_bitmask_array_as_uint32s(encoding, total_size, bitmask_arr.length());
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return ser.write_ulong_array(bitmask_arr.get_buffer(), bitmask_arr.length());
}

bool DynamicDataImpl::serialize_bitmask_array_as_uint32s(DCPS::Serializer& ser,
                                                         CORBA::ULong length) const
{
  DDS::UInt32Seq bitmask_arr;
  return reconstruct_bitmask_collection(bitmask_arr, length, length, CORBA::ULong()) &&
    serialize_bitmask_array_as_uints_i(ser, bitmask_arr);
}

// Bitmask array represented as uint64 array.
void DynamicDataImpl::serialized_size_bitmask_array_as_uint64s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::ULongLong(), length);
}

bool DynamicDataImpl::serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
  const DDS::UInt64Seq& bitmask_arr) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    serialized_size_bitmask_array_as_uint64s(encoding, total_size, bitmask_arr.length());
    if (!ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return ser.write_ulonglong_array(bitmask_arr.get_buffer(), bitmask_arr.length());
}

bool DynamicDataImpl::serialize_bitmask_array_as_uint64s(DCPS::Serializer& ser,
                                                         CORBA::ULong length) const
{
  DDS::UInt64Seq bitmask_arr;
  return reconstruct_bitmask_collection(bitmask_arr, length, length, CORBA::ULongLong()) &&
    serialize_bitmask_array_as_uints_i(ser, bitmask_arr);
}

void DynamicDataImpl::serialized_size_bitmask_array(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length, CORBA::ULong bitbound) const
{
  if (bitbound >= 1 && bitbound <= 8) {
    serialized_size_bitmask_array_as_uint8s(encoding, size, length);
  } else if (bitbound >= 9 && bitbound <= 16) {
    serialized_size_bitmask_array_as_uint16s(encoding, size, length);
  } else if (bitbound >= 17 && bitbound <= 32) {
    serialized_size_bitmask_array_as_uint32s(encoding, size, length);
  } else { // from 33 to 64
    serialized_size_bitmask_array_as_uint64s(encoding, size, length);
  }
}

bool DynamicDataImpl::serialize_bitmask_array(DCPS::Serializer& ser,
  CORBA::ULong bitbound, CORBA::ULong length) const
{
  if (bitbound >= 1 && bitbound <= 8) {
    return serialize_bitmask_array_as_uint8s(ser, length);
  } else if (bitbound >= 9 && bitbound <= 16) {
    return serialize_bitmask_array_as_uint16s(ser, length);
  } else if (bitbound >= 17 && bitbound <= 32) {
    return serialize_bitmask_array_as_uint32s(ser, length);
  } else if (bitbound >= 33 && bitbound <= 64) {
    return serialize_bitmask_array_as_uint64s(ser, length);
  }
  return false;
}

template<typename SequenceType>
bool DynamicDataImpl::serialized_size_nesting_basic_array(const DCPS::Encoding& encoding,
  size_t& size, const IndexToIdMap& index_to_id, SequenceType protoseq) const
{
  serialized_size_delimiter(encoding, size);
  return serialized_size_nested_basic_sequences(encoding, size, index_to_id, protoseq);
}

template<typename SequenceType>
bool DynamicDataImpl::serialize_nesting_basic_array_i(DCPS::Serializer& ser,
  CORBA::ULong length, SequenceType protoseq) const
{
  IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, length)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_nesting_basic_array(encoding, total_size, index_to_id, protoseq) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return serialize_nested_basic_sequences(ser, index_to_id, protoseq);
}

bool DynamicDataImpl::serialized_size_nesting_basic_array(const DCPS::Encoding& encoding,
  size_t& size, TypeKind nested_elem_tk, const IndexToIdMap& index_to_id) const
{
  switch (nested_elem_tk) {
  case TK_INT32:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::Int32Seq());
  case TK_UINT32:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::UInt32Seq());
  case TK_INT8:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::Int8Seq());
  case TK_UINT8:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::UInt8Seq());
  case TK_INT16:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::Int16Seq());
  case TK_UINT16:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::UInt16Seq());
  case TK_INT64:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::Int64Seq());
  case TK_UINT64:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::UInt64Seq());
  case TK_FLOAT32:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::Float32Seq());
  case TK_FLOAT64:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::Float64Seq());
  case TK_FLOAT128:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::Float128Seq());
  case TK_CHAR8:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::CharSeq());
  case TK_STRING8:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::StringSeq());
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::WcharSeq());
  case TK_STRING16:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::WstringSeq());
#endif
  case TK_BYTE:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::ByteSeq());
  case TK_BOOLEAN:
    return serialized_size_nesting_basic_array(encoding, size, index_to_id, DDS::BooleanSeq());
  }
  return false;
}

bool DynamicDataImpl::serialize_nesting_basic_array(DCPS::Serializer& ser,
  TypeKind nested_elem_tk, CORBA::ULong length) const
{
  switch (nested_elem_tk) {
  case TK_INT32:
    return serialize_nesting_basic_array_i(ser, length, DDS::Int32Seq());
  case TK_UINT32:
    return serialize_nesting_basic_array_i(ser, length, DDS::UInt32Seq());
  case TK_INT8:
    return serialize_nesting_basic_array_i(ser, length, DDS::Int8Seq());
  case TK_UINT8:
    return serialize_nesting_basic_array_i(ser, length, DDS::UInt8Seq());
  case TK_INT16:
    return serialize_nesting_basic_array_i(ser, length, DDS::Int16Seq());
  case TK_UINT16:
    return serialize_nesting_basic_array_i(ser, length, DDS::UInt16Seq());
  case TK_INT64:
    return serialize_nesting_basic_array_i(ser, length, DDS::Int64Seq());
  case TK_UINT64:
    return serialize_nesting_basic_array_i(ser, length, DDS::UInt64Seq());
  case TK_FLOAT32:
    return serialize_nesting_basic_array_i(ser, length, DDS::Float32Seq());
  case TK_FLOAT64:
    return serialize_nesting_basic_array_i(ser, length, DDS::Float64Seq());
  case TK_FLOAT128:
    return serialize_nesting_basic_array_i(ser, length, DDS::Float128Seq());
  case TK_CHAR8:
    return serialize_nesting_basic_array_i(ser, length, DDS::CharSeq());
  case TK_STRING8:
    return serialize_nesting_basic_array_i(ser, length, DDS::StringSeq());
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return serialize_nesting_basic_array_i(ser, length, DDS::WcharSeq());
  case TK_STRING16:
    return serialize_nesting_basic_array_i(ser, length, DDS::WstringSeq());
#endif
  case TK_BYTE:
    return serialize_nesting_basic_array_i(ser, length, DDS::ByteSeq());
  case TK_BOOLEAN:
    return serialize_nesting_basic_array_i(ser, length, DDS::BooleanSeq());
  }
  return false;
}

bool DynamicDataImpl::serialized_size_nesting_enum_array(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  return serialized_size_nested_enum_sequences(encoding, size, index_to_id);
}

bool DynamicDataImpl::serialize_nesting_enum_array(DCPS::Serializer& ser,
                                                   CORBA::ULong length) const
{
  IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, length)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_nesting_enum_array(encoding, total_size, index_to_id) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return serialize_nested_enum_sequences(ser, index_to_id);
}

bool DynamicDataImpl::serialized_size_nesting_bitmask_array(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  return serialized_size_nested_bitmask_sequences(encoding, size, index_to_id);
}

bool DynamicDataImpl::serialize_nesting_bitmask_array(DCPS::Serializer& ser,
                                                      CORBA::ULong length) const
{
  IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, length)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_nesting_bitmask_array(encoding, total_size, index_to_id) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return serialize_nested_bitmask_sequences(ser, index_to_id);
}

bool DynamicDataImpl::serialized_size_complex_array(const DCPS::Encoding& encoding,
  size_t& size, const IndexToIdMap& index_to_id, const DDS::DynamicType_var& elem_type,
  DCPS::Sample::Extent ext) const
{
  serialized_size_delimiter(encoding, size);
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    if (!serialized_size_complex_member(encoding, size, index_to_id[i], elem_type, ext)) {
      return false;
    }
  }
  return true;
}

bool DynamicDataImpl::serialize_complex_array(
  DCPS::Serializer& ser, CORBA::ULong length,
  const DDS::DynamicType_var& elem_type, DCPS::Sample::Extent ext) const
{
  IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
  for (const_complex_iterator it = container_.complex_map_.begin(); it != container_.complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!get_index_from_id(it->first, index, length)) {
      return false;
    }
    index_to_id[index] = it->first;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_complex_array(encoding, total_size, index_to_id, elem_type, ext) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return serialize_complex_sequence_i(ser, index_to_id, elem_type, ext);
}

bool DynamicDataImpl::serialized_size_array(const DCPS::Encoding& encoding,
                                            size_t& size, DCPS::Sample::Extent ext) const
{
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }

  const CORBA::ULong length = bound_total(type_desc_);
  if (is_basic(elem_tk)) {
    serialized_size_primitive_array(encoding, size, elem_tk, length);
    return true;
  } else if (elem_tk == TK_STRING8) {
    IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
    if (!get_index_to_id_map(index_to_id, length)) {
      return false;
    }
    return serialized_size_generic_string_array<const char*>(encoding, size, index_to_id);
  } else if (elem_tk == TK_STRING16) {
#ifdef DDS_HAS_WCHAR
    IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
    if (!get_index_to_id_map(index_to_id, length)) {
      return false;
    }
    return serialized_size_generic_string_array<const CORBA::WChar*>(encoding, size, index_to_id);
#else
    return false;
#endif
  } else if (elem_tk == TK_ENUM) {
    const CORBA::ULong bit_bound = elem_td->bound()[0];
    serialized_size_enum_array(encoding, size, length, bit_bound);
    return true;
  } else if (elem_tk == TK_BITMASK) {
    const CORBA::ULong bit_bound = elem_td->bound()[0];
    serialized_size_bitmask_array(encoding, size, length, bit_bound);
    return true;
  } else if (elem_tk == TK_SEQUENCE) {
    const DDS::DynamicType_var nested_elem_type = get_base_type(elem_td->element_type());
    const TypeKind nested_elem_tk = nested_elem_type->get_kind();
    IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
    if (!get_index_to_id_map(index_to_id, length)) {
      return false;
    }
    if (is_basic(nested_elem_tk)) {
      return serialized_size_nesting_basic_array(encoding, size, nested_elem_tk, index_to_id);
    } else if (nested_elem_tk == TK_ENUM) {
      return serialized_size_nesting_enum_array(encoding, size, index_to_id);
    } else if (nested_elem_tk == TK_BITMASK) {
      return serialized_size_nesting_bitmask_array(encoding, size, index_to_id);
    }
  }

  // Elements stored in complex map
  IndexToIdMap index_to_id;
  if (!get_index_to_id_from_complex(index_to_id, length)) {
    return false;
  }
  return serialized_size_complex_array(encoding, size, index_to_id, elem_type, ext);
}

bool DynamicDataImpl::serialize_array(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const
{
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }

  const CORBA::ULong length = bound_total(type_desc_);
  if (is_basic(elem_tk)) {
    return serialize_primitive_array(ser, elem_tk, length);
  } else if (elem_tk == TK_STRING8) {
    return serialize_generic_string_array<const char*>(ser, length);
  } else if (elem_tk == TK_STRING16) {
#ifdef DDS_HAS_WCHAR
    return serialize_generic_string_array<const CORBA::WChar*>(ser, length);
#else
    return false;
#endif
  } else if (elem_tk == TK_ENUM) {
    const CORBA::ULong bit_bound = elem_td->bound()[0];
    return serialize_enum_array(ser, bit_bound, length, elem_type);
  } else if (elem_tk == TK_BITMASK) {
    const CORBA::ULong bit_bound = elem_td->bound()[0];
    return serialize_bitmask_array(ser, bit_bound, length);
  } else if (elem_tk == TK_SEQUENCE) {
    const DDS::DynamicType_var nested_elem_type = get_base_type(elem_td->element_type());
    const TypeKind nested_elem_tk = nested_elem_type->get_kind();
    if (is_basic(nested_elem_tk)) {
      return serialize_nesting_basic_array(ser, nested_elem_tk, length);
    } else if (nested_elem_tk == TK_ENUM) {
      return serialize_nesting_enum_array(ser, length);
    } else if (nested_elem_tk == TK_BITMASK) {
      return serialize_nesting_bitmask_array(ser, length);
    }
  }
  return serialize_complex_array(ser, length, elem_type, ext);
}

bool DynamicDataImpl::serialized_size_primitive_member(const DCPS::Encoding& encoding,
  size_t& size, TypeKind member_tk) const
{
  using namespace OpenDDS::DCPS;

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

bool DynamicDataImpl::serialized_size_basic_member_default_value(
  const DCPS::Encoding& encoding, size_t& size, TypeKind member_tk) const
{
  if (is_primitive(member_tk)) {
    return serialized_size_primitive_member(encoding, size, member_tk);
  } else if (member_tk == TK_STRING8) {
    const char* str_default;
    set_default_basic_value(str_default);
    serialized_size_string_common(encoding, size, str_default);
    return true;
  }
#ifdef DDS_HAS_WCHAR
  else if (member_tk == TK_STRING16) {
    const CORBA::WChar* wstr_default;
    set_default_basic_value(wstr_default);
    serialized_size_string_common(encoding, size, wstr_default);
    return true;
  }
#endif
  return false;
}

// Serialized size of a basic member of an aggregated type. Member header size is not included.
bool DynamicDataImpl::serialized_size_basic_member(const DCPS::Encoding& encoding,
  size_t& size, TypeKind member_tk, const_single_iterator it) const
{
  if (is_primitive(member_tk)) {
    return serialized_size_primitive_member(encoding, size, member_tk);
  } else if (member_tk == TK_STRING8 || member_tk == TK_STRING16) {
    serialized_size_string_common(encoding, size, it->second);
    return true;
  }
  return false;
}

bool DynamicDataImpl::serialize_basic_member_default_value(DCPS::Serializer& ser,
                                                           TypeKind member_tk) const
{
  switch (member_tk) {
  case TK_INT32: {
    CORBA::Long value;
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_UINT32: {
    CORBA::ULong value;
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_INT8: {
    ACE_OutputCDR::from_int8 value(0);
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_UINT8: {
    ACE_OutputCDR::from_uint8 value(0);
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_INT16: {
    CORBA::Short value;
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_UINT16: {
    CORBA::UShort value;
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_INT64: {
    CORBA::LongLong value;
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_UINT64: {
    CORBA::ULongLong value;
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_FLOAT32: {
    CORBA::Float value;
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_FLOAT64: {
    CORBA::Double value;
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_FLOAT128: {
    CORBA::LongDouble value;
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_CHAR8: {
    ACE_OutputCDR::from_char value('\0');
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_STRING8: {
    const char* value;
    set_default_basic_value(value);
    return ser << value;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    ACE_OutputCDR::from_wchar value(0);
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_STRING16: {
    const CORBA::WChar* value;
    set_default_basic_value(value);
    return ser << value;
  }
#endif
  case TK_BYTE: {
    ACE_OutputCDR::from_octet value(0x00);
    set_default_basic_value(value);
    return ser << value;
  }
  case TK_BOOLEAN: {
    ACE_OutputCDR::from_boolean value(false);
    set_default_basic_value(value);
    return ser << value;
  }
  }
  return false;
}

// Serialize an aggregated member stored in the single map.
// The member type is basic or enum or bitmask.
bool DynamicDataImpl::serialized_size_single_aggregated_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, const_single_iterator it,
  const DDS::DynamicType_var& member_type, bool optional,
  DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
{
  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    primitive_serialized_size_boolean(encoding, size);
  } else if (extensibility == DDS::MUTABLE) {
    serialized_size_parameter_id(encoding, size, mutable_running_total);
  }
  const TypeKind member_tk = member_type->get_kind();
  if (is_basic(member_tk)) {
    return serialized_size_basic_member(encoding, size, member_tk, it);
  } else if (member_tk == TK_ENUM) {
    return serialized_size_enum(encoding, size, member_type);
  } else { // Bitmask
    return serialized_size_bitmask(encoding, size, member_type);
  }
}

bool DynamicDataImpl::serialize_single_aggregated_member_xcdr2(DCPS::Serializer& ser,
  const_single_iterator it, const DDS::DynamicType_var& member_type, bool optional,
  bool must_understand, DDS::ExtensibilityKind extensibility) const
{
  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    if (!(ser << ACE_OutputCDR::from_boolean(true))) {
      return false;
    }
  } else if (extensibility == DDS::MUTABLE) {
    const DCPS::Encoding& encoding = ser.encoding();
    const TypeKind member_tk = member_type->get_kind();
    size_t member_size = 0;
    if (is_basic(member_tk)) {
      serialized_size_basic_member(encoding, member_size, member_tk, it);
    } else if (member_tk == TK_ENUM) {
      serialized_size_enum(encoding, member_size, member_type);
    } else if (member_tk == TK_BITMASK) {
      serialized_size_bitmask(encoding, member_size, member_type);
    } else {
      return false;
    }
    if (!ser.write_parameter_id(it->first, member_size, must_understand)) {
      return false;
    }
  }
  return serialize_single_value(ser, it->second);
}

// Serialized size of the default value of a member of an aggregated type.
bool DynamicDataImpl::serialized_size_complex_aggregated_member_xcdr2_default(
  const DCPS::Encoding& encoding, size_t& size, const DDS::DynamicType_var& member_type,
  bool optional, DDS::ExtensibilityKind extensibility, size_t& mutable_running_total,
  DCPS::Sample::Extent ext) const
{
  if (optional) {
    if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
      primitive_serialized_size_boolean(encoding, size);
    }
    return true;
  }
  if (extensibility == DDS::MUTABLE) {
    serialized_size_parameter_id(encoding, size, mutable_running_total);
  }

  return DynamicDataImpl(member_type).serialized_size_i(encoding, size, ext);
}

bool DynamicDataImpl::serialize_complex_aggregated_member_xcdr2_default(
  DCPS::Serializer& ser, DDS::MemberId id, const DDS::DynamicType_var& member_type,
  bool optional, bool must_understand, DDS::ExtensibilityKind extensibility,
  DCPS::Sample::Extent ext) const
{
  if (optional) {
    if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
      return ser << ACE_OutputCDR::from_boolean(false);
    }
    return true;
  }
  // Use default value if the member is not optional.
  const DynamicDataImpl default_value(member_type);
  if (extensibility == DDS::MUTABLE) {
    size_t member_size = 0;
    default_value.serialized_size_i(ser.encoding(), member_size, ext);
    if (!ser.write_parameter_id(id, member_size, must_understand)) {
      return false;
    }
  }
  return default_value.serialize_i(ser, ext);
}

// Serialize a member of an aggregated type stored in the complex map,
// i.e., the member value is represented by a DynamicData object.
bool DynamicDataImpl::serialized_size_complex_aggregated_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, const_complex_iterator it,
  bool optional, DDS::ExtensibilityKind extensibility, size_t& mutable_running_total,
  DCPS::Sample::Extent ext) const
{
  const DDS::DynamicData_var& data_var = it->second;
  const DynamicDataImpl* data_impl = dynamic_cast<DynamicDataImpl*>(data_var.in());
  if (!data_impl) {
    return false;
  }

  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    primitive_serialized_size_boolean(encoding, size);
  } else if (extensibility == DDS::MUTABLE) {
    serialized_size_parameter_id(encoding, size, mutable_running_total);
  }
  return data_impl->serialized_size_i(encoding, size, ext);
}

bool DynamicDataImpl::serialize_complex_aggregated_member_xcdr2(
  DCPS::Serializer& ser, const_complex_iterator it, bool optional,
  bool must_understand, DDS::ExtensibilityKind extensibility, DCPS::Sample::Extent ext) const
{
  const DDS::DynamicData_var& data_var = it->second;
  const DynamicDataImpl* data_impl = dynamic_cast<DynamicDataImpl*>(data_var.in());
  if (!data_impl) {
    return false;
  }

  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    if (!(ser << ACE_OutputCDR::from_boolean(true))) {
      return false;
    }
  } else if (extensibility == DDS::MUTABLE) {
    size_t member_size = 0;
    if (!data_impl->serialized_size_i(ser.encoding(), member_size, ext) ||
        !ser.write_parameter_id(it->first, member_size, must_understand)) {
      return false;
    }
  }
  return data_impl->serialize_i(ser, ext);
}

// Serialize struct member whose type is basic or compatible with a basic type,
// that are enum and bitmask types.
bool DynamicDataImpl::serialized_size_basic_struct_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, DDS::MemberId id,
  const DDS::DynamicType_var& member_type, bool optional,
  DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
{
  const TypeKind member_tk = member_type->get_kind();
  const_single_iterator single_it = container_.single_map_.find(id);
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (single_it == container_.single_map_.end() && complex_it == container_.complex_map_.end()) {
    if (optional) {
      if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
        primitive_serialized_size_boolean(encoding, size);
      }
      return true;
    }
    if (extensibility == DDS::MUTABLE) {
      serialized_size_parameter_id(encoding, size, mutable_running_total);
    }
    if (is_basic(member_tk)) {
      return serialized_size_basic_member_default_value(encoding, size, member_tk);
    } else if (member_tk == TK_ENUM) {
      return serialized_size_enum(encoding, size, member_type);
    } else { // Bitmask
      return serialized_size_bitmask(encoding, size, member_type);
    }
  }

  if (single_it != container_.single_map_.end()) {
    return serialized_size_single_aggregated_member_xcdr2(encoding, size, single_it, member_type,
                                                          optional, extensibility, mutable_running_total);
  }
  return serialized_size_complex_aggregated_member_xcdr2(encoding, size, complex_it, optional,
                                                         extensibility, mutable_running_total,
                                                         DCPS::Sample::Full);
}

bool DynamicDataImpl::serialize_basic_struct_member_xcdr2(DCPS::Serializer& ser,
  DDS::MemberId id, const DDS::DynamicType_var& member_type, bool optional,
  bool must_understand, DDS::ExtensibilityKind extensibility) const
{
  const TypeKind member_tk = member_type->get_kind();
  const DCPS::Encoding& encoding = ser.encoding();
  const_single_iterator single_it = container_.single_map_.find(id);
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (single_it == container_.single_map_.end() && complex_it == container_.complex_map_.end()) {
    if (optional) {
      if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
        return ser << ACE_OutputCDR::from_boolean(false);
      }
      return true;
    }
    if (extensibility == DDS::MUTABLE) {
      size_t member_size = 0;
      if (is_basic(member_tk)) {
        serialized_size_basic_member_default_value(encoding, member_size, member_tk);
      } else if (member_tk == TK_ENUM) {
        serialized_size_enum(encoding, member_size, member_type);
      } else if (member_tk == TK_BITMASK) {
        serialized_size_bitmask(encoding, member_size, member_type);
      } else {
        return false;
      }
      if (!ser.write_parameter_id(id, member_size, must_understand)) {
        return false;
      }
    }
    if (is_basic(member_tk)) {
      return serialize_basic_member_default_value(ser, member_tk);
    } else if (member_tk == TK_ENUM) {
      return serialize_enum_default_value(ser, member_type);
    } else if (member_tk == TK_BITMASK) {
      return serialize_bitmask_default_value(ser, member_type);
    }
    return false;
  }

  if (single_it != container_.single_map_.end()) {
    return serialize_single_aggregated_member_xcdr2(ser, single_it, member_type, optional,
                                                    must_understand, extensibility);
  }
  return serialize_complex_aggregated_member_xcdr2(ser, complex_it, optional,
                                                   must_understand, extensibility,
                                                   DCPS::Sample::Full);
}

void DynamicDataImpl::serialized_size_sequence_member_default_value(
  const DCPS::Encoding& encoding, size_t& size, TypeKind elem_tk) const
{
  // Zero-length sequence
  if (!is_primitive(elem_tk)) {
    serialized_size_delimiter(encoding, size);
  }
  primitive_serialized_size_ulong(encoding, size);
}

bool DynamicDataImpl::serialize_sequence_member_default_value(DCPS::Serializer& ser,
                                                              TypeKind elem_tk) const
{
  // Zero-length sequence
  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2 && !is_primitive(elem_tk)) {
    if (!ser.write_delimiter(2 * DCPS::uint32_cdr_size)) {
      return false;
    }
  }
  return ser << static_cast<CORBA::ULong>(0);
}

bool DynamicDataImpl::serialized_size_basic_sequence(const DCPS::Encoding& encoding,
  size_t& size, const_sequence_iterator it) const
{
  switch (it->second.elem_kind_) {
  case TK_INT32: {
    const DDS::Int32Seq& seq = it->second.get<DDS::Int32Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_UINT32: {
    const DDS::UInt32Seq& seq = it->second.get<DDS::UInt32Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_INT8: {
    const DDS::Int8Seq& seq = it->second.get<DDS::Int8Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_UINT8: {
    const DDS::UInt8Seq& seq = it->second.get<DDS::UInt8Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_INT16: {
    const DDS::Int16Seq& seq = it->second.get<DDS::Int16Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_UINT16: {
    const DDS::UInt16Seq& seq = it->second.get<DDS::UInt16Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_INT64: {
    const DDS::Int64Seq& seq = it->second.get<DDS::Int64Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_UINT64: {
    const DDS::UInt64Seq& seq = it->second.get<DDS::UInt64Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_FLOAT32: {
    const DDS::Float32Seq& seq = it->second.get<DDS::Float32Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_FLOAT64: {
    const DDS::Float64Seq& seq = it->second.get<DDS::Float64Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_FLOAT128: {
    const DDS::Float128Seq& seq = it->second.get<DDS::Float128Seq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_CHAR8: {
    const DDS::CharSeq& seq = it->second.get<DDS::CharSeq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_BYTE: {
    const DDS::ByteSeq& seq = it->second.get<DDS::ByteSeq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_BOOLEAN: {
    const DDS::BooleanSeq& seq = it->second.get<DDS::BooleanSeq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_STRING8: {
    const DDS::StringSeq& seq = it->second.get<DDS::StringSeq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    const DDS::WcharSeq& seq = it->second.get<DDS::WcharSeq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
  case TK_STRING16: {
    const DDS::WstringSeq& seq = it->second.get<DDS::WstringSeq>();
    DCPS::serialized_size(encoding, size, seq);
    return true;
  }
#endif
  }
  return false;
}

bool DynamicDataImpl::serialize_basic_sequence(DCPS::Serializer& ser,
                                               const_sequence_iterator it) const
{
  switch (it->second.elem_kind_) {
  case TK_INT32: {
    const DDS::Int32Seq& seq = it->second.get<DDS::Int32Seq>();
    return ser << seq;
  }
  case TK_UINT32: {
    const DDS::UInt32Seq& seq = it->second.get<DDS::UInt32Seq>();
    return ser << seq;
  }
  case TK_INT8: {
    const DDS::Int8Seq& seq = it->second.get<DDS::Int8Seq>();
    return ser << seq;
  }
  case TK_UINT8: {
    const DDS::UInt8Seq& seq = it->second.get<DDS::UInt8Seq>();
    return ser << seq;
  }
  case TK_INT16: {
    const DDS::Int16Seq& seq = it->second.get<DDS::Int16Seq>();
    return ser << seq;
  }
  case TK_UINT16: {
    const DDS::UInt16Seq& seq = it->second.get<DDS::UInt16Seq>();
    return ser << seq;
  }
  case TK_INT64: {
    const DDS::Int64Seq& seq = it->second.get<DDS::Int64Seq>();
    return ser << seq;
  }
  case TK_UINT64: {
    const DDS::UInt64Seq& seq = it->second.get<DDS::UInt64Seq>();
    return ser << seq;
  }
  case TK_FLOAT32: {
    const DDS::Float32Seq& seq = it->second.get<DDS::Float32Seq>();
    return ser << seq;
  }
  case TK_FLOAT64: {
    const DDS::Float64Seq& seq = it->second.get<DDS::Float64Seq>();
    return ser << seq;
  }
  case TK_FLOAT128: {
    const DDS::Float128Seq& seq = it->second.get<DDS::Float128Seq>();
    return ser << seq;
  }
  case TK_CHAR8: {
    const DDS::CharSeq& seq = it->second.get<DDS::CharSeq>();
    return ser << seq;
  }
  case TK_BYTE: {
    const DDS::ByteSeq& seq = it->second.get<DDS::ByteSeq>();
    return ser << seq;
  }
  case TK_BOOLEAN: {
    const DDS::BooleanSeq& seq = it->second.get<DDS::BooleanSeq>();
    return ser << seq;
  }
  case TK_STRING8: {
    const DDS::StringSeq& seq = it->second.get<DDS::StringSeq>();
    return ser << seq;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    const DDS::WcharSeq& seq = it->second.get<DDS::WcharSeq>();
    return ser << seq;
  }
  case TK_STRING16: {
    const DDS::WstringSeq& seq = it->second.get<DDS::WstringSeq>();
    return ser << seq;
  }
#endif
  }
  return false;
}

bool DynamicDataImpl::serialized_size_enum_sequence(
  const DCPS::Encoding& encoding, size_t& size, const_sequence_iterator it) const
{
  switch (it->second.elem_kind_) {
  case TK_INT8: {
    const DDS::Int8Seq& seq = it->second.get<DDS::Int8Seq>();
    serialized_size_enum_sequence(encoding, size, seq);
    return true;
  }
  case TK_INT16: {
    const DDS::Int16Seq& seq = it->second.get<DDS::Int16Seq>();
    serialized_size_enum_sequence(encoding, size, seq);
    return true;
  }
  case TK_INT32: {
    const DDS::Int32Seq& seq = it->second.get<DDS::Int32Seq>();
    serialized_size_enum_sequence(encoding, size, seq);
    return true;
  }
  }
  return false;
}

bool DynamicDataImpl::serialize_enum_sequence(
  DCPS::Serializer& ser, const_sequence_iterator it) const
{
  switch (it->second.elem_kind_) {
  case TK_INT8: {
    const DDS::Int8Seq& seq = it->second.get<DDS::Int8Seq>();
    return serialize_enum_sequence_as_ints_i(ser, seq);
  }
  case TK_INT16: {
    const DDS::Int16Seq& seq = it->second.get<DDS::Int16Seq>();
    return serialize_enum_sequence_as_ints_i(ser, seq);
  }
  case TK_INT32: {
    const DDS::Int32Seq& seq = it->second.get<DDS::Int32Seq>();
    return serialize_enum_sequence_as_ints_i(ser, seq);
  }
  }
  return false;
}

bool DynamicDataImpl::serialized_size_bitmask_sequence(
  const DCPS::Encoding& encoding, size_t& size, const_sequence_iterator it) const
{
  switch (it->second.elem_kind_) {
  case TK_UINT8: {
    const DDS::UInt8Seq& seq = it->second.get<DDS::UInt8Seq>();
    serialized_size_bitmask_sequence(encoding, size, seq);
    return true;
  }
  case TK_UINT16: {
    const DDS::UInt16Seq& seq = it->second.get<DDS::UInt16Seq>();
    serialized_size_bitmask_sequence(encoding, size, seq);
    return true;
  }
  case TK_UINT32: {
    const DDS::UInt32Seq& seq = it->second.get<DDS::UInt32Seq>();
    serialized_size_bitmask_sequence(encoding, size, seq);
    return true;
  }
  case TK_UINT64: {
    const DDS::UInt64Seq& seq = it->second.get<DDS::UInt64Seq>();
    serialized_size_bitmask_sequence(encoding, size, seq);
    return true;
  }
  }
  return false;
}

bool DynamicDataImpl::serialize_bitmask_sequence(DCPS::Serializer& ser,
                                                 const_sequence_iterator it) const
{
  switch (it->second.elem_kind_) {
  case TK_UINT8: {
    const DDS::UInt8Seq& seq = it->second.get<DDS::UInt8Seq>();
    return serialize_bitmask_sequence_as_uints_i(ser, seq);
  }
  case TK_UINT16: {
    const DDS::UInt16Seq& seq = it->second.get<DDS::UInt16Seq>();
    return serialize_bitmask_sequence_as_uints_i(ser, seq);
  }
  case TK_UINT32: {
    const DDS::UInt32Seq& seq = it->second.get<DDS::UInt32Seq>();
    return serialize_bitmask_sequence_as_uints_i(ser, seq);
  }
  case TK_UINT64: {
    const DDS::UInt64Seq& seq = it->second.get<DDS::UInt64Seq>();
    return serialize_bitmask_sequence_as_uints_i(ser, seq);
  }
  }
  return false;
}

// Serialize an aggregated member stored in the sequence map.
// The member type is sequence of basic or enum or bitmask type.
void DynamicDataImpl::serialized_size_sequence_aggregated_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, const_sequence_iterator it, TypeKind elem_tk,
  bool optional, DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
{
  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    primitive_serialized_size_boolean(encoding, size);
  } else if (extensibility == DDS::MUTABLE) {
    serialized_size_parameter_id(encoding, size, mutable_running_total);
  }
  if (is_basic(elem_tk)) {
    serialized_size_basic_sequence(encoding, size, it);
  } else if (elem_tk == TK_ENUM) {
    serialized_size_enum_sequence(encoding, size, it);
  } else { // Bitmask
    serialized_size_bitmask_sequence(encoding, size, it);
  }
}

bool DynamicDataImpl::serialize_sequence_aggregated_member_xcdr2(DCPS::Serializer& ser,
  const_sequence_iterator it, TypeKind elem_tk, bool optional,
  bool must_understand, DDS::ExtensibilityKind extensibility) const
{
  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    if (!(ser << ACE_OutputCDR::from_boolean(true))) {
      return false;
    }
  } else if (extensibility == DDS::MUTABLE) {
    const DCPS::Encoding& encoding = ser.encoding();
    size_t member_size = 0;
    if (is_basic(elem_tk)) {
      serialized_size_basic_sequence(encoding, member_size, it);
    } else if (elem_tk == TK_ENUM) {
      serialized_size_enum_sequence(encoding, member_size, it);
    } else if (elem_tk == TK_BITMASK) {
      serialized_size_bitmask_sequence(encoding, member_size, it);
    } else {
      return false;
    }
    if (!ser.write_parameter_id(it->first, member_size, must_understand)) {
      return false;
    }
  }
  if (is_basic(elem_tk)) {
    return serialize_basic_sequence(ser, it);
  } else if (elem_tk == TK_ENUM) {
    return serialize_enum_sequence(ser, it);
  } else if (elem_tk == TK_BITMASK) {
    return serialize_bitmask_sequence(ser, it);
  }
  return false;
}

// Serialize a struct member whose type is sequence of basic type or enum or bitmask.
bool DynamicDataImpl::serialized_size_sequence_struct_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, DDS::MemberId id, TypeKind elem_tk,
  bool optional, DDS::ExtensibilityKind extensibility, size_t& mutable_running_total,
  DCPS::Sample::Extent) const
{
  const_sequence_iterator seq_it = container_.sequence_map_.find(id);
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (seq_it == container_.sequence_map_.end() && complex_it == container_.complex_map_.end()) {
    if (optional) {
      if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
        primitive_serialized_size_boolean(encoding, size);
      }
      return true;
    }
    if (extensibility == DDS::MUTABLE) {
      serialized_size_parameter_id(encoding, size, mutable_running_total);
    }
    serialized_size_sequence_member_default_value(encoding, size, elem_tk);
    return true;
  }
  if (seq_it != container_.sequence_map_.end()) {
    serialized_size_sequence_aggregated_member_xcdr2(encoding, size, seq_it, elem_tk, optional,
                                                     extensibility, mutable_running_total);
    return true;
  }
  return serialized_size_complex_aggregated_member_xcdr2(encoding, size, complex_it, optional,
                                                         extensibility, mutable_running_total,
                                                         DCPS::Sample::Full);
}

bool DynamicDataImpl::serialize_sequence_struct_member_xcdr2(DCPS::Serializer& ser,
  DDS::MemberId id, TypeKind elem_tk, bool optional,
  bool must_understand, DDS::ExtensibilityKind extensibility,
  DCPS::Sample::Extent ext) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  const_sequence_iterator seq_it = container_.sequence_map_.find(id);
  const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (seq_it == container_.sequence_map_.end() && complex_it == container_.complex_map_.end()) {
    if (optional) {
      if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
        return ser << ACE_OutputCDR::from_boolean(false);
      }
      return true;
    }
    if (extensibility == DDS::MUTABLE) {
      size_t member_size = 0;
      serialized_size_sequence_member_default_value(encoding, member_size, elem_tk);
      if (!ser.write_parameter_id(id, member_size, must_understand)) {
        return false;
      }
    }
    return serialize_sequence_member_default_value(ser, elem_tk);
  }

  if (seq_it != container_.sequence_map_.end()) {
    return serialize_sequence_aggregated_member_xcdr2(ser, seq_it, elem_tk, optional,
                                                      must_understand, extensibility);
  }
  return serialize_complex_aggregated_member_xcdr2(ser, complex_it, optional,
                                                   must_understand, extensibility, ext);
}

bool DynamicDataImpl::serialized_size_structure_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, DCPS::Sample::Extent ext) const
{
  const DDS::ExtensibilityKind extensibility = type_desc_->extensibility_kind();
  const bool struct_has_explicit_keys = has_explicit_keys(type_);

  // Delimiter
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    serialized_size_delimiter(encoding, size);
  }

  // Members
  size_t mutable_running_total = 0;
  const CORBA::ULong member_count = type_->get_member_count();
  for (CORBA::ULong i = 0; i < member_count; ++i) {
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member_by_index(dtm, i) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var md;
    if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }

    if (exclude_member(ext, md->is_key(), struct_has_explicit_keys)) {
      continue;
    }

    const DDS::MemberId id = md->id();
    const CORBA::Boolean optional = md->is_optional();
    const DDS::DynamicType_var member_type = get_base_type(md->type());
    const TypeKind member_tk = member_type->get_kind();

    if (is_basic(member_tk) || member_tk == TK_ENUM || member_tk == TK_BITMASK) {
      if (!serialized_size_basic_struct_member_xcdr2(encoding, size, id, member_type, optional,
                                                     extensibility, mutable_running_total)) {
        return false;
      }
      continue;
    } else if (member_tk == TK_SEQUENCE) {
      DDS::TypeDescriptor_var member_td;
      if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
        return false;
      }
      const DDS::DynamicType_var elem_type = get_base_type(member_td->element_type());
      const TypeKind elem_tk = elem_type->get_kind();
      if (is_basic(elem_tk) || elem_tk == TK_ENUM || elem_tk == TK_BITMASK) {
        if (!serialized_size_sequence_struct_member_xcdr2(encoding, size, id, elem_tk, optional,
                                                          extensibility, mutable_running_total, nested(ext))) {
          return false;
        }
        continue;
      }
    }

    const_complex_iterator it = container_.complex_map_.find(id);
    if (it != container_.complex_map_.end()) {
      if (!serialized_size_complex_aggregated_member_xcdr2(encoding, size, it, optional,
                                                           extensibility, mutable_running_total,
                                                           nested(ext))) {
        return false;
      }
    } else if (!serialized_size_complex_aggregated_member_xcdr2_default(encoding, size, member_type, optional,
                                                                        extensibility, mutable_running_total,
                                                                        nested(ext))) {
      return false;
    }
  }

  if (extensibility == DDS::MUTABLE) {
    serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
  }
  return true;
}

bool DynamicDataImpl::serialize_structure_xcdr2(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const
{
  const DDS::ExtensibilityKind extensibility = type_desc_->extensibility_kind();
  const bool struct_has_explicit_keys = has_explicit_keys(type_);

  // Delimiter
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    if (!serialized_size_i(encoding, total_size, ext) || !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  // Members
  const CORBA::ULong member_count = type_->get_member_count();
  for (CORBA::ULong i = 0; i < member_count; ++i) {
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member_by_index(dtm, i) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var md;
    if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }

    if (exclude_member(ext, md->is_key(), struct_has_explicit_keys)) {
      continue;
    }

    const DDS::MemberId id = md->id();
    const CORBA::Boolean optional = md->is_optional();
    const CORBA::Boolean must_understand = md->is_must_understand() || md->is_key();
    const DDS::DynamicType_var member_type = get_base_type(md->type());
    const TypeKind member_tk = member_type->get_kind();

    if (is_basic(member_tk) || member_tk == TK_ENUM || member_tk == TK_BITMASK) {
      if (!serialize_basic_struct_member_xcdr2(ser, id, member_type, optional,
                                               must_understand, extensibility)) {
        return false;
      }
      continue;
    } else if (member_tk == TK_SEQUENCE) {
      DDS::TypeDescriptor_var member_td;
      if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
        return false;
      }
      const DDS::DynamicType_var elem_type = get_base_type(member_td->element_type());
      const TypeKind elem_tk = elem_type->get_kind();
      if (is_basic(elem_tk) || elem_tk == TK_ENUM || elem_tk == TK_BITMASK) {
        if (!serialize_sequence_struct_member_xcdr2(ser, id, elem_tk, optional,
                                                    must_understand, extensibility, nested(ext))) {
          return false;
        }
        continue;
      }
    }

    const_complex_iterator it = container_.complex_map_.find(id);
    if (it != container_.complex_map_.end()) {
      if (!serialize_complex_aggregated_member_xcdr2(ser, it, optional,
                                                     must_understand, extensibility, nested(ext))) {
        return false;
      }
    } else if (!serialize_complex_aggregated_member_xcdr2_default(ser, id, member_type, optional,
                                                                  must_understand, extensibility, nested(ext))) {
      return false;
    }
  }
  return true;
}

bool DynamicDataImpl::serialized_size_structure_xcdr1(
  const DCPS::Encoding& /*encoding*/, size_t& /*size*/, DCPS::Sample::Extent /*ext*/) const
{
  // TODO: Support Final & Mutable extensibility?
  return false;
}

bool DynamicDataImpl::serialize_structure_xcdr1(DCPS::Serializer& /*ser*/, DCPS::Sample::Extent) const
{
  // TODO: Support only Final & Mutable extensibility?
  return false;
}

bool DynamicDataImpl::serialized_size_structure(const DCPS::Encoding& encoding,
                                                size_t& size, DCPS::Sample::Extent ext) const
{
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    return serialized_size_structure_xcdr2(encoding, size, ext);
  } else if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_1) {
    return serialized_size_structure_xcdr1(encoding, size, ext);
  }
  return false;
}

bool DynamicDataImpl::serialize_structure(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    return serialize_structure_xcdr2(ser, ext);
  } else if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_1) {
    return serialize_structure_xcdr1(ser, ext);
  }
  return false;
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

// Get discriminator value from the data container.
// The discriminator data must be present in either single map or complex map.
// TODO(sonndinh): Merge into the more general get_discriminator_value function?
bool DynamicDataImpl::get_discriminator_value(
  CORBA::Long& value, const_single_iterator single_it, const_complex_iterator complex_it,
  const DDS::DynamicType_var& disc_type) const
{
  if (single_it != container_.single_map_.end()) {
    read_disc_from_single_map(value, disc_type, single_it);
  } else { // Find in complex map
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
  }
  return true;
}

bool DynamicDataImpl::serialized_size_discriminator_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, const DDS::DynamicType_var& disc_type,
  DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
{
  if (extensibility == DDS::MUTABLE) {
    serialized_size_parameter_id(encoding, size, mutable_running_total);
  }
  const TypeKind disc_tk = disc_type->get_kind();
  if (is_primitive(disc_tk)) {
    return serialized_size_primitive_member(encoding, size, disc_tk);
  }
  return serialized_size_enum(encoding, size, disc_type);
}

bool DynamicDataImpl::serialize_discriminator_member_xcdr2(
  DCPS::Serializer& ser, CORBA::Long value, const DDS::DynamicType_var& disc_type,
  DDS::ExtensibilityKind extensibility) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  const TypeKind disc_tk = disc_type->get_kind();
  if (extensibility == DDS::MUTABLE) {
    size_t disc_size = 0;
    if (is_primitive(disc_tk)) {
      serialized_size_primitive_member(encoding, disc_size, disc_tk);
    } else {
      serialized_size_enum(encoding, disc_size, disc_type);
    }
    // Use member Id 0 for discriminator?
    if (!ser.write_parameter_id(0, disc_size, false)) {
      return false;
    }
  }

  switch (disc_tk) {
  case TK_BOOLEAN:
    return ser << static_cast<CORBA::Boolean>(value);
  case TK_BYTE:
    return ser << static_cast<CORBA::Octet>(value);
  case TK_CHAR8:
    return ser << static_cast<CORBA::Char>(value);
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return ser << static_cast<CORBA::WChar>(value);
#endif
  case TK_INT8:
    return ser << static_cast<CORBA::Int8>(value);
  case TK_UINT8:
    return ser << static_cast<CORBA::UInt8>(value);
  case TK_INT16:
    return ser << static_cast<CORBA::Short>(value);
  case TK_UINT16:
    return ser << static_cast<CORBA::UShort>(value);
  case TK_INT32:
    return ser << value;
  case TK_UINT32:
    return ser << static_cast<CORBA::ULong>(value);
  case TK_INT64:
    return ser << static_cast<CORBA::LongLong>(value);
  case TK_UINT64:
    return ser << static_cast<CORBA::ULongLong>(value);
  case TK_ENUM: {
    DDS::TypeDescriptor_var td;
    if (disc_type->get_descriptor(td) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::ULong bitbound = td->bound()[0];
    if (bitbound >= 1 && bitbound <= 8) {
      return ser << static_cast<CORBA::Int8>(value);
    } else if (bitbound >= 9 && bitbound <= 16) {
      return ser << static_cast<CORBA::Short>(value);
    } else if (bitbound >= 17 && bitbound <= 32) {
      return ser << value;
    }
  }
  }
  return false;
}

// Generic serialization functions for DynamicData.
bool DynamicDataImpl::serialized_size_dynamic_data(DDS::DynamicData_ptr data,
                                                   const DCPS::Encoding& encoding, size_t& size) const
{
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  switch (base_type->get_kind()) {
  case TK_STRUCTURE:
  case TK_UNION:
    return serialized_size_dynamic_data_members(data, encoding, size);
  }
  return false;
}

// Only called for aggregated types (struct and union).
bool DynamicDataImpl::serialized_size_dynamic_data_members(DDS::DynamicData_ptr data,
  const DCPS::Encoding& encoding, size_t& size) const
{
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);

  for (CORBA::ULong i = 0; i < data->get_item_count(); ++i) {
    const DDS::MemberId member_id = data->get_member_id_at_index(i);
    if (member_id == MEMBER_ID_INVALID) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::serialized_size_dynamic_data_members:"
                   " Failed to get member ID at index %u\n", i));
      }
      return false;
    }
    if (!serialized_size_dynamic_data_member(data, encoding, size, member_id)) {
      if (log_level >= LogLevel:: Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::serialized_size_dynamic_data_members:"
                   " Failed to compute serialized size for member ID %u\n", member_id));
      }
      return false;
    }
  }
  return true;
}

// Serialized size of a member of an aggregated type. The dynamic data instance
// of the containing type is passed.
bool DynamicDataImpl::serialized_size_dynamic_data_member(DDS::DynamicData_ptr data,
  const DCPS::Encoding& encoding, size_t& size, DDS::MemberId member_id) const
{
  const DDS::DynamicType_var type = data->type();

  DDS::DynamicTypeMember_var dtm;
  if (type->get_member(dtm, member_id) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var md;
  if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var member_type = md->type();
  const DDS::DynamicType_var member_base_type = get_base_type(member_type);
  const TypeKind member_tk = member_base_type->get_kind();
  TypeKind treat_as = member_tk;

  // TODO(sonndinh): Add size of the member header.
  if (member_tk == TK_ENUM && enum_bound(member_base_type, treat_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (member_tk == TK_BITMASK && bitmask_bound(member_base_type, treat_as) != DDS::RETCODE_OK) {
    return false;
  }

  switch (treat_as) {
  case TK_INT8:
  case TK_UINT8:
  case TK_INT16:
  case TK_UINT16:
  case TK_INT32:
  case TK_UINT32:
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_CHAR8:
  case TK_CHAR16:
  case TK_BYTE:
  case TK_BOOLEAN:
    return serialized_size_primitive_member(encoding, size, member_tk);
  case TK_STRING8: {
    CORBA::String_var str;
    if (data->get_string_value(str, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    serialized_size_string_common(encoding, size, str);
    return true;
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    CORBA::WString_var wstr;
    if (data->get_wstring_value(wstr, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    serialized_size_string_common(encoding, size, wstr);
    return true;
  }
#endif
  case TK_STRUCTURE:
  case TK_UNION: {
    DDS::DynamicData_var member_dd;
    if (data->get_complex_value(member_dd, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    return serialized_size_dynamic_data(member_dd, encoding, size);
  }
  case TK_ARRAY:
  case TK_SEQUENCE:
  default:
    return false;
  }
}

// Serialized size of a member whose data is found in the backing store.
bool DynamicDataImpl::serialized_size_member_backing_store_xcdr2(const DDS::MemberDescriptor_var& md,
  const DCPS::Encoding& encoding, size_t& size) const
{
  if (!backing_store_) {
    return false;
  }

  const DDS::DynamicType_var member_type = md->type();
  const DDS::DynamicType_var type = get_base_type(member_type);
  const DDS::MemberId id = md->id();
  const TypeKind tk = type->get_kind();
  TypeKind treat_as = tk;

  if (tk == TK_ENUM && enum_bound(type, treat_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (tk == TK_BITMASK && bitmask_bound(type, treat_as) != DDS::RETCODE_OK) {
    return false;
  }

  if (is_primitive(treat_as)) {
    return serialized_size_primitive_member(encoding, size, treat_as);
  }

  switch (treat_as) {
  case TK_STRING8: {
    CORBA::String_var str;
    if (backing_store_->get_string_value(str, id) != DDS::RETCODE_OK) {
      return false;
    }
    serialized_size_string_common(encoding, size, str);
    return true;
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    CORBA::WString_var wstr;
    if (backing_store_->get_wstring_value(wstr, id) != DDS::RETCODE_OK) {
      return false;
    }
    serialized_size_string_common(encoding, size, wstr);
    return true;
  }
#endif
  case TK_STRUCTURE: {

  }
  case TK_UNION:
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
  default:
    return false;
  }
}

bool DynamicDataImpl::serialized_size_selected_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, const DDS::MemberDescriptor_var& selected_md, //DDS::MemberId selected_id,
  DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
{
  // DDS::DynamicTypeMember_var selected_dtm;
  // if (type_->get_member(selected_dtm, selected_id) != DDS::RETCODE_OK) {
  //   return false;
  // }
  // DDS::MemberDescriptor_var selected_md;
  // if (selected_dtm->get_descriptor(selected_md) != DDS::RETCODE_OK) {
  //   return false;
  // }
  DDS::DynamicType_var selected_type = get_base_type(selected_md->type());
  const bool optional = selected_md->is_optional();
  const DDS::MemberId selected_id = selected_md->id();

  const_single_iterator single_it = container_.single_map_.find(selected_id);
  if (single_it != container_.single_map_.end()) {
    return serialized_size_single_aggregated_member_xcdr2(encoding, size, single_it, selected_type, optional,
                                                          extensibility, mutable_running_total);
  }

  const_sequence_iterator seq_it = container_.sequence_map_.find(selected_id);
  if (seq_it != container_.sequence_map_.end()) {
    DDS::TypeDescriptor_var selected_td;
    if (selected_type->get_descriptor(selected_td) != DDS::RETCODE_OK) {
      return false;
    }
    const TypeKind elem_tk = get_base_type(selected_td->element_type())->get_kind();
    serialized_size_sequence_aggregated_member_xcdr2(encoding, size, seq_it, elem_tk, optional,
                                                     extensibility, mutable_running_total);
    return true;
  }

  const_complex_iterator complex_it = container_.complex_map_.find(selected_id);
  if (complex_it != container_.complex_map_.end()) {
    return serialized_size_complex_aggregated_member_xcdr2(encoding, size, complex_it, optional,
                                                           extensibility, mutable_running_total,
                                                           DCPS::Sample::Full);
  }

  // TODO(sonndinh): Serialized size of the member from the backing store.
  return false;
}

bool DynamicDataImpl::serialize_selected_member_xcdr2(DCPS::Serializer& ser,
  DDS::MemberId selected_id, DDS::ExtensibilityKind extensibility) const
{
  DDS::DynamicTypeMember_var selected_dtm;
  if (type_->get_member(selected_dtm, selected_id) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var selected_md;
  if (selected_dtm->get_descriptor(selected_md) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::DynamicType_var selected_type = get_base_type(selected_md->type());
  const bool optional = selected_md->is_optional();
  const bool must_understand = selected_md->is_must_understand();

  const_single_iterator single_it = container_.single_map_.find(selected_id);
  if (single_it != container_.single_map_.end()) {
    return serialize_single_aggregated_member_xcdr2(ser, single_it, selected_type, optional,
                                                    must_understand, extensibility);
  }

  const_sequence_iterator seq_it = container_.sequence_map_.find(selected_id);
  if (seq_it != container_.sequence_map_.end()) {
    DDS::TypeDescriptor_var selected_td;
    if (selected_type->get_descriptor(selected_td) != DDS::RETCODE_OK) {
      return false;
    }
    const TypeKind elem_tk = get_base_type(selected_td->element_type())->get_kind();
    return serialize_sequence_aggregated_member_xcdr2(ser, seq_it, elem_tk, optional,
                                                      must_understand, extensibility);
  }

  const_complex_iterator complex_it = container_.complex_map_.find(selected_id);
  if (complex_it != container_.complex_map_.end()) {
    return serialize_complex_aggregated_member_xcdr2(ser, complex_it, optional,
                                                     must_understand, extensibility,
                                                     DCPS::Sample::Full);
  }
  return false;
}

bool DynamicDataImpl::serialized_size_union_xcdr2(const DCPS::Encoding& encoding,
                                                  size_t& size, DCPS::Sample::Extent ext) const
{
  if (ext == DCPS::Sample::KeyOnly && !has_explicit_keys(type_)) {
    // nothing is serialized (not even a delimiter) for key-only serialization when there is no @key
    return true;
  }

  const DDS::ExtensibilityKind extensibility = type_desc_->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    serialized_size_delimiter(encoding, size);
  }

  size_t mutable_running_total = 0;
  DDS::DynamicType_var disc_type = get_base_type(type_desc_->discriminator_type());
  if (!serialized_size_discriminator_member_xcdr2(encoding, size, disc_type,
                                                  extensibility, mutable_running_total)) {
    return false;
  }

  if (ext != DCPS::Sample::Full) {
    serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
    return true;
  }

  const_single_iterator single_it;
  const_complex_iterator complex_it;
  const bool has_disc = has_discriminator_value(single_it, complex_it);
  CORBA::Long disc_value;
  if (has_disc) {
    if (!get_discriminator_value(single_it, complex_it, disc_value, disc_type)) {
      return false;
    }
  } else if (!set_default_discriminator_value(disc_value, disc_type)) {
    return false;
  }

  bool select_member = false;
  DDS::MemberDescriptor_var selected_md;
  const DDS::ReturnCode_t rc =
    get_selected_union_branch(disc_value, select_member, selected_md);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::serialized_size_union_xcdr2:"
                 " get_selected_union_branch failed: %C\n", retcode_to_string(rc)));
    }
    return false;
  }

  if (select_member) {
    DDS::DynamicType_var selected_type = get_base_type(selected_md->type());
    const bool optional = selected_md->is_optional();
    if (!has_disc) {
      if (!serialized_size_complex_aggregated_member_xcdr2_default(encoding, size, selected_type,
            optional, extensibility, mutable_running_total, DCPS::Sample::Full)) {
        return false;
      }
    } else {
      if (!serialized_size_selected_member_xcdr2(encoding, size, selected_md,
                                                 extensibility, mutable_running_total)) {
        return false;
      }
    }
  }
  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
  return true;
}

bool DynamicDataImpl::serialize_union_xcdr2(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const
{
  if (ext == DCPS::Sample::KeyOnly && !has_explicit_keys(type_)) {
    // nothing is serialized (not even a delimiter) for key-only serialization when there is no @key
    return true;
  }

  const DDS::ExtensibilityKind extensibility = type_desc_->extensibility_kind();

  // Delimiter
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    if (!serialized_size_i(encoding, total_size, ext) || !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  const_single_iterator single_it = container_.single_map_.find(DISCRIMINATOR_ID);
  const_complex_iterator complex_it = container_.complex_map_.find(DISCRIMINATOR_ID);
  const bool has_disc = single_it != container_.single_map_.end() ||
    complex_it != container_.complex_map_.end();
  const DDS::MemberId selected_id = find_selected_member();
  DDS::DynamicType_var disc_type = get_base_type(type_desc_->discriminator_type());
  const TypeKind disc_tk = disc_type->get_kind();

  // Read the discriminator value if the user already set it. Otherwise,
  // set it to the default value of the corresponding type.
  CORBA::Long disc_value;
  if (has_disc) {
    if (!get_discriminator_value(disc_value, single_it, complex_it, disc_type)) {
      return false;
    }
  } else if (!set_default_discriminator_value(disc_value, disc_type)) {
    return false;
  }

  if (selected_id == MEMBER_ID_INVALID) {
    // If the defined discriminator value selects a member, serialize the member with
    // its default value. Otherwise, serialize only the discriminator.
    bool found_selected_member = false;
    DDS::MemberDescriptor_var selected_md;
    const DDS::ReturnCode_t rc =
      get_selected_union_branch(disc_value, found_selected_member, selected_md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::serialize_union_xcdr2:"
                   " get_selected_union_branch failed: %C\n", retcode_to_string(rc)));
      }
      return false;
    }
    // Discriminator
    if (!serialize_discriminator_member_xcdr2(ser, disc_value, disc_type, extensibility)) {
      return false;
    }
    if (ext == DCPS::Sample::KeyOnly) {
      return true;
    }
    // Selected member
    if (found_selected_member) {
      DDS::DynamicType_var selected_type = get_base_type(selected_md->type());
      const DDS::MemberId id = selected_md->id();
      const bool optional = selected_md->is_optional();
      const bool must_understand = selected_md->is_must_understand();
      return serialize_complex_aggregated_member_xcdr2_default(ser, id, selected_type, optional,
                                                               must_understand, extensibility,
                                                               DCPS::Sample::Full);
    }
    return true;
  }

  // Both discriminator and a selected member exist in the data container.
  if (single_it != container_.single_map_.end()) {
    if (extensibility == DDS::MUTABLE) {
      size_t disc_size = 0;
      if (is_primitive(disc_tk)) {
        serialized_size_primitive_member(encoding, disc_size, disc_tk);
      } else { // Enum is the only other type can be used for discriminator
        serialized_size_enum(encoding, disc_size, disc_type);
      }
      // Discriminator always has Id 0?
      if (!ser.write_parameter_id(0, disc_size, false)) {
        return false;
      }
    }
    if (!serialize_single_value(ser, single_it->second)) {
      return false;
    }
  } else {
    if (extensibility == DDS::MUTABLE) {
      size_t disc_size = 0;
      serialized_size_complex_member_i(encoding, disc_size, complex_it->first, DCPS::Sample::Full);
      if (!ser.write_parameter_id(0, disc_size, false)) {
        return false;
      }
    }
    if (!serialize_complex_member_i(ser, complex_it->first, DCPS::Sample::Full)) {
      return false;
    }
  }
  return ext == DCPS::Sample::KeyOnly ||
    serialize_selected_member_xcdr2(ser, selected_id, extensibility);
}

bool DynamicDataImpl::serialized_size_union_xcdr1(const DCPS::Encoding& /*encoding*/,
                                                  size_t& /*size*/, DCPS::Sample::Extent) const
{
  // TODO:
  return false;
}

bool DynamicDataImpl::serialize_union_xcdr1(DCPS::Serializer& /*ser*/, DCPS::Sample::Extent) const
{
  // TODO:
  return false;
}

bool DynamicDataImpl::serialized_size_union(const DCPS::Encoding& encoding,
                                            size_t& size, DCPS::Sample::Extent ext) const
{
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    return serialized_size_union_xcdr2(encoding, size, ext);
  } else if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_1) {
    return serialized_size_union_xcdr1(encoding, size, ext);
  }
  return false;
}

bool DynamicDataImpl::serialize_union(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    return serialize_union_xcdr2(ser, ext);
  } else if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_1) {
    return serialize_union_xcdr1(ser, ext);
  }
  return false;
}

bool DynamicDataImpl::serialized_size_i(const DCPS::Encoding& encoding, size_t& size, DCPS::Sample::Extent ext) const
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
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
  case TK_ENUM:
    return serialized_size_enum(encoding, size, type_);
  case TK_BITMASK:
    return serialized_size_bitmask(encoding, size, type_);
  case TK_STRING8:
    return serialized_size_string(encoding, size);
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
    return serialized_size_wstring(encoding, size);
#endif
  case TK_STRUCTURE:
    return serialized_size_structure(encoding, size, ext);
  case TK_UNION:
    return serialized_size_union(encoding, size, ext);
  case TK_SEQUENCE:
    return serialized_size_sequence(encoding, size, ext);
  case TK_ARRAY:
    return serialized_size_array(encoding, size, ext);
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::serialized_size_i: Serialization of map types is not supported\n"));
    }
  }
  return false;
}

bool DynamicDataImpl::serialize_i(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case TK_INT32:
    return serialize_primitive_value(ser, CORBA::Long());
  case TK_UINT32:
    return serialize_primitive_value(ser, CORBA::ULong());
  case TK_INT8:
    return serialize_primitive_value(ser, ACE_OutputCDR::from_int8(CORBA::Int8()));
  case TK_UINT8:
    return serialize_primitive_value(ser, ACE_OutputCDR::from_uint8(CORBA::UInt8()));
  case TK_INT16:
    return serialize_primitive_value(ser, CORBA::Short());
  case TK_UINT16:
    return serialize_primitive_value(ser, CORBA::UShort());
  case TK_INT64:
    return serialize_primitive_value(ser, CORBA::LongLong());
  case TK_UINT64:
    return serialize_primitive_value(ser, CORBA::ULongLong());
  case TK_FLOAT32:
    return serialize_primitive_value(ser, CORBA::Float());
  case TK_FLOAT64:
    return serialize_primitive_value(ser, CORBA::Double());
  case TK_FLOAT128:
    return serialize_primitive_value(ser, CORBA::LongDouble());
  case TK_CHAR8:
    return serialize_primitive_value(ser, ACE_OutputCDR::from_char(CORBA::Char()));
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return serialize_primitive_value(ser, ACE_OutputCDR::from_wchar(CORBA::WChar()));
#endif
  case TK_BYTE:
    return serialize_primitive_value(ser, ACE_OutputCDR::from_octet(CORBA::Octet()));
  case TK_BOOLEAN:
    return serialize_primitive_value(ser, ACE_OutputCDR::from_boolean(CORBA::Boolean()));
  case TK_ENUM:
    return serialize_enum_value(ser);
  case TK_BITMASK:
    return serialize_bitmask_value(ser);
  case TK_STRING8:
    return serialize_string_value(ser);
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
    return serialize_wstring_value(ser);
#endif
  case TK_STRUCTURE:
    return serialize_structure(ser, ext);
  case TK_UNION:
    return serialize_union(ser, ext);
  case TK_SEQUENCE:
    return serialize_sequence(ser, ext);
  case TK_ARRAY:
    return serialize_array(ser, ext);
  case TK_MAP:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::serialize_i: Serialization of map types is not supported\n"));
    }
  }
  return false;
}

bool DynamicDataImpl::serialize_single_value(DCPS::Serializer& ser, const SingleValue& sv) const
{
  switch (sv.kind_) {
  case TK_INT32:
    return ser << sv.get<CORBA::Long>();
  case TK_UINT32:
    return ser << sv.get<CORBA::ULong>();
  case TK_INT8:
    return ser << sv.get<ACE_OutputCDR::from_int8>();
  case TK_UINT8:
    return ser << sv.get<ACE_OutputCDR::from_uint8>();
  case TK_INT16:
    return ser << sv.get<CORBA::Short>();
  case TK_UINT16:
    return ser << sv.get<CORBA::UShort>();
  case TK_INT64:
    return ser << sv.get<CORBA::LongLong>();
  case TK_UINT64:
    return ser << sv.get<CORBA::ULongLong>();
  case TK_FLOAT32:
    return ser << sv.get<CORBA::Float>();
  case TK_FLOAT64:
    return ser << sv.get<CORBA::Double>();
  case TK_FLOAT128:
    return ser << sv.get<CORBA::LongDouble>();
  case TK_CHAR8:
    return ser << sv.get<ACE_OutputCDR::from_char>();
  case TK_BYTE:
    return ser << sv.get<ACE_OutputCDR::from_octet>();
  case TK_BOOLEAN:
    return ser << sv.get<ACE_OutputCDR::from_boolean>();
  case TK_STRING8:
    return ser << sv.get<const char*>();
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return ser << sv.get<ACE_OutputCDR::from_wchar>();
  case TK_STRING16:
    return ser << sv.get<const CORBA::WChar*>();
#endif
  default:
    return false;
  }
}

} // namespace XTypes

namespace DCPS {

bool serialized_size(const Encoding& encoding, size_t& size, const XTypes::DynamicDataImpl& data)
{
  return data.serialized_size_i(encoding, size, Sample::Full);
}

bool operator<<(Serializer& ser, const XTypes::DynamicDataImpl& data)
{
  return data.serialize_i(ser, Sample::Full);
}

bool serialized_size(const Encoding& encoding, size_t& size, const KeyOnly<const XTypes::DynamicDataImpl>& key)
{
  return key.value.serialized_size_i(encoding, size, Sample::KeyOnly);
}

bool operator<<(Serializer& ser, const KeyOnly<const XTypes::DynamicDataImpl>& key)
{
  return key.value.serialize_i(ser, Sample::KeyOnly);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
