/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicDataXcdrReadImpl.h"

#  include "DynamicTypeMemberImpl.h"
#  include "Utils.h"

#  include <dds/DCPS/debug.h>
#  include <dds/DCPS/FilterEvaluator.h>
#  include <dds/DCPS/SafetyProfileStreams.h>
#  include <dds/DCPS/ValueHelper.h>
#  include <dds/DCPS/DCPS_Utils.h>

#  include <dds/DdsDcpsCoreTypeSupportImpl.h>
#  include <dds/DdsDcpsInfrastructureC.h>
#  include <dds/DdsDynamicDataSeqTypeSupportImpl.h>
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

#  include <ace/OS_NS_string.h>

#  include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

using DCPS::log_level;
using DCPS::LogLevel;
using DCPS::retcode_to_string;

DynamicDataXcdrReadImpl::DynamicDataXcdrReadImpl()
  : chain_(0)
  , encoding_(DCPS::Encoding::KIND_XCDR2)
  , extent_(DCPS::Sample::Full)
  , reset_align_state_(false)
  , strm_(0, encoding_)
  , item_count_(ITEM_COUNT_INVALID)
{}

DynamicDataXcdrReadImpl::DynamicDataXcdrReadImpl(ACE_Message_Block* chain,
                                                 const DCPS::Encoding& encoding,
                                                 DDS::DynamicType_ptr type,
                                                 DCPS::Sample::Extent ext)
  : DynamicDataBase(type)
  , chain_(chain->duplicate())
  , encoding_(encoding)
  , extent_(ext)
  , reset_align_state_(false)
  , strm_(chain_, encoding_)
  , item_count_(ITEM_COUNT_INVALID)
{
  if (encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_1 &&
      encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_2) {
    throw std::runtime_error("DynamicDataXcdrReadImpl only supports XCDR1 and XCDR2");
  }
}

DynamicDataXcdrReadImpl::DynamicDataXcdrReadImpl(DCPS::Serializer& ser, DDS::DynamicType_ptr type,
                                                 DCPS::Sample::Extent ext)
  : DynamicDataBase(type)
  , chain_(ser.current()->duplicate())
  , encoding_(ser.encoding())
  , extent_(ext)
  , reset_align_state_(true)
  , align_state_(ser.rdstate())
  , strm_(chain_, encoding_)
  , item_count_(ITEM_COUNT_INVALID)
{
  if (encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_1 &&
      encoding_.xcdr_version() != DCPS::Encoding::XCDR_VERSION_2) {
    throw std::runtime_error("DynamicDataXcdrReadImpl only supports XCDR1 and XCDR2");
  }

  strm_.rdstate(align_state_);
}

DynamicDataXcdrReadImpl::DynamicDataXcdrReadImpl(const DynamicDataXcdrReadImpl& other)
  : CORBA::Object()
  , DDS::DynamicData()
  , CORBA::LocalObject()
  , RcObject()
  , DynamicDataBase()
  , chain_(0)
  , strm_(0, other.encoding_)
{
  copy(other);
}

DynamicDataXcdrReadImpl& DynamicDataXcdrReadImpl::operator=(const DynamicDataXcdrReadImpl& other)
{
  if (this != &other) {
    copy(other);
  }
  return *this;
}

DynamicDataXcdrReadImpl::~DynamicDataXcdrReadImpl()
{
  ACE_Message_Block::release(chain_);
}

void DynamicDataXcdrReadImpl::copy(const DynamicDataXcdrReadImpl& other)
{
  ACE_Message_Block::release(chain_);
  chain_ = other.chain_->duplicate();
  encoding_ = other.encoding_;
  extent_ = other.extent_;
  reset_align_state_ = other.reset_align_state_;
  align_state_ = other.align_state_;
  strm_ = other.strm_;
  type_ = other.type_;
  item_count_ = other.item_count_;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::set_descriptor(MemberId, DDS::MemberDescriptor*)
{
  return DDS::RETCODE_UNSUPPORTED;
}

bool DynamicDataXcdrReadImpl::has_optional_member(bool& has_optional) const
{
  if (type_->get_kind() != TK_STRUCTURE) {
    return false;
  }

  const ACE_CDR::ULong count = type_->get_member_count();
  for (unsigned i = 0; i < count; ++i) {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member_by_index(member, i) != DDS::RETCODE_OK) {
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

DDS::MemberId DynamicDataXcdrReadImpl::get_member_id_at_index(ACE_CDR::ULong index)
{
  if (item_count_ == ITEM_COUNT_INVALID) {
    get_item_count();
  }
  if (index >= item_count_) {
    return MEMBER_ID_INVALID;
  }

  ScopedChainManager chain_manager(*this);

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
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                 " Received invalid index (%d) for type %C\n", index, typekind_to_string(tk)));
    }
    return MEMBER_ID_INVALID;
  case TK_STRING8:
#ifdef DDS_HAS_WCHAR
  case TK_STRING16:
#endif
  case TK_BITMASK:
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    return index;
  case TK_STRUCTURE:
    {
      const DDS::ExtensibilityKind ek = type_desc_->extensibility_kind();
      if (ek == DDS::APPENDABLE || ek == DDS::MUTABLE) {
        if (!strm_.skip_delimiter()) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                       " Failed to skip delimiter\n"));
          }
          return MEMBER_ID_INVALID;
        }
      }

      DDS::ReturnCode_t rc;
      if (ek == DDS::FINAL || ek == DDS::APPENDABLE) {
        bool has_optional;
        if (!has_optional_member(has_optional)) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                       " has_optional_member failed\n"));
          }
          return MEMBER_ID_INVALID;
        }

        if (extent_ != DCPS::Sample::Full) {
          const bool struct_has_explicit_keys = has_explicit_keys(type_);
          if (struct_has_explicit_keys) {
            // All and only explicit keys will be present in the binary stream.
            // Index of the next member in the binary stream.
            ACE_CDR::ULong curr_index = 0;
            for (ACE_CDR::ULong i = 0; i < type_->get_member_count(); ++i) {
              DDS::DynamicTypeMember_var dtm;
              rc = type_->get_member_by_index(dtm, i);
              if (rc != DDS::RETCODE_OK) {
                if (log_level >= LogLevel::Warning) {
                  ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                             " get_member_by_index %u returned %C\n", i, retcode_to_string(rc)));
                }
                return MEMBER_ID_INVALID;
              }
              DDS::MemberDescriptor_var md;
              rc = dtm->get_descriptor(md);
              if (rc != DDS::RETCODE_OK) {
                if (log_level >= LogLevel::Warning) {
                  ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                             " get_descriptor for member at index %u returned %C\n",
                             i, retcode_to_string(rc)));
                }
                return MEMBER_ID_INVALID;
              }
              if (exclude_member(extent_, md->is_key(), struct_has_explicit_keys)) {
                continue;
              }
              if (curr_index == index) {
                return md->id();
              }
              ++curr_index;
            }
            // Should never reach here!
            OPENDDS_ASSERT(false);
            return MEMBER_ID_INVALID;
          } else if (extent_ == DCPS::Sample::KeyOnly) {
            return MEMBER_ID_INVALID;
          }
        }

        // A Full sample or a NestedKeyOnly sample with no explicit key.
        if (!has_optional) {
          DDS::DynamicTypeMember_var member;
          rc = type_->get_member_by_index(member, index);
          if (rc != DDS::RETCODE_OK) {
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                         " get_member_by_index %u returned %C\n", index, retcode_to_string(rc)));
            }
            return MEMBER_ID_INVALID;
          }
          return member->get_id();
        } else {
          MemberId id = MEMBER_ID_INVALID;
          ACE_CDR::ULong total_skipped = 0;
          for (ACE_CDR::ULong i = 0; i < type_->get_member_count(); ++i) {
            ACE_CDR::ULong num_skipped;
            if (!skip_struct_member_at_index(i, num_skipped)) {
              if (log_level >= LogLevel::Warning) {
                ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                           " skip_struct_member_at_index %u failed\n", i));
              }
              break;
            }
            total_skipped += num_skipped;
            if (total_skipped == index + 1) {
              DDS::DynamicTypeMember_var member;
              rc = type_->get_member_by_index(member, i);
              if (rc != DDS::RETCODE_OK) {
                if (log_level >= LogLevel::Warning) {
                  ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                             " get_member_by_index %u returned %C\n", i, retcode_to_string(rc)));
                }
              } else {
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
        for (unsigned i = 0; i < index; ++i) {
          if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                         " read_parameter_id for member at index %u failed\n", i));
            }
            return MEMBER_ID_INVALID;
          }
          if (!strm_.skip(member_size)) {
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                         " skip member with Id %u failed\n", member_id));
            }
            return MEMBER_ID_INVALID;
          }
        }

        if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                       " read_parameter_id for member at index %u failed\n", index));
          }
          return MEMBER_ID_INVALID;
        }
        return member_id;
      }
    }
  case TK_UNION:
    {
      const DDS::ExtensibilityKind ek = type_desc_->extensibility_kind();
      if (extent_ == DCPS::Sample::KeyOnly) {
        DDS::DynamicTypeMember_var disc_dtm;
        DDS::ReturnCode_t rc = type_->get_member(disc_dtm, DISCRIMINATOR_ID);
        if (rc != DDS::RETCODE_OK) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                       "get_member for discriminator returned %C\n", retcode_to_string(rc)));
          }
          return MEMBER_ID_INVALID;
        }
        DDS::MemberDescriptor_var disc_md;
        rc = disc_dtm->get_descriptor(disc_md);
        if (rc != DDS::RETCODE_OK) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                       " get_descriptor for discriminator returned %C\n", retcode_to_string(rc)));
          }
          return MEMBER_ID_INVALID;
        }
        if (!disc_md->is_key()) {
          // The sample is empty.
          return MEMBER_ID_INVALID;
        }
      }

      if (index == 0) {
        return DISCRIMINATOR_ID;
      }

      // KeyOnly or NestedKeyOnly sample can only contain discriminator.
      if (index > 1 || extent_ != DCPS::Sample::Full) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                     " Invalid index: %u\n", index));
        }
        return MEMBER_ID_INVALID;
      }

      // The selected member can be optional and omitted in the binary data.
      DDS::MemberDescriptor_var selected_md = get_union_selected_member();
      if (!selected_md) {
        return MEMBER_ID_INVALID;
      }
      const DDS::MemberId id = selected_md->id();
      if (!selected_md->is_optional()) {
        return id;
      }

      if (ek == DDS::FINAL || ek == DDS::APPENDABLE) {
        if (strm_.encoding().kind() == DCPS::Encoding::KIND_XCDR2) {
          CORBA::Boolean present;
          if (!(strm_ >> ACE_InputCDR::to_boolean(present))) {
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                         " Read optional member with Id %u failed\n", id));
            }
            return MEMBER_ID_INVALID;
          }
          if (!present) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                       " Optional member with Id %u is omitted\n", id));
            return MEMBER_ID_INVALID;
          }
          return id;
        }
      } else {
        DDS::MemberId member_id;
        size_t member_size;
        bool must_understand;
        if (strm_.read_parameter_id(member_id, member_size, must_understand)) {
          return id;
        }
      }
      return MEMBER_ID_INVALID;
    }
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_member_id_at_index:"
                 " Called on an unexpected type %C\n", typekind_to_string(tk)));
    }
  }

  return MEMBER_ID_INVALID;
}

