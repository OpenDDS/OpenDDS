/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicDataImpl.h"
#  include "DynamicTypeMemberImpl.h"

#  include <dds/DdsDynamicDataSeqTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicDataImpl::DynamicDataImpl(DDS::DynamicType_ptr type)
  : DynamicDataBase(type)
  , container_(type_, this)
{}

DDS::DynamicType_ptr DynamicDataImpl::type()
{
  return DDS::DynamicType::_duplicate(type_);
}

DDS::ReturnCode_t DynamicDataImpl::set_descriptor(MemberId, DDS::MemberDescriptor*)
{
  return DDS::RETCODE_UNSUPPORTED;
}

CORBA::Boolean DynamicDataImpl::equals(DDS::DynamicData_ptr /*other*/)
{
  // FUTURE: Implement this.
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataImpl::equals: Not implemented\n"));
  return false;
}

DDS::MemberId DynamicDataImpl::get_member_id_at_index(ACE_CDR::ULong /*index*/)
{
  // Not sure if this should be supported for the writing direction.
  // The index is counted from the serialized data but when this is
  // called the user may not finish setting all the fields they want yet.
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataImpl::get_member_id_at_index: Not implemented\n"));
  return MEMBER_ID_INVALID;
}

ACE_CDR::ULong DynamicDataImpl::get_item_count()
{
  return static_cast<ACE_CDR::ULong>(
    container_.single_map_.size() + container_.sequence_map_.size() + container_.complex_map_.size());
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

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_int8& value)
{
  // The same member might be already written to complex_map_.
  // Make sure there is only one entry for each member.
  container_.complex_map_.erase(id);
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_uint8& value)
{
  container_.complex_map_.erase(id);
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_char& value)
{
  container_.complex_map_.erase(id);
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_octet& value)
{
  container_.complex_map_.erase(id);
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_boolean& value)
{
  container_.complex_map_.erase(id);
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

#ifdef DDS_HAS_WCHAR
bool DynamicDataImpl::insert_single(DDS::MemberId id, const ACE_OutputCDR::from_wchar& value)
{
  container_.complex_map_.erase(id);
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}
#endif

template<typename SingleType>
bool DynamicDataImpl::insert_single(DDS::MemberId id, const SingleType& value)
{
  container_.complex_map_.erase(id);
  return container_.single_map_.insert(std::make_pair(id, value)).second;
}

bool DynamicDataImpl::insert_complex(DDS::MemberId id, const DDS::DynamicData_var& value)
{
  if (container_.single_map_.erase(id) == 0) {
    container_.sequence_map_.erase(id);
  }
  return container_.complex_map_.insert(std::make_pair(id, value)).second;
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
  : kind_(TK_STRING8), str_(ACE_OS::strdup(str))
{}

#ifdef DDS_HAS_WCHAR
DynamicDataImpl::SingleValue::SingleValue(ACE_OutputCDR::from_wchar value)
  : kind_(TK_CHAR16), active_(new(char16_) ACE_OutputCDR::from_wchar(value.val_))
{}

DynamicDataImpl::SingleValue::SingleValue(const CORBA::WChar* wstr)
  : kind_(TK_STRING16), wstr_(ACE_OS::strdup(wstr))
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
    ACE_OS::free((void*)str_);
    break;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    SINGLE_VALUE_DESTRUCT(from_wchar);
  case TK_STRING16:
    ACE_OS::free((void*)wstr_);
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

bool DynamicDataImpl::read_discriminator(CORBA::Long& disc_val, const DDS::DynamicType_var& disc_type,
                                         DataContainer::const_single_iterator it) const
{
  switch (disc_type->get_kind()) {
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
    disc_val = static_cast<CORBA::Long>(value);
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
  case TK_ENUM: {
    DDS::TypeDescriptor_var td;
    if (disc_type->get_descriptor(td) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::ULong bitbound = td->bound()[0];
    if (bitbound >= 1 && bitbound <= 8) {
      const ACE_OutputCDR::from_int8& value = it->second.get<ACE_OutputCDR::from_int8>();
      disc_val = static_cast<CORBA::Long>(value.val_);
    } else if (bitbound >= 9 && bitbound <= 16) {
      CORBA::Short value = it->second.get<CORBA::Short>();
      disc_val = static_cast<CORBA::Long>(value);
    } else {
      disc_val = it->second.get<CORBA::Long>();
    }
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
  DataContainer::const_single_iterator it = container_.single_map_.find(MEMBER_ID_INVALID);
  if (it == container_.single_map_.end()) {
    return false;
  }
  return read_discriminator(disc_val, type_, it);
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
  bool& has_disc, CORBA::Long& disc_val, const DDS::DynamicType_var& disc_type) const
{
  for (DataContainer::const_single_iterator single_it = container_.single_map_.begin();
       single_it != container_.single_map_.end(); ++single_it) {
    if (single_it->first == DISCRIMINATOR_ID) {
      has_disc = true;
      if (!read_discriminator(disc_val, disc_type, single_it)) {
        return false;
      }
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
        const DynamicDataImpl* dd_impl = dynamic_cast<const DynamicDataImpl*>(cmpl_it->second.in());
        if (!dd_impl || !dd_impl->read_discriminator(disc_val)) {
          return false;
        }
      } else {
        selected_id = cmpl_it->first;
      }
    }
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
    if (!is_valid_discriminator_type(MemberTypeKind)) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_value_to_union:"
                   " Type %C cannot be used for union discriminator\n",
                   typekind_to_string(MemberTypeKind)));
      }
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

      CORBA::Long disc_value;
      if (!cast_to_discriminator_value(disc_value, value)) {
        return false;
      }
      if (!validate_discriminator(disc_value, selected_md)) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_value_to_union:"
                     " Discriminator value %d does not select existing selected member (ID %d)",
                     disc_value, selected_id));
        }
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
    if (!find_selected_member_and_discriminator(selected_id, has_disc, disc_val,
                                                get_base_type(descriptor->discriminator_type()))) {
      return false;
    }

    // Prohibit writing another member if a member was already written.
    // Overwrite the same member is allowed.
    if (selected_id != MEMBER_ID_INVALID && selected_id != id) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
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
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const TypeKind elem_tk = elem_type->get_kind();

  if (elem_tk != ElementTypeKind && elem_tk != enum_or_bitmask) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
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

  return validate_member_id_collection(descriptor, id, collection_tk) &&
    insert_single(id, value);
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
      insert_single(id, value);
  } else {
    switch (tk) {
    case ValueTypeKind:
      good = is_primitive(tk) && id == MEMBER_ID_INVALID && insert_single(id, value);
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

  if (!good && DCPS::log_level >= DCPS::LogLevel::Notice) {
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
    DDS::TypeDescriptor_var descriptor;
    if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
      good = false;
      break;
    }
    const CORBA::ULong bound = descriptor->bound()[0];
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

  if (!good && DCPS::log_level >= DCPS::LogLevel::Notice) {
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
    DDS::TypeDescriptor_var descriptor;
    if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
      good = false;
      break;
    }
    const CORBA::ULong bit_bound = descriptor->bound()[0];
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

  if (!good && DCPS::log_level >= DCPS::LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_boolean_value:"
               " Failed to write boolean to DynamicData object of type %C\n",
               typekind_to_string(tk)));
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

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
DDS::ReturnCode_t DynamicDataImpl::get_simple_value_boolean(DCPS::Value& value,
                                                            DDS::MemberId id) const
{
  DataContainer::const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get<ACE_OutputCDR::from_boolean>().val_;
    return DDS::RETCODE_OK;
  }
  DataContainer::const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    const DynamicDataImpl* inner_dd = dynamic_cast<DynamicDataImpl*>(complex_it->second.in());
    if (!inner_dd) {
      return DDS::RETCODE_ERROR;
    }
    DataContainer::const_single_iterator inner_it =
      inner_dd->container_.single_map_.find(MEMBER_ID_INVALID);
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
  DataContainer::const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get<ACE_OutputCDR::from_char>().val_;
    return DDS::RETCODE_OK;
  }
  DataContainer::const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    const DynamicDataImpl* inner_dd = dynamic_cast<DynamicDataImpl*>(complex_it->second.in());
    if (!inner_dd) {
      return DDS::RETCODE_ERROR;
    }
    DataContainer::const_single_iterator inner_it =
      inner_dd->container_.single_map_.find(MEMBER_ID_INVALID);
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
  DataContainer::const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get<ValueType>();
    return DDS::RETCODE_OK;
  }
  DataContainer::const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    const DynamicDataImpl* inner_dd = dynamic_cast<DynamicDataImpl*>(complex_it->second.in());
    if (!inner_dd) {
      return DDS::RETCODE_ERROR;
    }
    DataContainer::const_single_iterator inner_it =
      inner_dd->container_.single_map_.find(MEMBER_ID_INVALID);
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
  DataContainer::const_single_iterator single_it = container_.single_map_.find(id);
  if (single_it != container_.single_map_.end()) {
    value = single_it->second.get<const char*>();
    return DDS::RETCODE_OK;
  }
  DataContainer::const_complex_iterator complex_it = container_.complex_map_.find(id);
  if (complex_it != container_.complex_map_.end()) {
    // The string member has its own DynamicData object.
    const DynamicDataImpl* str_dd = dynamic_cast<const DynamicDataImpl*>(complex_it->second.in());
    if (!str_dd) {
      return DDS::RETCODE_ERROR;
    }
    const bool is_empty = str_dd->container_.single_map_.empty() &&
      str_dd->container_.complex_map_.empty();
    if (is_empty) { // The DynamicData object contains no data. Treat as an empty string.
      value = "";
      return DDS::RETCODE_OK;
    }
    CORBA::ULong largest_index;
    if (!str_dd->container_.get_largest_index_basic(largest_index)) {
      return DDS::RETCODE_ERROR;
    }
    const CORBA::ULong length = largest_index + 2;
    CORBA::String_var str = CORBA::string_alloc(length);
    ACE_OS::memset(str._retn(), 0, length);
    if (!str_dd->container_.reconstruct_string_value(str._retn())) {
      return DDS::RETCODE_ERROR;
    }
    value = str._retn();
    return DDS::RETCODE_OK;
  }
  return DDS::RETCODE_ERROR;
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
  default:
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::get_simple_value:"
                 " Member type %C is not supported by DCPS::Value\n",
                 typekind_to_string(member_kind)));
    }
  }
  return DDS::RETCODE_ERROR;
}
#endif

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
  const DDS::DynamicType_var value_type = value->type();
  if (!member_type || !value_type || !member_type->equals(value_type)) {
    return false;
  }
  return insert_complex(id, value);
}

