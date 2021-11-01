/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#include "DynamicData.h"

#include "DynamicTypeMember.h"

#include <dds/CorbaSeq/LongSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/ULongSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/Int8SeqTypeSupportImpl.h>
#include <dds/CorbaSeq/UInt8SeqTypeSupportImpl.h>
#include <dds/CorbaSeq/ShortSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/UShortSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/LongLongSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/ULongLongSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/FloatSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/DoubleSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/LongDoubleSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/CharSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/WCharSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/OctetSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/BooleanSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/StringSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/WStringSeqTypeSupportImpl.h>

#include <ace/OS_NS_string.h>

#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicData::DynamicData()
  : chain_(0)
  , encoding_(DCPS::Encoding::KIND_XCDR2)
  , reset_align_state_(false)
  , strm_(0, encoding_)
  , item_count_(ITEM_COUNT_INVALID)
{}

DynamicData::DynamicData(const DynamicData& dd)
  : chain_(dd.chain_->duplicate())
  , encoding_(dd.encoding_)
  , reset_align_state_(dd.reset_align_state_)
  , align_state_(dd.align_state_)
  , strm_(chain_, encoding_)
  , type_(dd.type_)
  , descriptor_(dd.type_->get_descriptor())
  , item_count_(ITEM_COUNT_INVALID)
{}

DynamicData::DynamicData(ACE_Message_Block* chain,
                         const DCPS::Encoding& encoding,
                         const DynamicType_rch& type)
  : chain_(chain->duplicate())
  , encoding_(encoding)
  , reset_align_state_(false)
  , strm_(chain_, encoding_)
  , type_(get_base_type(type))
  , descriptor_(type_->get_descriptor())
  , item_count_(ITEM_COUNT_INVALID)
{
  if (encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_2) {
    throw std::runtime_error("DynamicData only supports XCDR2 at this time");
  }
}

DynamicData::DynamicData(DCPS::Serializer& ser, const DynamicType_rch& type)
  : chain_(ser.current()->duplicate())
  , encoding_(ser.encoding())
  , reset_align_state_(true)
  , align_state_(ser.rdstate())
  , strm_(chain_, encoding_)
  , type_(get_base_type(type))
  , descriptor_(type_->get_descriptor())
  , item_count_(ITEM_COUNT_INVALID)
{
  if (encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_2) {
    throw std::runtime_error("DynamicData only supports XCDR2 at this time");
  }
}

DynamicData& DynamicData::operator=(const DynamicData& other)
{
  chain_ = other.chain_->duplicate();
  encoding_ = other.encoding_;
  reset_align_state_ = other.reset_align_state_;
  align_state_ = other.align_state_;
  strm_ = other.strm_;
  type_ = other.type_;
  descriptor_ = other.descriptor_;
  item_count_ = other.item_count_;
  return *this;
}

DynamicData::~DynamicData()
{
  ACE_DEBUG((LM_DEBUG, "DynamicData::~DynamicData() DynamicType\n"));
  ACE_Message_Block::release(chain_);
}

DDS::ReturnCode_t DynamicData::get_descriptor(MemberDescriptor& value, MemberId id) const
{
  DynamicTypeMember_rch dtm;
  if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  dtm->get_descriptor(value);
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicData::set_descriptor(MemberId id, const MemberDescriptor& value)
{
  DynamicTypeMember_rch dtm;
  if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  dtm->set_descriptor(value);
  return DDS::RETCODE_OK;
}

MemberId DynamicData::get_member_id_by_name(DCPS::String name) const
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
  case TK_CHAR16:
  case TK_ENUM:
    return MEMBER_ID_INVALID;
  case TK_STRING8:
  case TK_STRING16:
  case TK_SEQUENCE:
  case TK_ARRAY:
    // Elements of string, sequence, array must be accessed by index.
    return MEMBER_ID_INVALID;
  case TK_MAP:
    // Values in map can be accessed by strings which is converted from map keys.
    // But need to find out how this conversion works. In the meantime, only allow
    // accessing map using index.
    return MEMBER_ID_INVALID;
  case TK_BITMASK:
  case TK_STRUCTURE:
  case TK_UNION:
    {
      DynamicTypeMember_rch member;
      if (type_->get_member_by_name(member, name) != DDS::RETCODE_OK) {
        return MEMBER_ID_INVALID;
      }
      if (tk == TK_BITMASK) {
        // Bitmask's flags don't have ID, so use index instead.
        return member->get_descriptor().index;
      } else {
        return member->get_descriptor().id;
      }
    }
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::get_member_id_by_name -")
               ACE_TEXT(" Calling on an unexpected type %C\n"), typekind_to_string(tk)));
  }
  return MEMBER_ID_INVALID;
}

MemberId DynamicData::get_member_id_at_index(ACE_CDR::ULong index)
{
  const ACE_CDR::ULong count = get_item_count();
  if (index >= count) {
    return MEMBER_ID_INVALID;
  }

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
  case TK_CHAR16:
  case TK_ENUM:
    // Value of enum or primitive types can be indicated by MEMBER_ID_INVALID Id
    // (Section 7.5.2.11.1)
    return MEMBER_ID_INVALID;
  case TK_STRING8:
  case TK_STRING16:
  case TK_BITMASK:
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    return index;
  case TK_STRUCTURE:
  case TK_UNION:
    {
      DynamicTypeMember_rch member;
      if (type_->get_member_by_index(member, index) != DDS::RETCODE_OK) {
        return MEMBER_ID_INVALID;
      }
      return member->get_descriptor().id;
    }
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::get_member_id_at_index -")
               ACE_TEXT(" Calling on an unexpected type %C\n"), typekind_to_string(tk)));
  }
  return MEMBER_ID_INVALID;
}

ACE_CDR::ULong DynamicData::get_item_count()
{
  if (item_count_ != ITEM_COUNT_INVALID) {
    return item_count_;
  }

  DCPS::Message_Block_Ptr dup(chain_->duplicate());
  setup_stream(dup.get());

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
  case TK_CHAR16:
  case TK_ENUM:
    return 1;
  case TK_STRING8:
  case TK_STRING16:
    {
      ACE_CDR::ULong bytes;
      if (!(strm_ >> bytes)) {
        return 0;
      }
      return (type_->get_kind() == TK_STRING8) ? bytes : bytes/2;
    }
  case TK_BITMASK:
    {
      return descriptor_.bound[0];
    }
  case TK_STRUCTURE:
    // When optional members are supported, this needs to be updated to count
    // the number of members actually present in the DynamicData object.
    return type_->get_member_count();
  case TK_UNION:
    {
      const DynamicType_rch disc_type = get_base_type(descriptor_.discriminator_type);
      const ExtensibilityKind extend = descriptor_.extensibility_kind;
      ACE_CDR::Long label;
      if (!read_discriminator(disc_type, extend, label)) {
        return 0;
      }

      DynamicTypeMembersById members;
      type_->get_all_members(members);
      for (DynamicTypeMembersById::const_iterator it = members.begin(); it != members.end(); ++it) {
        const MemberDescriptor md = it->second->get_descriptor();
        if (md.is_default_label) {
          return 2;
        }
        const UnionCaseLabelSeq& labels = md.label;
        for (ACE_CDR::ULong i = 0; i < labels.length(); ++i) {
          if (label == labels[i]) {
            return 2;
          }
        }
      }
      return 1;
    }
  case TK_SEQUENCE:
    {
      const DynamicType_rch elem_type = get_base_type(descriptor_.element_type);
      ACE_CDR::ULong size;
      if (!is_primitive(elem_type->get_kind(), size)) {
        size_t dheader;
        if (!strm_.read_delimiter(dheader)) {
          return 0;
        }
      }
      ACE_CDR::ULong length;
      if (!(strm_ >> length)) {
        return 0;
      }
      return length;
    }
  case TK_ARRAY:
    {
      ACE_CDR::ULong length = 1;
      for (ACE_CDR::ULong i = 0; i < descriptor_.bound.length(); ++i) {
        length *= descriptor_.bound[i];
      }
      return length;
    }
  case TK_MAP:
    {
      const DynamicType_rch key_type = get_base_type(descriptor_.key_element_type);
      const DynamicType_rch elem_type = get_base_type(descriptor_.element_type);
      ACE_CDR::ULong key_size, elem_size;
      if (is_primitive(key_type->get_kind(), key_size) &&
          is_primitive(elem_type->get_kind(), elem_size)) {
        ACE_CDR::ULong length;
        if (!(strm_ >> length)) {
          return 0;
        }
        return length;
      }

      size_t dheader;
      if (!strm_.read_delimiter(dheader)) {
        return 0;
      }
      const size_t end_of_map = strm_.rpos() + dheader;

      ACE_CDR::ULong length = 0;
      while (strm_.rpos() < end_of_map) {
        if (!skip_member(key_type) || !skip_member(elem_type)) {
          return 0;
        }
        ++length;
      }
      return length;
    }
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::get_item_count -")
               ACE_TEXT(" Calling on an unexpected type %C\n"), typekind_to_string(tk)));
  }
  return 0;
}

