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

namespace {

template <typename T>
bool write_enum(OpenDDS::DCPS::ValueWriter& vw, const DDS::DynamicType_var& enum_type,
                T enum_val, DDS::TypeKind as_int)
{
  const DDS::MemberId enumerator_id = static_cast<DDS::MemberId>(enum_val);
  DDS::DynamicTypeMember_var dtm;
  if (enum_type->get_member(dtm, enumerator_id) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::MemberDescriptor_var md;
  if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
    return false;
  }
  return vw.write_enum(md->name(), enum_val, as_int);
}

template <typename T>
bool write_bitmask(OpenDDS::DCPS::ValueWriter& vw, const DDS::DynamicType_var& bitmask_type, T bitmask_val)
{
  DDS::TypeDescriptor_var td;
  if (bitmask_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  return vw.write_bitmask(bitmask_val, td->bound()[0]);
}

}

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export
bool vwrite(ValueWriter& vw, DDS::DynamicData_ptr value);

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_VWRITE_H