bool DynamicDataImpl::set_complex_to_union(DDS::MemberId id, DDS::DynamicData_ptr value,
                                           const DDS::TypeDescriptor_var& descriptor)
{
  if (id == DISCRIMINATOR_ID) {
    DDS::DynamicType_var disc_type = get_base_type(descriptor->discriminator_type());
    const DDS::DynamicType_var value_type = value->type();
    if (!disc_type->equals(value_type)) {
      return false;
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
      CORBA::Long disc_val;
      const DynamicDataImpl* dd_impl = dynamic_cast<const DynamicDataImpl*>(value);
      if (!dd_impl || !dd_impl->read_discriminator(disc_val)) {
        return false;
      }
      if (!validate_discriminator(disc_val, selected_md)) {
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
    const DDS::DynamicType_var value_type = value->type();
    if (get_base_type(md->type())->equals(value_type)) {
      return false;
    }

    DDS::MemberId selected_id = MEMBER_ID_INVALID;
    bool has_disc = false;
    CORBA::Long disc_val;
    if (!find_selected_member_and_discriminator(selected_id, has_disc, disc_val,
                                                get_base_type(descriptor->discriminator_type()))) {
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
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
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
  const DDS::DynamicType_var value_type = value->type();
  if (!elem_type->equals(value_type)) {
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
    good = set_complex_to_union(id, value, descriptor);
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

  if (!good && DCPS::log_level >= DCPS::LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_complex_value:"
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
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_values_to_union:"
                 " Union discriminator cannot be a sequence\n"));
    }
    return false;
  }

  // Check the member type against the input type parameters.
  if (!check_seqmem_in_struct_and_union<ElementTypeKind>(id, enum_or_bitmask, lower, upper)) {
    return false;
  }

  DDS::TypeDescriptor_var td;
  if (type_->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }

  // If discriminator was already written, make sure it selects the target member.
  DDS::MemberId selected_id = MEMBER_ID_INVALID;
  bool has_disc = false;
  CORBA::Long disc_val;
  if (!find_selected_member_and_discriminator(selected_id, has_disc, disc_val,
                                              get_base_type(td->discriminator_type()))) {
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
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_values_to_union:"
                 " Already had an active member (%d)\n", selected_id));
    }
    return false;
  }
  return insert_sequence(id, value);
}