DynamicData DynamicData::clone() const
{
  return DynamicData(chain_, strm_.encoding(), type_);
}

bool DynamicData::is_type_supported(TypeKind tk, const char* func_name)
{
  ACE_CDR::ULong size;
  if (!is_primitive(tk, size) && tk != TK_STRING8 && tk != TK_STRING16) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::%C -")
                 ACE_TEXT(" Called on an unsupported type (%C)\n"), func_name, typekind_to_string(tk)));
    }
    return false;
  }
  return true;
}

template<typename ValueType>
bool DynamicData::read_value(ValueType& value, TypeKind tk)
{
  switch (tk) {
  case TK_INT32:
  case TK_UINT32:
  case TK_INT16:
  case TK_UINT16:
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_CHAR16:
  case TK_BYTE:
  case TK_BOOLEAN:
  case TK_STRING8:
  case TK_STRING16:
    return strm_ >> value;
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::read_value -")
               ACE_TEXT(" Calling on an unexpected type %C\n"), typekind_to_string(tk)));
  }
  return false;
}

template<TypeKind MemberTypeKind, typename MemberType>
bool DynamicData::get_value_from_struct(MemberType& value, MemberId id,
                                        TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  MemberDescriptor md;
  if (get_from_struct_common_checks(md, id, MemberTypeKind)) {
    return skip_to_struct_member(md, id) && read_value(value, MemberTypeKind);
  } else if (get_from_struct_common_checks(md, id, enum_or_bitmask)) {
    const DynamicType_rch member_type = md.type.lock();
    if (member_type) {
      const LBound bit_bound = get_base_type(member_type)->get_descriptor().bound[0];
      return bit_bound >= lower && bit_bound <= upper &&
        skip_to_struct_member(md, id) && read_value(value, MemberTypeKind);
    }
  }

  return false;
}

bool DynamicData::get_union_selected_member(MemberDescriptor& out_md)
{
  const ExtensibilityKind ek = descriptor_.extensibility_kind;
  if (ek == APPENDABLE || ek == MUTABLE) {
    size_t size;
    if (!strm_.read_delimiter(size)) {
      return false;
    }
  }

  const DynamicType_rch disc_type = get_base_type(descriptor_.discriminator_type);
  ACE_CDR::Long label;
  if (!read_discriminator(disc_type, ek, label)) {
    return false;
  }

  DynamicTypeMembersById members;
  type_->get_all_members(members);

  bool has_default = false;
  MemberDescriptor default_member;
  for (DynamicTypeMembersById::const_iterator it = members.begin(); it != members.end(); ++it) {
    const MemberDescriptor md = it->second->get_descriptor();
    const UnionCaseLabelSeq& labels = md.label;
    for (ACE_CDR::ULong i = 0; i < labels.length(); ++i) {
      if (label == labels[i]) {
        out_md = md;
        return true;
      }
    }

    if (md.is_default_label) {
      has_default = true;
      default_member = md;
    }
  }

  if (has_default) {
    out_md = default_member;
  } else {
    // The union has no selected member.
    out_md.id = MEMBER_ID_INVALID;
  }
  return true;
}

bool DynamicData::get_from_union_common_checks(MemberId id, const char* func_name, MemberDescriptor& md)
{
  if (!get_union_selected_member(md)) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::%C - Could not find")
                 ACE_TEXT(" MemberDescriptor for the selected union member\n"), func_name));
    }
    return false;
  }

  if (md.id == MEMBER_ID_INVALID) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::%C - Union has no selected member\n"), func_name));
    }
    return false;
  }

  if (md.id != id) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::%C -")
                 ACE_TEXT(" ID of the selected member (%d) is not the requested ID (%d)\n"),
                 func_name, md.id, id));
    }
    return false;
  }
  return true;
}

template<TypeKind MemberTypeKind, typename MemberType>
bool DynamicData::get_value_from_union(MemberType& value, MemberId id,
                                       TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  MemberDescriptor md;
  if (!get_from_union_common_checks(id, "get_value_from_union", md)) {
    return false;
  }

  const DynamicType_rch member_type = md.type.lock();
  if (!member_type) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::get_value_from_union -")
                 ACE_TEXT(" Could not get DynamicType of the selected member\n")));
    }
    return false;
  }

  const DynamicType_rch selected_type = get_base_type(member_type);
  const TypeKind selected_tk = selected_type->get_kind();
  if (selected_tk != MemberTypeKind && selected_tk != enum_or_bitmask) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_value_from_union -")
                 ACE_TEXT(" Could not read a value of type %C from type %C\n"),
                 typekind_to_string(MemberTypeKind), typekind_to_string(selected_tk)));
    }
    return false;
  }

  if (descriptor_.extensibility_kind == MUTABLE) {
    unsigned member_id;
    size_t member_size;
    bool must_understand;
    if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
      return false;
    }
  }

  if (selected_tk == MemberTypeKind) {
    return read_value(value, MemberTypeKind);
  }

  const LBound bit_bound = selected_type->get_descriptor().bound[0];
  return bit_bound >= lower && bit_bound <= upper &&
    read_value(value, MemberTypeKind);
}

bool DynamicData::skip_to_sequence_element(MemberId id)
{
  const DynamicType_rch elem_type = get_base_type(descriptor_.element_type);
  ACE_CDR::ULong size;
  if (is_primitive(elem_type->get_kind(), size)) {
    ACE_CDR::ULong length, index;
    return (strm_ >> length) &&
      get_index_from_id(id, index, length) &&
      strm_.skip(index, size);
  } else {
    size_t dheader;
    ACE_CDR::ULong length, index;
    if (!strm_.read_delimiter(dheader) || !(strm_ >> length) ||
        !get_index_from_id(id, index, length)) {
      return false;
    }

    for (ACE_CDR::ULong i = 0; i < index; ++i) {
      if (!skip_member(elem_type)) {
        return false;
      }
    }
    return true;
  }
}