bool DynamicDataXcdrReadImpl::get_struct_item_count()
{
  bool has_optional;
  if (!has_optional_member(has_optional)) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                 " has_optional_member failed\n"));
    }
    return false;
  }

  if (extent_ != DCPS::Sample::Full) {
    const bool struct_has_explicit_keys = has_explicit_keys(type_);
    if (struct_has_explicit_keys) {
      ACE_CDR::ULong num_explicit_keys = 0;
      for (ACE_CDR::ULong i = 0; i < type_->get_member_count(); ++i) {
        DDS::DynamicTypeMember_var dtm;
        DDS::ReturnCode_t rc = type_->get_member_by_index(dtm, i);
        if (rc != DDS::RETCODE_OK) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                       " get_member_by_index %u failed: %C\n", i, retcode_to_string(rc)));
          }
          return false;
        }
        DDS::MemberDescriptor_var md;
        rc = dtm->get_descriptor(md);
        if (rc != DDS::RETCODE_OK) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                       " get_descriptor for member at index %u failed: %C\n", i, retcode_to_string(rc)));
          }
          return false;
        }
        if (md->is_key()) {
          ++num_explicit_keys;
        }
      }
      item_count_ = num_explicit_keys;
      return true;
    } else if (extent_ == DCPS::Sample::KeyOnly) {
      item_count_ = 0;
      return true;
    }
  }

  // Full sample or NestedKeyOnly sample with no explicit key.
  if (!has_optional) {
    item_count_ = type_->get_member_count();
    return true;
  }

  // Optional members can be omitted, so we need to count members one by one.
  ACE_CDR::ULong actual_count = 0;
  const DDS::ExtensibilityKind ek = type_desc_->extensibility_kind();
  if (ek == DDS::FINAL || ek == DDS::APPENDABLE) {
    if (ek == DDS::APPENDABLE) {
      if (!strm_.skip_delimiter()) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: Skip delimiter failed\n"));
        }
        return false;
      }
    }
    for (ACE_CDR::ULong i = 0; i < type_->get_member_count(); ++i) {
      ACE_CDR::ULong num_skipped;
      if (!skip_struct_member_at_index(i, num_skipped)) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                     " Failed to skip struct member at index %u\n", i));
        }
        return false;
      }
      actual_count += num_skipped;
    }
  } else { // Mutable
    size_t dheader = 0;
    if (!strm_.read_delimiter(dheader)) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: Read delimiter failed\n"));
      }
      return false;
    }

    const size_t end_of_struct = strm_.rpos() + dheader;
    while (strm_.rpos() < end_of_struct) {
      ACE_CDR::ULong member_id;
      size_t member_size;
      bool must_understand;
      if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: read_parameter_id failed\n"));
        }
        return false;
      }
      if (!strm_.skip(member_size)) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                     " Failed to skip %u bytes from the stream\n", member_size));
        }
        return false;
      }
      ++actual_count;
    }
  }
  item_count_ = actual_count;
  return true;
}

bool DynamicDataXcdrReadImpl::get_union_item_count()
{
  DDS::ReturnCode_t rc;
  if (extent_ == DCPS::Sample::KeyOnly) {
    DDS::DynamicTypeMember_var disc_dtm;
    rc = type_->get_member(disc_dtm, DISCRIMINATOR_ID);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                   " get_member for discriminator returned %C\n", retcode_to_string(rc)));
      }
      return false;
    }
    DDS::MemberDescriptor_var disc_md;
    rc = disc_dtm->get_descriptor(disc_md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                   " get_descriptor for discriminator returned %C\n", retcode_to_string(rc)));
      }
      return false;
    }
    if (!disc_md->is_key()) {
      item_count_ = 0;
      return true;
    }
  }
  if (extent_ != DCPS::Sample::Full) {
    item_count_ = 1;
    return true;
  }

  const DDS::ExtensibilityKind ek = type_desc_->extensibility_kind();
  if (ek == DDS::APPENDABLE || ek == DDS::MUTABLE) {
    if (!strm_.skip_delimiter()) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: skip_delimiter failed\n"));
      }
      return false;
    }
  }
  const DDS::DynamicType_var disc_type = get_base_type(type_desc_->discriminator_type());
  const DDS::ExtensibilityKind extend = type_desc_->extensibility_kind();
  ACE_CDR::Long label;
  if (!read_discriminator(disc_type, extend, label)) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: read_discriminator failed\n"));
    }
    return false;
  }

  DDS::DynamicTypeMembersById_var members;
  rc = type_->get_all_members(members);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                 " get_all_members returned %C\n", retcode_to_string(rc)));
    }
    return false;
  }
  DynamicTypeMembersByIdImpl* members_impl = dynamic_cast<DynamicTypeMembersByIdImpl*>(members.in());
  if (!members_impl) {
    return false;
  }
  for (DynamicTypeMembersByIdImpl::const_iterator it = members_impl->begin(); it != members_impl->end(); ++it) {
    DDS::MemberDescriptor_var md;
    rc = it->second->get_descriptor(md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                   " get_descriptor returned %C\n", retcode_to_string(rc)));
      }
      return false;
    }
    if (md->is_default_label()) {
      item_count_ = 2;
      return true;
    }
    const DDS::UnionCaseLabelSeq& labels = md->label();
    for (ACE_CDR::ULong i = 0; i < labels.length(); ++i) {
      if (label == labels[i]) {
        item_count_ = 2;
        return true;
      }
    }
  }

  item_count_ = 1;
  return true;
}

