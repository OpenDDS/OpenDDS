/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#include "DynamicDataImpl.h"

#include "DynamicTypeMemberImpl.h"

#include <dds/DCPS/debug.h>
#include <dds/DCPS/SafetyProfileStreams.h>

#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/DdsDynamicDataSeqTypeSupportImpl.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/CorbaSeq/LongSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/ULongSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/Int8SeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/UInt8SeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/ShortSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/UShortSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/LongLongSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/ULongLongSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/FloatSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/DoubleSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/LongDoubleSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/CharSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/WCharSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/OctetSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/BooleanSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/StringSeqTypeSupportImpl.h>
#  include <dds/CorbaSeq/WStringSeqTypeSupportImpl.h>
#  include <dds/DCPS/ValueHelper.h>
#endif
#include <dds/DdsDcpsInfrastructureC.h>

#include <ace/OS_NS_string.h>

#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

DynamicDataImpl::DynamicDataImpl()
  : chain_(0)
  , encoding_(DCPS::Encoding::KIND_XCDR2)
  , reset_align_state_(false)
  , strm_(0, encoding_)
  , item_count_(ITEM_COUNT_INVALID)
{}

DynamicDataImpl::DynamicDataImpl(ACE_Message_Block* chain,
                                 const DCPS::Encoding& encoding,
                                 DDS::DynamicType_ptr type)
  : chain_(chain->duplicate())
  , encoding_(encoding)
  , reset_align_state_(false)
  , strm_(chain_, encoding_)
  , type_(DDS::DynamicType::_duplicate(type))
  , item_count_(ITEM_COUNT_INVALID)
{
  if (encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_1 &&
      encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_2) {
    throw std::runtime_error("DynamicData only supports XCDR1 and XCDR2 at this time");
  }
}

DynamicDataImpl::DynamicDataImpl(DCPS::Serializer& ser, DDS::DynamicType_ptr type)
  : chain_(ser.current()->duplicate())
  , encoding_(ser.encoding())
  , reset_align_state_(true)
  , align_state_(ser.rdstate())
  , strm_(chain_, encoding_)
  , type_(DDS::DynamicType::_duplicate(type))
  , item_count_(ITEM_COUNT_INVALID)
{
  if (encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_1 &&
      encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_2) {
    throw std::runtime_error("DynamicData only supports XCDR1 and XCDR2 at this time");
  }

  strm_.rdstate(align_state_);
}

DynamicDataImpl::DynamicDataImpl(const DynamicDataImpl& other)
  : CORBA::Object()
  , DDS::DynamicData()
  , CORBA::LocalObject()
  , RcObject()
  , chain_(0)
  , strm_(0, other.encoding_)
{
  copy(other);
}

DynamicDataImpl& DynamicDataImpl::operator=(const DynamicDataImpl& other)
{
  if (this != &other) {
    copy(other);
  }
  return *this;
}

DynamicDataImpl::~DynamicDataImpl()
{
  ACE_Message_Block::release(chain_);
}

void DynamicDataImpl::copy(const DynamicDataImpl& other)
{
  ACE_Message_Block::release(chain_);
  chain_ = other.chain_->duplicate();
  encoding_ = other.encoding_;
  reset_align_state_ = other.reset_align_state_;
  align_state_ = other.align_state_;
  strm_ = other.strm_;
  type_ = other.type_;
  item_count_ = other.item_count_;
}

DDS::ReturnCode_t DynamicDataImpl::get_descriptor(DDS::MemberDescriptor*& value, MemberId id)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::DynamicTypeMember_var dtm;
  if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  return dtm->get_descriptor(value);
}

DDS::ReturnCode_t DynamicDataImpl::set_descriptor(MemberId, DDS::MemberDescriptor*)
{
  return DDS::RETCODE_UNSUPPORTED;
}

MemberId DynamicDataImpl::get_member_id_by_name(const char* name)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);

  const TypeKind tk = base_type->get_kind();
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
      DDS::DynamicTypeMember_var member;
      if (base_type->get_member_by_name(member, name) != DDS::RETCODE_OK) {
        return MEMBER_ID_INVALID;
      }
      DDS::MemberDescriptor_var descriptor;
      if (member->get_descriptor(descriptor) != DDS::RETCODE_OK) {
        return MEMBER_ID_INVALID;
      }
      if (tk == TK_BITMASK) {
        // Bitmask's flags don't have ID, so use index instead.
        return descriptor->index();
      } else {
        return descriptor->id();
      }
    }
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::get_member_id_by_name -")
               ACE_TEXT(" Calling on an unexpected type %C\n"), typekind_to_string(tk)));
  }
  return MEMBER_ID_INVALID;
}

bool DynamicDataImpl::has_optional_member(bool& has_optional) const
{
  const DDS::DynamicType_var base_type = get_base_type(type_);

  if (base_type->get_kind() != TK_STRUCTURE) {
    return false;
  }

  const ACE_CDR::ULong count = base_type->get_member_count();
  for (unsigned i = 0; i < count; ++i) {
    DDS::DynamicTypeMember_var member;
    if (base_type->get_member_by_index(member, i) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var descriptor;
    if (member->get_descriptor(descriptor) != DDS::RETCODE_OK) {
      return false;
    }
    if (descriptor->is_optional()) {
      has_optional = true;
      return true;
    }
  }
  has_optional = false;
  return true;
}

MemberId DynamicDataImpl::get_member_id_at_index(ACE_CDR::ULong index)
{
  if (item_count_ == ITEM_COUNT_INVALID) {
    get_item_count();
  }
  if (index >= item_count_) {
    return MEMBER_ID_INVALID;
  }

  ScopedChainManager chain_manager(*this);

  const DDS::DynamicType_var base_type = get_base_type(type_);

  const TypeKind tk = base_type->get_kind();
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
    {
      DDS::TypeDescriptor_var descriptor;
      if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
        return MEMBER_ID_INVALID;
      }
      const DDS::ExtensibilityKind ek = descriptor->extensibility_kind();
      if (ek == DDS::APPENDABLE || ek == DDS::MUTABLE) {
        size_t dheader;
        if (!strm_.read_delimiter(dheader)) {
          return MEMBER_ID_INVALID;
        }
      }

      if (ek == DDS::FINAL || ek == DDS::APPENDABLE) {
        bool has_optional;
        if (!has_optional_member(has_optional)) {
          return MEMBER_ID_INVALID;
        }

        if (!has_optional) {
          DDS::DynamicTypeMember_var member;
          if (base_type->get_member_by_index(member, index) != DDS::RETCODE_OK) {
            return MEMBER_ID_INVALID;
          }
          return member->get_id();
        } else {
          MemberId id = MEMBER_ID_INVALID;
          ACE_CDR::ULong total_skipped = 0;
          for (ACE_CDR::ULong i = 0; i < base_type->get_member_count(); ++i) {
            ACE_CDR::ULong num_skipped;
            if (!skip_struct_member_at_index(i, num_skipped)) {
              break;
            }
            total_skipped += num_skipped;
            if (total_skipped == index + 1) {
              DDS::DynamicTypeMember_var member;
              if (base_type->get_member_by_index(member, i) == DDS::RETCODE_OK) {
                id = member->get_id();
              }
              break;
            }
          }
          return id;
        }
      } else { // Mutable
        ACE_CDR::ULong member_id;
        size_t member_size;
        bool must_understand;
        bool good = true;
        for (unsigned i = 0; i < index; ++i) {
          if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
            good = false;
            break;
          }

          DDS::DynamicTypeMember_var dtm;
          if (base_type->get_member(dtm, member_id) != DDS::RETCODE_OK) {
            good = false;
            break;
          }
          DDS::MemberDescriptor_var descriptor;
          if (dtm->get_descriptor(descriptor) != DDS::RETCODE_OK) {
            good = false;
            break;
          }
          const DDS::DynamicType_ptr mt = descriptor->type();
          if (!mt) {
            good = false;
            break;
          }
          const DDS::DynamicType_var member = get_base_type(mt);
          if (member->get_kind() == TK_SEQUENCE) {
            if (!skip_sequence_member(member)) {
              good = false;
              break;
            }
          } else if (!strm_.skip(member_size)) {
            good = false;
            break;
          }
        }

        if (!good || !strm_.read_parameter_id(member_id, member_size, must_understand)) {
          member_id = MEMBER_ID_INVALID;
        }
        return member_id;
      }
    }
  case TK_UNION:
    {
      if (index == 0) {
        return DISCRIMINATOR_ID;
      }

      // Get the Id of the selected member.
      DDS::MemberDescriptor_var selected_md = get_union_selected_member();
      if (!selected_md) {
        return MEMBER_ID_INVALID;
      }

      return selected_md->id();
    }
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::get_member_id_at_index -")
               ACE_TEXT(" Calling on an unexpected type %C\n"), typekind_to_string(tk)));
  }
  return MEMBER_ID_INVALID;
}