bool DynamicData::skip_to_array_element(MemberId id)
{
  const DynamicType_rch elem_type = get_base_type(descriptor_.element_type);
  ACE_CDR::ULong length = 1;
  for (ACE_CDR::ULong i = 0; i < descriptor_.bound.length(); ++i) {
    length *= descriptor_.bound[i];
  }

  ACE_CDR::ULong size;
  if (is_primitive(elem_type->get_kind(), size)) {
    ACE_CDR::ULong index;
    return get_index_from_id(id, index, length) && strm_.skip(index, size);
  } else {
    size_t dheader;
    ACE_CDR::ULong index;
    if (!strm_.read_delimiter(dheader) || !get_index_from_id(id, index, length)) {
      return false;
    }

    for (ACE_CDR::ULong i = 0; i < index; ++i) {
      if (!skip_member(elem_type)) {
        return false;
      }
    }
    return true;
  }
}

bool DynamicData::skip_to_map_element(MemberId id)
{
  const DynamicType_rch key_type = get_base_type(descriptor_.key_element_type);
  const DynamicType_rch elem_type = get_base_type(descriptor_.element_type);
  ACE_CDR::ULong key_size, elem_size;

  if (is_primitive(key_type->get_kind(), key_size) &&
      is_primitive(elem_type->get_kind(), elem_size)) {
    ACE_CDR::ULong length, index;
    if (!(strm_ >> length) || !get_index_from_id(id, index, length)) {
      return false;
    }

    for (ACE_CDR::ULong i = 0; i < index; ++i) {
      if (!strm_.skip(1, key_size) || !strm_.skip(1, elem_size)) {
        return false;
      }
    }
    return strm_.skip(1, key_size);
  } else {
    size_t dheader;
    ACE_CDR::ULong index;
    if (!strm_.read_delimiter(dheader) || !get_index_from_id(id, index, ACE_UINT32_MAX)) {
      return false;
    }
    const size_t end_of_map = strm_.rpos() + dheader;

    for (ACE_CDR::ULong i = 0; i < index; ++i) {
      if (strm_.rpos() >= end_of_map || !skip_member(key_type) || !skip_member(elem_type)) {
        return false;
      }
    }
    return (strm_.rpos() < end_of_map) && skip_member(key_type);
  }
}

template<TypeKind ElementTypeKind, typename ElementType>
bool DynamicData::get_value_from_collection(ElementType& value, MemberId id, TypeKind collection_tk,
                                            TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DynamicType_rch elem_type = get_base_type(descriptor_.element_type);
  const TypeKind elem_tk = elem_type->get_kind();

  if (elem_tk != ElementTypeKind && elem_tk != enum_or_bitmask) {
    if (DCPS::DCPS_debug_level >= 1) {
      const char* collection_str;
      switch (collection_tk) {
      case TK_SEQUENCE:
        collection_str = "sequence";
        break;
      case TK_ARRAY:
        collection_str = "array";
        break;
      case TK_MAP:
        collection_str = "map";
        break;
      default:
        return false;
      }

      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_value_from_collection -")
                 ACE_TEXT(" Could not read a value of type %C from %C with element type %C\n"),
                 typekind_to_string(ElementTypeKind), collection_str, typekind_to_string(elem_tk)));
    }
    return false;
  }

  if (elem_tk == enum_or_bitmask) {
    const LBound bit_bound = elem_type->get_descriptor().bound[0];
    if (bit_bound < lower || bit_bound > upper) {
      return false;
    }
  }

  switch (collection_tk) {
  case TK_SEQUENCE:
    if (!skip_to_sequence_element(id)) {
      return false;
    }
    break;
  case TK_ARRAY:
    if (!skip_to_array_element(id)) {
      return false;
    }
    break;
  case TK_MAP:
    if (!skip_to_map_element(id)) {
      return false;
    }
    break;
  default:
    return false;
  }

  return read_value(value, ElementTypeKind);
}