DDS::UInt32 DynamicDataXcdrReadImpl::get_item_count()
{
  if (item_count_ != ITEM_COUNT_INVALID) {
    return item_count_;
  }

  ScopedChainManager chain_manager(*this);

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
    item_count_ = 1;
    break;
  case TK_STRING8:
  case TK_STRING16:
    {
      ACE_CDR::ULong bytes;
      if (!(strm_ >> bytes)) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                     " Failed to read %C\n", typekind_to_string(tk)));
        }
        return 0;
      }
      item_count_ = (tk == TK_STRING8) ? bytes : bytes/2;
    }
    break;
  case TK_BITMASK:
    item_count_ = type_desc_->bound()[0];
    break;
  case TK_STRUCTURE:
    if (!get_struct_item_count()) {
      return 0;
    }
    break;
  case TK_UNION:
    if (!get_union_item_count()) {
      return 0;
    }
    break;
  case TK_SEQUENCE:
    {
      const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
      if (!is_primitive(elem_type->get_kind()) && !strm_.skip_delimiter()) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: skip delimiter failed\n"));
        }
        return 0;
      }
      ACE_CDR::ULong length;
      if (!(strm_ >> length)) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: read sequence length failed\n"));
        }
        return 0;
      }
      item_count_ = length;
    }
    break;
  case TK_ARRAY:
    item_count_ = bound_total(type_desc_);
    break;
  case TK_MAP:
    {
      const DDS::DynamicType_var key_type = get_base_type(type_desc_->key_element_type());
      const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
      if (is_primitive(key_type->get_kind()) && is_primitive(elem_type->get_kind())) {
        ACE_CDR::ULong length;
        if (!(strm_ >> length)) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: read map length failed\n"));
          }
          return 0;
        }
        item_count_ = length;
        break;
      }

      size_t dheader;
      if (!strm_.read_delimiter(dheader)) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: read delimiter failed\n"));
        }
        return 0;
      }
      const size_t end_of_map = strm_.rpos() + dheader;

      ACE_CDR::ULong length = 0;
      while (strm_.rpos() < end_of_map) {
        if (!skip_member(key_type) || !skip_member(elem_type)) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count: skip map entry failed\n"));
          }
          return 0;
        }
        ++length;
      }
      item_count_ = length;
    }
    break;
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataXcdrReadImpl::get_item_count:"
                 " Calling on an unexpected type %C\n", typekind_to_string(tk)));
    }
    return 0;
  }

  return item_count_;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::clear_all_values()
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::clear_nonkey_values()
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::clear_value(DDS::MemberId /*id*/)
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::DynamicData_ptr DynamicDataXcdrReadImpl::loan_value(DDS::MemberId /*id*/)
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataXcdrReadImpl::loan_value: Not implemented\n"));
  return 0;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::return_loaned_value(DDS::DynamicData_ptr /*value*/)
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataXcdrReadImpl::return_loaned_value: Not implemented\n"));
  return DDS::RETCODE_UNSUPPORTED;
}


DDS::DynamicData_ptr DynamicDataXcdrReadImpl::clone()
{
  return new DynamicDataXcdrReadImpl(chain_, strm_.encoding(), type_, extent_);
}

template<typename ValueType>
bool DynamicDataXcdrReadImpl::read_value(ValueType& value, TypeKind tk)
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
    if (strm_ >> value) {
      return true;
    }
    break;
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::read_value: "
                 "Calling on an unexpected type %C\n", typekind_to_string(tk)));
    }
    return false;
  }

  if (log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::read_value: "
               "failed to deserialize type %C\n", typekind_to_string(tk)));
  }
  return false;
}

// Check if a struct member with the given Id is excluded from a sample (in case of
// KeyOnly or NestedKeyOnly serialization).
bool DynamicDataXcdrReadImpl::exclude_struct_member(MemberId id, DDS::MemberDescriptor_var& md) const
{
  DDS::DynamicTypeMember_var dtm;
  if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::exclude_struct_member:"
                 " Failed to get DynamicTypeMember for member with ID %d\n", id));

    }
    return false;
  }
  if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::exclude_struct_member:"
                 " Failed to get MemberDescriptor for member with ID %d\n", id));
    }
    return false;
  }
  return exclude_member(extent_, md->is_key(), has_explicit_keys(type_));
}

template<TypeKind MemberTypeKind, typename MemberType>
DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_value_from_struct(MemberType& value, MemberId id)
{
  DDS::MemberDescriptor_var md;
  if (exclude_struct_member(id, md)) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::get_value_from_struct:"
                 " Attempted to read an excluded member from a %C sample\n",
                 extent_ == DCPS::Sample::KeyOnly ? "KeyOnly" : "NestedKeyOnly"));
    }
    return DDS::RETCODE_NO_DATA;
  }

  DDS::DynamicType_var member_type;
  DDS::ReturnCode_t rc = check_member(
    md, member_type, "DynamicDataXcdrReadImpl::get_value_from_struct", "get", id, MemberTypeKind);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  rc = skip_to_struct_member(md, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return read_value(value, MemberTypeKind) ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::MemberDescriptor* DynamicDataXcdrReadImpl::get_union_selected_member()
{
  const DDS::ExtensibilityKind ek = type_desc_->extensibility_kind();
  if (ek == DDS::APPENDABLE || ek == DDS::MUTABLE) {
    if (!strm_.skip_delimiter()) {
      return 0;
    }
  }

  const DDS::DynamicType_var disc_type = get_base_type(type_desc_->discriminator_type());
  ACE_CDR::Long label;
  if (!read_discriminator(disc_type, ek, label)) {
    return 0;
  }

  DDS::DynamicTypeMembersById_var members;
  if (type_->get_all_members(members) != DDS::RETCODE_OK) {
    return 0;
  }
  DynamicTypeMembersByIdImpl* members_impl = dynamic_cast<DynamicTypeMembersByIdImpl*>(members.in());
  if (!members_impl) {
    return 0;
  }

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
    return default_member._retn();
  }

  // The union has no selected member.
  return 0;
}

DDS::MemberDescriptor* DynamicDataXcdrReadImpl::get_from_union_common_checks(MemberId id, const char* func_name)
{
  DDS::MemberDescriptor_var md = get_union_selected_member();
  if (!md) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::%C - Could not find")
                 ACE_TEXT(" MemberDescriptor for the selected union member\n"), func_name));
    }
    return 0;
  }

  if (md->id() == MEMBER_ID_INVALID) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::%C - Union has no selected member\n"), func_name));
    }
    return 0;
  }

  if (md->id() != id) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::%C -")
                 ACE_TEXT(" ID of the selected member (%d) is not the requested ID (%d)\n"),
                 func_name, md->id(), id));
    }
    return 0;
  }

  return md._retn();
}

bool DynamicDataXcdrReadImpl::exclude_union_member(MemberId id) const
{
  if (extent_ == DCPS::Sample::Full) {
    return false;
  }

  if (id != DISCRIMINATOR_ID) {
    return true;
  }

  if (extent_ == DCPS::Sample::KeyOnly) {
    // Discriminator not marked as key is not included in a KeyOnly sample.
    DDS::DynamicTypeMember_var disc_dtm;
    if (type_->get_member(disc_dtm, DISCRIMINATOR_ID) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::exclude_union_member:"
                   " Failed to get DynamicTypeMember for discriminator\n"));
      }
      return false;
    }
    DDS::MemberDescriptor_var disc_md;
    if (disc_dtm->get_descriptor(disc_md) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::exclude_union_member:"
                   " Failed to get MemberDescriptor for discriminator\n"));
      }
      return false;
    }
    if (!disc_md->is_key()) {
      return true;
    }
  }
  return false;
}

