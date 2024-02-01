/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_VWRITE_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_VWRITE_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/ValueWriter.h>
#  include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
void write_enum(ValueWriter& vw, const DDS::DynamicType_var& enum_type,
                T enum_val, DDS::TypeKind as_int)
{
  const DDS::MemberId enumerator_id = static_cast<DDS::MemberId>(enum_val);
  DDS::DynamicTypeMember_var dtm;
  if (enum_type->get_member(dtm, enumerator_id) != DDS::RETCODE_OK) {
    return;
  }
  DDS::MemberDescriptor_var md;
  if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
    return;
  }
  vw.write_enum(md->name(), enum_val, as_int);
}

OpenDDS_Dcps_Export
void vwrite(ValueWriter& vw, DDS::DynamicData_ptr value);

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_VWRITE_H