ACE_CDR::ULong DynamicDataImpl::get_item_count()
{
  if (item_count_ != ITEM_COUNT_INVALID) {
    return item_count_;
  }

  ScopedChainManager chain_manager(*this);

  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return 0;
  }

  const TypeKind tk = base_type->get_kind();
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
      return (base_type->get_kind() == TK_STRING8) ? bytes : bytes/2;
    }
  case TK_BITMASK:
    return descriptor->bound()[0];
  case TK_STRUCTURE:
    {
      bool has_optional;
      if (!has_optional_member(has_optional)) {
        return 0;
      }

      if (!has_optional) {
        return base_type->get_member_count();
      }

      // Optional members can be omitted, so we need to count members one by one.
      ACE_CDR::ULong actual_count = 0;
      const DDS::ExtensibilityKind ek = descriptor->extensibility_kind();
      if (ek == DDS::FINAL || ek == DDS::APPENDABLE) {
        if (ek == DDS::APPENDABLE) {
          size_t dheader = 0;
          if (!strm_.read_delimiter(dheader)) {
            return 0;
          }
        }
        for (ACE_CDR::ULong i = 0; i < base_type->get_member_count(); ++i) {
          ACE_CDR::ULong num_skipped;
          if (!skip_struct_member_at_index(i, num_skipped)) {
            actual_count = 0;
            break;
          }
          actual_count += num_skipped;
        }
      } else { // Mutable
        size_t dheader = 0;
        if (!strm_.read_delimiter(dheader)) {
          return 0;
        }

        const size_t end_of_sample = strm_.rpos() + dheader;
        while (strm_.rpos() < end_of_sample) {
          ACE_CDR::ULong member_id;
          size_t member_size;
          bool must_understand;
          if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
            actual_count = 0;
            break;
          }

          DDS::DynamicTypeMember_var dtm;
          if (base_type->get_member(dtm, member_id) != DDS::RETCODE_OK) {
            actual_count = 0;
            break;
          }
          DDS::MemberDescriptor_var descriptor;
          if (dtm->get_descriptor(descriptor) != DDS::RETCODE_OK) {
            actual_count = 0;
            break;
          }
          const DDS::DynamicType_ptr mt = descriptor->type();
          if (!mt) {
            actual_count = 0;
            break;
          }
          const DDS::DynamicType_var member = get_base_type(mt);
          if (member->get_kind() == TK_SEQUENCE) {
            if (!skip_sequence_member(member)) {
              actual_count = 0;
              break;
            }
          } else if (!strm_.skip(member_size)) {
            actual_count = 0;
            break;
          }
          ++actual_count;
        }
      }
      return actual_count;
    }
  case TK_UNION:
    {
      const DDS::ExtensibilityKind ek = descriptor->extensibility_kind();
      if (ek == DDS::APPENDABLE || ek == DDS::MUTABLE) {
        size_t size;
        if (!strm_.read_delimiter(size)) {
          return 0;
        }
      }
      const DDS::DynamicType_var disc_type = get_base_type(descriptor->discriminator_type());
      const DDS::ExtensibilityKind extend = descriptor->extensibility_kind();
      ACE_CDR::Long label;
      if (!read_discriminator(disc_type, extend, label)) {
        return 0;
      }

      DDS::DynamicTypeMembersById_var members;
      if (base_type->get_all_members(members) != DDS::RETCODE_OK) {
        return 0;
      }
      DynamicTypeMembersByIdImpl* members_impl = dynamic_cast<DynamicTypeMembersByIdImpl*>(members.in());
      for (DynamicTypeMembersByIdImpl::const_iterator it = members_impl->begin(); it != members_impl->end(); ++it) {
        DDS::MemberDescriptor_var md;
        if (it->second->get_descriptor(md) != DDS::RETCODE_OK) {
          return 0;
        }
        if (md->is_default_label()) {
          return 2;
        }
        const DDS::UnionCaseLabelSeq& labels = md->label();
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
      const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
      if (!is_primitive(elem_type->get_kind())) {
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
      for (ACE_CDR::ULong i = 0; i < descriptor->bound().length(); ++i) {
        length *= descriptor->bound()[i];
      }
      return length;
    }
  case TK_MAP:
    {
      const DDS::DynamicType_var key_type = get_base_type(descriptor->key_element_type());
      const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
      if (is_primitive(key_type->get_kind()) &&
          is_primitive(elem_type->get_kind())) {
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
          length = 0;
          break;
        }
        ++length;
      }
      return length;
    }
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::get_item_count -")
               ACE_TEXT(" Calling on an unexpected type %C\n"), typekind_to_string(tk)));
  }
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
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataImpl::loan_value is not implemented\n"));
  return 0;
}

DDS::ReturnCode_t DynamicDataImpl::return_loaned_value(DDS::DynamicData_ptr /*value*/)
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataImpl::return_loaned_value is not implemented\n"));
  return DDS::RETCODE_UNSUPPORTED;
}


DDS::DynamicData_ptr DynamicDataImpl::clone()
{
  return new DynamicDataImpl(chain_, strm_.encoding(), type_);
}

bool DynamicDataImpl::is_type_supported(TypeKind tk, const char* func_name)
{
  if (!is_primitive(tk) && tk != TK_STRING8 && tk != TK_STRING16) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::%C -")
                 ACE_TEXT(" Called on an unsupported type (%C)\n"), func_name, typekind_to_string(tk)));
    }
    return false;
  }
  return true;
}

template<typename ValueType>
bool DynamicDataImpl::read_value(ValueType& value, TypeKind tk)
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
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::read_value -")
               ACE_TEXT(" Calling on an unexpected type %C\n"), typekind_to_string(tk)));
  }
  return false;
}

template<TypeKind MemberTypeKind, typename MemberType>
bool DynamicDataImpl::get_value_from_struct(MemberType& value, MemberId id,
                                            TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  DDS::MemberDescriptor_var md;
  if (get_from_struct_common_checks(md, id, MemberTypeKind)) {
    return skip_to_struct_member(md, id) && read_value(value, MemberTypeKind);
  }

  if (get_from_struct_common_checks(md, id, enum_or_bitmask)) {
    const DDS::DynamicType_ptr member_type = md->type();
    if (member_type) {
      DDS::TypeDescriptor_var td;
      if (get_base_type(member_type)->get_descriptor(td) != DDS::RETCODE_OK) {
        return false;
      }
      const LBound bit_bound = td->bound()[0];
      return bit_bound >= lower && bit_bound <= upper &&
        skip_to_struct_member(md, id) && read_value(value, MemberTypeKind);
    }
  }

  return false;
}

DDS::MemberDescriptor* DynamicDataImpl::get_union_selected_member()
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return 0;
  }

  const DDS::ExtensibilityKind ek = descriptor->extensibility_kind();
  if (ek == DDS::APPENDABLE || ek == DDS::MUTABLE) {
    size_t size;
    if (!strm_.read_delimiter(size)) {
      return 0;
    }
  }

  const DDS::DynamicType_var disc_type = get_base_type(descriptor->discriminator_type());
  ACE_CDR::Long label;
  if (!read_discriminator(disc_type, ek, label)) {
    return 0;
  }

  DDS::DynamicTypeMembersById_var members;
  if (base_type->get_all_members(members) != DDS::RETCODE_OK) {
    return 0;
  }
  DynamicTypeMembersByIdImpl* members_impl = dynamic_cast<DynamicTypeMembersByIdImpl*>(members.in());

  bool has_default = false;
  DDS::MemberDescriptor_var default_member;
  for (DynamicTypeMembersByIdImpl::const_iterator it = members_impl->begin(); it != members_impl->end(); ++it) {
    DDS::MemberDescriptor_var md;
    if (it->second->get_descriptor(md) != DDS::RETCODE_OK) {
      return 0;
    }
    const DDS::UnionCaseLabelSeq& labels = md->label();
    for (ACE_CDR::ULong i = 0; i < labels.length(); ++i) {
      if (label == labels[i]) {
        return md._retn();
      }
    }

    if (md->is_default_label()) {
      has_default = true;
      default_member = md;
    }
  }

  if (has_default) {
    return default_member._retn();;
  }

  // The union has no selected member.
  return 0;
}

DDS::MemberDescriptor* DynamicDataImpl::get_from_union_common_checks(MemberId id, const char* func_name)
{
  DDS::MemberDescriptor_var md = get_union_selected_member();
  if (!md) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::%C - Could not find")
                 ACE_TEXT(" MemberDescriptor for the selected union member\n"), func_name));
    }
    return 0;
  }

  if (md->id() == MEMBER_ID_INVALID) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::%C - Union has no selected member\n"), func_name));
    }
    return 0;
  }

  if (md->id() != id) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::%C -")
                 ACE_TEXT(" ID of the selected member (%d) is not the requested ID (%d)\n"),
                 func_name, md->id(), id));
    }
    return 0;
  }

  return md._retn();
}