template<TypeKind MemberTypeKind, typename MemberType>
DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_value_from_union(
  MemberType& value, MemberId id, TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (exclude_union_member(id)) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::get_value_from_union:"
                 " Reading an excluded member with Id %u\n", id));
    }
    return DDS::RETCODE_NO_DATA;
  }

  const DDS::ExtensibilityKind ek = type_desc_->extensibility_kind();
  DDS::DynamicType_var member_type;
  if (id == DISCRIMINATOR_ID) {
    if (ek == DDS::APPENDABLE || ek == DDS::MUTABLE) {
      if (!strm_.skip_delimiter()) {
        return DDS::RETCODE_ERROR;
      }
    }
    member_type = get_base_type(type_desc_->discriminator_type());
  } else {
    DDS::MemberDescriptor_var md = get_from_union_common_checks(id, "get_value_from_union");
    if (!md) {
      return DDS::RETCODE_ERROR;
    }

    const DDS::DynamicType_ptr type = md->type();
    if (!type) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_value_from_union -")
                   ACE_TEXT(" Could not get DynamicType of the selected member\n")));
      }
      return DDS::RETCODE_ERROR;
    }
    member_type = get_base_type(type);
  }

  const TypeKind member_tk = member_type->get_kind();
  if (member_tk != MemberTypeKind && member_tk != enum_or_bitmask) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_value_from_union -")
                 ACE_TEXT(" Could not read a value of type %C from type %C\n"),
                 typekind_to_string(MemberTypeKind), typekind_to_string(member_tk)));
    }
    return DDS::RETCODE_ERROR;
  }

  if (ek == DDS::MUTABLE) {
    unsigned member_id;
    size_t member_size;
    bool must_understand;
    if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
      return DDS::RETCODE_ERROR;
    }
  }

  if (member_tk == MemberTypeKind) {
    return read_value(value, MemberTypeKind) ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
  }

  DDS::TypeDescriptor_var td;
  DDS::ReturnCode_t rc = member_type->get_descriptor(td);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  const LBound bit_bound = td->bound()[0];
  return bit_bound >= lower && bit_bound <= upper &&
    read_value(value, MemberTypeKind) ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