void DynamicData::setup_stream(ACE_Message_Block* chain)
{
  strm_ = DCPS::Serializer(chain, encoding_);
  if (reset_align_state_) {
    strm_.rdstate(align_state_);
  }
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicData::get_single_value(ValueType& value, MemberId id,
                                                TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (!is_type_supported(ValueTypeKind, "get_single_value")) {
    return DDS::RETCODE_ERROR;
  }

  DCPS::Message_Block_Ptr dup(chain_->duplicate());
  setup_stream(dup.get());

  const TypeKind tk = type_->get_kind();
  bool good = true;

  if (tk == enum_or_bitmask) {
    // Per XTypes spec, the value of a DynamicData object of primitive type or TK_ENUM is
    // accessed with MEMBER_ID_INVALID Id. However, there is only a single value in such
    // a DynamicData object, and checking for MEMBER_ID_INVALID from the input is perhaps
    // unnecessary. So, we read the value immediately here.
    const LBound bit_bound = descriptor_.bound[0];
    good = bit_bound >= lower && bit_bound <= upper && read_value(value, ValueTypeKind);
  } else {
    switch (tk) {
    case ValueTypeKind:
      good = read_value(value, ValueTypeKind);
      break;
    case TK_STRUCTURE:
      good = get_value_from_struct<ValueTypeKind>(value, id, enum_or_bitmask, lower, upper);
      break;
    case TK_UNION:
      good = get_value_from_union<ValueTypeKind>(value, id, enum_or_bitmask, lower, upper);
      break;
    case TK_SEQUENCE:
    case TK_ARRAY:
    case TK_MAP:
      good = get_value_from_collection<ValueTypeKind>(value, id, tk, enum_or_bitmask, lower, upper);
      break;
    default:
      good = false;
      break;
    }
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_single_value -")
               ACE_TEXT(" Failed to read a value of %C from a DynamicData object of type %C\n"),
               typekind_to_string(ValueTypeKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicData::get_int32_value(ACE_CDR::Long& value, MemberId id)
{
  return get_single_value<TK_INT32>(value, id, TK_ENUM,
                                    static_cast<LBound>(17), static_cast<LBound>(32));
}

DDS::ReturnCode_t DynamicData::get_uint32_value(ACE_CDR::ULong& value, MemberId id)
{
  return get_single_value<TK_UINT32>(value, id, TK_BITMASK,
                                     static_cast<LBound>(17), static_cast<LBound>(32));
}

DDS::ReturnCode_t DynamicData::get_int8_value(ACE_CDR::Int8& value, MemberId id)
{
  ACE_InputCDR::to_int8 to_int8(value);
  return get_single_value<TK_INT8>(to_int8, id, TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicData::get_uint8_value(ACE_CDR::UInt8& value, MemberId id)
{
  ACE_InputCDR::to_uint8 to_uint8(value);
  return get_single_value<TK_UINT8>(to_uint8, id, TK_BITMASK, 1, 8);
}

DDS::ReturnCode_t DynamicData::get_int16_value(ACE_CDR::Short& value, MemberId id)
{
  return get_single_value<TK_INT16>(value, id, TK_ENUM, 9, 16);
}

DDS::ReturnCode_t DynamicData::get_uint16_value(ACE_CDR::UShort& value, MemberId id)
{
  return get_single_value<TK_UINT16>(value, id, TK_BITMASK, 9, 16);
}

DDS::ReturnCode_t DynamicData::get_int64_value(ACE_CDR::LongLong& value, MemberId id)
{
  return get_single_value<TK_INT64>(value, id);
}

DDS::ReturnCode_t DynamicData::get_uint64_value(ACE_CDR::ULongLong& value, MemberId id)
{
  return get_single_value<TK_UINT64>(value, id, TK_BITMASK, 33, 64);
}

DDS::ReturnCode_t DynamicData::get_float32_value(ACE_CDR::Float& value, MemberId id)
{
  return get_single_value<TK_FLOAT32>(value, id);
}

DDS::ReturnCode_t DynamicData::get_float64_value(ACE_CDR::Double& value, MemberId id)
{
  return get_single_value<TK_FLOAT64>(value, id);
}

DDS::ReturnCode_t DynamicData::get_float128_value(ACE_CDR::LongDouble& value, MemberId id)
{
  return get_single_value<TK_FLOAT128>(value, id);
}

template<TypeKind CharKind, TypeKind StringKind, typename ToCharT, typename CharT>
DDS::ReturnCode_t DynamicData::get_char_common(CharT& value, MemberId id)
{
  DCPS::Message_Block_Ptr dup(chain_->duplicate());
  setup_stream(dup.get());

  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case CharKind:
    {
      ToCharT wrap(value);
      good = strm_ >> wrap;
      break;
    }
  case StringKind:
    {
      CharT* str;
      if (!(strm_ >> str)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_char_common -")
                     ACE_TEXT(" Failed to read wstring with ID %d\n"), id));
        }
        good = false;
        break;
      }
      ACE_CDR::ULong index;
      const size_t str_len = ACE_OS::strlen(str);
      if (!get_index_from_id(id, index, str_len)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_char_common -")
                     ACE_TEXT(" ID %d is not valid in a string (or wstring) with length %d\n"),
                     id, str_len));
        }
        good = false;
      } else {
        value = str[index];
      }
      break;
    }
  case TK_STRUCTURE:
    {
      ToCharT wrap(value);
      good = get_value_from_struct<CharKind>(wrap, id);
      break;
    }
  case TK_UNION:
    {
      ToCharT wrap(value);
      good = get_value_from_union<CharKind>(wrap, id);
      break;
    }
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    {
      ToCharT wrap(value);
      good = get_value_from_collection<CharKind>(wrap, id, tk);
      break;
    }
  default:
    good = false;
    break;
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Dynamic::get_char_common -")
               ACE_TEXT(" Failed to read DynamicData object of type %C\n"), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicData::get_char8_value(ACE_CDR::Char& value, MemberId id)
{
  return get_char_common<TK_CHAR8, TK_STRING8, ACE_InputCDR::to_char>(value, id);
}

DDS::ReturnCode_t DynamicData::get_char16_value(ACE_CDR::WChar& value, MemberId id)
{
  return get_char_common<TK_CHAR16, TK_STRING16, ACE_InputCDR::to_wchar>(value, id);
}

DDS::ReturnCode_t DynamicData::get_byte_value(ACE_CDR::Octet& value, MemberId id)
{
  ACE_InputCDR::to_octet to_octet(value);
  return get_single_value<TK_BYTE>(to_octet, id);
}

template<typename UIntType, TypeKind UIntTypeKind>
bool DynamicData::get_boolean_from_bitmask(ACE_CDR::ULong index, ACE_CDR::Boolean& value)
{
  UIntType bitmask;
  if (!read_value(bitmask, UIntTypeKind)) {
    return false;
  }

  value = ((1ULL << index) & bitmask) ? true : false;
  return true;
}

DDS::ReturnCode_t DynamicData::get_boolean_value(ACE_CDR::Boolean& value, MemberId id)
{
  DCPS::Message_Block_Ptr dup(chain_->duplicate());
  setup_stream(dup.get());

  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case TK_BOOLEAN:
    good = strm_ >> ACE_InputCDR::to_boolean(value);
    break;
  case TK_BITMASK:
    {
      const LBound bit_bound = descriptor_.bound[0];
      ACE_CDR::ULong index;
      if (!get_index_from_id(id, index, bit_bound)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_boolean_value -")
                     ACE_TEXT(" Id %d is not valid in a bitmask with bit_bound %d\n"),
                     id, bit_bound));
        }
        good = false;
        break;
      }

      // Bit with index 0 is the least-significant bit of the representing integer.
      if (bit_bound >= 1 && bit_bound <= 8) {
        ACE_CDR::UInt8 bitmask;
        ACE_InputCDR::to_uint8 to_uint8(bitmask);
        if (!read_value(to_uint8, TK_UINT8)) {
          good = false;
        } else {
          value = ((1 << index) & bitmask) ? true : false;
        }
      } else if (bit_bound >= 9 && bit_bound <= 16) {
        good = get_boolean_from_bitmask<ACE_CDR::UInt16, TK_UINT16>(index, value);
      } else if (bit_bound >= 17 && bit_bound <= 33) {
        good = get_boolean_from_bitmask<ACE_CDR::UInt32, TK_UINT32>(index, value);
      } else {
        good = get_boolean_from_bitmask<ACE_CDR::UInt64, TK_UINT64>(index, value);
      }
      break;
    }
  case TK_STRUCTURE:
    {
      ACE_InputCDR::to_boolean to_bool(value);
      good = get_value_from_struct<TK_BOOLEAN>(to_bool, id);
      break;
    }
  case TK_UNION:
    {
      ACE_InputCDR::to_boolean to_bool(value);
      good = get_value_from_union<TK_BOOLEAN>(to_bool, id);
      break;
    }
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    {
      ACE_InputCDR::to_boolean to_bool(value);
      good = get_value_from_collection<TK_BOOLEAN>(to_bool, id, tk);
      break;
    }
  default:
    good = false;
    break;
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Dynamic::get_boolean_value -")
               ACE_TEXT(" Failed to read DynamicData object of type %C\n"), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicData::get_string_value(ACE_CDR::Char*& value, MemberId id)
{
  return get_single_value<TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicData::get_wstring_value(ACE_CDR::WChar*& value, MemberId id)
{
  return get_single_value<TK_STRING16>(value, id);
}

DDS::ReturnCode_t DynamicData::get_complex_value(DynamicData& value, MemberId id)
{
  DCPS::Message_Block_Ptr dup(chain_->duplicate());
  setup_stream(dup.get());

  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case TK_STRUCTURE:
  case TK_UNION:
    {
      DynamicTypeMember_rch member;
      const DDS::ReturnCode_t retcode = type_->get_member(member, id);
      if (retcode != DDS::RETCODE_OK) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_complex_value -")
                     ACE_TEXT(" Failed to get DynamicTypeMember for member with ID %d\n"), id));
        }
        good = false;
        break;
      }

      if (tk == TK_STRUCTURE) {
        const MemberDescriptor md = member->get_descriptor();
        if (!skip_to_struct_member(md, id)) {
          good = false;
        } else {
          const DynamicType_rch member_type = md.type.lock();
          if (!member_type) {
            good = false;
          } else {
            value = DynamicData(strm_, member_type);
          }
        }
        break;
      } else {
        MemberDescriptor md;
        if (!get_union_selected_member(md)) {
          if (DCPS::DCPS_debug_level >= 1) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_complex_value -")
                       ACE_TEXT(" Could not find MemberDescriptor for the selected union member\n")));
          }
          good = false;
          break;
        }

        if (md.id != id) {
          if (DCPS::DCPS_debug_level >= 1) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_complex_value -")
                       ACE_TEXT(" ID of the selected member (%d) is not the requested ID (%d)"),
                       md.id, id));
          }
          good = false;
          break;
        }

        if (descriptor_.extensibility_kind == MUTABLE) {
          unsigned id;
          size_t size;
          bool must_understand;
          if (!strm_.read_parameter_id(id, size, must_understand)) {
            good = false;
            break;
          }
        }
        const DynamicType_rch member_type = md.type.lock();
        if (!member_type) {
          good = false;
        } else {
          value = DynamicData(strm_, member_type);
        }
        break;
      }
    }
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    {
      if ((tk == TK_SEQUENCE && !skip_to_sequence_element(id)) ||
          (tk == TK_ARRAY && !skip_to_array_element(id)) ||
          (tk == TK_MAP && !skip_to_map_element(id))) {
        good = false;
      } else {
        value = DynamicData(strm_, descriptor_.element_type);
      }
      break;
    }
  default:
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_complex_value -")
                 ACE_TEXT(" Called on an unsupported type (%C)\n"), typekind_to_string(tk)));
    }
    good = false;
    break;
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