template<TypeKind ElementTypeKind>
bool DynamicDataImpl::check_seqmem_in_sequence_and_array(DDS::MemberId id, CORBA::ULong bound,
  TypeKind enum_or_bitmask, LBound lower, LBound upper) const
{
  if (!check_index_from_id(type_->get_kind(), id, bound)) {
    return false;
  }
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
  const CORBA::ULong bound = descriptor->bound()[0];
  return
    check_seqmem_in_sequence_and_array<ElementTypeKind>(id, bound, enum_or_bitmask, lower, upper) &&
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
  CORBA::ULong length = 1;
  const DDS::BoundSeq& bounds = descriptor->bound();
  for (CORBA::ULong i = 0; i < bounds.length(); ++i) {
    length *= bounds[i];
  }
  return
    check_seqmem_in_sequence_and_array<ElementTypeKind>(id, length, enum_or_bitmask, lower, upper) &&
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
    good = set_values_to_sequence<ElementTypeKind>(id, value, enum_or_bitmask, lower, upper);
    break;
  case TK_ARRAY:
    good = set_values_to_array<ElementTypeKind>(id, value, enum_or_bitmask, lower, upper);
    break;
  case TK_MAP:
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_sequence_values:"
                 " Map is currently not supported\n"));
    }
    return DDS::RETCODE_ERROR;
  default:
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::set_sequence_values:"
                 " Write to unsupported type (%C)\n", typekind_to_string(tk)));
    }
    return DDS::RETCODE_ERROR;
  }

  if (!good && DCPS::log_level >= DCPS::LogLevel::Notice) {
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

// Get largest index among elements of a sequence-like type written to the single map.
bool DynamicDataImpl::DataContainer::get_largest_single_index(CORBA::ULong& largest_index) const
{
  const TypeKind tk = type_->get_kind();
  if (tk != TK_STRING8 && tk != TK_STRING16 && tk != TK_SEQUENCE) {
    return false;
  }
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bound = descriptor->bound()[0];

  // Since ID is used as index in this implementation, the last element has largest index.
  // A different implementation (ID-to-index mapping) may need to iterate through all
  // stored elements to find the one with the largest index.
  return data_->get_index_from_id(single_map_.rbegin()->first, largest_index, bound);
}

// Get largest index among elements of a nesting sequence type written to the sequence map.
bool DynamicDataImpl::DataContainer::get_largest_sequence_index(CORBA::ULong& largest_index) const
{
  if (type_->get_kind() != TK_SEQUENCE) {
    return false;
  }
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bound = descriptor->bound()[0];
  return data_->get_index_from_id(sequence_map_.rbegin()->first, largest_index, bound);
}

// Get largest index among elements of a sequence-like type written to the complex map.
bool DynamicDataImpl::DataContainer::get_largest_complex_index(CORBA::ULong& largest_index) const
{
  const TypeKind tk = type_->get_kind();
  if (tk != TK_STRING8 && tk != TK_STRING16 && tk != TK_SEQUENCE) {
    return false;
  }
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bound = descriptor->bound()[0];
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

bool DynamicDataImpl::DataContainer::serialize_single_value(DCPS::Serializer& ser,
                                                            const SingleValue& sv) const
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

template<typename PrimitiveType>
bool DynamicDataImpl::DataContainer::serialize_primitive_value(DCPS::Serializer& ser,
                                                               PrimitiveType default_value) const
{
  const_single_iterator it = single_map_.find(MEMBER_ID_INVALID);
  if (it != single_map_.end()) {
    return serialize_single_value(ser, it->second);
  }

  // No data stored. Use default value.
  set_default_basic_value(default_value);
  return ser << default_value;
}

bool DynamicDataImpl::DataContainer::serialized_size_enum(const DCPS::Encoding& encoding,
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

bool DynamicDataImpl::DataContainer::serialize_enum_default_value(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_enum_value(DCPS::Serializer& ser) const
{
  const_single_iterator it = single_map_.find(MEMBER_ID_INVALID);
  if (it != single_map_.end()) {
    return serialize_single_value(ser, it->second);
  }
  return serialize_enum_default_value(ser, type_);
}

bool DynamicDataImpl::DataContainer::serialized_size_bitmask(const DCPS::Encoding& encoding,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_default_value(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_value(DCPS::Serializer& ser) const
{
  const_single_iterator it = single_map_.find(MEMBER_ID_INVALID);
  if (it != single_map_.end()) {
    return serialize_single_value(ser, it->second);
  }
  return serialize_bitmask_default_value(ser, type_);
}

bool DynamicDataImpl::DataContainer::reconstruct_string_value(CORBA::Char* str) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bound = descriptor->bound()[0];
  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    str[index] = it->second.get<ACE_OutputCDR::from_char>().val_;
  }
  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
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

bool DynamicDataImpl::DataContainer::serialized_size_string(const DCPS::Encoding& encoding,
                                                            size_t& size) const
{
  const bool is_empty = single_map_.empty() && complex_map_.empty();
  if (!is_empty) {
    CORBA::ULong largest_index;
    if (!get_largest_index_basic(largest_index)) {
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

bool DynamicDataImpl::DataContainer::serialize_string_value(DCPS::Serializer& ser) const
{
  const bool is_empty = single_map_.empty() && complex_map_.empty();
  if (!is_empty) {
    CORBA::ULong largest_index;
    if (!get_largest_index_basic(largest_index)) {
      return false;
    }
    const CORBA::ULong length = largest_index + 2;
    CORBA::String_var str = CORBA::string_alloc(length);
    ACE_OS::memset(str._retn(), 0, length);
    return reconstruct_string_value(str._retn()) && (ser << str._retn());
  }

  // This DynamicData object contains no data for the string.
  // Use default value which is an empty string.
  const CORBA::Char* empty_str = "";
  return ser << empty_str;
}

#ifdef DDS_HAS_WCHAR
bool DynamicDataImpl::DataContainer::reconstruct_wstring_value(CORBA::WChar* wstr) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bound = descriptor->bound()[0];
  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    wstr[index] = it->second.get<ACE_OutputCDR::from_wchar>().val_;
  }
  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
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

bool DynamicDataImpl::DataContainer::serialized_size_wstring(const DCPS::Encoding& encoding,
                                                             size_t& size) const
{
  const bool is_empty = single_map_.empty() && complex_map_.empty();
  if (!is_empty) {
    CORBA::ULong largest_index;
    if (!get_largest_index_basic(largest_index)) {
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

bool DynamicDataImpl::DataContainer::serialize_wstring_value(DCPS::Serializer& ser) const
{
  const bool is_empty = single_map_.empty() && complex_map_.empty();
  if (!is_empty) {
    CORBA::ULong largest_index;
    if (!get_largest_index_basic(largest_index)) {
      return false;
    }
    const CORBA::ULong length = largest_index + 2;
    CORBA::WString_var wstr = CORBA::wstring_alloc(length);
    ACE_OS::memset(wstr._retn(), 0, length * sizeof(CORBA::WChar));
    return reconstruct_wstring_value(wstr._retn()) && (ser << wstr._retn());
  }

  // Serialized empty wstring only has length.
  return ser << CORBA::ULong(0);
}
#endif

void DynamicDataImpl::DataContainer::serialized_size_primitive_sequence(const DCPS::Encoding& encoding,
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
void DynamicDataImpl::DataContainer::set_default_basic_value(CORBA::Long& value) const
{
  value = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(CORBA::ULong& value) const
{
  value = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(ACE_OutputCDR::from_int8& value) const
{
  value.val_ = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(ACE_OutputCDR::from_uint8& value) const
{
  value.val_ = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(CORBA::Short& value) const
{
  value = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(CORBA::UShort& value) const
{
  value = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(CORBA::LongLong& value) const
{
  value = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(CORBA::ULongLong& value) const
{
  value = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(CORBA::Float& value) const
{
  value = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(CORBA::Double& value) const
{
  value = 0;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(CORBA::LongDouble& value) const
{
#if ACE_SIZEOF_LONG_DOUBLE == 16
  value = 0;
#else
  ACE_OS::memset(value.ld, 0, 16);
#endif
}

void DynamicDataImpl::DataContainer::set_default_basic_value(ACE_OutputCDR::from_char& value) const
{
  value.val_ = '\0';
}

void DynamicDataImpl::DataContainer::set_default_basic_value(ACE_OutputCDR::from_octet& value) const
{
  value.val_ = 0x00;
}

void DynamicDataImpl::DataContainer::set_default_basic_value(const char*& value) const
{
  value = "";
}

void DynamicDataImpl::DataContainer::set_default_basic_value(ACE_OutputCDR::from_boolean& value) const
{
  value.val_ = false;
}

#ifdef DDS_HAS_WCHAR
void DynamicDataImpl::DataContainer::set_default_basic_value(ACE_OutputCDR::from_wchar& value) const
{
  value.val_ = '\0';
}

void DynamicDataImpl::DataContainer::set_default_basic_value(const CORBA::WChar*& value) const
{
  value = L"";
}
#endif

void DynamicDataImpl::DataContainer::set_default_primitive_values(DDS::Int8Seq& collection) const
{
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    ACE_OutputCDR::from_int8 value(0);
    set_default_basic_value(value);
    collection[i] = value.val_;
  }
}

void DynamicDataImpl::DataContainer::set_default_primitive_values(DDS::UInt8Seq& collection) const
{
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    ACE_OutputCDR::from_uint8 value(0);
    set_default_basic_value(value);
    collection[i] = value.val_;
  }
}

void DynamicDataImpl::DataContainer::set_default_primitive_values(DDS::CharSeq& collection) const
{
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    ACE_OutputCDR::from_char value('\0');
    set_default_basic_value(value);
    collection[i] = value.val_;
  }
}

void DynamicDataImpl::DataContainer::set_default_primitive_values(DDS::ByteSeq& collection) const
{
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    ACE_OutputCDR::from_octet value(0x00);
    set_default_basic_value(value);
    collection[i] = value.val_;
  }
}

void DynamicDataImpl::DataContainer::set_default_primitive_values(DDS::BooleanSeq& collection) const
{
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    ACE_OutputCDR::from_boolean value(false);
    set_default_basic_value(value);
    collection[i] = value.val_;
  }
}

#ifdef DDS_HAS_WCHAR
void DynamicDataImpl::DataContainer::set_default_primitive_values(DDS::WcharSeq& collection) const
{
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    ACE_OutputCDR::from_wchar value(0);
    set_default_basic_value(value);
    collection[i] = value.val_;
  }
}
#endif

template<typename CollectionType>
void DynamicDataImpl::DataContainer::set_default_primitive_values(CollectionType& collection) const
{
  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    set_default_basic_value(collection[i]);
  }
}

// Set elements for a sequence of primitive type
template<>
bool DynamicDataImpl::DataContainer::set_primitive_values(DDS::BooleanSeq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_boolean& /*elem_tag*/) const
{
  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_boolean>().val_;
  }

  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
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
bool DynamicDataImpl::DataContainer::set_primitive_values(DDS::ByteSeq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_octet& /*elem_tag*/) const
{
  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_octet>().val_;
  }

  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
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
bool DynamicDataImpl::DataContainer::set_primitive_values(DDS::Int8Seq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_int8& /*elem_tag*/) const
{
  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_int8>().val_;
  }

  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
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
bool DynamicDataImpl::DataContainer::set_primitive_values(DDS::UInt8Seq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_uint8& /*elem_tag*/) const
{
  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_uint8>().val_;
  }

  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
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
bool DynamicDataImpl::DataContainer::set_primitive_values(DDS::CharSeq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_char& /*elem_tag*/) const
{
  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_char>().val_;
  }

  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
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
bool DynamicDataImpl::DataContainer::set_primitive_values(DDS::WcharSeq& collection,
  CORBA::ULong bound, const ACE_OutputCDR::from_wchar& /*elem_tag*/) const
{
  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ACE_OutputCDR::from_wchar>().val_;
  }

  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
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
bool DynamicDataImpl::DataContainer::set_primitive_values(CollectionType& collection,
  CORBA::ULong bound, const ElementType& /*elem_tag*/) const
{
  for (const_single_iterator it = single_map_.begin(); it != single_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    collection[index] = it->second.get<ElementType>();
  }

  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
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
bool DynamicDataImpl::DataContainer::reconstruct_primitive_collection(
  CollectionType& collection, CORBA::ULong size, CORBA::ULong bound, const ElementType& elem_tag) const
{
  collection.length(size);
  set_default_primitive_values(collection);
  return set_primitive_values(collection, bound, elem_tag);
}

// Reconstruct the primitive sequence written by the user (elements that are not
// explicitly written are set to default value of the corresponding type).
// Then serialize the constructed sequence.
bool DynamicDataImpl::DataContainer::serialize_primitive_sequence(DCPS::Serializer& ser,
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
void DynamicDataImpl::DataContainer::serialized_size_string_common(const DCPS::Encoding& encoding,
  size_t& size, const char* str) const
{
  primitive_serialized_size_ulong(encoding, size);
  if (str) {
    size += ACE_OS::strlen(str) + 1; // Include null termination
  }
}

#ifdef DDS_HAS_WCHAR
void DynamicDataImpl::DataContainer::serialized_size_string_common(const DCPS::Encoding& encoding,
  size_t& size, const CORBA::WChar* wstr) const
{
  primitive_serialized_size_ulong(encoding, size);
  if (wstr) {
    size += ACE_OS::strlen(wstr) * DCPS::char16_cdr_size; // Not include null termination
  }
}
#endif

void DynamicDataImpl::DataContainer::serialized_size_string_common(const DCPS::Encoding& encoding,
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
bool DynamicDataImpl::DataContainer::serialized_size_generic_string_collection(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const DDS::MemberId id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_single_iterator single_it = single_map_.find(id);
      if (single_it != single_map_.end()) {
        serialized_size_string_common(encoding, size, single_it->second);
      } else if (!serialized_size_complex_member_i(encoding, size, id)) {
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
bool DynamicDataImpl::DataContainer::serialized_size_generic_string_sequence(
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
bool DynamicDataImpl::DataContainer::serialize_generic_string_collection(DCPS::Serializer& ser,
  const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const DDS::MemberId id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_single_iterator single_it = single_map_.find(id);
      if (single_it != single_map_.end()) {
        if (!serialize_single_value(ser, single_it->second)) {
          return false;
        }
      } else if (!serialize_complex_member_i(ser, id)) {
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
bool DynamicDataImpl::DataContainer::serialize_generic_string_sequence(DCPS::Serializer& ser,
  CORBA::ULong length, CORBA::ULong bound) const
{
  IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, bound)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    size_t total_size;
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
bool DynamicDataImpl::DataContainer::set_default_enum_values(CollectionType& collection,
  const DDS::DynamicType_var& enum_type) const
{
  // From Table 9, default enum value is the first enumerator.
  DDS::DynamicTypeMember_var first_dtm;
  if (enum_type->get_member_by_index(first_dtm, 0) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var first_md;
  if (first_dtm->get_descriptor(first_md) != DDS::RETCODE_OK) {
    return false;
  }

  for (CORBA::ULong i = 0; i < collection.length(); ++i) {
    collection[i] = static_cast<ElementType>(first_md->id());
  }
  return true;
}

template<typename ElementType, typename WrapElementType, typename CollectionType>
bool DynamicDataImpl::DataContainer::reconstruct_enum_collection(CollectionType& collection,
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
void DynamicDataImpl::DataContainer::serialized_size_enum_sequence_as_int8s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size_int8(encoding, size, length);
}

void DynamicDataImpl::DataContainer::serialized_size_enum_sequence(
  const DCPS::Encoding& encoding, size_t& size, const DDS::Int8Seq& seq) const
{
  serialized_size_enum_sequence_as_int8s(encoding, size, seq.length());
}

bool DynamicDataImpl::DataContainer::serialize_enum_sequence_as_ints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_enum_sequence_as_int8s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int8Seq enumseq;
  return reconstruct_enum_collection<CORBA::Int8>(enumseq, size, bound, enum_type, ACE_OutputCDR::from_int8(0)) &&
    serialize_enum_sequence_as_ints_i(ser, enumseq);
}

// Serialize enum sequence represented as int16 sequence
void DynamicDataImpl::DataContainer::serialized_size_enum_sequence_as_int16s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::Short(), length);
}

void DynamicDataImpl::DataContainer::serialized_size_enum_sequence(
  const DCPS::Encoding& encoding, size_t& size, const DDS::Int16Seq& seq) const
{
  serialized_size_enum_sequence_as_int16s(encoding, size, seq.length());
}

bool DynamicDataImpl::DataContainer::serialize_enum_sequence_as_ints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_enum_sequence_as_int16s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int16Seq enumseq;
  return reconstruct_enum_collection<CORBA::Short>(enumseq, size, bound, enum_type, CORBA::Short()) &&
    serialize_enum_sequence_as_ints_i(ser, enumseq);
}

// Serialize enum sequence represented as int32 sequence
void DynamicDataImpl::DataContainer::serialized_size_enum_sequence_as_int32s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::Long(), length);
}

void DynamicDataImpl::DataContainer::serialized_size_enum_sequence(
  const DCPS::Encoding& encoding, size_t& size, const DDS::Int32Seq& seq) const
{
  serialized_size_enum_sequence_as_int32s(encoding, size, seq.length());
}

bool DynamicDataImpl::DataContainer::serialize_enum_sequence_as_ints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_enum_sequence_as_int32s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int32Seq enumseq;
  return reconstruct_enum_collection<CORBA::Long>(enumseq, size, bound, enum_type, CORBA::Long()) &&
    serialize_enum_sequence_as_ints_i(ser, enumseq);
}

void DynamicDataImpl::DataContainer::serialized_size_enum_sequence(const DCPS::Encoding& encoding,
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

bool DynamicDataImpl::DataContainer::serialize_enum_sequence(DCPS::Serializer& ser,
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
void DynamicDataImpl::DataContainer::set_default_bitmask_values(CollectionType& col) const
{
  // Table 9 doesn't mention default value for bitmask. Use 0 as default here.
  for (CORBA::ULong i = 0; i < col.length(); ++i) {
    col[i] = 0;
  }
}

template<typename WrapElementType, typename CollectionType>
bool DynamicDataImpl::DataContainer::reconstruct_bitmask_collection(CollectionType& collection,
  CORBA::ULong size, CORBA::ULong bound, const WrapElementType& elem_tag) const
{
  collection.length(size);
  set_default_bitmask_values(collection);
  return set_primitive_values(collection, bound, elem_tag);
}

// Bitmask sequence represented as uint8 sequence.
void DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence_as_uint8s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size_uint8(encoding, size, length);
}

void DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::UInt8Seq& seq) const
{
  serialized_size_bitmask_sequence_as_uint8s(encoding, size, seq.length());
}

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence_as_uint8s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::UInt8Seq bitmask_seq;
  return reconstruct_bitmask_collection(bitmask_seq, size, bound, ACE_OutputCDR::from_uint8(0)) &&
    serialize_bitmask_sequence_as_uints_i(ser, bitmask_seq);
}

// Bitmask sequence represented as uint16 sequence
void DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence_as_uint16s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::UShort(), length);
}

void DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::UInt16Seq& seq) const
{
  serialized_size_bitmask_sequence_as_uint16s(encoding, size, seq.length());
}

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence_as_uint16s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::UInt16Seq bitmask_seq;
  return reconstruct_bitmask_collection(bitmask_seq, size, bound, CORBA::UShort()) &&
    serialize_bitmask_sequence_as_uints_i(ser, bitmask_seq);
}

// Bitmask sequence represented as uint32 sequence
void DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence_as_uint32s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::ULong(), length);
}

void DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::UInt32Seq& seq) const
{
  serialized_size_bitmask_sequence_as_uint32s(encoding, size, seq.length());
}

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence_as_uint32s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::UInt32Seq bitmask_seq;
  return reconstruct_bitmask_collection(bitmask_seq, size, bound, CORBA::ULong()) &&
    serialize_bitmask_sequence_as_uints_i(ser, bitmask_seq);
}

// Bitmask sequence represented as uint64 sequence
void DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence_as_uint64s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (length == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::ULongLong(), length);
}

void DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence(const DCPS::Encoding& encoding,
  size_t& size, const DDS::UInt64Seq& seq) const
{
  serialized_size_bitmask_sequence_as_uint64s(encoding, size, seq.length());
}

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence_as_uint64s(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound) const
{
  DDS::UInt64Seq bitmask_seq;
  return reconstruct_bitmask_collection(bitmask_seq, size, bound, CORBA::ULongLong()) &&
    serialize_bitmask_sequence_as_uints_i(ser, bitmask_seq);
}

void DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence(
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence(DCPS::Serializer& ser,
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
bool DynamicDataImpl::DataContainer::serialized_size_sequence_value(
  const DCPS::Encoding& encoding, size_t& size, const SequenceValue& sv) const
{
  switch (sv.elem_kind_) {
  case TK_INT32:
    serialized_size(encoding, size, sv.get<DDS::Int32Seq>());
    return true;
  case TK_UINT32:
    serialized_size(encoding, size, sv.get<DDS::UInt32Seq>());
    return true;
  case TK_INT8:
    serialized_size(encoding, size, sv.get<DDS::Int8Seq>());
    return true;
  case TK_UINT8:
    serialized_size(encoding, size, sv.get<DDS::UInt8Seq>());
    return true;
  case TK_INT16:
    serialized_size(encoding, size, sv.get<DDS::Int16Seq>());
    return true;
  case TK_UINT16:
    serialized_size(encoding, size, sv.get<DDS::UInt16Seq>());
    return true;
  case TK_INT64:
    serialized_size(encoding, size, sv.get<DDS::Int64Seq>());
    return true;
  case TK_UINT64:
    serialized_size(encoding, size, sv.get<DDS::UInt64Seq>());
    return true;
  case TK_FLOAT32:
    serialized_size(encoding, size, sv.get<DDS::Float32Seq>());
    return true;
  case TK_FLOAT64:
    serialized_size(encoding, size, sv.get<DDS::Float64Seq>());
    return true;
  case TK_FLOAT128:
    serialized_size(encoding, size, sv.get<DDS::Float128Seq>());
    return true;
  case TK_CHAR8:
    serialized_size(encoding, size, sv.get<DDS::CharSeq>());
    return true;
  case TK_BYTE:
    serialized_size(encoding, size, sv.get<DDS::ByteSeq>());
    return true;
  case TK_BOOLEAN:
    serialized_size(encoding, size, sv.get<DDS::BooleanSeq>());
    return true;
  case TK_STRING8:
    serialized_size(encoding, size, sv.get<DDS::StringSeq>());
    return true;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    serialized_size(encoding, size, sv.get<DDS::WcharSeq>());
    return true;
  case TK_STRING16:
    serialized_size(encoding, size, sv.get<DDS::WstringSeq>());
    return true;
#endif
  default:
    return false;
  }
}

bool DynamicDataImpl::DataContainer::serialize_sequence_value(DCPS::Serializer& ser,
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

// Helper function for serializing sequence or array where element type is sequence
// of basic or enumerated type.
bool DynamicDataImpl::DataContainer::get_index_to_id_map(IndexToIdMap& index_to_id,
                                                         CORBA::ULong bound) const
{
  for (const_sequence_iterator it = sequence_map_.begin(); it != sequence_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    index_to_id[index] = it->first;
  }
  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    index_to_id[index] = it->first;
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialized_size_complex_member_i(
  const DCPS::Encoding& encoding, size_t& size, DDS::MemberId id) const
{
  const DDS::DynamicData_var& dd_var = complex_map_.at(id);
  const DynamicDataImpl* data_impl = dynamic_cast<const DynamicDataImpl*>(dd_var.in());
  if (!data_impl) {
    return false;
  }
  return serialized_size(encoding, size, *data_impl);
}

template<typename SequenceType>
bool DynamicDataImpl::DataContainer::serialized_size_nested_basic_sequences(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id,
  SequenceType protoseq) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = sequence_map_.find(id);
      if (it != sequence_map_.end()) {
        serialized_size_sequence_value(encoding, size, it->second);
      } else if (!serialized_size_complex_member_i(encoding, size, id)) {
        return false;
      }
    } else { // Empty sequence
      protoseq.length(0);
      serialized_size(encoding, size, protoseq);
    }
  }
  return true;
}

template<typename SequenceType>
bool DynamicDataImpl::DataContainer::serialized_size_nesting_basic_sequence(
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

bool DynamicDataImpl::DataContainer::serialize_complex_member_i(DCPS::Serializer& ser,
                                                                DDS::MemberId id) const
{
  const DDS::DynamicData_var& dd_var = complex_map_.at(id);
  const DynamicDataImpl* data_impl = dynamic_cast<const DynamicDataImpl*>(dd_var.in());
  if (!data_impl) {
    return false;
  }
  return ser << *data_impl;
}

template<typename SequenceType>
bool DynamicDataImpl::DataContainer::serialize_nested_basic_sequences(DCPS::Serializer& ser,
  const IndexToIdMap& index_to_id, SequenceType protoseq) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = sequence_map_.find(id);
      if (it != sequence_map_.end()) {
        if (!serialize_sequence_value(ser, it->second)) {
          return false;
        }
      } else if (!serialize_complex_member_i(ser, id)) {
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
bool DynamicDataImpl::DataContainer::serialize_nesting_basic_sequence_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialized_size_nesting_basic_sequence(
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

bool DynamicDataImpl::DataContainer::serialize_nesting_basic_sequence(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialized_size_nested_enum_sequences(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = sequence_map_.find(id);
      if (it != sequence_map_.end()) {
        serialized_size_enum_sequence(encoding, size, it);
      } else if (!serialized_size_complex_member_i(encoding, size, id)) {
        return false;
      }
    } else {
      serialized_size_delimiter(encoding, size);
      primitive_serialized_size_ulong(encoding, size);
    }
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialized_size_nesting_enum_sequence(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (index_to_id.empty()) {
    return true;
  }
  return serialized_size_nested_enum_sequences(encoding, size, index_to_id);
}

bool DynamicDataImpl::DataContainer::serialize_nested_enum_sequences(
  DCPS::Serializer& ser, const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = sequence_map_.find(id);
      if (it != sequence_map_.end()) {
        if (!serialize_enum_sequence(ser, it)) {
          return false;
        }
      } else if (!serialize_complex_member_i(ser, id)) {
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

bool DynamicDataImpl::DataContainer::serialize_nesting_enum_sequence(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialized_size_nested_bitmask_sequences(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = sequence_map_.find(id);
      if (it != sequence_map_.end()) {
        serialized_size_bitmask_sequence(encoding, size, it);
      } else if (!serialized_size_complex_member_i(encoding, size, id)) {
        return false;
      }
    } else {
      serialized_size_delimiter(encoding, size);
      primitive_serialized_size_ulong(encoding, size);
    }
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialized_size_nesting_bitmask_sequence(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (index_to_id.empty()) {
    return true;
  }
  return serialized_size_nested_bitmask_sequences(encoding, size, index_to_id);
}

bool DynamicDataImpl::DataContainer::serialize_nested_bitmask_sequences(DCPS::Serializer& ser,
  const IndexToIdMap& index_to_id) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      const_sequence_iterator it = sequence_map_.find(id);
      if (it != sequence_map_.end()) {
        if (!serialize_bitmask_sequence(ser, it)) {
          return false;
        }
      } else if (!serialize_complex_member_i(ser, id)) {
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

bool DynamicDataImpl::DataContainer::serialize_nesting_bitmask_sequence(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialized_size_complex_member(const DCPS::Encoding& encoding,
  size_t& size, DDS::MemberId id, const DDS::DynamicType_var& elem_type) const
{
  if (id != MEMBER_ID_INVALID) {
    return serialized_size_complex_member_i(encoding, size, id);
  } else {
    return serialized_size(encoding, size, DynamicDataImpl(elem_type));
  }
}

bool DynamicDataImpl::DataContainer::serialized_size_complex_sequence(const DCPS::Encoding& encoding,
  size_t& size, const IndexToIdMap& index_to_id, const DDS::DynamicType_var& elem_type) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  if (index_to_id.empty()) {
    return true;
  }
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    if (!serialized_size_complex_member(encoding, size, index_to_id[i], elem_type)) {
      return false;
    }
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialize_complex_sequence_i(DCPS::Serializer& ser,
  const IndexToIdMap& index_to_id, const DDS::DynamicType_var& elem_type) const
{
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    const CORBA::ULong id = index_to_id[i];
    if (id != MEMBER_ID_INVALID) {
      if (!serialize_complex_member_i(ser, id)) {
        return false;
      }
    } else {
      if (!(ser << DynamicDataImpl(elem_type))) {
        return false;
      }
    }
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialize_complex_sequence(DCPS::Serializer& ser,
  CORBA::ULong size, CORBA::ULong bound, const DDS::DynamicType_var& elem_type) const
{
  IndexToIdMap index_to_id(size, MEMBER_ID_INVALID);
  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    index_to_id[index] = it->first;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_complex_sequence(encoding, total_size, index_to_id, elem_type) ||
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
  return serialize_complex_sequence_i(ser, index_to_id, elem_type);
}

bool DynamicDataImpl::DataContainer::get_index_to_id_from_complex(IndexToIdMap& index_to_id,
                                                                  CORBA::ULong bound) const
{
  CORBA::ULong length = 0;
  if (!complex_map_.empty()) {
    CORBA::ULong largest_index;
    if (!get_largest_complex_index(largest_index)) {
      return false;
    }
    length = largest_index + 1;
  }
  index_to_id.resize(length, MEMBER_ID_INVALID);
  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, bound)) {
      return false;
    }
    index_to_id[index] = it->first;
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialized_size_sequence(const DCPS::Encoding& encoding,
                                                              size_t& size) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bound = descriptor->bound()[0];

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }

  if (data_->is_basic_type(elem_tk) || elem_tk == TK_ENUM || elem_tk == TK_BITMASK) {
    const bool is_empty = single_map_.empty() && complex_map_.empty();
    CORBA::ULong length = 0;
    if (!is_empty) {
      CORBA::ULong largest_index;
      if (!get_largest_index_basic(largest_index)) {
        return false;
      }
      length = largest_index + 1;
    }
    if (data_->is_primitive(elem_tk)) {
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
    if (data_->is_basic_type(nested_elem_tk) || nested_elem_tk == TK_ENUM ||
        nested_elem_tk == TK_BITMASK) {
      const bool is_empty = sequence_map_.empty() && complex_map_.empty();
      CORBA::ULong length = 0;
      if (!is_empty) {
        CORBA::ULong largest_index;
        if (!get_largest_index_basic_sequence(largest_index)) {
          return false;
        }
        length = largest_index + 1;
      }
      IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
      if (!get_index_to_id_map(index_to_id, bound)) {
        return false;
      }
      if (data_->is_basic_type(nested_elem_tk)) {
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
  return serialized_size_complex_sequence(encoding, size, index_to_id, elem_type);
}

bool DynamicDataImpl::DataContainer::serialize_sequence(DCPS::Serializer& ser) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const CORBA::ULong bound = descriptor->bound()[0];

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }

  if (data_->is_basic_type(elem_tk) || elem_tk == TK_ENUM || elem_tk == TK_BITMASK) {
    const bool is_empty = single_map_.empty() && complex_map_.empty();
    CORBA::ULong length = 0;
    if (!is_empty) {
      CORBA::ULong largest_index;
      if (!get_largest_index_basic(largest_index)) {
        return false;
      }
      length = largest_index + 1;
    }
    if (data_->is_primitive(elem_tk)) {
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
    if (data_->is_basic_type(nested_elem_tk) || nested_elem_tk == TK_ENUM ||
        nested_elem_tk == TK_BITMASK) {
      const bool is_empty = sequence_map_.empty() && complex_map_.empty();
      CORBA::ULong length = 0;
      if (!is_empty) {
        CORBA::ULong largest_index;
        if (!get_largest_index_basic_sequence(largest_index)) {
          return false;
        }
        length = largest_index + 1;
      }
      if (data_->is_basic_type(nested_elem_tk)) {
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
  if (!complex_map_.empty()) {
    CORBA::ULong largest_index;
    if (!get_largest_complex_index(largest_index)) {
      return false;
    }
    length = largest_index + 1;
  }
  return serialize_complex_sequence(ser, length, bound, elem_type);
}

void DynamicDataImpl::DataContainer::serialized_size_primitive_array(const DCPS::Encoding& encoding,
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

bool DynamicDataImpl::DataContainer::serialize_primitive_array(DCPS::Serializer& ser,
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
bool DynamicDataImpl::DataContainer::serialized_size_generic_string_array(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  return serialized_size_generic_string_collection<StringType>(encoding, size, index_to_id);
}

template<typename StringType>
bool DynamicDataImpl::DataContainer::serialize_generic_string_array(DCPS::Serializer& ser,
                                                                    CORBA::ULong length) const
{
  IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
  if (!get_index_to_id_map(index_to_id, length)) {
    return false;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    size_t total_size;
    if (!serialized_size_generic_string_array<StringType>(encoding, total_size, index_to_id) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return serialize_generic_string_collection<StringType>(ser, index_to_id);
}

// Serialize enum array represented as int8 array
void DynamicDataImpl::DataContainer::serialized_size_enum_array_as_int8s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_int8(encoding, size, length);
}

bool DynamicDataImpl::DataContainer::serialize_enum_array_as_ints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_enum_array_as_int8s(DCPS::Serializer& ser,
  CORBA::ULong length, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int8Seq enumarr;
  return reconstruct_enum_collection<CORBA::Int8>(enumarr, length, length, enum_type, ACE_OutputCDR::from_int8(0)) &&
    serialize_enum_array_as_ints_i(ser, enumarr);
}

// Serialize enum array represented as int16 array
void DynamicDataImpl::DataContainer::serialized_size_enum_array_as_int16s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::Short(), length);
}

bool DynamicDataImpl::DataContainer::serialize_enum_array_as_ints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_enum_array_as_int16s(DCPS::Serializer& ser,
  CORBA::ULong length, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int16Seq enumarr;
  return reconstruct_enum_collection<CORBA::Short>(enumarr, length, length, enum_type, CORBA::Short()) &&
    serialize_enum_array_as_ints_i(ser, enumarr);
}

// Serialize enum array represented as int32 array
void DynamicDataImpl::DataContainer::serialized_size_enum_array_as_int32s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::Long(), length);
}

bool DynamicDataImpl::DataContainer::serialize_enum_array_as_ints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_enum_array_as_int32s(DCPS::Serializer& ser,
  CORBA::ULong length, const DDS::DynamicType_var& enum_type) const
{
  DDS::Int32Seq enumarr;
  return reconstruct_enum_collection<CORBA::Long>(enumarr, length, length, enum_type, CORBA::Long()) &&
    serialize_enum_array_as_ints_i(ser, enumarr);
}

void DynamicDataImpl::DataContainer::serialized_size_enum_array(const DCPS::Encoding& encoding,
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

bool DynamicDataImpl::DataContainer::serialize_enum_array(DCPS::Serializer& ser,
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
void DynamicDataImpl::DataContainer::serialized_size_bitmask_array_as_uint8s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size_uint8(encoding, size, length);
}

bool DynamicDataImpl::DataContainer::serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_array_as_uint8s(DCPS::Serializer& ser,
                                                                       CORBA::ULong length) const
{
  DDS::UInt8Seq bitmask_arr;
  return reconstruct_bitmask_collection(bitmask_arr, length, length, ACE_OutputCDR::from_uint8(0)) &&
    serialize_bitmask_array_as_uints_i(ser, bitmask_arr);
}

// Bitmask array represented as uint16 array.
void DynamicDataImpl::DataContainer::serialized_size_bitmask_array_as_uint16s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::UShort(), length);
}

bool DynamicDataImpl::DataContainer::serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_array_as_uint16s(DCPS::Serializer& ser,
                                                                        CORBA::ULong length) const
{
  DDS::UInt16Seq bitmask_arr;
  return reconstruct_bitmask_collection(bitmask_arr, length, length, CORBA::UShort()) &&
    serialize_bitmask_array_as_uints_i(ser, bitmask_arr);
}

// Bitmask array represented as uint32 array.
void DynamicDataImpl::DataContainer::serialized_size_bitmask_array_as_uint32s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::ULong(), length);
}

bool DynamicDataImpl::DataContainer::serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_array_as_uint32s(DCPS::Serializer& ser,
                                                                        CORBA::ULong length) const
{
  DDS::UInt32Seq bitmask_arr;
  return reconstruct_bitmask_collection(bitmask_arr, length, length, CORBA::ULong()) &&
    serialize_bitmask_array_as_uints_i(ser, bitmask_arr);
}

// Bitmask array represented as uint64 array.
void DynamicDataImpl::DataContainer::serialized_size_bitmask_array_as_uint64s(
  const DCPS::Encoding& encoding, size_t& size, CORBA::ULong length) const
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, CORBA::ULongLong(), length);
}

bool DynamicDataImpl::DataContainer::serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_array_as_uint64s(DCPS::Serializer& ser,
                                                                        CORBA::ULong length) const
{
  DDS::UInt64Seq bitmask_arr;
  return reconstruct_bitmask_collection(bitmask_arr, length, length, CORBA::ULongLong()) &&
    serialize_bitmask_array_as_uints_i(ser, bitmask_arr);
}

void DynamicDataImpl::DataContainer::serialized_size_bitmask_array(
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_array(DCPS::Serializer& ser,
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
bool DynamicDataImpl::DataContainer::serialized_size_nesting_basic_array(const DCPS::Encoding& encoding,
  size_t& size, const IndexToIdMap& index_to_id, SequenceType protoseq) const
{
  serialized_size_delimiter(encoding, size);
  return serialized_size_nested_basic_sequences(encoding, size, index_to_id, protoseq);
}

template<typename SequenceType>
bool DynamicDataImpl::DataContainer::serialize_nesting_basic_array_i(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialized_size_nesting_basic_array(
  const DCPS::Encoding& encoding, size_t& size, TypeKind nested_elem_tk,
  const IndexToIdMap& index_to_id) const
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

bool DynamicDataImpl::DataContainer::serialize_nesting_basic_array(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialized_size_nesting_enum_array(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  return serialized_size_nested_enum_sequences(encoding, size, index_to_id);
}

bool DynamicDataImpl::DataContainer::serialize_nesting_enum_array(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialized_size_nesting_bitmask_array(
  const DCPS::Encoding& encoding, size_t& size, const IndexToIdMap& index_to_id) const
{
  serialized_size_delimiter(encoding, size);
  return serialized_size_nested_bitmask_sequences(encoding, size, index_to_id);
}

bool DynamicDataImpl::DataContainer::serialize_nesting_bitmask_array(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialized_size_complex_array(const DCPS::Encoding& encoding,
  size_t& size, const IndexToIdMap& index_to_id, const DDS::DynamicType_var& elem_type) const
{
  serialized_size_delimiter(encoding, size);
  for (CORBA::ULong i = 0; i < index_to_id.size(); ++i) {
    if (!serialized_size_complex_member(encoding, size, index_to_id[i], elem_type)) {
      return false;
    }
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialize_complex_array(
  DCPS::Serializer& ser, CORBA::ULong length, const DDS::DynamicType_var& elem_type) const
{
  IndexToIdMap index_to_id(length, MEMBER_ID_INVALID);
  for (const_complex_iterator it = complex_map_.begin(); it != complex_map_.end(); ++it) {
    CORBA::ULong index;
    if (!data_->get_index_from_id(it->first, index, length)) {
      return false;
    }
    index_to_id[index] = it->first;
  }

  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    if (!serialized_size_complex_array(encoding, total_size, index_to_id, elem_type) ||
        !ser.write_delimiter(total_size)) {
      return false;
    }
  }
  return serialize_complex_sequence_i(ser, index_to_id, elem_type);
}

bool DynamicDataImpl::DataContainer::serialized_size_array(const DCPS::Encoding& encoding,
                                                           size_t& size) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  CORBA::ULong length = 1;
  const DDS::BoundSeq& bound = descriptor->bound();
  for (CORBA::ULong i = 0; i < bound.length(); ++i) {
    length *= bound[i];
  }

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }

  if (data_->is_basic_type(elem_tk)) {
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
    if (data_->is_basic_type(nested_elem_tk)) {
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
  return serialized_size_complex_array(encoding, size, index_to_id, elem_type);
}

bool DynamicDataImpl::DataContainer::serialize_array(DCPS::Serializer& ser) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  CORBA::ULong length = 1;
  const DDS::BoundSeq& bound = descriptor->bound();
  for (CORBA::ULong i = 0; i < bound.length(); ++i) {
    length *= bound[i];
  }

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeDescriptor_var elem_td;
  if (elem_type->get_descriptor(elem_td) != DDS::RETCODE_OK) {
    return false;
  }

  if (data_->is_basic_type(elem_tk)) {
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
    if (data_->is_basic_type(nested_elem_tk)) {
      return serialize_nesting_basic_array(ser, nested_elem_tk, length);
    } else if (nested_elem_tk == TK_ENUM) {
      return serialize_nesting_enum_array(ser, length);
    } else if (nested_elem_tk == TK_BITMASK) {
      return serialize_nesting_bitmask_array(ser, length);
    }
  }
  return serialize_complex_array(ser, length, elem_type);
}

bool DynamicDataImpl::DataContainer::serialized_size_primitive_member(const DCPS::Encoding& encoding,
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

bool DynamicDataImpl::DataContainer::serialized_size_basic_member_default_value(
  const DCPS::Encoding& encoding, size_t& size, TypeKind member_tk) const
{
  if (data_->is_primitive(member_tk)) {
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
bool DynamicDataImpl::DataContainer::serialized_size_basic_member(const DCPS::Encoding& encoding,
  size_t& size, TypeKind member_tk, const_single_iterator it) const
{
  if (data_->is_primitive(member_tk)) {
    return serialized_size_primitive_member(encoding, size, member_tk);
  } else if (member_tk == TK_STRING8 || member_tk == TK_STRING16) {
    serialized_size_string_common(encoding, size, it->second);
    return true;
  }
  return false;
}

bool DynamicDataImpl::DataContainer::serialize_basic_member_default_value(DCPS::Serializer& ser,
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
bool DynamicDataImpl::DataContainer::serialized_size_single_aggregated_member_xcdr2(
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
  if (data_->is_basic_type(member_tk)) {
    return serialized_size_basic_member(encoding, size, member_tk, it);
  } else if (member_tk == TK_ENUM) {
    return serialized_size_enum(encoding, size, member_type);
  } else { // Bitmask
    return serialized_size_bitmask(encoding, size, member_type);
  }
}

bool DynamicDataImpl::DataContainer::serialize_single_aggregated_member_xcdr2(DCPS::Serializer& ser,
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
    if (data_->is_basic_type(member_tk)) {
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

// Serialize a member of an aggregated type whose value must be represented by a DynamicData
// object. However, the data for the member is missing from the complex map. So default value
// of the corresponding type is used for serialization.
bool DynamicDataImpl::DataContainer::serialized_size_complex_aggregated_member_xcdr2_default(
  const DCPS::Encoding& encoding, size_t& size, const DDS::DynamicType_var& member_type,
  bool optional, DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
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
  return serialized_size(encoding, size, DynamicDataImpl(member_type));
}

bool DynamicDataImpl::DataContainer::serialize_complex_aggregated_member_xcdr2_default(
  DCPS::Serializer& ser, DDS::MemberId id, const DDS::DynamicType_var& member_type,
  bool optional, bool must_understand, DDS::ExtensibilityKind extensibility) const
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
    serialized_size(ser.encoding(), member_size, default_value);
    if (!ser.write_parameter_id(id, member_size, must_understand)) {
      return false;
    }
  }
  return ser << default_value;
}

// Serialize a member of an aggregated type stored in the complex map,
// i.e., the member value is represented by a DynamicData object.
bool DynamicDataImpl::DataContainer::serialized_size_complex_aggregated_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, const_complex_iterator it,
  bool optional, DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
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
  return serialized_size(encoding, size, *data_impl);
}

bool DynamicDataImpl::DataContainer::serialize_complex_aggregated_member_xcdr2(
  DCPS::Serializer& ser, const_complex_iterator it, bool optional,
  bool must_understand, DDS::ExtensibilityKind extensibility) const
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
    if (!serialized_size(ser.encoding(), member_size, *data_impl) ||
        !ser.write_parameter_id(it->first, member_size, must_understand)) {
      return false;
    }
  }
  return ser << *data_impl;
}

// Serialize struct member whose type is basic or compatible with a basic type,
// that are enum and bitmask types.
bool DynamicDataImpl::DataContainer::serialized_size_basic_struct_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, DDS::MemberId id,
  const DDS::DynamicType_var& member_type, bool optional,
  DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
{
  const TypeKind member_tk = member_type->get_kind();
  const_single_iterator single_it = single_map_.find(id);
  const_complex_iterator complex_it = complex_map_.find(id);
  if (single_it == single_map_.end() && complex_it == complex_map_.end()) {
    if (optional) {
      if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
        primitive_serialized_size_boolean(encoding, size);
      }
      return true;
    }
    if (extensibility == DDS::MUTABLE) {
      serialized_size_parameter_id(encoding, size, mutable_running_total);
    }
    if (data_->is_basic_type(member_tk)) {
      return serialized_size_basic_member_default_value(encoding, size, member_tk);
    } else if (member_tk == TK_ENUM) {
      return serialized_size_enum(encoding, size, member_type);
    } else { // Bitmask
      return serialized_size_bitmask(encoding, size, member_type);
    }
  }

  if (single_it != single_map_.end()) {
    return serialized_size_single_aggregated_member_xcdr2(encoding, size, single_it, member_type,
                                                          optional, extensibility, mutable_running_total);
  }
  return serialized_size_complex_aggregated_member_xcdr2(encoding, size, complex_it, optional,
                                                         extensibility, mutable_running_total);
}

bool DynamicDataImpl::DataContainer::serialize_basic_struct_member_xcdr2(DCPS::Serializer& ser,
  DDS::MemberId id, const DDS::DynamicType_var& member_type, bool optional,
  bool must_understand, DDS::ExtensibilityKind extensibility) const
{
  const TypeKind member_tk = member_type->get_kind();
  const DCPS::Encoding& encoding = ser.encoding();
  const_single_iterator single_it = single_map_.find(id);
  const_complex_iterator complex_it = complex_map_.find(id);
  if (single_it == single_map_.end() && complex_it == complex_map_.end()) {
    if (optional) {
      if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
        return ser << ACE_OutputCDR::from_boolean(false);
      }
      return true;
    }
    if (extensibility == DDS::MUTABLE) {
      size_t member_size = 0;
      if (data_->is_basic_type(member_tk)) {
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
    if (data_->is_basic_type(member_tk)) {
      return serialize_basic_member_default_value(ser, member_tk);
    } else if (member_tk == TK_ENUM) {
      return serialize_enum_default_value(ser, member_type);
    } else if (member_tk == TK_BITMASK) {
      return serialize_bitmask_default_value(ser, member_type);
    }
    return false;
  }

  if (single_it != single_map_.end()) {
    return serialize_single_aggregated_member_xcdr2(ser, single_it, member_type, optional,
                                                    must_understand, extensibility);
  }
  return serialize_complex_aggregated_member_xcdr2(ser, complex_it, optional,
                                                   must_understand, extensibility);
}

void DynamicDataImpl::DataContainer::serialized_size_sequence_member_default_value(
  const DCPS::Encoding& encoding, size_t& size, TypeKind elem_tk) const
{
  // Zero-length sequence
  if (!data_->is_primitive(elem_tk)) {
    serialized_size_delimiter(encoding, size);
  }
  primitive_serialized_size_ulong(encoding, size);
}

bool DynamicDataImpl::DataContainer::serialize_sequence_member_default_value(DCPS::Serializer& ser,
                                                                             TypeKind elem_tk) const
{
  // Zero-length sequence
  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2 && !data_->is_primitive(elem_tk)) {
    if (!ser.write_delimiter(2 * DCPS::uint32_cdr_size)) {
      return false;
    }
  }
  return ser << static_cast<CORBA::ULong>(0);
}

bool DynamicDataImpl::DataContainer::serialized_size_basic_sequence(const DCPS::Encoding& encoding,
  size_t& size, const_sequence_iterator it) const
{
  switch (it->second.elem_kind_) {
  case TK_INT32: {
    const DDS::Int32Seq& seq = it->second.get<DDS::Int32Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_UINT32: {
    const DDS::UInt32Seq& seq = it->second.get<DDS::UInt32Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_INT8: {
    const DDS::Int8Seq& seq = it->second.get<DDS::Int8Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_UINT8: {
    const DDS::UInt8Seq& seq = it->second.get<DDS::UInt8Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_INT16: {
    const DDS::Int16Seq& seq = it->second.get<DDS::Int16Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_UINT16: {
    const DDS::UInt16Seq& seq = it->second.get<DDS::UInt16Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_INT64: {
    const DDS::Int64Seq& seq = it->second.get<DDS::Int64Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_UINT64: {
    const DDS::UInt64Seq& seq = it->second.get<DDS::UInt64Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_FLOAT32: {
    const DDS::Float32Seq& seq = it->second.get<DDS::Float32Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_FLOAT64: {
    const DDS::Float64Seq& seq = it->second.get<DDS::Float64Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_FLOAT128: {
    const DDS::Float128Seq& seq = it->second.get<DDS::Float128Seq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_CHAR8: {
    const DDS::CharSeq& seq = it->second.get<DDS::CharSeq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_BYTE: {
    const DDS::ByteSeq& seq = it->second.get<DDS::ByteSeq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_BOOLEAN: {
    const DDS::BooleanSeq& seq = it->second.get<DDS::BooleanSeq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_STRING8: {
    const DDS::StringSeq& seq = it->second.get<DDS::StringSeq>();
    serialized_size(encoding, size, seq);
    return true;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    const DDS::WcharSeq& seq = it->second.get<DDS::WcharSeq>();
    serialized_size(encoding, size, seq);
    return true;
  }
  case TK_STRING16: {
    const DDS::WstringSeq& seq = it->second.get<DDS::WstringSeq>();
    serialized_size(encoding, size, seq);
    return true;
  }
#endif
  }
  return false;
}

bool DynamicDataImpl::DataContainer::serialize_basic_sequence(DCPS::Serializer& ser,
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

bool DynamicDataImpl::DataContainer::serialized_size_enum_sequence(
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

bool DynamicDataImpl::DataContainer::serialize_enum_sequence(
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

bool DynamicDataImpl::DataContainer::serialized_size_bitmask_sequence(
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

bool DynamicDataImpl::DataContainer::serialize_bitmask_sequence(DCPS::Serializer& ser,
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
void DynamicDataImpl::DataContainer::serialized_size_sequence_aggregated_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, const_sequence_iterator it, TypeKind elem_tk,
  bool optional, DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
{
  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    primitive_serialized_size_boolean(encoding, size);
  } else if (extensibility == DDS::MUTABLE) {
    serialized_size_parameter_id(encoding, size, mutable_running_total);
  }
  if (data_->is_basic_type(elem_tk)) {
    serialized_size_basic_sequence(encoding, size, it);
  } else if (elem_tk == TK_ENUM) {
    serialized_size_enum_sequence(encoding, size, it);
  } else { // Bitmask
    serialized_size_bitmask_sequence(encoding, size, it);
  }
}

bool DynamicDataImpl::DataContainer::serialize_sequence_aggregated_member_xcdr2(DCPS::Serializer& ser,
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
    if (data_->is_basic_type(elem_tk)) {
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
  if (data_->is_basic_type(elem_tk)) {
    return serialize_basic_sequence(ser, it);
  } else if (elem_tk == TK_ENUM) {
    return serialize_enum_sequence(ser, it);
  } else if (elem_tk == TK_BITMASK) {
    return serialize_bitmask_sequence(ser, it);
  }
  return false;
}

// Serialize a struct member whose type is sequence of basic type or enum or bitmask.
bool DynamicDataImpl::DataContainer::serialized_size_sequence_struct_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, DDS::MemberId id, TypeKind elem_tk,
  bool optional, DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
{
  const_sequence_iterator seq_it = sequence_map_.find(id);
  const_complex_iterator complex_it = complex_map_.find(id);
  if (seq_it == sequence_map_.end() && complex_it == complex_map_.end()) {
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
  if (seq_it != sequence_map_.end()) {
    serialized_size_sequence_aggregated_member_xcdr2(encoding, size, seq_it, elem_tk, optional,
                                                     extensibility, mutable_running_total);
    return true;
  }
  return serialized_size_complex_aggregated_member_xcdr2(encoding, size, complex_it, optional,
                                                         extensibility, mutable_running_total);
}

bool DynamicDataImpl::DataContainer::serialize_sequence_struct_member_xcdr2(DCPS::Serializer& ser,
  DDS::MemberId id, TypeKind elem_tk, bool optional,
  bool must_understand, DDS::ExtensibilityKind extensibility) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  const_sequence_iterator seq_it = sequence_map_.find(id);
  const_complex_iterator complex_it = complex_map_.find(id);
  if (seq_it == sequence_map_.end() && complex_it == complex_map_.end()) {
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

  if (seq_it != sequence_map_.end()) {
    return serialize_sequence_aggregated_member_xcdr2(ser, seq_it, elem_tk, optional,
                                                      must_understand, extensibility);
  }
  return serialize_complex_aggregated_member_xcdr2(ser, complex_it, optional,
                                                   must_understand, extensibility);
}

bool DynamicDataImpl::DataContainer::serialized_size_structure_xcdr2(
  const DCPS::Encoding& encoding, size_t& size) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::ExtensibilityKind extensibility = descriptor->extensibility_kind();

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
    const DDS::MemberId id = md->id();
    const CORBA::Boolean optional = md->is_optional();
    const DDS::DynamicType_var member_type = get_base_type(md->type());
    const TypeKind member_tk = member_type->get_kind();

    if (data_->is_basic_type(member_tk) || member_tk == TK_ENUM || member_tk == TK_BITMASK) {
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
      if (data_->is_basic_type(elem_tk) || elem_tk == TK_ENUM || elem_tk == TK_BITMASK) {
        if (!serialized_size_sequence_struct_member_xcdr2(encoding, size, id, elem_tk, optional,
                                                          extensibility, mutable_running_total)) {
          return false;
        }
        continue;
      }
    }

    const_complex_iterator it = complex_map_.find(id);
    if (it != complex_map_.end()) {
      if (!serialized_size_complex_aggregated_member_xcdr2(encoding, size, it, optional,
                                                           extensibility, mutable_running_total)) {
        return false;
      }
    } else if (!serialized_size_complex_aggregated_member_xcdr2_default(encoding, size, member_type, optional,
                                                                        extensibility, mutable_running_total)) {
      return false;
    }
  }

  if (extensibility == DDS::MUTABLE) {
    serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialize_structure_xcdr2(DCPS::Serializer& ser) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::ExtensibilityKind extensibility = descriptor->extensibility_kind();

  // Delimiter
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    if (!serialized_size(encoding, total_size, *data_) || !ser.write_delimiter(total_size)) {
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
    const DDS::MemberId id = md->id();
    const CORBA::Boolean optional = md->is_optional();
    const CORBA::Boolean must_understand = md->is_must_understand();
    const DDS::DynamicType_var member_type = get_base_type(md->type());
    const TypeKind member_tk = member_type->get_kind();

    if (data_->is_basic_type(member_tk) || member_tk == TK_ENUM || member_tk == TK_BITMASK) {
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
      if (data_->is_basic_type(elem_tk) || elem_tk == TK_ENUM || elem_tk == TK_BITMASK) {
        if (!serialize_sequence_struct_member_xcdr2(ser, id, elem_tk, optional,
                                                    must_understand, extensibility)) {
          return false;
        }
        continue;
      }
    }

    const_complex_iterator it = complex_map_.find(id);
    if (it != complex_map_.end()) {
      return serialize_complex_aggregated_member_xcdr2(ser, it, optional,
                                                       must_understand, extensibility);
    }
    return serialize_complex_aggregated_member_xcdr2_default(ser, id, member_type, optional,
                                                             must_understand, extensibility);
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialized_size_structure_xcdr1(
  const DCPS::Encoding& /*encoding*/, size_t& /*size*/) const
{
  // TODO: Support Final & Mutable extensibility?
  return false;
}

bool DynamicDataImpl::DataContainer::serialize_structure_xcdr1(DCPS::Serializer& /*ser*/) const
{
  // TODO: Support only Final & Mutable extensibility?
  return false;
}

bool DynamicDataImpl::DataContainer::serialized_size_structure(const DCPS::Encoding& encoding,
                                                               size_t& size) const
{
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    return serialized_size_structure_xcdr2(encoding, size);
  } else if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_1) {
    return serialized_size_structure_xcdr1(encoding, size);
  }
  return false;
}

bool DynamicDataImpl::DataContainer::serialize_structure(DCPS::Serializer& ser) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    return serialize_structure_xcdr2(ser);
  } else if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_1) {
    return serialize_structure_xcdr1(ser);
  }
  return false;
}

// Set discriminator to the default value of the corresponding type.
bool DynamicDataImpl::DataContainer::set_default_discriminator_value(
  CORBA::Long& value, const DDS::DynamicType_var& disc_type) const
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
    DDS::DynamicTypeMember_var first_dtm;
    if (disc_type->get_member_by_index(first_dtm, 0) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var first_md;
    if (first_dtm->get_descriptor(first_md) != DDS::RETCODE_OK) {
      return false;
    }
    value = static_cast<CORBA::Long>(first_md->id());
    return true;
  }
  }
  return false;
}

// Get discriminator value from the data container. The discriminator data must present
// in either single map or complex map.
bool DynamicDataImpl::DataContainer::get_discriminator_value(
  CORBA::Long& value, const_single_iterator single_it, const_complex_iterator complex_it,
  const DDS::DynamicType_var& disc_type) const
{
  if (single_it != single_map_.end()) {
    data_->read_discriminator(value, disc_type, single_it);
  } else { // Find in complex map
    const DynamicDataImpl* dd_impl = dynamic_cast<const DynamicDataImpl*>(complex_it->second.in());
    if (!dd_impl) {
      return false;
    }
    const_single_iterator it = dd_impl->container_.single_map_.find(MEMBER_ID_INVALID);
    if (it != dd_impl->container_.single_map_.end()) {
      data_->read_discriminator(value, disc_type, it);
    } else {
      set_default_discriminator_value(value, disc_type);
    }
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialized_size_discriminator_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, const DDS::DynamicType_var& disc_type,
  DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
{
  if (extensibility == DDS::MUTABLE) {
    serialized_size_parameter_id(encoding, size, mutable_running_total);
  }
  const TypeKind disc_tk = disc_type->get_kind();
  if (data_->is_primitive(disc_tk)) {
    return serialized_size_primitive_member(encoding, size, disc_tk);
  }
  return serialized_size_enum(encoding, size, disc_type);
}

bool DynamicDataImpl::DataContainer::serialize_discriminator_member_xcdr2(
  DCPS::Serializer& ser, CORBA::Long value, const DDS::DynamicType_var& disc_type,
  DDS::ExtensibilityKind extensibility) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  const TypeKind disc_tk = disc_type->get_kind();
  if (extensibility == DDS::MUTABLE) {
    size_t disc_size = 0;
    if (data_->is_primitive(disc_tk)) {
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
  case TK_CHAR16:
    return ser << static_cast<CORBA::WChar>(value);
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

bool DynamicDataImpl::DataContainer::serialized_size_selected_member_xcdr2(
  const DCPS::Encoding& encoding, size_t& size, DDS::MemberId selected_id,
  DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const
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

  const_single_iterator single_it = single_map_.find(selected_id);
  if (single_it != single_map_.end()) {
    return serialized_size_single_aggregated_member_xcdr2(encoding, size, single_it, selected_type, optional,
                                                          extensibility, mutable_running_total);
  }

  const_sequence_iterator seq_it = sequence_map_.find(selected_id);
  if (seq_it != sequence_map_.end()) {
    DDS::TypeDescriptor_var selected_td;
    if (selected_type->get_descriptor(selected_td) != DDS::RETCODE_OK) {
      return false;
    }
    const TypeKind elem_tk = get_base_type(selected_td->element_type())->get_kind();
    serialized_size_sequence_aggregated_member_xcdr2(encoding, size, seq_it, elem_tk, optional,
                                                     extensibility, mutable_running_total);
    return true;
  }

  const_complex_iterator complex_it = complex_map_.find(selected_id);
  if (complex_it != complex_map_.end()) {
    return serialized_size_complex_aggregated_member_xcdr2(encoding, size, complex_it, optional,
                                                           extensibility, mutable_running_total);
  }
  return false;
}

bool DynamicDataImpl::DataContainer::serialize_selected_member_xcdr2(DCPS::Serializer& ser,
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

  const_single_iterator single_it = single_map_.find(selected_id);
  if (single_it != single_map_.end()) {
    return serialize_single_aggregated_member_xcdr2(ser, single_it, selected_type, optional,
                                                    must_understand, extensibility);
  }

  const_sequence_iterator seq_it = sequence_map_.find(selected_id);
  if (seq_it != sequence_map_.end()) {
    DDS::TypeDescriptor_var selected_td;
    if (selected_type->get_descriptor(selected_td) != DDS::RETCODE_OK) {
      return false;
    }
    const TypeKind elem_tk = get_base_type(selected_td->element_type())->get_kind();
    return serialize_sequence_aggregated_member_xcdr2(ser, seq_it, elem_tk, optional,
                                                      must_understand, extensibility);
  }

  const_complex_iterator complex_it = complex_map_.find(selected_id);
  if (complex_it != complex_map_.end()) {
    return serialize_complex_aggregated_member_xcdr2(ser, complex_it, optional,
                                                     must_understand, extensibility);
  }
  return false;
}

bool DynamicDataImpl::DataContainer::select_union_member(CORBA::Long disc_value,
  bool& found_selected_member, DDS::MemberDescriptor_var& selected_md) const
{
  found_selected_member = false;
  bool has_default = false;
  DDS::MemberDescriptor_var default_md;
  for (CORBA::ULong i = 0; i < type_->get_member_count(); ++i) {
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member_by_index(dtm, i) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var md;
    if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }
    bool found_matched_label = false;
    const DDS::UnionCaseLabelSeq labels = md->label();
    for (CORBA::ULong j = 0; !found_matched_label && j < labels.length(); ++j) {
      if (disc_value == labels[j]) {
        found_matched_label = true;
      }
    }
    if (found_matched_label) {
      selected_md = md;
      found_selected_member = true;
      break;
    }
    if (md->is_default_label()) {
      default_md = md;
      has_default = true;
    }
  }
  if (!found_selected_member && has_default) {
    selected_md = default_md;
    found_selected_member = true;
  }
  return true;
}

bool DynamicDataImpl::DataContainer::serialized_size_union_xcdr2(const DCPS::Encoding& encoding,
                                                                 size_t& size) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::ExtensibilityKind extensibility = descriptor->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    serialized_size_delimiter(encoding, size);
  }

  const_single_iterator single_it = single_map_.find(DISCRIMINATOR_ID);
  const_complex_iterator complex_it = complex_map_.find(DISCRIMINATOR_ID);
  const bool has_disc = single_it != single_map_.end() || complex_it != complex_map_.end();
  const DDS::MemberId selected_id = data_->find_selected_member();
  DDS::DynamicType_var disc_type = get_base_type(descriptor->discriminator_type());

  CORBA::Long disc_value;
  if (has_disc) {
    if (!get_discriminator_value(disc_value, single_it, complex_it, disc_type)) {
      return false;
    }
  } else if (!set_default_discriminator_value(disc_value, disc_type)) {
    return false;
  }

  size_t mutable_running_total = 0;
  if (selected_id == MEMBER_ID_INVALID) {
    bool found_selected_member = false;
    DDS::MemberDescriptor_var  selected_md;
    if (!select_union_member(disc_value, found_selected_member, selected_md) != DDS::RETCODE_OK) {
      return false;
    }
    if (!serialized_size_discriminator_member_xcdr2(encoding, size, disc_type,
                                                    extensibility, mutable_running_total)) {
      return false;
    }
    if (found_selected_member) {
      DDS::DynamicType_var selected_type = get_base_type(selected_md->type());
      const bool optional = selected_md->is_optional();
      if (!serialized_size_complex_aggregated_member_xcdr2_default(encoding, size, selected_type, optional,
                                                                   extensibility, mutable_running_total)) {
        return false;
      }
    }
    serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
    return true;
  }

  if (!has_disc) {
    DDS::DynamicTypeMember_var selected_dtm;
    if (type_->get_member(selected_dtm, selected_id) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var selected_md;
    if (selected_dtm->get_descriptor(selected_md) != DDS::RETCODE_OK) {
      return false;
    }
    if (!data_->validate_discriminator(disc_value, selected_md)) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::DataContainer::serialized_size_union_xcdr2:"
                   " Default discriminator value does not select the existing selected member\n"));
      }
      return false;
    }
  }

  if (!serialized_size_discriminator_member_xcdr2(encoding, size, disc_type,
                                                  extensibility, mutable_running_total) ||
      !serialized_size_selected_member_xcdr2(encoding, size, selected_id,
                                             extensibility, mutable_running_total)) {
    return false;
  }
  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
  return true;
}

bool DynamicDataImpl::DataContainer::serialize_union_xcdr2(DCPS::Serializer& ser) const
{
  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::ExtensibilityKind extensibility = descriptor->extensibility_kind();

  // Delimiter
  const DCPS::Encoding& encoding = ser.encoding();
  size_t total_size = 0;
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    if (!serialized_size(encoding, total_size, *data_) || !ser.write_delimiter(total_size)) {
      return false;
    }
  }

  const_single_iterator single_it = single_map_.find(DISCRIMINATOR_ID);
  const_complex_iterator complex_it = complex_map_.find(DISCRIMINATOR_ID);
  const bool has_disc = single_it != single_map_.end() || complex_it != complex_map_.end();
  const DDS::MemberId selected_id = data_->find_selected_member();
  DDS::DynamicType_var disc_type = get_base_type(descriptor->discriminator_type());
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
    if (!select_union_member(disc_value, found_selected_member, selected_md)) {
      return false;
    }
    // Discriminator
    if (!serialize_discriminator_member_xcdr2(ser, disc_value, disc_type, extensibility)) {
      return false;
    }
    // Selected member
    if (found_selected_member) {
      DDS::DynamicType_var selected_type = get_base_type(selected_md->type());
      const DDS::MemberId id = selected_md->id();
      const bool optional = selected_md->is_optional();
      const bool must_understand = selected_md->is_must_understand();
      return serialize_complex_aggregated_member_xcdr2_default(ser, id, selected_type, optional,
                                                               must_understand, extensibility);
    }
    return true;
  }

  if (!has_disc) {
    // If the default value of the discriminator doesn't select the member written
    // by user, return an error. Otherwise, serialize both as normal.
    DDS::DynamicTypeMember_var selected_dtm;
    if (type_->get_member(selected_dtm, selected_id) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var selected_md;
    if (selected_dtm->get_descriptor(selected_md) != DDS::RETCODE_OK) {
      return false;
    }
    if (!data_->validate_discriminator(disc_value, selected_md)) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::DataContainer::serialize_union_xcdr2:"
                   " Default discriminator value does not select the existing selected member\n"));
      }
      return false;
    }
    return serialize_discriminator_member_xcdr2(ser, disc_value, disc_type, extensibility) &&
      serialize_selected_member_xcdr2(ser, selected_id, extensibility);
  }

  // Both discriminator and a selected member exist in the data container.
  if (single_it != single_map_.end()) {
    if (extensibility == DDS::MUTABLE) {
      size_t disc_size = 0;
      if (data_->is_primitive(disc_tk)) {
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
      serialized_size_complex_member_i(encoding, disc_size, complex_it->first);
      if (!ser.write_parameter_id(0, disc_size, false)) {
        return false;
      }
    }
    if (!serialize_complex_member_i(ser, complex_it->first)) {
      return false;
    }
  }
  return serialize_selected_member_xcdr2(ser, selected_id, extensibility);
}

bool DynamicDataImpl::DataContainer::serialized_size_union_xcdr1(const DCPS::Encoding& /*encoding*/,
                                                                 size_t& /*size*/) const
{
  // TODO:
  return false;
}

bool DynamicDataImpl::DataContainer::serialize_union_xcdr1(DCPS::Serializer& /*ser*/) const
{
  // TODO:
  return false;
}

bool DynamicDataImpl::DataContainer::serialized_size_union(const DCPS::Encoding& encoding,
                                                           size_t& size) const
{
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    return serialized_size_union_xcdr2(encoding, size);
  } else if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_1) {
    return serialized_size_union_xcdr1(encoding, size);
  }
  return false;
}

bool DynamicDataImpl::DataContainer::serialize_union(DCPS::Serializer& ser) const
{
  const DCPS::Encoding& encoding = ser.encoding();
  if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2) {
    return serialize_union_xcdr2(ser);
  } else if (encoding.xcdr_version() == DCPS::Encoding::XCDR_VERSION_1) {
    return serialize_union_xcdr1(ser);
  }
  return false;
}
} // namespace XTypes

namespace DCPS {
bool serialized_size(const Encoding& encoding, size_t& size, const XTypes::DynamicDataImpl& data)
{
  using namespace XTypes;
  const TypeKind tk = data.type_->get_kind();
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
    return data.container_.serialized_size_enum(encoding, size, data.type_);
  case TK_BITMASK:
    return data.container_.serialized_size_bitmask(encoding, size, data.type_);
  case TK_STRING8:
    return data.container_.serialized_size_string(encoding, size);
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
    return data.container_.serialized_size_wstring(encoding, size);
#endif
  case TK_STRUCTURE:
    return data.container_.serialized_size_structure(encoding, size);
  case TK_UNION:
    return data.container_.serialized_size_union(encoding, size);
  case TK_SEQUENCE:
    return data.container_.serialized_size_sequence(encoding, size);
  case TK_ARRAY:
    return data.container_.serialized_size_array(encoding, size);
  case TK_MAP:
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: operator<<: Serialization of map types is not supported\n"));
    }
  }
  return false;
}

bool operator<<(Serializer& ser, const XTypes::DynamicDataImpl& data)
{
  using namespace XTypes;
  const TypeKind tk = data.type_->get_kind();
  switch (tk) {
  case TK_INT32:
    return data.container_.serialize_primitive_value(ser, CORBA::Long());
  case TK_UINT32:
    return data.container_.serialize_primitive_value(ser, CORBA::ULong());
  case TK_INT8:
    return data.container_.serialize_primitive_value(ser, ACE_OutputCDR::from_int8(CORBA::Int8()));
  case TK_UINT8:
    return data.container_.serialize_primitive_value(ser, ACE_OutputCDR::from_uint8(CORBA::UInt8()));
  case TK_INT16:
    return data.container_.serialize_primitive_value(ser, CORBA::Short());
  case TK_UINT16:
    return data.container_.serialize_primitive_value(ser, CORBA::UShort());
  case TK_INT64:
    return data.container_.serialize_primitive_value(ser, CORBA::LongLong());
  case TK_UINT64:
    return data.container_.serialize_primitive_value(ser, CORBA::ULongLong());
  case TK_FLOAT32:
    return data.container_.serialize_primitive_value(ser, CORBA::Float());
  case TK_FLOAT64:
    return data.container_.serialize_primitive_value(ser, CORBA::Double());
  case TK_FLOAT128:
    return data.container_.serialize_primitive_value(ser, CORBA::LongDouble());
  case TK_CHAR8:
    return data.container_.serialize_primitive_value(ser, ACE_OutputCDR::from_char(CORBA::Char()));
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    return data.container_.serialize_primitive_value(ser, ACE_OutputCDR::from_wchar(CORBA::WChar()));
#endif
  case TK_BYTE:
    return data.container_.serialize_primitive_value(ser, ACE_OutputCDR::from_octet(CORBA::Octet()));
  case TK_BOOLEAN:
    return data.container_.serialize_primitive_value(ser, ACE_OutputCDR::from_boolean(CORBA::Boolean()));
  case TK_ENUM:
    return data.container_.serialize_enum_value(ser);
  case TK_BITMASK:
    return data.container_.serialize_bitmask_value(ser);
  case TK_STRING8:
    return data.container_.serialize_string_value(ser);
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
    return data.container_.serialize_wstring_value(ser);
#endif
  case TK_STRUCTURE:
    return data.container_.serialize_structure(ser);
  case TK_UNION:
    return data.container_.serialize_union(ser);
  case TK_SEQUENCE:
    return data.container_.serialize_sequence(ser);
  case TK_ARRAY:
    return data.container_.serialize_array(ser);
  case TK_MAP:
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: operator<<: Serialization of map types is not supported\n"));
    }
  }
  return false;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