bool DynamicDataXcdrReadImpl::skip_to_sequence_element(MemberId id, DDS::DynamicType_ptr coll_type)
{
  DDS::DynamicType_var elem_type;
  bool skip_all = false;
  if (!coll_type) {
    elem_type = get_base_type(type_desc_->element_type());
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
    ACE_CDR::ULong length, index;
    if (!strm_.skip_delimiter() || !(strm_ >> length)) {
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

bool DynamicDataXcdrReadImpl::skip_to_array_element(MemberId id, DDS::DynamicType_ptr coll_type)
{
  bool skip_all = false;
  DDS::TypeDescriptor_var coll_descriptor;

  if (!coll_type) {
    coll_descriptor = type_desc_;
  } else {
    if (coll_type->get_descriptor(coll_descriptor) != DDS::RETCODE_OK) {
      return false;
    }
    skip_all = true;
  }
  DDS::DynamicType_var elem_type = get_base_type(coll_descriptor->element_type());

  const ACE_CDR::ULong length = bound_total(coll_descriptor);

  ACE_CDR::ULong size;
  if (get_primitive_size(elem_type, size)) {
    ACE_CDR::ULong index;
    return get_index_from_id(id, index, length) && strm_.skip(index, size);
  } else {
    if (!strm_.skip_delimiter()) {
      return false;
    }
    ACE_CDR::ULong index;
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

bool DynamicDataXcdrReadImpl::skip_to_map_element(MemberId id)
{
  const DDS::DynamicType_var key_type = get_base_type(type_desc_->key_element_type());
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
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
bool DynamicDataXcdrReadImpl::get_value_from_collection(ElementType& value, MemberId id, TypeKind collection_tk,
                                                        TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  const TypeKind elem_tk = elem_type->get_kind();

  if (elem_tk != ElementTypeKind && elem_tk != enum_or_bitmask) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_value_from_collection -")
                 ACE_TEXT(" Could not read a value of type %C from %C with element type %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(collection_tk),
                 typekind_to_string(elem_tk)));
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

void DynamicDataXcdrReadImpl::setup_stream(ACE_Message_Block* chain)
{
  strm_ = DCPS::Serializer(chain, encoding_);
  if (reset_align_state_) {
    strm_.rdstate(align_state_);
  }
}

template<TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_single_value(ValueType& value, MemberId id,
                                                            TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (!is_type_supported(ValueTypeKind, "get_single_value")) {
    return DDS::RETCODE_ERROR;
  }

  ScopedChainManager chain_manager(*this);

  const TypeKind tk = type_->get_kind();
  TypeKind treat_as = tk;
  DDS::ReturnCode_t rc;
  if ((tk == TK_ENUM && (rc = enum_bound(type_, treat_as)) != DDS::RETCODE_OK) ||
      (tk == TK_BITMASK && (rc = bitmask_bound(type_, treat_as)) != DDS::RETCODE_OK)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::get_single_value: %C returned: %C\n",
                 tk == TK_ENUM ? "enum_bound" : "bitmask_bound", retcode_to_string(rc)));
    }
    return DDS::RETCODE_ERROR;
  }

  bool good = true;

  // An extension to the XTypes spec where the value of a bitmask DynamicData
  // can be read as a whole as a unsigned integer.
  switch (treat_as) {
  case ValueTypeKind:
    good = is_primitive(treat_as) && id == MEMBER_ID_INVALID && read_value(value, ValueTypeKind);
    break;
  case TK_STRUCTURE:
    rc = get_value_from_struct<ValueTypeKind>(value, id);
    if (rc == DDS::RETCODE_NO_DATA) {
      return rc;
    }
    good = rc == DDS::RETCODE_OK;
    break;
  case TK_UNION:
    rc = get_value_from_union<ValueTypeKind>(value, id, enum_or_bitmask, lower, upper);
    if (rc == DDS::RETCODE_NO_DATA) {
      return rc;
    }
    good = rc == DDS::RETCODE_OK;
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

  if (!good && log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::get_single_value:"
               " Failed to read a value of type %C from a DynamicData object of type %C\n",
               typekind_to_string(ValueTypeKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_int32_value(ACE_CDR::Long& value, MemberId id)
{
  return get_single_value<TK_INT32>(value, id, TK_ENUM,
                                    static_cast<LBound>(17), static_cast<LBound>(32));
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_uint32_value(ACE_CDR::ULong& value, MemberId id)
{
  return get_single_value<TK_UINT32>(value, id, TK_BITMASK,
                                     static_cast<LBound>(17), static_cast<LBound>(32));
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_int8_value(ACE_CDR::Int8& value, MemberId id)
{
  ACE_InputCDR::to_int8 to_int8(value);
  return get_single_value<TK_INT8>(to_int8, id, TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_uint8_value(ACE_CDR::UInt8& value, MemberId id)
{
  ACE_InputCDR::to_uint8 to_uint8(value);
  return get_single_value<TK_UINT8>(to_uint8, id, TK_BITMASK, 1, 8);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_int16_value(ACE_CDR::Short& value, MemberId id)
{
  return get_single_value<TK_INT16>(value, id, TK_ENUM, 9, 16);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_uint16_value(ACE_CDR::UShort& value, MemberId id)
{
  return get_single_value<TK_UINT16>(value, id, TK_BITMASK, 9, 16);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_int64_value_impl(ACE_CDR::LongLong& value, MemberId id)
{
  return get_single_value<TK_INT64>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_uint64_value_impl(ACE_CDR::ULongLong& value, MemberId id)
{
  return get_single_value<TK_UINT64>(value, id, TK_BITMASK, 33, 64);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_float32_value(ACE_CDR::Float& value, MemberId id)
{
  return get_single_value<TK_FLOAT32>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_float64_value(ACE_CDR::Double& value, MemberId id)
{
  return get_single_value<TK_FLOAT64>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_float128_value(ACE_CDR::LongDouble& value, MemberId id)
{
  return get_single_value<TK_FLOAT128>(value, id);
}

template<TypeKind CharKind, TypeKind StringKind, typename ToCharT, typename CharT>
DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_char_common(CharT& value, MemberId id)
{
  ScopedChainManager chain_manager(*this);

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
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_char_common -")
                     ACE_TEXT(" Failed to read wstring with ID %d\n"), id));
        }
        good = false;
        break;
      }
      ACE_CDR::ULong index;
      const size_t str_len = ACE_OS::strlen(str);
      if (!get_index_from_id(id, index, static_cast<ACE_CDR::ULong>(str_len))) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_char_common -")
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
      const DDS::ReturnCode_t rc = get_value_from_struct<CharKind>(wrap, id);
      if (rc == DDS::RETCODE_NO_DATA) {
        return rc;
      }
      good = rc == DDS::RETCODE_OK;
      break;
    }
  case TK_UNION:
    {
      ToCharT wrap(value);
      const DDS::ReturnCode_t rc = get_value_from_union<CharKind>(wrap, id);
      if (rc == DDS::RETCODE_NO_DATA) {
        return rc;
      }
      good = rc == DDS::RETCODE_OK;
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_char_common -")
               ACE_TEXT(" Failed to read DynamicData object of type %C\n"), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_char8_value(ACE_CDR::Char& value, MemberId id)
{
  return get_char_common<TK_CHAR8, TK_STRING8, ACE_InputCDR::to_char>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_char16_value(ACE_CDR::WChar& value, MemberId id)
{
#ifdef DDS_HAS_WCHAR
  return get_char_common<TK_CHAR16, TK_STRING16, ACE_InputCDR::to_wchar>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_byte_value(ACE_CDR::Octet& value, MemberId id)
{
  ACE_InputCDR::to_octet to_octet(value);
  return get_single_value<TK_BYTE>(to_octet, id);
}

template<typename UIntType, TypeKind UIntTypeKind>
bool DynamicDataXcdrReadImpl::get_boolean_from_bitmask(ACE_CDR::ULong index, ACE_CDR::Boolean& value)
{
  UIntType bitmask;
  if (!read_value(bitmask, UIntTypeKind)) {
    return false;
  }

  value = ((1ULL << index) & bitmask) ? true : false;
  return true;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_boolean_value(ACE_CDR::Boolean& value, MemberId id)
{
  ScopedChainManager chain_manager(*this);

  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case TK_BOOLEAN:
    good = strm_ >> ACE_InputCDR::to_boolean(value);
    break;
  case TK_BITMASK:
    {
      const LBound bit_bound = type_desc_->bound()[0];
      ACE_CDR::ULong index;
      if (!get_index_from_id(id, index, bit_bound)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_boolean_value -")
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
      } else if (bit_bound >= 17 && bit_bound <= 32) {
        good = get_boolean_from_bitmask<ACE_CDR::UInt32, TK_UINT32>(index, value);
      } else {
        good = get_boolean_from_bitmask<ACE_CDR::UInt64, TK_UINT64>(index, value);
      }
      break;
    }
  case TK_STRUCTURE:
    {
      ACE_InputCDR::to_boolean to_bool(value);
      const DDS::ReturnCode_t rc = get_value_from_struct<TK_BOOLEAN>(to_bool, id);
      if (rc == DDS::RETCODE_NO_DATA) {
        return rc;
      }
      good = rc == DDS::RETCODE_OK;
      break;
    }
  case TK_UNION:
    {
      ACE_InputCDR::to_boolean to_bool(value);
      const DDS::ReturnCode_t rc = get_value_from_union<TK_BOOLEAN>(to_bool, id);
      if (rc == DDS::RETCODE_NO_DATA) {
        return rc;
      }
      good = rc == DDS::RETCODE_OK;
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_boolean_value -")
               ACE_TEXT(" Failed to read DynamicData object of type %C\n"), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_string_value(ACE_CDR::Char*& value, MemberId id)
{
  if (enum_string_helper(value, id)) {
    return DDS::RETCODE_OK;
  }
  CORBA::string_free(value);
  return get_single_value<TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_wstring_value(ACE_CDR::WChar*& value, MemberId id)
{
#ifdef DDS_HAS_WCHAR
  CORBA::wstring_free(value);
  return get_single_value<TK_STRING16>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_complex_value(DDS::DynamicData_ptr& value, MemberId id)
{
  ScopedChainManager chain_manager(*this);

  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case TK_STRUCTURE:
    {
      DDS::MemberDescriptor_var md;
      if (exclude_struct_member(id, md)) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::get_complex_value:"
                     " Attempted to read an excluded member from a %C struct sample\n",
                     extent_ == DCPS::Sample::KeyOnly ? "KeyOnly" : "NestedKeyOnly"));
        }
        return DDS::RETCODE_NO_DATA;
      }

      const DDS::ReturnCode_t rc = skip_to_struct_member(md, id);
      if (rc == DDS::RETCODE_NO_DATA) {
        return rc;
      } else if (rc == DDS::RETCODE_OK) {
        DDS::DynamicType_ptr member_type = md->type();
        if (!member_type) {
          good = false;
        } else {
          CORBA::release(value);
          value = new DynamicDataXcdrReadImpl(strm_, member_type, nested(extent_));
        }
      } else {
        good = false;
      }
      break;
    }
  case TK_UNION:
    {
      if (exclude_union_member(id)) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::get_complex_value:"
                     " Attempted to read an excluded member from a %C union sample\n",
                     extent_ == DCPS::Sample::KeyOnly ? "KeyOnly" : "NestedKeyOnly"));
        }
        good = false;
        break;
      }

      const DDS::ExtensibilityKind ek = type_desc_->extensibility_kind();
      if (id == DISCRIMINATOR_ID) {
        if (ek == DDS::APPENDABLE || ek == DDS::MUTABLE) {
          if (!strm_.skip_delimiter()) {
            good = false;
            break;
          }
        }

        const DDS::DynamicType_var disc_type = get_base_type(type_desc_->discriminator_type());
        if (ek == DDS::MUTABLE) {
          unsigned id;
          size_t size;
          bool must_understand;
          if (!strm_.read_parameter_id(id, size, must_understand)) {
            good = false;
            break;
          }
        }
        CORBA::release(value);
        value = new DynamicDataXcdrReadImpl(strm_, disc_type, nested(extent_));
        break;
      }

      DDS::MemberDescriptor_var md = get_from_union_common_checks(id, "get_complex_value");
      if (!md) {
        good = false;
        break;
      }

      if (ek == DDS::MUTABLE) {
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
        CORBA::release(value);
        value = new DynamicDataXcdrReadImpl(strm_, member_type, nested(extent_));
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
        CORBA::release(value);
        value = new DynamicDataXcdrReadImpl(strm_, type_desc_->element_type(), nested(extent_));
      }
      break;
    }
  default:
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_complex_value -")
                 ACE_TEXT(" Called on an unsupported type (%C)\n"), typekind_to_string(tk)));
    }
    good = false;
    break;
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

template<typename SequenceType>
bool DynamicDataXcdrReadImpl::read_values(SequenceType& value, TypeKind elem_tk)
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
    if (strm_ >> value) {
      return true;
    }
    break;
  case TK_ENUM:
  case TK_BITMASK:
    if (strm_.skip_delimiter() && strm_ >> value) {
      return true;
    }
    break;
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::read_values: "
                 "Calling on an unexpected element type %C\n", typekind_to_string(elem_tk)));
    }
    return false;
  }

  if (log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::read_values: "
               "failed to deserialize element type %C\n", typekind_to_string(elem_tk)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_values_from_struct(SequenceType& value, MemberId id,
  TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  DDS::MemberDescriptor_var md;
  if (exclude_struct_member(id, md)) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::get_values_from_struct:"
                 " Attempted to read a member not included in a %C sample\n",
                 extent_ == DCPS::Sample::KeyOnly ? "KeyOnly" : "NestedKeyOnly"));
    }
    return DDS::RETCODE_NO_DATA;
  }

  if (get_from_struct_common_checks(md, id, ElementTypeKind, true)) {
    const DDS::ReturnCode_t rc = skip_to_struct_member(md, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    return read_values(value, ElementTypeKind) ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
  }

  if (get_from_struct_common_checks(md, id, enum_or_bitmask, true)) {
    const DDS::DynamicType_ptr member_type = md->type();
    if (member_type) {
      DDS::TypeDescriptor_var td;
      DDS::ReturnCode_t rc = get_base_type(member_type)->get_descriptor(td);
      if (rc != DDS::RETCODE_OK) {
        return rc;
      }
      DDS::TypeDescriptor_var etd;
      rc = get_base_type(td->element_type())->get_descriptor(etd);
      if (rc != DDS::RETCODE_OK) {
        return rc;
      }
      const LBound bit_bound = etd->bound()[0];
      if (bit_bound >= lower && bit_bound <= upper) {
        const DDS::ReturnCode_t rc = skip_to_struct_member(md, id);
        if (rc != DDS::RETCODE_OK) {
          return rc;
        }
        if (read_values(value, enum_or_bitmask)) {
          return DDS::RETCODE_OK;
        }
      }
    }
  }

  return DDS::RETCODE_ERROR;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataXcdrReadImpl::get_values_from_union(SequenceType& value, MemberId id,
                                                    TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (id == DISCRIMINATOR_ID) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::get_values_from_union:"
                 " Attempted to read discriminator as a sequence\n"));
    }
    return false;
  }

  if (exclude_union_member(id)) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::get_values_from_union:"
                 " Attempted to read an excluded member with Id %u\n", id));
    }
    return false;
  }

  DDS::MemberDescriptor_var md = get_from_union_common_checks(id, "get_values_from_union");
  if (!md) {
    return false;
  }

  const DDS::DynamicType_ptr member_type = md->type();
  if (!member_type) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_values_from_union -")
                 ACE_TEXT(" Could not get DynamicType of the selected member\n")));
    }
    return false;
  }

  DDS::DynamicType_var selected_type = get_base_type(member_type);
  const TypeKind selected_tk = selected_type->get_kind();
  if (selected_tk != TK_SEQUENCE) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_values_from_union -")
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_values_from_union -")
                 ACE_TEXT(" Could not read a sequence of %C from a sequence of %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_tk)));
    }
    return false;
  }

  if (type_desc_->extensibility_kind() == DDS::MUTABLE) {
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

  if (elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  const LBound bit_bound = td->bound()[0];
  return bit_bound >= lower && bit_bound <= upper && read_values(value, enum_or_bitmask);
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataXcdrReadImpl::get_values_from_sequence(SequenceType& value, MemberId id,
                                                       TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
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
    DDS::TypeDescriptor_var td;
    if (elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
      return false;
    }
    const DDS::DynamicType_var nested_elem_type = get_base_type(td->element_type());
    const TypeKind nested_elem_tk = nested_elem_type->get_kind();
    if (nested_elem_tk == ElementTypeKind) {
      // Read from a sequence of sequences of ElementTypeKind.
      return skip_to_sequence_element(id) && read_values(value, ElementTypeKind);
    } else if (nested_elem_tk == enum_or_bitmask) {
      // Read from a sequence of sequences of enums or bitmasks.
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_values_from_sequence -")
               ACE_TEXT(" Could not read a sequence of %C from an incompatible type\n"),
               typekind_to_string(ElementTypeKind)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataXcdrReadImpl::get_values_from_array(SequenceType& value, MemberId id,
                                                    TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  if (elem_type->get_kind() != TK_SEQUENCE) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_values_from_array -")
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_values_from_array -")
               ACE_TEXT(" Could not read a sequence of %C from an array of sequence of %C\n"),
               typekind_to_string(ElementTypeKind), typekind_to_string(nested_elem_tk)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
bool DynamicDataXcdrReadImpl::get_values_from_map(SequenceType& value, MemberId id,
                                                  TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  const DDS::DynamicType_var elem_type = get_base_type(type_desc_->element_type());
  if (elem_type->get_kind() != TK_SEQUENCE) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_values_from_map -")
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_values_from_map -")
               ACE_TEXT(" Could not read a sequence of %C from a map with element type sequence of %C\n"),
               typekind_to_string(ElementTypeKind), typekind_to_string(nested_elem_tk)));
  }
  return false;
}

template<TypeKind ElementTypeKind, typename SequenceType>
DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_sequence_values(SequenceType& value, MemberId id,
                                                               TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (!is_type_supported(ElementTypeKind, "get_sequence_values")) {
    return DDS::RETCODE_ERROR;
  }

  ScopedChainManager chain_manager(*this);

  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case TK_STRUCTURE:
    {
      const DDS::ReturnCode_t rc =
        get_values_from_struct<ElementTypeKind>(value, id, enum_or_bitmask, lower, upper);
      if (rc == DDS::RETCODE_NO_DATA) {
        return rc;
      }
      good = rc == DDS::RETCODE_OK;
    }
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_sequence_values -")
                 ACE_TEXT(" A sequence<%C> can't be read as a member of type %C"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(tk)));
    }
    return DDS::RETCODE_ERROR;
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_sequence_values -")
               ACE_TEXT(" Failed to read sequence<%C> from a DynamicData object of type %C\n"),
               typekind_to_string(ElementTypeKind), typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_int32_values(DDS::Int32Seq& value, MemberId id)
{
  return get_sequence_values<TK_INT32>(value, id, TK_ENUM, 17, 32);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_uint32_values(DDS::UInt32Seq& value, MemberId id)
{
  return get_sequence_values<TK_UINT32>(value, id, TK_BITMASK, 17, 32);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_int8_values(DDS::Int8Seq& value, MemberId id)
{
  return get_sequence_values<TK_INT8>(value, id, TK_ENUM, 1, 8);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_uint8_values(DDS::UInt8Seq& value, MemberId id)
{
  return get_sequence_values<TK_UINT8>(value, id, TK_BITMASK, 1, 8);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_int16_values(DDS::Int16Seq& value, MemberId id)
{
  return get_sequence_values<TK_INT16>(value, id, TK_ENUM, 9, 16);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_uint16_values(DDS::UInt16Seq& value, MemberId id)
{
  return get_sequence_values<TK_UINT16>(value, id, TK_BITMASK, 9, 16);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_int64_values(DDS::Int64Seq& value, MemberId id)
{
  return get_sequence_values<TK_INT64>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_uint64_values(DDS::UInt64Seq& value, MemberId id)
{
  return get_sequence_values<TK_UINT64>(value, id, TK_BITMASK, 33, 64);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_float32_values(DDS::Float32Seq& value, MemberId id)
{
  return get_sequence_values<TK_FLOAT32>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_float64_values(DDS::Float64Seq& value, MemberId id)
{
  return get_sequence_values<TK_FLOAT64>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_float128_values(DDS::Float128Seq& value, MemberId id)
{
  return get_sequence_values<TK_FLOAT128>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_char8_values(DDS::CharSeq& value, MemberId id)
{
  return get_sequence_values<TK_CHAR8>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_char16_values(DDS::WcharSeq& value, MemberId id)
{
#ifdef DDS_HAS_WCHAR
  return get_sequence_values<TK_CHAR16>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_byte_values(DDS::ByteSeq& value, MemberId id)
{
  return get_sequence_values<TK_BYTE>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_boolean_values(DDS::BooleanSeq& value, MemberId id)
{
  return get_sequence_values<TK_BOOLEAN>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_string_values(DDS::StringSeq& value, MemberId id)
{
  return get_sequence_values<TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::get_wstring_values(DDS::WstringSeq& value, MemberId id)
{
#ifdef DDS_HAS_WCHAR
  return get_sequence_values<TK_STRING16>(value, id);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DDS::DynamicType_ptr DynamicDataXcdrReadImpl::type()
{
  return DDS::DynamicType::_duplicate(type_);
}

bool DynamicDataXcdrReadImpl::check_xcdr1_mutable(DDS::DynamicType_ptr dt)
{
  DynamicTypeNameSet dtns;
  return check_xcdr1_mutable_i(dt, dtns);
}

CORBA::Boolean DynamicDataXcdrReadImpl::equals(DDS::DynamicData_ptr)
{
  // FUTURE: Implement this.
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataXcdrReadImpl::equals: Not implemented\n"));
  return false;
}

DDS::ReturnCode_t DynamicDataXcdrReadImpl::skip_to_struct_member(DDS::MemberDescriptor* member_desc, MemberId id)
{
  const DDS::ExtensibilityKind ek = type_desc_->extensibility_kind();
  if (ek == DDS::FINAL || ek == DDS::APPENDABLE) {
    size_t dheader = 0;
    const bool xcdr2_appendable = encoding_.xcdr_version() == DCPS::Encoding::XCDR_VERSION_2 &&
      ek == DDS::APPENDABLE;
    if (xcdr2_appendable && !strm_.read_delimiter(dheader)) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::skip_to_struct_member: "
                   "Failed to read DHEADER for member ID %d\n", id));
      }
      return DDS::RETCODE_ERROR;
    }
    const size_t end_of_struct = strm_.rpos() + dheader;

    for (ACE_CDR::ULong i = 0; i < member_desc->index(); ++i) {
      DDS::DynamicTypeMember_var dtm;
      DDS::ReturnCode_t rc = type_->get_member_by_index(dtm, i);
      if (rc != DDS::RETCODE_OK) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::skip_to_struct_member:"
                     " Failed to get DynamicTypeMember for member at index %d\n", i));
        }
        return rc;
      }
      DDS::MemberDescriptor_var md;
      rc = dtm->get_descriptor(md);
      if (rc != DDS::RETCODE_OK) {
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::skip_to_struct_member:"
                     " Failed to get MemberDescriptor for member at index %d\n", i));
        }
        return rc;
      }
      if (exclude_member(extent_, md->is_key(), has_explicit_keys(type_))) {
        // This member is not present in the sample, don't need to do anything.
        continue;
      }

      ACE_CDR::ULong num_skipped;
      if (!skip_struct_member_at_index(i, num_skipped)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_to_struct_member -")
                     ACE_TEXT(" Failed to skip member at index %d\n"), i));
        }
        return DDS::RETCODE_ERROR;
      }
      if (xcdr2_appendable && strm_.rpos() >= end_of_struct) {
        return DDS::RETCODE_NO_DATA;
      }
    }

    if (member_desc->is_optional()) {
      bool has_value = false;
      if (!(strm_ >> ACE_InputCDR::to_boolean(has_value))) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataXcdrReadImpl::skip_to_struct_member: "
                     "Failed to read is_present for member ID %d\n", id));
        }
        return DDS::RETCODE_ERROR;
      }
      if (!has_value) {
        return DDS::RETCODE_NO_DATA;
      }
    }

    return DDS::RETCODE_OK;
  } else {
    size_t dheader = 0;
    if (!strm_.read_delimiter(dheader)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_to_struct_member -")
                   ACE_TEXT(" Failed to read DHEADER for member ID %d\n"), id));
      }
      return DDS::RETCODE_ERROR;
    }

    const size_t end_of_struct = strm_.rpos() + dheader;
    while (true) {
      if (strm_.rpos() >= end_of_struct) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_to_struct_member -")
                     ACE_TEXT(" Could not find a member with ID %d\n"), id));
        }
        return DDS::RETCODE_NO_DATA;
      }

      ACE_CDR::ULong member_id;
      size_t member_size;
      bool must_understand;
      if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_to_struct_member -")
                     ACE_TEXT(" Failed to read EMHEADER while finding member ID %d\n"), id));
        }
        return DDS::RETCODE_ERROR;
      }

      if (member_id == id) {
        return DDS::RETCODE_OK;
      }

      if (!strm_.skip(member_size)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_to_struct_member -")
                     ACE_TEXT(" Failed to skip a member with ID %d\n"), member_id));
        }
        return DDS::RETCODE_ERROR;
      }
    }
  }
}