template<typename SequenceType>
bool DynamicData::read_values(SequenceType& value, TypeKind elem_tk)
{
  switch(elem_tk) {
  case TK_INT32:
  case TK_UINT32:
  case TK_INT16:
  case TK_UINT16:
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_CHAR16:
  case TK_BYTE:
  case TK_BOOLEAN:
  case TK_STRING8:
  case TK_STRING16:
    return strm_ >> value;
  }

  //TODO CLAYTON: FIND WHAT HAS A DHEADER
  if(elem_tk == TK_ENUM) {
    size_t dheader = 0;
    if (!strm_.read_delimiter(dheader)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_to_struct_member -")
                    ACE_TEXT(" Failed to read DHEADER for member ID\n")));
      }
      return false;
    }
    return strm_ >> value;
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::read_values -")
               ACE_TEXT(" Calling on an unexpected element type %C\n"), typekind_to_string(elem_tk)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicData::get_values_from_struct(SequenceType& value, MemberId id,
                                         TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  MemberDescriptor md;
  if (get_from_struct_common_checks(md, id, ElementTypeKind, true)) {
    return skip_to_struct_member(md, id) && read_values(value, ElementTypeKind);
  } else if (get_from_struct_common_checks(md, id, enum_or_bitmask, true)) {
    const DynamicType_rch member_type = md.type.lock();
    if (member_type) {
      const LBound bit_bound = get_base_type(member_type)->get_descriptor().element_type->get_descriptor().bound[0];
      return bit_bound >= lower && bit_bound <= upper &&
        skip_to_struct_member(md, id) && read_values(value, enum_or_bitmask);
    }
  }

  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicData::get_values_from_union(SequenceType& value, MemberId id,
                                        TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  MemberDescriptor md;
  if (!get_from_union_common_checks(id, "get_values_from_union", md)) {
    return false;
  }

  const DynamicType_rch member_type = md.type.lock();
  if (!member_type) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::get_values_from_union -")
                 ACE_TEXT(" Could not get DynamicType of the selected member\n")));
    }
    return false;
  }

  const DynamicType_rch selected_type = get_base_type(member_type);
  const TypeKind selected_tk = selected_type->get_kind();
  if (selected_tk != TK_SEQUENCE) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_values_from_union -")
                 ACE_TEXT(" The selected member is not a sequence, but %C\n"),
                 typekind_to_string(selected_tk)));
    }
    return false;
  }

  const DynamicType_rch elem_type = get_base_type(selected_type->get_descriptor().element_type);
  const TypeKind elem_tk = elem_type->get_kind();
  if (elem_tk != ElementTypeKind && elem_tk != enum_or_bitmask) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_values_from_union -")
                 ACE_TEXT(" Could not read a sequence of %C from a sequence of %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_tk)));
    }
    return false;
  }

  if (descriptor_.extensibility_kind == MUTABLE) {
    unsigned member_id;
    size_t member_size;
    bool must_understand;
    if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
      return false;
    }
  }

  if (elem_tk == ElementTypeKind) {
    return read_values(value, elem_tk);
  }

  const LBound bit_bound = elem_type->get_descriptor().bound[0];
  return bit_bound >= lower && bit_bound <= upper && read_values(value, elem_tk);
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicData::get_values_from_sequence(SequenceType& value, MemberId id,
                                           TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DynamicType_rch elem_type = get_base_type(descriptor_.element_type);
  const TypeKind elem_tk = elem_type->get_kind();

  if (elem_tk == ElementTypeKind) {
    return read_values(value, elem_tk);
  } else if (elem_tk == enum_or_bitmask) {
    // Read from a sequence of enums or bitmasks.
    const LBound bit_bound = elem_type->get_descriptor().bound[0];
    return bit_bound >= lower && bit_bound <= upper && read_values(value, elem_tk);
  } else if (elem_tk == TK_SEQUENCE) {
    const DynamicType_rch nested_elem_type = get_base_type(elem_type->get_descriptor().element_type);
    const TypeKind nested_elem_tk = nested_elem_type->get_kind();
    if (nested_elem_tk == ElementTypeKind) {
      // Read from a sequence of sequence of ElementTypeKind.
      return skip_to_sequence_element(id) && read_values(value, nested_elem_tk);
    } else if (nested_elem_tk == enum_or_bitmask) {
      // Read from a sequence of sequence of enums or bitmasks.
      const LBound bit_bound = nested_elem_type->get_descriptor().bound[0];
      return bit_bound >= lower && bit_bound <= upper &&
        skip_to_sequence_element(id) && read_values(value, nested_elem_tk);
    }
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_values_from_sequence -")
               ACE_TEXT(" Could not read a sequence of %C from an incompatible type\n"),
               typekind_to_string(ElementTypeKind)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicData::get_values_from_array(SequenceType& value, MemberId id,
                                        TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DynamicType_rch elem_type = get_base_type(descriptor_.element_type);
  if (elem_type->get_kind() != TK_SEQUENCE) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_values_from_array -")
                 ACE_TEXT(" Could not read a sequence of %C from an array of %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_type->get_kind())));
    }
    return false;
  }

  const DynamicType_rch nested_elem_type = get_base_type(elem_type->get_descriptor().element_type);
  const TypeKind nested_elem_tk = nested_elem_type->get_kind();
  if (nested_elem_tk == ElementTypeKind) {
    return skip_to_array_element(id) && read_values(value, nested_elem_tk);
  } else if (nested_elem_tk == enum_or_bitmask) {
    const LBound bit_bound = nested_elem_type->get_descriptor().bound[0];
    return bit_bound >= lower && bit_bound <= upper &&
      skip_to_array_element(id) && read_values(value, nested_elem_tk);
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_values_from_array -")
               ACE_TEXT(" Could not read a sequence of %C from an array of sequence of %C\n"),
               typekind_to_string(ElementTypeKind), typekind_to_string(nested_elem_tk)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicData::get_values_from_map(SequenceType& value, MemberId id,
                                      TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DynamicType_rch elem_type = get_base_type(descriptor_.element_type);
  if (elem_type->get_kind() != TK_SEQUENCE) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_values_from_map -")
                 ACE_TEXT(" Getting sequence<%C> from a map with element type of %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_type->get_kind())));
    }
    return false;
  }

  const DynamicType_rch nested_elem_type = get_base_type(elem_type->get_descriptor().element_type);
  const TypeKind nested_elem_tk = nested_elem_type->get_kind();
  if (nested_elem_tk == ElementTypeKind) {
    return skip_to_map_element(id) && read_values(value, nested_elem_tk);
  } else if (nested_elem_tk == enum_or_bitmask) {
    const LBound bit_bound = nested_elem_type->get_descriptor().bound[0];
    return bit_bound >= lower && bit_bound <= upper &&
      skip_to_map_element(id) && read_values(value, nested_elem_tk);
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_values_from_map -")
               ACE_TEXT(" Could not read a sequence of %C from a map with element type sequence of %C\n"),
               typekind_to_string(ElementTypeKind), typekind_to_string(nested_elem_tk)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
DDS::ReturnCode_t DynamicData::get_sequence_values(SequenceType& value, MemberId id,
                                                   TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (!is_type_supported(ElementTypeKind, "get_sequence_values")) {
    return DDS::RETCODE_ERROR;
  }

  DCPS::Message_Block_Ptr dup(chain_->duplicate());
  setup_stream(dup.get());

  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case TK_STRUCTURE:
    good = get_values_from_struct<ElementTypeKind>(value, id, enum_or_bitmask, lower, upper);
    break;
  case TK_UNION:
    good = get_values_from_union<ElementTypeKind>(value, id, enum_or_bitmask, lower, upper);
    break;
  case TK_SEQUENCE:
    good = get_values_from_sequence<ElementTypeKind>(value, id, enum_or_bitmask, lower, upper);
    break;
  case TK_ARRAY:
    good = get_values_from_array<ElementTypeKind>(value, id, enum_or_bitmask, lower, upper);
    break;
  case TK_MAP:
    good = get_values_from_map<ElementTypeKind>(value, id, enum_or_bitmask, lower, upper);
    break;
  default:
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_sequence_values -")
                 ACE_TEXT(" A sequence<%C> can't be read as a member of type %C"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(tk)));
    }
    return DDS::RETCODE_ERROR;
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_sequence_values -")
               ACE_TEXT(" Failed to read sequence<%C> from a DynamicData object of type %C\n"),
               typekind_to_string(ElementTypeKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicData::get_int32_values(CORBA::LongSeq& value, MemberId id)
{
  return get_sequence_values<TK_INT32>(value, id, TK_ENUM, 17, 32);
}

DDS::ReturnCode_t DynamicData::get_uint32_values(CORBA::ULongSeq& value, MemberId id)
{
  return get_sequence_values<TK_UINT32>(value, id, TK_BITMASK, 17, 32);
}

DDS::ReturnCode_t DynamicData::get_int8_values(CORBA::Int8Seq& value, MemberId id)
{
  return get_sequence_values<TK_INT8>(value, id, TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicData::get_uint8_values(CORBA::UInt8Seq& value, MemberId id)
{
  return get_sequence_values<TK_UINT8>(value, id, TK_BITMASK, 1, 8);
}

DDS::ReturnCode_t DynamicData::get_int16_values(CORBA::ShortSeq& value, MemberId id)
{
  return get_sequence_values<TK_INT16>(value, id, TK_ENUM, 9, 16);
}

DDS::ReturnCode_t DynamicData::get_uint16_values(CORBA::UShortSeq& value, MemberId id)
{
  return get_sequence_values<TK_UINT16>(value, id, TK_BITMASK, 9, 16);
}

DDS::ReturnCode_t DynamicData::get_int64_values(CORBA::LongLongSeq& value, MemberId id)
{
  return get_sequence_values<TK_INT64>(value, id);
}

DDS::ReturnCode_t DynamicData::get_uint64_values(CORBA::ULongLongSeq& value, MemberId id)
{
  return get_sequence_values<TK_UINT64>(value, id, TK_BITMASK, 33, 64);
}

DDS::ReturnCode_t DynamicData::get_float32_values(CORBA::FloatSeq& value, MemberId id)
{
  return get_sequence_values<TK_FLOAT32>(value, id);
}

DDS::ReturnCode_t DynamicData::get_float64_values(CORBA::DoubleSeq& value, MemberId id)
{
  return get_sequence_values<TK_FLOAT64>(value, id);
}

DDS::ReturnCode_t DynamicData::get_float128_values(CORBA::LongDoubleSeq& value, MemberId id)
{
  return get_sequence_values<TK_FLOAT128>(value, id);
}

DDS::ReturnCode_t DynamicData::get_char8_values(CORBA::CharSeq& value, MemberId id)
{
  return get_sequence_values<TK_CHAR8>(value, id);
}

DDS::ReturnCode_t DynamicData::get_char16_values(CORBA::WCharSeq& value, MemberId id)
{
  return get_sequence_values<TK_CHAR16>(value, id);
}

DDS::ReturnCode_t DynamicData::get_byte_values(CORBA::OctetSeq& value, MemberId id)
{
  return get_sequence_values<TK_BYTE>(value, id);
}

DDS::ReturnCode_t DynamicData::get_boolean_values(CORBA::BooleanSeq& value, MemberId id)
{
  return get_sequence_values<TK_BOOLEAN>(value, id);
}

DDS::ReturnCode_t DynamicData::get_string_values(CORBA::StringSeq& value, MemberId id)
{
  return get_sequence_values<TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicData::get_wstring_values(CORBA::WStringSeq& value, MemberId id)
{
  return get_sequence_values<TK_STRING16>(value, id);
}

bool DynamicData::skip_to_struct_member(const MemberDescriptor& member_desc, MemberId id)
{
  const ExtensibilityKind ek = descriptor_.extensibility_kind;
  if (ek == FINAL || ek == APPENDABLE) {
    if (ek == APPENDABLE) {
      size_t dheader = 0;
      if (!strm_.read_delimiter(dheader)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_to_struct_member -")
                     ACE_TEXT(" Failed to read DHEADER for member ID %d\n"), id));
        }
        return false;
      }
    }

    for (ACE_CDR::ULong i = 0; i < member_desc.index; ++i) {
      if (!skip_struct_member_by_index(i)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_to_struct_member -")
                     ACE_TEXT(" Failed to skip member at index %d\n"), i));
        }
        return false;
      }
    }
    return true;
  } else {
    size_t dheader = 0;
    if (!strm_.read_delimiter(dheader)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_to_struct_member -")
                   ACE_TEXT(" Failed to read DHEADER for member ID %d\n"), id));
      }
      return false;
    }

    const size_t end_of_sample = strm_.rpos() + dheader;
    ACE_CDR::ULong member_id;
    size_t member_size;
    while (true) {
      if (strm_.rpos() >= end_of_sample) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_to_struct_member -")
                     ACE_TEXT(" Could not find a member with ID %d\n"), id));
        }
        return false;
      }

      bool must_understand = false;
      if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_to_struct_member -")
                     ACE_TEXT(" Failed to read EMHEADER while finding member ID %d\n"), id));
        }
        return false;
      }

      if (member_id == id) {
        return true;
      }
      if (!strm_.skip(member_size)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_to_struct_member -")
                     ACE_TEXT(" Failed to skip a member with ID %d\n"), member_id));
        }
        return false;
      }
    }
  }
}