template<TypeKind MemberTypeKind, typename MemberType>
bool DynamicDataImpl::get_value_from_union(MemberType& value, MemberId id,
                                           TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::DynamicType_var member_type;
  if (id == DISCRIMINATOR_ID) {
    const DDS::ExtensibilityKind ek = descriptor->extensibility_kind();
    if (ek == DDS::APPENDABLE || ek == DDS::MUTABLE) {
      size_t size;
      if (!strm_.read_delimiter(size)) {
        return false;
      }
    }
    member_type = get_base_type(descriptor->discriminator_type());
  } else {
    DDS::MemberDescriptor_var md = get_from_union_common_checks(id, "get_value_from_union");
    if (!md) {
      return false;
    }

    const DDS::DynamicType_ptr type = md->type();
    if (!type) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::get_value_from_union -")
                   ACE_TEXT(" Could not get DynamicType of the selected member\n")));
      }
      return false;
    }
    member_type = get_base_type(type);
  }

  const TypeKind member_tk = member_type->get_kind();
  if (member_tk != MemberTypeKind && member_tk != enum_or_bitmask) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_value_from_union -")
                 ACE_TEXT(" Could not read a value of type %C from type %C\n"),
                 typekind_to_string(MemberTypeKind), typekind_to_string(member_tk)));
    }
    return false;
  }

  if (descriptor->extensibility_kind() == DDS::MUTABLE) {
    unsigned member_id;
    size_t member_size;
    bool must_understand;
    if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
      return false;
    }
  }

  if (member_tk == MemberTypeKind) {
    return read_value(value, MemberTypeKind);
  }

  DDS::TypeDescriptor_var td;
  if (member_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  const LBound bit_bound = td->bound()[0];
  return bit_bound >= lower && bit_bound <= upper &&
    read_value(value, MemberTypeKind);
}