bool DynamicDataXcdrReadImpl::get_from_struct_common_checks(const DDS::MemberDescriptor_var& md,
  MemberId id, TypeKind kind, bool is_sequence)
{
  const DDS::DynamicType_ptr member_dt = md->type();
  if (!member_dt) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_from_struct_common_checks -")
                 ACE_TEXT(" Could not get DynamicType for member with ID %d\n"), id));
    }
    return false;
  }

  const DDS::DynamicType_var member_type = get_base_type(member_dt);
  const TypeKind member_kind = member_type->get_kind();

  if ((!is_sequence && member_kind != kind) || (is_sequence && member_kind != TK_SEQUENCE)) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_from_struct_common_checks -")
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
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_from_struct_common_checks -")
                   ACE_TEXT(" Could not get type descriptor for member %d\n"),
                   id));
      }
      return false;
    }
    const TypeKind elem_kind = get_base_type(member_td->element_type())->get_kind();
    if (elem_kind != kind) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::get_from_struct_common_checks -")
                   ACE_TEXT(" Member with ID %d is a sequence of %C, not %C\n"),
                   id, typekind_to_string(elem_kind), typekind_to_string(kind)));
      }
      return false;
    }
  }

  return true;
}

bool DynamicDataXcdrReadImpl::skip_struct_member_at_index(ACE_CDR::ULong index, ACE_CDR::ULong& num_skipped)
{
  DDS::DynamicTypeMember_var member;
  if (type_->get_member_by_index(member, index) != DDS::RETCODE_OK) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_struct_member_at_index -")
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

bool DynamicDataXcdrReadImpl::skip_member(DDS::DynamicType_ptr type)
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
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_member -")
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
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_member - Found a%C")
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
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_member -")
                   ACE_TEXT(" Found a member of kind %C\n"), typekind_to_string(member_kind)));
      }
      return false;
    }
  }

  return true;
}

