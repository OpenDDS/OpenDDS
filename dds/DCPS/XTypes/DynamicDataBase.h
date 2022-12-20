/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_BASE_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_BASE_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicTypeImpl.h"

#  include <dds/DCPS/debug.h>
#  include <dds/DCPS/LocalObject.h>
#  include <dds/DCPS/Sample.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class DynamicDataBase : public virtual DCPS::LocalObject<DDS::DynamicData> {
public:
  DynamicDataBase();
  DynamicDataBase(DDS::DynamicType_ptr type);

  DDS::ReturnCode_t get_descriptor(DDS::MemberDescriptor*& value, MemberId id);
  DDS::MemberId get_member_id_by_name(const char* name);

  static bool has_explicit_keys(DDS::DynamicType* dt);
  static bool exclude_member(DCPS::Sample::Extent ext, bool is_key, bool has_explicit_keys);
  static DCPS::Sample::Extent nested(DCPS::Sample::Extent ext);

protected:
  /// Verify that a given type is primitive or string or wstring.
  ///
  bool is_type_supported(TypeKind tk, const char* func_name);
  bool is_primitive(TypeKind tk) const;
  bool is_basic(TypeKind tk) const;
  bool is_complex(TypeKind tk) const;
  bool get_index_from_id(DDS::MemberId id, ACE_CDR::ULong& index, ACE_CDR::ULong bound) const;

  bool check_member(DDS::MemberDescriptor_var& md, DDS::DynamicType_var& type,
    const char* method, const char* what, DDS::MemberId id, DDS::TypeKind tk = TK_NONE);

  static CORBA::ULong bound_total(DDS::TypeDescriptor_var descriptor);
  static DDS::MemberId get_union_default_member(DDS::DynamicType* type);

  /// The actual (i.e., non-alias) DynamicType of the associated type.
  DDS::DynamicType_var type_;
};

inline bool DynamicDataBase::exclude_member(DCPS::Sample::Extent ext, bool is_key, bool has_explicit_keys)
{
  // see Fields::Iterator and explicit_keys_only() in opendds_idl's dds_generator.h
  const bool explicit_keys_only = ext == DCPS::Sample::KeyOnly || (ext == DCPS::Sample::NestedKeyOnly && has_explicit_keys);
  return explicit_keys_only && !is_key;
}

inline DCPS::Sample::Extent DynamicDataBase::nested(DCPS::Sample::Extent ext)
{
  return ext == DCPS::Sample::KeyOnly ? DCPS::Sample::NestedKeyOnly : ext;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_BASE_H