bool DynamicData::get_from_struct_common_checks(MemberDescriptor& md, MemberId id, TypeKind kind, bool is_sequence)
{
  DynamicTypeMember_rch member;
  const DDS::ReturnCode_t retcode = type_->get_member(member, id);
  if (retcode != DDS::RETCODE_OK) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_from_struct_common_checks -")
                 ACE_TEXT(" Failed to get DynamicTypeMember for member with ID %d\n"), id));
    }
    return false;
  }

  md = member->get_descriptor();
  const DynamicType_rch member_dt = md.type.lock();
  if (!member_dt) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::get_from_struct_common_checks -")
                 ACE_TEXT(" Could not get DynamicType for member with ID %d\n"), id));
    }
    return false;
  }

  const DynamicType_rch member_type = get_base_type(member_dt);
  const TypeKind member_kind = member_type->get_kind();

  if ((!is_sequence && member_kind != kind) || (is_sequence && member_kind != TK_SEQUENCE)) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::get_from_struct_common_checks -")
                 ACE_TEXT(" Member with ID %d has kind %C, not %C\n"),
                 id, typekind_to_string(member_kind),
                 is_sequence ? typekind_to_string(TK_SEQUENCE) : typekind_to_string(kind)));
    }
    return false;
  }

  if (member_kind == TK_SEQUENCE) {
    const TypeDescriptor member_td = member_type->get_descriptor();
    const TypeKind elem_kind = get_base_type(member_td.element_type)->get_kind();
    if (elem_kind != kind) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::get_from_struct_common_checks -")
                   ACE_TEXT(" Member with ID %d is a sequence of %C, not %C\n"),
                   id, typekind_to_string(elem_kind), typekind_to_string(kind)));
      }
      return false;
    }
  }

  return true;
}