bool DynamicDataImpl::skip_to_sequence_element(MemberId id, DDS::DynamicType_ptr coll_type)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::DynamicType_var elem_type;
  bool skip_all = false;
  if (!coll_type) {
    elem_type = get_base_type(descriptor->element_type());
  } else {
    DDS::TypeDescriptor_var descriptor;
    if (coll_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
      return false;
    }
    elem_type = get_base_type(descriptor->element_type());
    skip_all = true;
  }
  ACE_CDR::ULong size;
  if (get_primitive_size(elem_type, size)) {
    ACE_CDR::ULong length, index;
    return (strm_ >> length) &&
      get_index_from_id(id, index, length) &&
      strm_.skip(index, size);
  } else {
    size_t dheader;
    ACE_CDR::ULong length, index;
    if (!strm_.read_delimiter(dheader) || !(strm_ >> length)) {
      return false;
    }
    if (skip_all) {
      index = length;
    } else if (!get_index_from_id(id, index, length)) {
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

bool DynamicDataImpl::skip_to_array_element(MemberId id, DDS::DynamicType_ptr coll_type)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::DynamicType_var elem_type;
  bool skip_all = false;
  DDS::TypeDescriptor_var coll_descriptor;

  if (!coll_type) {
    elem_type = get_base_type(descriptor->element_type());
    coll_type = get_base_type(type_);
    if (coll_type->get_descriptor(coll_descriptor) != DDS::RETCODE_OK) {
      return false;
    }
  } else {
    if (coll_type->get_descriptor(coll_descriptor) != DDS::RETCODE_OK) {
      return false;
    }
    elem_type = get_base_type(coll_descriptor->element_type());
    skip_all = true;
  }

  ACE_CDR::ULong length = 1;
  for (ACE_CDR::ULong i = 0; i < coll_descriptor->bound().length(); ++i) {
    length *= coll_descriptor->bound()[i];
  }

  ACE_CDR::ULong size;
  if (get_primitive_size(elem_type, size)) {
    ACE_CDR::ULong index;
    return get_index_from_id(id, index, length) && strm_.skip(index, size);
  } else {
    size_t dheader;
    ACE_CDR::ULong index;
    if (!strm_.read_delimiter(dheader)) {
      return false;
    }
    if (skip_all) {
      index = length;
    } else if (!get_index_from_id(id, index, length)) {
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

bool DynamicDataImpl::skip_to_map_element(MemberId id)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var key_type = get_base_type(descriptor->key_element_type());
  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  ACE_CDR::ULong key_size, elem_size;

  if (get_primitive_size(key_type, key_size) &&
      get_primitive_size(elem_type, elem_size)) {
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
bool DynamicDataImpl::get_value_from_collection(ElementType& value, MemberId id, TypeKind collection_tk,
                                                TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
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

      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_value_from_collection -")
                 ACE_TEXT(" Could not read a value of type %C from %C with element type %C\n"),
                 typekind_to_string(ElementTypeKind), collection_str, typekind_to_string(elem_tk)));
    }
    return false;
  }

  if (elem_tk == enum_or_bitmask) {
    DDS::TypeDescriptor_var td;
    if (elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
      return false;
    }
    const LBound bit_bound = td->bound()[0];
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

void DynamicDataImpl::setup_stream(ACE_Message_Block* chain)
{
  strm_ = DCPS::Serializer(chain, encoding_);
  if (reset_align_state_) {
    strm_.rdstate(align_state_);
  }
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataImpl::get_single_value(ValueType& value, MemberId id,
                                                    TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (!is_type_supported(ValueTypeKind, "get_single_value")) {
    return DDS::RETCODE_ERROR;
  }

  ScopedChainManager chain_manager(*this);

  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const TypeKind tk = base_type->get_kind();
  bool good = true;

  if (tk == enum_or_bitmask) {
    // Per XTypes spec, the value of a DynamicData object of primitive type or TK_ENUM is
    // accessed with MEMBER_ID_INVALID Id. However, there is only a single value in such
    // a DynamicData object, and checking for MEMBER_ID_INVALID from the input is perhaps
    // unnecessary. So, we read the value immediately here.
    const LBound bit_bound = descriptor->bound()[0];
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_single_value -")
               ACE_TEXT(" Failed to read a value of %C from a DynamicData object of type %C\n"),
               typekind_to_string(ValueTypeKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_int32_value(ACE_CDR::Long& value, MemberId id)
{
  return get_single_value<TK_INT32>(value, id, TK_ENUM,
                                    static_cast<LBound>(17), static_cast<LBound>(32));
}

DDS::ReturnCode_t DynamicDataImpl::get_uint32_value(ACE_CDR::ULong& value, MemberId id)
{
  return get_single_value<TK_UINT32>(value, id, TK_BITMASK,
                                     static_cast<LBound>(17), static_cast<LBound>(32));
}

DDS::ReturnCode_t DynamicDataImpl::get_int8_value(ACE_CDR::Int8& value, MemberId id)
{
  ACE_InputCDR::to_int8 to_int8(value);
  return get_single_value<TK_INT8>(to_int8, id, TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint8_value(ACE_CDR::UInt8& value, MemberId id)
{
  ACE_InputCDR::to_uint8 to_uint8(value);
  return get_single_value<TK_UINT8>(to_uint8, id, TK_BITMASK, 1, 8);
}

DDS::ReturnCode_t DynamicDataImpl::get_int16_value(ACE_CDR::Short& value, MemberId id)
{
  return get_single_value<TK_INT16>(value, id, TK_ENUM, 9, 16);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint16_value(ACE_CDR::UShort& value, MemberId id)
{
  return get_single_value<TK_UINT16>(value, id, TK_BITMASK, 9, 16);
}

DDS::ReturnCode_t DynamicDataImpl::get_int64_value(ACE_CDR::LongLong& value, MemberId id)
{
  return get_single_value<TK_INT64>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint64_value(ACE_CDR::ULongLong& value, MemberId id)
{
  return get_single_value<TK_UINT64>(value, id, TK_BITMASK, 33, 64);
}

DDS::ReturnCode_t DynamicDataImpl::get_float32_value(ACE_CDR::Float& value, MemberId id)
{
  return get_single_value<TK_FLOAT32>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_float64_value(ACE_CDR::Double& value, MemberId id)
{
  return get_single_value<TK_FLOAT64>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_float128_value(ACE_CDR::LongDouble& value, MemberId id)
{
  return get_single_value<TK_FLOAT128>(value, id);
}

template<TypeKind CharKind, TypeKind StringKind, typename ToCharT, typename CharT>
DDS::ReturnCode_t DynamicDataImpl::get_char_common(CharT& value, MemberId id)
{
  ScopedChainManager chain_manager(*this);

  const DDS::DynamicType_var base_type = get_base_type(type_);

  const TypeKind tk = base_type->get_kind();
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
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_char_common -")
                     ACE_TEXT(" Failed to read wstring with ID %d\n"), id));
        }
        good = false;
        break;
      }
      ACE_CDR::ULong index;
      const size_t str_len = ACE_OS::strlen(str);
      if (!get_index_from_id(id, index, static_cast<ACE_CDR::ULong>(str_len))) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_char_common -")
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

DDS::ReturnCode_t DynamicDataImpl::get_char8_value(ACE_CDR::Char& value, MemberId id)
{
  return get_char_common<TK_CHAR8, TK_STRING8, ACE_InputCDR::to_char>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_char16_value(ACE_CDR::WChar& value, MemberId id)
{
#ifdef DDS_HAS_WCHAR
  return get_char_common<TK_CHAR16, TK_STRING16, ACE_InputCDR::to_wchar>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataImpl::get_byte_value(ACE_CDR::Octet& value, MemberId id)
{
  ACE_InputCDR::to_octet to_octet(value);
  return get_single_value<TK_BYTE>(to_octet, id);
}

template<typename UIntType, TypeKind UIntTypeKind>
bool DynamicDataImpl::get_boolean_from_bitmask(ACE_CDR::ULong index, ACE_CDR::Boolean& value)
{
  UIntType bitmask;
  if (!read_value(bitmask, UIntTypeKind)) {
    return false;
  }

  value = ((1ULL << index) & bitmask) ? true : false;
  return true;
}

DDS::ReturnCode_t DynamicDataImpl::get_boolean_value(ACE_CDR::Boolean& value, MemberId id)
{
  ScopedChainManager chain_manager(*this);

  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const TypeKind tk = base_type->get_kind();
  bool good = true;

  switch (tk) {
  case TK_BOOLEAN:
    good = strm_ >> ACE_InputCDR::to_boolean(value);
    break;
  case TK_BITMASK:
    {
      const LBound bit_bound = descriptor->bound()[0];
      ACE_CDR::ULong index;
      if (!get_index_from_id(id, index, bit_bound)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_boolean_value -")
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

DDS::ReturnCode_t DynamicDataImpl::get_string_value(ACE_CDR::Char*& value, MemberId id)
{
  return get_single_value<TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_wstring_value(ACE_CDR::WChar*& value, MemberId id)
{
#ifdef DDS_HAS_WCHAR
  return get_single_value<TK_STRING16>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataImpl::get_complex_value(DDS::DynamicData_ptr& value, MemberId id)
{
  ScopedChainManager chain_manager(*this);

  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const TypeKind tk = base_type->get_kind();
  bool good = true;

  switch (tk) {
  case TK_STRUCTURE:
    {
      DDS::DynamicTypeMember_var member;
      const DDS::ReturnCode_t retcode = base_type->get_member(member, id);
      if (retcode != DDS::RETCODE_OK) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_complex_value -")
                     ACE_TEXT(" Failed to get DynamicTypeMember for member with ID %d\n"), id));
        }
        good = false;
        break;
      }

      DDS::MemberDescriptor_var md;
      if (member->get_descriptor(md) != DDS::RETCODE_OK) {
        good = false;
        break;
      }
      if (!skip_to_struct_member(md, id)) {
        good = false;
      } else {
        DDS::DynamicType_ptr member_type = md->type();
        if (!member_type) {
          good = false;
        } else {
          value = new DynamicDataImpl(strm_, member_type);
        }
      }
      break;
    }
  case TK_UNION:
    {
      if (id == DISCRIMINATOR_ID) {
        if (descriptor->extensibility_kind() == DDS::APPENDABLE || descriptor->extensibility_kind() == DDS::MUTABLE) {
          size_t size;
          if (!strm_.read_delimiter(size)) {
            good = false;
            break;
          }
        }

        const DDS::DynamicType_var disc_type = get_base_type(descriptor->discriminator_type());
        if (descriptor->extensibility_kind() == DDS::MUTABLE) {
          unsigned id;
          size_t size;
          bool must_understand;
          if (!strm_.read_parameter_id(id, size, must_understand)) {
            good = false;
            break;
          }
        }
        value = new DynamicDataImpl(strm_, disc_type);
        break;
      }

      DDS::MemberDescriptor_var md = get_from_union_common_checks(id, "get_complex_value");
      if (!md) {
        good = false;
        break;
      }

      if (descriptor->extensibility_kind() == DDS::MUTABLE) {
        unsigned id;
        size_t size;
        bool must_understand;
        if (!strm_.read_parameter_id(id, size, must_understand)) {
          good = false;
          break;
        }
      }
      const DDS::DynamicType_ptr member_type = md->type();
      if (!member_type) {
        good = false;
      } else {
        value = new DynamicDataImpl(strm_, member_type);
      }
      break;
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
        value = new DynamicDataImpl(strm_, descriptor->element_type());
      }
      break;
    }
  default:
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_complex_value -")
                 ACE_TEXT(" Called on an unsupported type (%C)\n"), typekind_to_string(tk)));
    }
    good = false;
    break;
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

template<typename SequenceType>
bool DynamicDataImpl::read_values(SequenceType& value, TypeKind elem_tk)
{
  using OpenDDS::DCPS::operator>>;

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
    {
      return strm_ >> value;
    }
  case TK_ENUM:
  case TK_BITMASK:
    {
      size_t dheader;
      if (!strm_.read_delimiter(dheader)) {
        return false;
      }
      return strm_ >> value;
    }
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::read_values -")
               ACE_TEXT(" Calling on an unexpected element type %C\n"), typekind_to_string(elem_tk)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::get_values_from_struct(SequenceType& value, MemberId id,
                                             TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  DDS::MemberDescriptor_var md;
  if (get_from_struct_common_checks(md, id, ElementTypeKind, true)) {
    return skip_to_struct_member(md, id) && read_values(value, ElementTypeKind);
  }

  if (get_from_struct_common_checks(md, id, enum_or_bitmask, true)) {
    const DDS::DynamicType_ptr member_type = md->type();
    if (member_type) {
      DDS::TypeDescriptor_var td;
      if (get_base_type(member_type)->get_descriptor(td) != DDS::RETCODE_OK) {
        return false;
      }
      DDS::TypeDescriptor_var etd;
      if (td->element_type()->get_descriptor(etd) != DDS::RETCODE_OK) {
        return false;
      }
      const LBound bit_bound = etd->bound()[0];
      return bit_bound >= lower && bit_bound <= upper &&
        skip_to_struct_member(md, id) && read_values(value, enum_or_bitmask);
    }
  }

  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::get_values_from_union(SequenceType& value, MemberId id,
                                        TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  DDS::MemberDescriptor_var md = get_from_union_common_checks(id, "get_values_from_union");
  if (!md) {
    return false;
  }

  const DDS::DynamicType_ptr member_type = md->type();
  if (!member_type) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::get_values_from_union -")
                 ACE_TEXT(" Could not get DynamicType of the selected member\n")));
    }
    return false;
  }

  DDS::DynamicType_var selected_type = get_base_type(member_type);
  const TypeKind selected_tk = selected_type->get_kind();
  if (selected_tk != TK_SEQUENCE) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_values_from_union -")
                 ACE_TEXT(" The selected member is not a sequence, but %C\n"),
                 typekind_to_string(selected_tk)));
    }
    return false;
  }

  DDS::TypeDescriptor_var td;
  if (selected_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var elem_type = get_base_type(td->element_type());
  const TypeKind elem_tk = elem_type->get_kind();
  if (elem_tk != ElementTypeKind && elem_tk != enum_or_bitmask) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_values_from_union -")
                 ACE_TEXT(" Could not read a sequence of %C from a sequence of %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_tk)));
    }
    return false;
  }

  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  if (descriptor->extensibility_kind() == DDS::MUTABLE) {
    unsigned member_id;
    size_t member_size;
    bool must_understand;
    if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
      return false;
    }
  }

  if (elem_tk == ElementTypeKind) {
    return read_values(value, ElementTypeKind);
  }

  // Reset.
  td = 0;
  if (elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  const LBound bit_bound = td->bound()[0];
  return bit_bound >= lower && bit_bound <= upper && read_values(value, enum_or_bitmask);
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::get_values_from_sequence(SequenceType& value, MemberId id,
                                               TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const TypeKind elem_tk = elem_type->get_kind();

  if (elem_tk == ElementTypeKind) {
    return read_values(value, ElementTypeKind);
  } else if (elem_tk == enum_or_bitmask) {
    // Read from a sequence of enums or bitmasks.
    DDS::TypeDescriptor_var td;
    if (elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
      return false;
    }
    const LBound bit_bound = td->bound()[0];
    return bit_bound >= lower && bit_bound <= upper && read_values(value, enum_or_bitmask);
  } else if (elem_tk == TK_SEQUENCE) {
    // Read from a sequence of enums or bitmasks.
    DDS::TypeDescriptor_var td;
    if (elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
      return false;
    }
    const DDS::DynamicType_var nested_elem_type = get_base_type(td->element_type());
    const TypeKind nested_elem_tk = nested_elem_type->get_kind();
    if (nested_elem_tk == ElementTypeKind) {
      // Read from a sequence of sequence of ElementTypeKind.
      return skip_to_sequence_element(id) && read_values(value, ElementTypeKind);
    } else if (nested_elem_tk == enum_or_bitmask) {
      // Read from a sequence of sequence of enums or bitmasks.
      DDS::TypeDescriptor_var td;
      if (nested_elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
        return false;
      }
      const LBound bit_bound = td->bound()[0];
      return bit_bound >= lower && bit_bound <= upper &&
        skip_to_sequence_element(id) && read_values(value, enum_or_bitmask);
    }
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_values_from_sequence -")
               ACE_TEXT(" Could not read a sequence of %C from an incompatible type\n"),
               typekind_to_string(ElementTypeKind)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::get_values_from_array(SequenceType& value, MemberId id,
                                            TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  if (elem_type->get_kind() != TK_SEQUENCE) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_values_from_array -")
                 ACE_TEXT(" Could not read a sequence of %C from an array of %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_type->get_kind())));
    }
    return false;
  }

  DDS::TypeDescriptor_var td;
  if (elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var nested_elem_type = get_base_type(td->element_type());
  const TypeKind nested_elem_tk = nested_elem_type->get_kind();
  if (nested_elem_tk == ElementTypeKind) {
    return skip_to_array_element(id) && read_values(value, nested_elem_tk);
  } else if (nested_elem_tk == enum_or_bitmask) {
    DDS::TypeDescriptor_var td;
    if (nested_elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
      return false;
    }
    const LBound bit_bound = td->bound()[0];
    return bit_bound >= lower && bit_bound <= upper &&
      skip_to_array_element(id) && read_values(value, nested_elem_tk);
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_values_from_array -")
               ACE_TEXT(" Could not read a sequence of %C from an array of sequence of %C\n"),
               typekind_to_string(ElementTypeKind), typekind_to_string(nested_elem_tk)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataImpl::get_values_from_map(SequenceType& value, MemberId id,
                                          TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  if (elem_type->get_kind() != TK_SEQUENCE) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_values_from_map -")
                 ACE_TEXT(" Getting sequence<%C> from a map with element type of %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_type->get_kind())));
    }
    return false;
  }

  DDS::TypeDescriptor_var td;
  if (elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var nested_elem_type = get_base_type(td->element_type());
  const TypeKind nested_elem_tk = nested_elem_type->get_kind();
  if (nested_elem_tk == ElementTypeKind) {
    return skip_to_map_element(id) && read_values(value, nested_elem_tk);
  } else if (nested_elem_tk == enum_or_bitmask) {
    DDS::TypeDescriptor_var td;
    if (nested_elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
      return false;
    }
    const LBound bit_bound = td->bound()[0];
    return bit_bound >= lower && bit_bound <= upper &&
      skip_to_map_element(id) && read_values(value, nested_elem_tk);
  }

  if (DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_values_from_map -")
               ACE_TEXT(" Could not read a sequence of %C from a map with element type sequence of %C\n"),
               typekind_to_string(ElementTypeKind), typekind_to_string(nested_elem_tk)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
DDS::ReturnCode_t DynamicDataImpl::get_sequence_values(SequenceType& value, MemberId id,
                                                       TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (!is_type_supported(ElementTypeKind, "get_sequence_values")) {
    return DDS::RETCODE_ERROR;
  }

  ScopedChainManager chain_manager(*this);

  const DDS::DynamicType_var base_type = get_base_type(type_);

  const TypeKind tk = base_type->get_kind();
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_sequence_values -")
                 ACE_TEXT(" A sequence<%C> can't be read as a member of type %C"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(tk)));
    }
    return DDS::RETCODE_ERROR;
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_sequence_values -")
               ACE_TEXT(" Failed to read sequence<%C> from a DynamicData object of type %C\n"),
               typekind_to_string(ElementTypeKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataImpl::get_int32_values(DDS::Int32Seq& value, MemberId id)
{
  return get_sequence_values<TK_INT32>(value, id, TK_ENUM, 17, 32);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint32_values(DDS::UInt32Seq& value, MemberId id)
{
  return get_sequence_values<TK_UINT32>(value, id, TK_BITMASK, 17, 32);
}

DDS::ReturnCode_t DynamicDataImpl::get_int8_values(DDS::Int8Seq& value, MemberId id)
{
  return get_sequence_values<TK_INT8>(value, id, TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint8_values(DDS::UInt8Seq& value, MemberId id)
{
  return get_sequence_values<TK_UINT8>(value, id, TK_BITMASK, 1, 8);
}

DDS::ReturnCode_t DynamicDataImpl::get_int16_values(DDS::Int16Seq& value, MemberId id)
{
  return get_sequence_values<TK_INT16>(value, id, TK_ENUM, 9, 16);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint16_values(DDS::UInt16Seq& value, MemberId id)
{
  return get_sequence_values<TK_UINT16>(value, id, TK_BITMASK, 9, 16);
}

DDS::ReturnCode_t DynamicDataImpl::get_int64_values(DDS::Int64Seq& value, MemberId id)
{
  return get_sequence_values<TK_INT64>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_uint64_values(DDS::UInt64Seq& value, MemberId id)
{
  return get_sequence_values<TK_UINT64>(value, id, TK_BITMASK, 33, 64);
}

DDS::ReturnCode_t DynamicDataImpl::get_float32_values(DDS::Float32Seq& value, MemberId id)
{
  return get_sequence_values<TK_FLOAT32>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_float64_values(DDS::Float64Seq& value, MemberId id)
{
  return get_sequence_values<TK_FLOAT64>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_float128_values(DDS::Float128Seq& value, MemberId id)
{
  return get_sequence_values<TK_FLOAT128>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_char8_values(DDS::CharSeq& value, MemberId id)
{
  return get_sequence_values<TK_CHAR8>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_char16_values(DDS::WcharSeq& value, MemberId id)
{
#ifdef DDS_HAS_WCHAR
  return get_sequence_values<TK_CHAR16>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataImpl::get_byte_values(DDS::ByteSeq& value, MemberId id)
{
  return get_sequence_values<TK_BYTE>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_boolean_values(DDS::BooleanSeq& value, MemberId id)
{
  return get_sequence_values<TK_BOOLEAN>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_string_values(DDS::StringSeq& value, MemberId id)
{
  return get_sequence_values<TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicDataImpl::get_wstring_values(DDS::WstringSeq& value, MemberId id)
{
#ifdef DDS_HAS_WCHAR
  return get_sequence_values<TK_STRING16>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::DynamicType_ptr DynamicDataImpl::type()
{
  return DDS::DynamicType::_duplicate(type_);
}

bool DynamicDataImpl::check_xcdr1_mutable(DDS::DynamicType_ptr dt)
{
  DynamicTypeNameSet dtns;
  return check_xcdr1_mutable_i(dt, dtns);
}

CORBA::Boolean DynamicDataImpl::equals(DDS::DynamicData_ptr)
{
  // FUTURE: Implement this.
  ACE_ERROR((LM_ERROR, "(%P|%t) DynamiceDataImpl::equals is not implemented\n"));
  return false;
}

bool DynamicDataImpl::skip_to_struct_member(DDS::MemberDescriptor* member_desc, MemberId id)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::ExtensibilityKind ek = descriptor->extensibility_kind();
  if (ek == DDS::FINAL || ek == DDS::APPENDABLE) {
    if (ek == DDS::APPENDABLE) {
      size_t dheader = 0;
      if (!strm_.read_delimiter(dheader)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
                     ACE_TEXT(" Failed to read DHEADER for member ID %d\n"), id));
        }
        return false;
      }
    }

    for (ACE_CDR::ULong i = 0; i < member_desc->index(); ++i) {
      ACE_CDR::ULong num_skipped;
      if (!skip_struct_member_at_index(i, num_skipped)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
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
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
                   ACE_TEXT(" Failed to read DHEADER for member ID %d\n"), id));
      }
      return false;
    }

    const size_t end_of_sample = strm_.rpos() + dheader;
    while (true) {
      if (strm_.rpos() >= end_of_sample) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
                     ACE_TEXT(" Could not find a member with ID %d\n"), id));
        }
        return false;
      }

      ACE_CDR::ULong member_id;
      size_t member_size;
      bool must_understand;
      if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
                     ACE_TEXT(" Failed to read EMHEADER while finding member ID %d\n"), id));
        }
        return false;
      }

      if (member_id == id) {
        return true;
      }

      DDS::DynamicTypeMember_var dtm;
      if (base_type->get_member(dtm, member_id) != DDS::RETCODE_OK) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
                     ACE_TEXT(" Failed to get DynamicTypeMember at ID %d\n"), member_id));
        }
        return false;
      }
      DDS::MemberDescriptor_var descriptor;
      if (dtm->get_descriptor(descriptor) != DDS::RETCODE_OK) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
                     ACE_TEXT(" Failed to get member descriptor at ID %d\n"), member_id));
        }
        return false;
      }
      const DDS::DynamicType_ptr mt = descriptor->type();
      if (!mt) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
                     ACE_TEXT(" Failed to get DynamicType for member at ID %d\n"), member_id));
        }
        return false;
      }
      const DDS::DynamicType_var member = get_base_type(mt);
      if (member->get_kind() == TK_SEQUENCE) {
        // Sequence is a special case where the NEXTINT header can also be used for
        // the length of the sequence (when LC is 5, 6, or 7). And thus skipping such a
        // sequence member using member_size can be incorrect.
        if (!skip_sequence_member(member)) {
          if (DCPS::DCPS_debug_level >= 1) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
                       ACE_TEXT(" Failed to skip a sequence member at ID %d\n"), member_id));
          }
          return false;
        }
      } else if (!strm_.skip(member_size)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_to_struct_member -")
                     ACE_TEXT(" Failed to skip a member with ID %d\n"), member_id));
        }
        return false;
      }
    }
  }
}