bool DynamicDataXcdrReadImpl::skip_sequence_member(DDS::DynamicType_ptr seq_type)
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
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_sequence_member -")
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

bool DynamicDataXcdrReadImpl::skip_array_member(DDS::DynamicType_ptr array_type)
{
  DDS::TypeDescriptor_var descriptor;
  if (array_type->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::DynamicType_var elem_type = get_base_type(descriptor->element_type());

  ACE_CDR::ULong primitive_size = 0;
  if (get_primitive_size(elem_type, primitive_size)) {
    return skip("skip_array_member", "Failed to skip a primitive array member",
                bound_total(descriptor), primitive_size);
  } else {
    return skip_collection_member(array_type);
  }
}

bool DynamicDataXcdrReadImpl::skip_map_member(DDS::DynamicType_ptr map_type)
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
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_map_member -")
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

bool DynamicDataXcdrReadImpl::skip_collection_member(DDS::DynamicType_ptr coll_type)
{
  const TypeKind kind = coll_type->get_kind();
  if (kind == TK_SEQUENCE || kind == TK_ARRAY || kind == TK_MAP) {
    const char* kind_str = typekind_to_string(kind);
    if (strm_.encoding().kind() == DCPS::Encoding::KIND_XCDR2) {
      size_t dheader;
      if (!strm_.read_delimiter(dheader)) {
        if (DCPS::DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_collection_member -")
                     ACE_TEXT(" Failed to deserialize DHEADER of a non-primitive %C member\n"),
                     kind_str));
        }
        return false;
      }
      const DCPS::String err_msg = DCPS::String("Failed to skip a non-primitive ") + kind_str + " member";
      return skip("skip_collection_member", err_msg.c_str(), dheader);
    } else if (kind == TK_SEQUENCE) {
      return skip_to_sequence_element(0, coll_type);
    } else if (kind == TK_ARRAY) {
      return skip_to_array_element(0, coll_type);
    } else if (kind == TK_MAP) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::skip_collection_member: "
                   "DynamicData does not currently support XCDR1 maps\n"));
      }
      return false;
    }
  }

  return false;
}

bool DynamicDataXcdrReadImpl::skip_aggregated_member(DDS::DynamicType_ptr member_type)
{
  DynamicDataXcdrReadImpl nested_data(strm_, member_type);
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

void DynamicDataXcdrReadImpl::release_chains()
{
  for (ACE_CDR::ULong i = 0; i < chains_to_release.size(); ++i) {
    ACE_Message_Block::release(chains_to_release[i]);
  }
  chains_to_release.clear();
}

bool DynamicDataXcdrReadImpl::read_discriminator(const DDS::DynamicType_ptr disc_type, DDS::ExtensibilityKind union_ek, ACE_CDR::Long& label)
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::read_discriminator - Union has")
                 ACE_TEXT(" unsupported discriminator type (%C)\n"), typekind_to_string(disc_tk)));
    }
    return false;
  }
}