bool DynamicData::skip_struct_member_by_index(ACE_CDR::ULong index)
{
  DynamicTypeMember_rch member;
  if (type_->get_member_by_index(member, index) != DDS::RETCODE_OK) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_struct_member_by_index -")
                 ACE_TEXT(" Failed to get DynamicTypeMember for member index %d\n"), index));
    }
    return DDS::RETCODE_ERROR;
  }

  const MemberDescriptor md = member->get_descriptor();
  const DynamicType_rch member_type = md.type.lock();
  return member_type && skip_member(member_type);
}

bool DynamicData::skip_member(DynamicType_rch member_type)
{
  member_type = get_base_type(member_type);
  const TypeKind member_kind = member_type->get_kind();

  switch (member_kind) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
    if (!skip("skip_member", "Failed to skip a member of size 1 byte", 1, 1)) {
      return false;
    }
    break;
  case TK_INT16:
  case TK_UINT16:
  case TK_CHAR16:
    if (!skip("skip_member", "Failed to skip a member of size 2 bytes", 1, 2)) {
      return false;
    }
    break;
  case TK_INT32:
  case TK_UINT32:
  case TK_FLOAT32:
    if (!skip("skip_member", "Failed to skip a member of size 4 bytes", 1, 4)) {
      return false;
    }
    break;
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT64:
    if (!skip("skip_member", "Failed to skip a member of size 8 bytes", 1, 8)) {
      return false;
    }
    break;
  case TK_FLOAT128:
    if (!skip("skip_member", "Failed to skip a member of size 16 bytes", 1, 16)) {
      return false;
    }
    break;
  case TK_STRING8:
  case TK_STRING16:
    {
      const char* str_kind = member_kind == TK_STRING8 ? "string" : "wstring";
      ACE_CDR::ULong bytes;
      if (!(strm_ >> bytes)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                     ACE_TEXT(" Failed to read length of a %C member\n"), str_kind));
        }
        return false;
      }

      const DCPS::String err_msg = DCPS::String("Failed to skip a ") + str_kind + " member";
      if (!skip("skip_member", err_msg.c_str(), bytes)) {
        return false;
      }
      break;
    }
  case TK_ENUM:
  case TK_BITMASK:
    {
      const TypeDescriptor member_td = member_type->get_descriptor();
      const ACE_CDR::ULong bit_bound = member_td.bound[0];
      const char* err_msg = member_kind == TK_ENUM ?
        "Failed to skip an enum member" : "Failed to skip a bitmask member";

      if (bit_bound >= 1 && bit_bound <= 8) {
        if (!skip("skip_member", err_msg, 1, 1)) {
          return false;
        }
      } else if (bit_bound >= 9 && bit_bound <= 16) {
        if (!skip("skip_member", err_msg, 1, 2)) {
          return false;
        }
      } else if (bit_bound >= 17 && bit_bound <= 32) {
        if (!skip("skip_member", err_msg, 1, 4)) {
          return false;
        }
      } else if (bit_bound >= 33 && bit_bound <= 64 && member_kind == TK_BITMASK) {
        if (!skip("skip_member", err_msg, 1, 8)) {
          return false;
        }
      } else {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member - Found a%C")
                     ACE_TEXT(" member with bit bound %d\n"),
                     member_kind == TK_ENUM ? "n enum" : " bitmask", bit_bound));
        }
        return false;
      }
      break;
    }
  case TK_STRUCTURE:
    return skip_struct_member(member_type);
  case TK_UNION:
    return skip_union_member(member_type);
  case TK_SEQUENCE:
    return skip_sequence_member(member_type);
  case TK_ARRAY:
    return skip_array_member(member_type);
  case TK_MAP:
    return skip_map_member(member_type);
  default:
    {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                   ACE_TEXT(" Found a member of kind %C\n"), typekind_to_string(member_kind)));
      }
      return false;
    }
  }

  return true;
}

bool DynamicData::skip_sequence_member(DynamicType_rch seq_type)
{
  const TypeDescriptor descriptor = seq_type->get_descriptor();
  const DynamicType_rch elem_type = get_base_type(descriptor.element_type);

  ACE_CDR::ULong primitive_size = 0;
  if (is_primitive(elem_type->get_kind(), primitive_size)) {
    ACE_CDR::ULong length;
    if (!(strm_ >> length)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_sequence_member -")
                   ACE_TEXT(" Failed to deserialize a primitive sequence member\n")));
      }
      return false;
    }

    return skip("skip_sequence_member", "Failed to skip a primitive sequence member",
                length, primitive_size);
  } else {
    return skip_collection_member(TK_SEQUENCE);
  }
}

bool DynamicData::skip_array_member(DynamicType_rch array_type)
{
  const TypeDescriptor descriptor = array_type->get_descriptor();
  const DynamicType_rch elem_type = get_base_type(descriptor.element_type);

  ACE_CDR::ULong primitive_size = 0;
  if (is_primitive(elem_type->get_kind(), primitive_size)) {
    const LBoundSeq& bounds = descriptor.bound;
    ACE_CDR::ULong num_elems = 1;
    for (unsigned i = 0; i < bounds.length(); ++i) {
      num_elems *= bounds[i];
    }

    return skip("skip_array_member", "Failed to skip a primitive array member",
                num_elems, primitive_size);
  } else {
    return skip_collection_member(TK_ARRAY);
  }
}

bool DynamicData::skip_map_member(DynamicType_rch map_type)
{
  const TypeDescriptor descriptor = map_type->get_descriptor();
  const DynamicType_rch elem_type = get_base_type(descriptor.element_type);
  const DynamicType_rch key_type = get_base_type(descriptor.key_element_type);

  ACE_CDR::ULong key_primitive_size = 0, elem_primitive_size = 0;
  if (is_primitive(key_type->get_kind(), key_primitive_size) &&
      is_primitive(elem_type->get_kind(), elem_primitive_size)) {
    ACE_CDR::ULong length;
    if (!(strm_ >> length)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_map_member -")
                   ACE_TEXT(" Failed to deserialize length of a primitive map member\n")));
      }
      return false;
    }

    for (unsigned i = 0; i < length; ++i) {
      if (!skip("skip_map_member", "Failed to skip a key of a primitive map member",
                1, key_primitive_size)) {
        return false;
      }
      if (!skip("skip_map_member", "Failed to skip an element of a primitive map member",
                1, elem_primitive_size)) {
        return false;
      }
    }
    return true;
  } else {
    return skip_collection_member(TK_MAP);
  }
}

bool DynamicData::skip_collection_member(TypeKind kind)
{
  if (kind != TK_SEQUENCE && kind != TK_ARRAY && kind != TK_MAP) {
    return false;
  }
  const char* kind_str = typekind_to_string(kind);

  size_t dheader;
  if (!strm_.read_delimiter(dheader)) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_collection_member -")
                 ACE_TEXT(" Failed to deserialize DHEADER of a non-primitive %C member\n"),
                 kind_str));
    }
    return false;
  }

  const DCPS::String err_msg = DCPS::String("Failed to skip a non-primitive ") + kind_str + " member";
  return skip("skip_collection_member", err_msg.c_str(), dheader);
}

bool DynamicData::skip_struct_member(const DynamicType_rch& struct_type)
{
  DynamicData struct_data(strm_.current(), strm_.encoding(), struct_type);
  return struct_data.skip_all();
}

bool DynamicData::skip_union_member(const DynamicType_rch& union_type)
{
  DynamicData union_data(strm_.current(), strm_.encoding(), union_type);
  return union_data.skip_all();
}