bool DynamicDataImpl::get_from_struct_common_checks(DDS::MemberDescriptor_var& md, MemberId id, TypeKind kind, bool is_sequence)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);

  DDS::DynamicTypeMember_var member;
  const DDS::ReturnCode_t retcode = base_type->get_member(member, id);
  if (retcode != DDS::RETCODE_OK) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_from_struct_common_checks -")
                 ACE_TEXT(" Failed to get DynamicTypeMember for member with ID %d\n"), id));
    }
    return false;
  }

  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::get_from_struct_common_checks -")
                 ACE_TEXT(" Failed to get member descriptor for member with ID %d\n"), id));
    }
    return false;
  }
  const DDS::DynamicType_ptr member_dt = md->type();
  if (!member_dt) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::get_from_struct_common_checks -")
                 ACE_TEXT(" Could not get DynamicType for member with ID %d\n"), id));
    }
    return false;
  }

  const DDS::DynamicType_var member_type = get_base_type(member_dt);
  const TypeKind member_kind = member_type->get_kind();

  if ((!is_sequence && member_kind != kind) || (is_sequence && member_kind != TK_SEQUENCE)) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::get_from_struct_common_checks -")
                 ACE_TEXT(" Member with ID %d has kind %C, not %C\n"),
                 id, typekind_to_string(member_kind),
                 is_sequence ? typekind_to_string(TK_SEQUENCE) : typekind_to_string(kind)));
    }
    return false;
  }

  if (member_kind == TK_SEQUENCE) {
    DDS::TypeDescriptor_var member_td;
    if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::get_from_struct_common_checks -")
                   ACE_TEXT(" Could not get type descriptor for member %d\n"),
                   id));
      }
      return false;
    }
    const TypeKind elem_kind = get_base_type(member_td->element_type())->get_kind();
    if (elem_kind != kind) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::get_from_struct_common_checks -")
                   ACE_TEXT(" Member with ID %d is a sequence of %C, not %C\n"),
                   id, typekind_to_string(elem_kind), typekind_to_string(kind)));
      }
      return false;
    }
  }

  return true;
}

