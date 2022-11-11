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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class DynamicDataBase : public virtual DCPS::LocalObject<DDS::DynamicData> {
public:
  DynamicDataBase();
  DynamicDataBase(DDS::DynamicType_ptr type);

  DDS::ReturnCode_t get_descriptor(DDS::MemberDescriptor*& value, MemberId id);
  DDS::MemberId get_member_id_by_name(const char* name);

protected:
  /// Verify that a given type is primitive or string or wstring.
  ///
  bool is_type_supported(TypeKind tk, const char* func_name);
  bool is_primitive(TypeKind tk) const;
  bool get_index_from_id(DDS::MemberId id, ACE_CDR::ULong& index, ACE_CDR::ULong bound) const;

  /// The actual (i.e., non-alias) DynamicType of the associated type.
  DDS::DynamicType_var type_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_BASE_H
