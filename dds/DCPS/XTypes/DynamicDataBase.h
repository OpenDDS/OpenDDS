/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_BASE_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_BASE_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicTypeImpl.h"

#  include <dds/DCPS/LocalObject.h>
#  include <dds/DCPS/Sample.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace DCPS {
  struct Value;
}

namespace XTypes {

class OpenDDS_Dcps_Export DynamicDataBase : public virtual DCPS::LocalObject<DDS::DynamicData> {
public:
  DynamicDataBase();
  DynamicDataBase(DDS::DynamicType_ptr type);

  DDS::ReturnCode_t get_descriptor(DDS::MemberDescriptor*& value, MemberId id);
  DDS::ReturnCode_t set_descriptor(DDS::MemberId id, DDS::MemberDescriptor* value);
  DDS::MemberId get_member_id_by_name(const char* name);
  DDS::DynamicType_ptr type();
  DDS::Boolean equals(DDS::DynamicData_ptr other);
  DDS::DynamicData_ptr loan_value(DDS::MemberId id);
  DDS::ReturnCode_t return_loaned_value(DDS::DynamicData_ptr other);
  DDS::DynamicData_ptr clone();

  DDS::ReturnCode_t get_int64_value(DDS::Int64& value, DDS::MemberId id);
  virtual DDS::ReturnCode_t get_int64_value_impl(DDS::Int64& value, DDS::MemberId id) = 0;
  DDS::ReturnCode_t get_uint64_value(DDS::UInt64& value, DDS::MemberId id);
  virtual DDS::ReturnCode_t get_uint64_value_impl(DDS::UInt64& value, DDS::MemberId id) = 0;

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual DDS::ReturnCode_t get_simple_value(DCPS::Value& value, DDS::MemberId id);
#endif

  virtual bool serialized_size(const DCPS::Encoding& enc, size_t& size, DCPS::Sample::Extent extent) const = 0;
  virtual bool serialize(DCPS::Serializer& ser, DCPS::Sample::Extent extent) const = 0;

protected:
  /// Verify that a given type is primitive or string or wstring.
  bool is_type_supported(TypeKind tk, const char* func_name);

  bool get_index_from_id(DDS::MemberId id, ACE_CDR::ULong& index, ACE_CDR::ULong bound) const;
  bool enum_string_helper(char*& strInOut, MemberId id);

  DDS::ReturnCode_t check_member(
    DDS::MemberDescriptor_var& member_desc, DDS::DynamicType_var& member_type,
    const char* method, const char* action, DDS::MemberId id, DDS::TypeKind tk = TK_NONE);

  DDS::ReturnCode_t check_member(
    DDS::DynamicType_var& member_type, const char* method, const char* action,
    DDS::MemberId id, DDS::TypeKind tk = TK_NONE)
  {
    DDS::MemberDescriptor_var md;
    return check_member(md, member_type, method, action, id, tk);
  }

  static DDS::MemberId get_union_default_member(DDS::DynamicType* type);
  DDS::ReturnCode_t get_selected_union_branch(
    bool& found_selected_member, DDS::MemberDescriptor_var& selected_md);
  bool discriminator_selects_no_member(DDS::Int32 disc) const;

  /// Similar idea to std::shared_from_this(), provide a type compatible with parameter
  /// passing rules for IDL interfaces that are arguments to operations.
  /// Doesn't change the reference count.
  DDS::DynamicData* interface_from_this() const;

  DDS::ReturnCode_t unsupported_method(const char* method_name, bool warning = false) const;

  /// The actual (i.e., non-alias) DynamicType of the associated type.
  DDS::DynamicType_var type_;
  DDS::TypeDescriptor_var type_desc_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_BASE_H