bool DynamicDataImpl::skip_struct_member_at_index(ACE_CDR::ULong index, ACE_CDR::ULong& num_skipped)
{
  const DDS::DynamicType_var base_type = get_base_type(type_);

  DDS::DynamicTypeMember_var member;
  if (base_type->get_member_by_index(member, index) != DDS::RETCODE_OK) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_struct_member_at_index -")
                 ACE_TEXT(" Failed to get DynamicTypeMember for member index %d\n"), index));
    }
    return false;
  }

  DDS::MemberDescriptor_var md;
  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }
  if (strm_.encoding().kind() == DCPS::Encoding::KIND_XCDR2 && md->is_optional()) {
    ACE_CDR::Boolean present;
    if (!(strm_ >> ACE_InputCDR::to_boolean(present))) {
      return false;
    }
    // Optional member that is omitted is not counted as a skipped member.
    if (!present) {
      num_skipped = 0;
      return true;
    }
  }

  num_skipped = 1;
  const DDS::DynamicType_ptr member_type = md->type();
  return member_type && skip_member(member_type);
}

bool DynamicDataImpl::skip_member(DDS::DynamicType_ptr type)
{
  const DDS::DynamicType_var member_type = get_base_type(type);
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
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_member -")
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
      DDS::TypeDescriptor_var member_td;
      if (member_type->get_descriptor(member_td) != DDS::RETCODE_OK) {
        return false;
      }
      const ACE_CDR::ULong bit_bound = member_td->bound()[0];
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
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_member - Found a%C")
                     ACE_TEXT(" member with bit bound %d\n"),
                     member_kind == TK_ENUM ? "n enum" : " bitmask", bit_bound));
        }
        return false;
      }
      break;
    }
  case TK_STRUCTURE:
  case TK_UNION:
    return skip_aggregated_member(member_type);
  case TK_SEQUENCE:
    return skip_sequence_member(member_type);
  case TK_ARRAY:
    return skip_array_member(member_type);
  case TK_MAP:
    return skip_map_member(member_type);
  default:
    {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_member -")
                   ACE_TEXT(" Found a member of kind %C\n"), typekind_to_string(member_kind)));
      }
      return false;
    }
  }

  return true;
}

bool DynamicDataImpl::skip_sequence_member(DDS::DynamicType_ptr seq_type)
{
  DDS::TypeDescriptor_var descriptor;
  if (seq_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());

  ACE_CDR::ULong primitive_size = 0;
  if (get_primitive_size(elem_type, primitive_size)) {
    ACE_CDR::ULong length;
    if (!(strm_ >> length)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_sequence_member -")
                   ACE_TEXT(" Failed to deserialize a primitive sequence member\n")));
      }
      return false;
    }

    return skip("skip_sequence_member", "Failed to skip a primitive sequence member",
                length, primitive_size);
  } else {
    return skip_collection_member(seq_type);
  }
}

bool DynamicDataImpl::skip_array_member(DDS::DynamicType_ptr array_type)
{
  DDS::TypeDescriptor_var descriptor;
  if (array_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());

  ACE_CDR::ULong primitive_size = 0;
  if (get_primitive_size(elem_type, primitive_size)) {
    const DDS::BoundSeq& bounds = descriptor->bound();
    ACE_CDR::ULong num_elems = 1;
    for (unsigned i = 0; i < bounds.length(); ++i) {
      num_elems *= bounds[i];
    }

    return skip("skip_array_member", "Failed to skip a primitive array member",
                num_elems, primitive_size);
  } else {
    return skip_collection_member(array_type);
  }
}

bool DynamicDataImpl::skip_map_member(DDS::DynamicType_ptr map_type)
{
  DDS::TypeDescriptor_var descriptor;
  if (map_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());
  const DDS::DynamicType_var key_type = get_base_type(descriptor->key_element_type());

  ACE_CDR::ULong key_primitive_size = 0, elem_primitive_size = 0;
  if (get_primitive_size(key_type, key_primitive_size) &&
      get_primitive_size(elem_type, elem_primitive_size)) {
    ACE_CDR::ULong length;
    if (!(strm_ >> length)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_map_member -")
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
    return skip_collection_member(map_type);
  }
}

bool DynamicDataImpl::skip_collection_member(DDS::DynamicType_ptr coll_type)
{
  const TypeKind kind = coll_type->get_kind();
  if (kind != TK_SEQUENCE && kind != TK_ARRAY && kind != TK_MAP) {
    return false;
  }
  const char* kind_str = typekind_to_string(kind);
  size_t dheader;
  if (!strm_.read_delimiter(dheader)) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_collection_member -")
                 ACE_TEXT(" Failed to deserialize DHEADER of a non-primitive %C member\n"),
                 kind_str));
    }
    return false;
  }
  if (strm_.encoding().kind() == DCPS::Encoding::KIND_XCDR2) {
    const DCPS::String err_msg = DCPS::String("Failed to skip a non-primitive ") + kind_str + " member";
    return skip("skip_collection_member", err_msg.c_str(), dheader);
  } else if (kind == TK_SEQUENCE) {
    return skip_to_sequence_element(0, coll_type);
  } else if (kind == TK_ARRAY) {
    return skip_to_array_element(0, coll_type);
  } else if (kind == TK_MAP) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::skip_collection_member: "
                 "DynamicData does not currently support XCDR1 maps\n"));
    }
    return false;
  }
  return true;
}

bool DynamicDataImpl::skip_aggregated_member(DDS::DynamicType_ptr member_type)
{
  DynamicDataImpl nested_data(strm_, member_type);
  if (!nested_data.skip_all()) {
    return false;
  }

  // Collect the intermediate message block chains that were created recursively
  // when skipping nested_data.
  const IntermediateChains& chains = nested_data.get_intermediate_chains();
  chains_to_release.insert(chains_to_release.end(), chains.begin(), chains.end());

  ACE_Message_Block* const result_chain = nested_data.strm_.current()->duplicate();
  strm_ = DCPS::Serializer(result_chain, encoding_);
  const DCPS::Serializer::RdState curr_state = nested_data.strm_.rdstate();
  strm_.rdstate(curr_state);
  chains_to_release.push_back(result_chain);
  return true;
}

void DynamicDataImpl::release_chains()
{
  for (ACE_CDR::ULong i = 0; i < chains_to_release.size(); ++i) {
    ACE_Message_Block::release(chains_to_release[i]);
  }
  chains_to_release.clear();
}

bool DynamicDataImpl::read_discriminator(const DDS::DynamicType_ptr disc_type, DDS::ExtensibilityKind union_ek, ACE_CDR::Long& label)
{
  if (union_ek == DDS::MUTABLE) {
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
      DDS::TypeDescriptor_var disc_td;
      if (disc_type->get_descriptor(disc_td) != DDS::RETCODE_OK) {
        return false;
      }
      const ACE_CDR::ULong bit_bound = disc_td->bound()[0];
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::read_discriminator - Union has")
                 ACE_TEXT(" unsupported discriminator type (%C)\n"), typekind_to_string(disc_tk)));
    }
    return false;
  }
}