bool DynamicDataXcdrReadImpl::skip_all()
{
  const TypeKind tk = type_->get_kind();
  if (tk != TK_STRUCTURE && tk != TK_UNION) {
    return false;
  }

  const DDS::ExtensibilityKind extensibility = type_desc_->extensibility_kind();
  if (strm_.encoding().kind() == DCPS::Encoding::KIND_XCDR2 && (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE)) {
    size_t dheader;
    if (!strm_.read_delimiter(dheader)) {
      if (DCPS::DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_all - Failed to read")
                   ACE_TEXT(" DHEADER of the DynamicData object\n")));
      }
      return false;
    }
    return skip("skip_all", "Failed to skip the whole DynamicData object\n", dheader);
  } else {
    if (tk == TK_STRUCTURE) {
      const ACE_CDR::ULong member_count = type_->get_member_count();
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
      const DDS::DynamicType_var disc_type = get_base_type(type_desc_->discriminator_type());
      ACE_CDR::Long label;
      if (!read_discriminator(disc_type, extensibility, label)) {
        return false;
      }

      DDS::DynamicTypeMembersById_var members;
      if (type_->get_all_members(members) != DDS::RETCODE_OK) {
        return false;
      }
      DynamicTypeMembersByIdImpl* members_impl = dynamic_cast<DynamicTypeMembersByIdImpl*>(members.in());
      if (!members_impl) {
        return false;
      }

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
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::skip_all - Skip a union with no")
                   ACE_TEXT(" selected member and a discriminator with value %d\n"), label));
      }
      return true;
    }
  }
}

bool DynamicDataXcdrReadImpl::skip(const char* func_name, const char* description, size_t n, int size)
{
  if (!strm_.skip(n, size)) {
    if (DCPS::DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicDataXcdrReadImpl::%C - %C\n"), func_name, description));
    }
    return false;
  }
  return true;
}

bool DynamicDataXcdrReadImpl::get_primitive_size(DDS::DynamicType_ptr dt, ACE_CDR::ULong& size) const
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

bool DynamicDataXcdrReadImpl::check_xcdr1_mutable_i(DDS::DynamicType_ptr dt, DynamicTypeNameSet& dtns)
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
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::check_xcdr1_mutable: "
                 "XCDR1 mutable is not currently supported in OpenDDS\n"));
    }
    return false;
  }
  dtns.insert(descriptor->name());
  for (ACE_CDR::ULong i = 0; i < dt->get_member_count(); ++i) {
    DDS::DynamicTypeMember_var dtm;
    if (dt->get_member_by_index(dtm, i) != DDS::RETCODE_OK) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataXcdrReadImpl::check_xcdr1_mutable: "
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
                  MemberId member_id);

bool print_members(DDS::DynamicData_ptr dd, DCPS::String& type_string, DCPS::String& indent, bool print_name)
{
  DCPS::String temp_indent = indent;
  indent += "  ";
  const DDS::DynamicType_var type = dd->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  const char* const kind = base_type->get_kind() == TK_STRUCTURE ? "struct" : "union";
  if (print_name) {
    CORBA::String_var type_name = type->get_name();
    type_string += DCPS::String(kind) + " " + DCPS::String(type_name);
  }
  type_string += "\n";

  const ACE_CDR::ULong item_count = dd->get_item_count();
  for (ACE_CDR::ULong idx = 0; idx != item_count; ++idx) {
    // Translate the item number into a MemberId.
    const MemberId member_id = dd->get_member_id_at_index(idx);
    if (member_id == MEMBER_ID_INVALID) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_members: "
          "failed to get %C member at index %u\n", kind, idx));
      }
      return false;
    }

    if (!print_member(dd, type_string, indent, member_id)) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_members: "
          "failed to print %C member with id %u\n", kind, member_id));
      }
      return false;
    }
  }

  indent = temp_indent;
  return true;
}

bool print_member(DDS::DynamicData_ptr dd, DCPS::String& type_string, DCPS::String& indent,
                  MemberId member_id)
{
  DDS::DynamicType_ptr member_type;
  DDS::MemberDescriptor_var member_descriptor;
  const DDS::DynamicType_var container_type = dd->type();
  const DDS::TypeKind container_kind = container_type->get_kind();
  const bool sequence_like = is_sequence_like(container_kind);
  if (sequence_like) {
    DDS::TypeDescriptor_var td;
    if (container_type->get_descriptor(td) != DDS::RETCODE_OK) {
      return false;
    }
    member_type = td->element_type();

  } else {
    DDS::DynamicTypeMember_var dtm;
    if (container_type->get_member(dtm, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    if (dtm->get_descriptor(member_descriptor) != DDS::RETCODE_OK) {
      return false;
    }
    member_type = member_descriptor->type();
  }

  const DDS::DynamicType_var member_base_type = get_base_type(member_type);
  const DDS::TypeKind tk = member_base_type->get_kind();
  DDS::TypeDescriptor_var member_type_descriptor;
  if (member_base_type->get_descriptor(member_type_descriptor) != DDS::RETCODE_OK) {
    return false;
  }

  if (member_descriptor) {
    type_string += indent;
    const bool struct_or_union = tk == TK_STRUCTURE || tk == TK_UNION;
    if (struct_or_union) {
      type_string += (tk == TK_STRUCTURE ? "struct " : "union ");
    }
    const CORBA::String_var member_type_name = member_type->get_name();
    const DCPS::String member_name = member_descriptor->name();
    type_string += DCPS::String(member_type_name.in()) + " " + member_name;
    if (!struct_or_union) {
      type_string += " ";
    }
    if (tk == TK_ARRAY || tk == TK_SEQUENCE) {
      DDS::TypeDescriptor_var td;
      const DDS::DynamicType_var elem_type = get_base_type(member_type_descriptor->element_type());
      if (elem_type->get_descriptor(td) != DDS::RETCODE_OK) {
        return false;
      }
      DCPS::String ele_type_name = td->name();
      type_string += ele_type_name;
    }
  }

  if (!is_complex(tk)) {
    type_string += "= ";
  }

  switch (tk) {
  case TK_ENUM: {
    DDS::Int32 value;
    if (get_enum_value(value, member_base_type, dd, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::String8_var name;
    if (get_enumerator_name(name, value, member_base_type) == DDS::RETCODE_OK) {
       type_string += name.in();
    } else {
       type_string += DCPS::to_dds_string(value);
    }
    type_string += "\n";
    break;
  }
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64: {
    DDS::Int64 value;
    if (get_int_value(value, dd, member_id, tk) != DDS::RETCODE_OK) {
      return false;
    }
    type_string += DCPS::to_dds_string(value) + "\n";
    break;
  }
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64: {
    DDS::UInt64 value;
    if (get_uint_value(value, dd, member_id, tk) != DDS::RETCODE_OK) {
      return false;
    }
    type_string += DCPS::to_dds_string(value) + "\n";
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
    type_string += bool_string + "\n";
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
    type_string += "0x" + os.str() + "\n";
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
    type_string += "'" + os.str() + "'\n";
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
    type_string += "L'" + os.str() + "'\n";
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
    type_string += os.str() + "\n";
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
    type_string += os.str() + "\n";
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
    type_string += os.str() + "\n";
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
    type_string += DCPS::String("\"") + os.str() + "\"\n";
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
    type_string += "L\"" + os.str() + "\"\n";
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
  case TK_ARRAY:
  case TK_SEQUENCE: {
    DCPS::String temp_indent = indent;
    indent += "  ";
    DDS::DynamicData_var temp_dd;
    if (dd->get_complex_value(temp_dd, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    const ACE_CDR::ULong count = temp_dd->get_item_count();
    type_string += "[" + DCPS::to_dds_string(count) + "] =\n";
    for (ACE_CDR::ULong i = 0; i < count; ++i) {
      type_string += indent + "[" + DCPS::to_dds_string(i) + "] ";
      if (!print_member(temp_dd, type_string, indent, temp_dd->get_member_id_at_index(i))) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: print_member: "
            "failed to read array/sequence member\n"));
        }
        return false;
      }
    }
    indent = temp_indent;
    break;
  }
  case TK_UNION:
  case TK_STRUCTURE: {
    DDS::DynamicData_var temp_dd;
    if (dd->get_complex_value(temp_dd, member_id) != DDS::RETCODE_OK) {
      return false;
    }
    if (!print_members(temp_dd, type_string, indent, sequence_like)) {
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
  case TK_UNION:
    return print_members(dd, type_string, indent, true);
  }
  return false;
}
#endif

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