bool DynamicData::read_discriminator(const DynamicType_rch& disc_type, ExtensibilityKind union_ek, ACE_CDR::Long& label)
{
  if (union_ek == MUTABLE) {
    unsigned id;
    size_t size;
    bool must_understand;
    if (!strm_.read_parameter_id(id, size, must_understand)) { return false; }
  }

  const TypeKind disc_tk = disc_type->get_kind();
  switch (disc_tk) {
  case TK_BOOLEAN:
    {
      ACE_CDR::Boolean value;
      if (!(strm_ >> ACE_InputCDR::to_boolean(value))) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_BYTE:
    {
      ACE_CDR::Octet value;
      if (!(strm_ >> ACE_InputCDR::to_octet(value))) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_CHAR8:
    {
      ACE_CDR::Char value;
      if (!(strm_ >> ACE_InputCDR::to_char(value))) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_CHAR16:
    {
      ACE_CDR::WChar value;
      if (!(strm_ >> ACE_InputCDR::to_wchar(value))) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_INT8:
    {
      ACE_CDR::Int8 value;
      if (!(strm_ >> ACE_InputCDR::to_int8(value))) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_UINT8:
    {
      ACE_CDR::UInt8 value;
      if (!(strm_ >> ACE_InputCDR::to_uint8(value))) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_INT16:
    {
      ACE_CDR::Short value;
      if (!(strm_ >> value)) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_UINT16:
    {
      ACE_CDR::UShort value;
      if (!(strm_ >> value)) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_INT32:
    return strm_ >> label;
  case TK_UINT32:
    {
      ACE_CDR::ULong value;
      if (!(strm_ >> value)) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_INT64:
    {
      ACE_CDR::LongLong value;
      if (!(strm_ >> value)) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_UINT64:
    {
      ACE_CDR::ULongLong value;
      if (!(strm_ >> value)) { return false; }
      label = static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_ENUM:
    {
      const TypeDescriptor disc_td = disc_type->get_descriptor();
      const ACE_CDR::ULong bit_bound = disc_td.bound[0];
      if (bit_bound >= 1 && bit_bound <= 8) {
        ACE_CDR::Int8 value;
        if (!(strm_ >> ACE_InputCDR::to_int8(value))) { return false; }
        label = static_cast<ACE_CDR::Long>(value);
      } else if (bit_bound >= 9 && bit_bound <= 16) {
        ACE_CDR::Short value;
        if (!(strm_ >> value)) { return false; }
        label = static_cast<ACE_CDR::Long>(value);
      } else {
        return strm_ >> label;
      }
      return true;
    }
  default:
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::read_discriminator - Union has")
                 ACE_TEXT(" unsupported discriminator type (%C)\n"), typekind_to_string(disc_tk)));
    }
    return false;
  }
}

bool DynamicData::skip_all()
{
  const TypeKind tk = type_->get_kind();
  if (tk != TK_STRUCTURE && tk != TK_UNION) {
    return false;
  }

  const ExtensibilityKind extensibility = descriptor_.extensibility_kind;
  if (extensibility == APPENDABLE || extensibility == MUTABLE) {
    size_t dheader;
    if (!strm_.read_delimiter(dheader)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_all - Failed to read")
                   ACE_TEXT(" DHEADER of the DynamicData object\n")));
      }
      return false;
    }
    return skip("skip_all", "Failed to skip the whole DynamicData object\n", dheader);
  } else {
    const TypeKind tk = type_->get_kind();
    if (tk == TK_STRUCTURE) {
      const ACE_CDR::ULong member_count = type_->get_member_count();
      for (ACE_CDR::ULong i = 0; i < member_count; ++i) {
        if (!skip_struct_member_by_index(i)) {
          return false;
        }
      }
      return true;
    } else { // Union
      const DynamicType_rch disc_type = get_base_type(descriptor_.discriminator_type);
      ACE_CDR::Long label;
      if (!read_discriminator(disc_type, extensibility, label)) {
        return false;
      }

      DynamicTypeMembersById members;
      type_->get_all_members(members);

      bool has_default = false;
      MemberDescriptor default_member;
      for (DynamicTypeMembersById::const_iterator it = members.begin(); it != members.end(); ++it) {
        const MemberDescriptor md = it->second->get_descriptor();
        const UnionCaseLabelSeq& labels = md.label;
        for (ACE_CDR::ULong i = 0; i < labels.length(); ++i) {
          if (label == labels[i]) {
            const DynamicType_rch selected_member = md.type.lock();
            return selected_member && skip_member(selected_member);
          }
        }

        if (md.is_default_label) {
          has_default = true;
          default_member = md;
        }
      }

      if (has_default) {
        const DynamicType_rch default_dt = default_member.type.lock();
        return default_dt && skip_member(default_dt);
      }
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::skip_all - Skip a union with no")
                   ACE_TEXT(" selected member and a discriminator with value %d\n"), label));
      }
      return true;
    }
  }
}

bool DynamicData::skip(const char* func_name, const char* description, size_t n, int size)
{
  if (!strm_.skip(n, size)) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::%C - %C\n"), func_name, description));
    }
    return false;
  }
  return true;
}

DynamicType_rch DynamicData::get_base_type(const DynamicType_rch& type) const
{
  if (type->get_kind() != TK_ALIAS) {
    return type;
  }

  const TypeDescriptor descriptor = type->get_descriptor();
  return get_base_type(descriptor.base_type);
}

bool DynamicData::is_primitive(TypeKind tk, ACE_CDR::ULong& size) const
{
  size = 0;

  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
    size = 1;
    break;
  case TK_INT16:
  case TK_UINT16:
  case TK_CHAR16:
    size = 2;
    break;
  case TK_INT32:
  case TK_UINT32:
  case TK_FLOAT32:
    size = 4;
    break;
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT64:
    size = 8;
    break;
  case TK_FLOAT128:
    size = 16;
    break;
  default:
    return false;
  }
  return true;
}

bool DynamicData::get_index_from_id(MemberId id, ACE_CDR::ULong& index, ACE_CDR::ULong bound) const
{
  switch (type_->get_kind()) {
  case TK_STRING8:
  case TK_STRING16:
  case TK_BITMASK:
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    // The mapping from id to index must be consistent with get_member_id_at_index
    // for these types. In particular, index and id are equal given that it doesn't
    // go out of bound.
    if (id < bound) {
      index = id;
      return true;
    }
  }
  return false;
}

const char* DynamicData::typekind_to_string(TypeKind tk) const
{
  switch (tk) {
  case TK_BOOLEAN:
    return "boolean";
  case TK_BYTE:
    return "byte";
  case TK_INT16:
    return "int16";
  case TK_INT32:
    return "int32";
  case TK_INT64:
    return "int64";
  case TK_UINT16:
    return "uint16";
  case TK_UINT32:
    return "uint32";
  case TK_UINT64:
    return "uint64";
  case TK_FLOAT32:
    return "float32";
  case TK_FLOAT64:
    return "float64";
  case TK_FLOAT128:
    return "float128";
  case TK_INT8:
    return "int8";
  case TK_UINT8:
    return "uint8";
  case TK_CHAR8:
    return "char8";
  case TK_CHAR16:
    return "char16";
  case TK_STRING8:
    return "string";
  case TK_STRING16:
    return "wstring";
  case TK_ALIAS:
    return "alias";
  case TK_ENUM:
    return "enum";
  case TK_BITMASK:
    return "bitmask";
  case TK_ANNOTATION:
    return "annotation";
  case TK_STRUCTURE:
    return "structure";
  case TK_UNION:
    return "union";
  case TK_BITSET:
    return "bitset";
  case TK_SEQUENCE:
    return "sequence";
  case TK_ARRAY:
    return "array";
  case TK_MAP:
    return "map";
  default:
    return "unknown";
  }
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