bool DynamicDataImpl::skip_all()
{
  const DDS::DynamicType_var base_type = get_base_type(type_);
  DDS::TypeDescriptor_var descriptor;
  if (base_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  const TypeKind tk = base_type->get_kind();
  if (tk != TK_STRUCTURE && tk != TK_UNION) {
    return false;
  }

  const DDS::ExtensibilityKind extensibility = descriptor->extensibility_kind();
  if (strm_.encoding().kind() == DCPS::Encoding::KIND_XCDR2 && (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE)) {
    size_t dheader;
    if (!strm_.read_delimiter(dheader)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_all - Failed to read")
                   ACE_TEXT(" DHEADER of the DynamicData object\n")));
      }
      return false;
    }
    return skip("skip_all", "Failed to skip the whole DynamicData object\n", dheader);
  } else {
    const TypeKind tk = base_type->get_kind();
    if (tk == TK_STRUCTURE) {
      const ACE_CDR::ULong member_count = base_type->get_member_count();
      bool good = true;
      for (ACE_CDR::ULong i = 0; i < member_count; ++i) {
        ACE_CDR::ULong num_skipped;
        if (!skip_struct_member_at_index(i, num_skipped)) {
          good = false;
          break;
        }
      }
      return good;
    } else { // Union
      const DDS::DynamicType_var disc_type = get_base_type(descriptor->discriminator_type());
      ACE_CDR::Long label;
      if (!read_discriminator(disc_type, extensibility, label)) {
        return false;
      }

      DDS::DynamicTypeMembersById_var members;
      if (base_type->get_all_members(members) != DDS::RETCODE_OK) {
        return false;
      }
      DynamicTypeMembersByIdImpl* members_impl = dynamic_cast<DynamicTypeMembersByIdImpl*>(members.in());

      bool has_default = false;
      DDS::MemberDescriptor_var default_member;
      for (DynamicTypeMembersByIdImpl::const_iterator it = members_impl->begin(); it != members_impl->end(); ++it) {
        DDS::MemberDescriptor_var md;
        if (it->second->get_descriptor(md) != DDS::RETCODE_OK) {
          return false;
        }
        const DDS::UnionCaseLabelSeq& labels = md->label();
        for (ACE_CDR::ULong i = 0; i < labels.length(); ++i) {
          if (label == labels[i]) {
            const DDS::DynamicType_ptr selected_member = md->type();
            bool good = selected_member && skip_member(selected_member);
            return good;
          }
        }

        if (md->is_default_label()) {
          has_default = true;
          default_member = md;
        }
      }

      if (has_default) {
        const DDS::DynamicType_ptr default_dt = default_member->type();
        bool good = default_dt && skip_member(default_dt);
        return good;
      }
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataImpl::skip_all - Skip a union with no")
                   ACE_TEXT(" selected member and a discriminator with value %d\n"), label));
      }
      return true;
    }
  }
}

bool DynamicDataImpl::skip(const char* func_name, const char* description, size_t n, int size)
{
  if (!strm_.skip(n, size)) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataImpl::%C - %C\n"), func_name, description));
    }
    return false;
  }
  return true;
}

bool DynamicDataImpl::is_primitive(TypeKind tk) const
{
  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_INT16:
  case TK_UINT16:
  case TK_CHAR16:
  case TK_INT32:
  case TK_UINT32:
  case TK_FLOAT32:
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT64:
  case TK_FLOAT128:
    return true;
  default:
    return false;
  }
}

bool DynamicDataImpl::get_primitive_size(DDS::DynamicType_ptr dt, ACE_CDR::ULong& size) const
{
  size = 0;

  switch (dt->get_kind()) {
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

bool DynamicDataImpl::get_index_from_id(MemberId id, ACE_CDR::ULong& index, ACE_CDR::ULong bound) const
{
  const DDS::DynamicType_var base_type = get_base_type(type_);

  switch (base_type->get_kind()) {
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

const char* DynamicDataImpl::typekind_to_string(TypeKind tk) const
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

bool DynamicDataImpl::check_xcdr1_mutable_i(DDS::DynamicType_ptr dt, DynamicTypeNameSet& dtns)
{
  DDS::TypeDescriptor_var descriptor;
  if (dt->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  if (dtns.find(descriptor->name()) != dtns.end()) {
    return true;
  }
  if (descriptor->extensibility_kind() == DDS::MUTABLE &&
      encoding_.kind() == DCPS::Encoding::KIND_XCDR1) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::check_xcdr1_mutable: "
                 "XCDR1 mutable is not currently supported in OpenDDS\n"));
    }
    return false;
  }
  dtns.insert(descriptor->name());
  for (ACE_CDR::ULong i = 0; i < dt->get_member_count(); ++i) {
    DDS::DynamicTypeMember_var dtm;
    if (dt->get_member_by_index(dtm, i) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataImpl::check_xcdr1_mutable: "
                  "Failed to get member from DynamicType\n"));
      }
      return false;
    }
    DDS::MemberDescriptor_var descriptor;
    if (dtm->get_descriptor(descriptor) != DDS::RETCODE_OK) {
      return false;
    }
    if (!check_xcdr1_mutable_i(descriptor->type(), dtns)) {
      return false;
    }
  }
  return true;
}

#ifndef OPENDDS_SAFETY_PROFILE
bool print_member(DDS::DynamicData_ptr dd, DCPS::String& type_string, DCPS::String& indent,
                  DDS::DynamicType_ptr member_base_type, MemberId member_id);

bool print_struct(DDS::DynamicData_ptr dd, DCPS::String& type_string, DCPS::String& indent)
{
  DCPS::String temp_indent = indent;
  indent += "  ";
  const DDS::DynamicType_var type = dd->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  CORBA::String_var type_name = type->get_name();
  type_string += "struct " + DCPS::String(type_name) + "\n";

  const ACE_CDR::ULong item_count = dd->get_item_count();
  for (ACE_CDR::ULong idx = 0; idx != item_count; ++idx) {
    // Translate the item number into a MemberId.
    const MemberId member_id = dd->get_member_id_at_index(idx);

    DDS::MemberDescriptor_var member_descriptor;
    if (dd->get_descriptor(member_descriptor, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    const DCPS::String member_name = member_descriptor->name();
    const DDS::DynamicType_ptr member_type = member_descriptor->type();
    const DDS::DynamicType_var member_base_type = get_base_type(member_descriptor->type());
    DDS::TypeDescriptor_var member_base_type_descriptor;
    if (member_base_type->get_descriptor(member_base_type_descriptor) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::String_var member_type_name = member_type->get_name();
    if (member_base_type->get_kind() == TK_STRUCTURE ||
        member_base_type->get_kind() == TK_UNION) {
      type_string += indent;
    } else {
      type_string += indent + DCPS::String(member_type_name.in()) + " " + member_name;
    }
    if (member_base_type_descriptor->kind() == TK_SEQUENCE ||
        member_base_type_descriptor->kind() == TK_ARRAY) {
      DDS::TypeDescriptor_var td;
      member_base_type_descriptor->element_type()->get_descriptor(td);
      DCPS::String ele_type_name = td->name();
      type_string += " " + ele_type_name;
    }

    if (!print_member(dd, type_string, indent, member_base_type, member_id)) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to read struct member\n"));
      }
      return false;
    }
  }

  indent = temp_indent;
  return true;
}

bool print_union(DDS::DynamicData_ptr dd, DCPS::String& type_string, DCPS::String& indent)
{
  DCPS::String temp_indent = indent;
  indent += "  ";
  const DDS::DynamicType_var type = dd->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  DDS::TypeDescriptor_var type_descriptor;
  if (base_type->get_descriptor(type_descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  CORBA::String_var type_name = type->get_name();
  type_string += "union " + DCPS::String(type_name) + "\n";
  const ACE_CDR::ULong item_count = dd->get_item_count();
  for (ACE_CDR::ULong idx = 0; idx != item_count; ++idx) {
    // Translate the item number into a MemberId.
    const MemberId member_id = dd->get_member_id_at_index(idx);

    if (member_id == DISCRIMINATOR_ID) {
      const DDS::DynamicType_ptr discriminator_type = type_descriptor->discriminator_type();
      const DDS::DynamicType_var discriminator_base_type = get_base_type(discriminator_type);
      DDS::TypeDescriptor_var discriminator_descriptor;
      if (discriminator_base_type->get_descriptor(discriminator_descriptor) != DDS::RETCODE_OK) {
        return false;
      }
      const DCPS::String member_name = discriminator_descriptor->name();
      type_string += indent + member_name + " discriminator";
      if (!print_member(dd, type_string, indent, discriminator_base_type, DISCRIMINATOR_ID)) {
        return false;
      }
    } else {

      DDS::MemberDescriptor_var member_descriptor;
      if (dd->get_descriptor(member_descriptor, member_id) != DDS::RETCODE_OK) {
        return false;
      }

      const DCPS::String member_name = member_descriptor->name();
      const DDS::DynamicType_ptr member_type = member_descriptor->type();
      const DDS::DynamicType_var member_base_type = get_base_type(member_type);
      DDS::TypeDescriptor_var member_base_type_descriptor;
      if (member_base_type->get_descriptor(member_base_type_descriptor) != DDS::RETCODE_OK) {
        return false;
      }

      const CORBA::String_var member_type_name = member_type->get_name();
      if (member_base_type->get_kind() == TK_STRUCTURE ||
          member_base_type->get_kind() == TK_UNION) {
        type_string += indent;
      } else {
        type_string += indent + DCPS::String(member_type_name.in()) + " " + member_name;
      }
      if (member_base_type_descriptor->kind() == TK_SEQUENCE ||
          member_base_type_descriptor->kind() == TK_ARRAY) {
        DDS::TypeDescriptor_var td;
        member_base_type_descriptor->element_type()->get_descriptor(td);
        DCPS::String ele_type_name = td->name();
        type_string += " " + ele_type_name;
      }

      if (!print_member(dd, type_string, indent, member_base_type, member_id)) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to read union branch\n"));
        }
        return false;
      }
    }
  }

  indent = temp_indent;
  return true;
}

bool print_member(DDS::DynamicData_ptr dd, DCPS::String& type_string, DCPS::String& indent,
                  DDS::DynamicType_ptr member_base_type, MemberId member_id)
{
  DDS::TypeDescriptor_var member_type_descriptor;
  if (member_base_type->get_descriptor(member_type_descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  switch (member_type_descriptor->kind()) {
  case TK_ENUM: {
    ACE_CDR::Long val;
    LBound bit_bound = member_type_descriptor->bound()[0];
    if (bit_bound <= 8) {
      ACE_CDR::Int8 my_int8;
      if (dd->get_int8_value(my_int8, member_id) != DDS::RETCODE_OK) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_int8_value\n"));
        }
        return false;
      }
      val = my_int8;
    } else if (bit_bound <= 16) {
      ACE_CDR::Short my_short;
      if (dd->get_int16_value(my_short, member_id) != DDS::RETCODE_OK) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_int16_value\n"));
        }
        return false;
      }
      val = my_short;
    } else {
      ACE_CDR::Long my_long;
      if (dd->get_int32_value(my_long, member_id) != DDS::RETCODE_OK) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_int32_value\n"));
        }
        return false;
      }
      val = my_long;
    }
    DDS::DynamicTypeMember_var temp_dtm;
    member_base_type->get_member_by_index(temp_dtm, val);
    DDS::MemberDescriptor_var descriptor;
    temp_dtm->get_descriptor(descriptor);
    type_string += " = " + DCPS::String(descriptor->name()) + "\n";
    break;
  }
  case TK_INT8: {
    ACE_CDR::Int8 my_int8;
    if (dd->get_int8_value(my_int8, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_int8_value\n"));
      }
      return false;
    }
    type_string += " = " + DCPS::to_dds_string(my_int8) + "\n";
    break;
  }
  case TK_UINT8: {
    ACE_CDR::UInt8 my_uint8;
    if (dd->get_uint8_value(my_uint8, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_uint8_value\n"));
      }
      return false;
    }
    type_string += " = " + DCPS::to_dds_string(my_uint8) + "\n";
    break;
  }
  case TK_INT16: {
    ACE_CDR::Short my_short;
    if (dd->get_int16_value(my_short, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_int16_value\n"));
      }
      return false;
    }
    type_string += " = " + DCPS::to_dds_string(my_short) + "\n";
    break;
  }
  case TK_UINT16: {
    ACE_CDR::UShort my_ushort;
    if (dd->get_uint16_value(my_ushort, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_uint16_value\n"));
      }
      return false;
    }
    type_string += " = " + DCPS::to_dds_string(my_ushort) + "\n";
    break;
  }
  case TK_INT32: {
    ACE_CDR::Long my_long;
    if (dd->get_int32_value(my_long, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_int32_value\n"));
      }
      return false;
    }
    type_string += " = " + DCPS::to_dds_string(my_long) + "\n";
    break;
  }
  case TK_UINT32: {
    ACE_CDR::ULong my_ulong;
    if (dd->get_uint32_value(my_ulong, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_uint32_value\n"));
      }
      return false;
    }
    type_string += " = " + DCPS::to_dds_string(my_ulong) + "\n";
    break;
  }
  case TK_INT64: {
    ACE_CDR::LongLong my_longlong;
    if (dd->get_int64_value(my_longlong, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_int64_value\n"));
      }
      return false;
    }
    type_string += " = " + DCPS::to_dds_string(my_longlong) + "\n";
    break;
  }
  case TK_UINT64: {
    ACE_CDR::ULongLong my_ulonglong;
    if (dd->get_uint64_value(my_ulonglong, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_uint64_value\n"));
      }
      return false;
    }
    type_string += " = " + DCPS::to_dds_string(my_ulonglong) + "\n";
    break;
  }
  case TK_BOOLEAN: {
    ACE_CDR::Boolean my_bool;
    if (dd->get_boolean_value(my_bool, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_boolean_value\n"));
      }
      return false;
    }
    DCPS::String bool_string = my_bool ? "true" : "false";
    type_string += " = " + bool_string + "\n";
    break;
  }
  case TK_BYTE: {
    ACE_CDR::Octet my_byte;
    if (dd->get_byte_value(my_byte, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_byte_value\n"));
      }
      return false;
    }
    std::stringstream os;
    os << std::hex << std::setfill('0') << std::setw(2) << int(my_byte);
    type_string += " = 0x" + os.str() + "\n";
    break;
  }
  case TK_CHAR8: {
    ACE_CDR::Char my_char;
    if (dd->get_char8_value(my_char, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_char8_value\n"));
      }
      return false;
    }
    std::stringstream os;
    DCPS::char_helper<ACE_CDR::Char>(os, my_char);
    type_string += " = '" + os.str() + "'\n";
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    ACE_CDR::WChar my_wchar;
    if (dd->get_char16_value(my_wchar, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_char16_value\n"));
      }
      return false;
    }
    std::stringstream os;
    DCPS::char_helper<ACE_CDR::WChar>(os, my_wchar);
    type_string += " = L'" + os.str() + "'\n";
    break;
  }
#endif
  case TK_FLOAT32: {
    ACE_CDR::Float my_float;
    if (dd->get_float32_value(my_float, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_float32_value\n"));
      }
      return false;
    }
    std::stringstream os;
    os << my_float;
    type_string += " = " + os.str() + "\n";
    break;
  }
  case TK_FLOAT64: {
    ACE_CDR::Double my_double;
    if (dd->get_float64_value(my_double, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_float64_value\n"));
      }
      return false;
    }
    std::stringstream os;
    os << my_double;
    type_string += " = " + os.str() + "\n";
    break;
  }
  case TK_FLOAT128: {
    ACE_CDR::LongDouble my_longdouble;
    if (dd->get_float128_value(my_longdouble, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_float128_value\n"));
      }
      return false;
    }
    std::stringstream os;
    os << my_longdouble;
    type_string += " = " + os.str() + "\n";
    break;
  }
  case TK_STRING8: {
    CORBA::String_var my_string;
    if (dd->get_string_value(my_string, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_string_value\n"));
      }
      return false;
    }
    std::stringstream os;
    DCPS::string_helper(os, my_string.inout());
    type_string += DCPS::String(" = \"") + os.str() + "\"\n";
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    CORBA::WString_var my_wstring;
    if (dd->get_wstring_value(my_wstring, member_id) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to get_wstring_value\n"));
      }
      return false;
    }
    std::stringstream os;
    DCPS::string_helper(os, my_wstring.inout());
    type_string += " = L\"" + os.str() + "\"\n";
    break;
  }
#endif
  case TK_BITMASK:
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: Bitmask is an unsupported type in OpenDDS\n"));
    }
    break;
  case TK_BITSET:
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: Bitset is an unsupported type in OpenDDS\n"));
    }
    break;
  case TK_SEQUENCE: {
    DCPS::String temp_indent = indent;
    indent += "  ";
    DDS::DynamicData_var temp_dd;
    if (dd->get_complex_value(temp_dd, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    const ACE_CDR::ULong seq_length = temp_dd->get_item_count();
    type_string += "[" + DCPS::to_dds_string(seq_length) + "] =\n";
    for (ACE_CDR::ULong i = 0; i < seq_length; ++i) {
      type_string += indent + "[" + DCPS::to_dds_string(i) + "]";
      if (!print_member(temp_dd, type_string, indent, member_type_descriptor->element_type(), i)) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to read sequence member\n"));
        }
        return false;
      }
    }
    indent = temp_indent;
    break;
  }
  case TK_ARRAY: {
    DCPS::String temp_indent = indent;
    indent += "  ";
    DDS::DynamicData_var temp_dd;
    if (dd->get_complex_value(temp_dd, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    const LBound bound = member_type_descriptor->bound()[0];
    type_string += "[" + DCPS::to_dds_string(bound) + "] =\n";
    for (ACE_CDR::ULong i = 0; i < bound; ++i) {
      type_string += indent + "[" + DCPS::to_dds_string(i) + "]";
      if (!print_member(temp_dd, type_string, indent, member_type_descriptor->element_type(), i)) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: failed to read array member\n"));
        }
        return false;
      }
    }
    indent = temp_indent;
    break;
  }
  case TK_STRUCTURE: {
    DDS::DynamicData_var temp_dd;
    if (dd->get_complex_value(temp_dd, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    if (!print_struct(temp_dd, type_string, indent)) {
      return false;
    }
    break;
  }
  case TK_UNION: {
    DDS::DynamicData_var temp_dd;
    if (dd->get_complex_value(temp_dd, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    if (!print_union(temp_dd, type_string, indent)) {
      return false;
    }
    break;
  }
  }

  return true;
}

bool print_dynamic_data(DDS::DynamicData_ptr dd, DCPS::String& type_string, DCPS::String& indent)
{
  const DDS::DynamicType_var type = dd->type();
  const DDS::DynamicType_var base_type = get_base_type(type);

  switch (base_type->get_kind()) {
  case TK_STRUCTURE:
    return print_struct(dd, type_string, indent);
    break;
  case TK_UNION:
    return print_union(dd, type_string, indent);
    break;
  default:
    return false;
  }
}
#endif

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
