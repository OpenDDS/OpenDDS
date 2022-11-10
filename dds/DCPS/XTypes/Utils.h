/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_UTILS_H
#define OPENDDS_DCPS_XTYPES_UTILS_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/Serializer.h>

#  include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

OpenDDS_Dcps_Export DDS::ReturnCode_t extensibility(
  DDS::DynamicType_ptr type, DCPS::Extensibility& ext);
OpenDDS_Dcps_Export DDS::ReturnCode_t max_extensibility(
  DDS::DynamicType_ptr type, DCPS::Extensibility& ext);
OpenDDS_Dcps_Export DCPS::Extensibility dds_to_opendds_ext(DDS::ExtensibilityKind ext);

/*
 * Provides access to a specific member using MemberIds that may be nested
 * inside a DynamicType or DynamicData.
 */
class OpenDDS_Dcps_Export MemberPath {
public:
  typedef OPENDDS_VECTOR(DDS::MemberId) MemberIdVec;
  MemberIdVec ids;

  MemberPath()
  {
  }

  MemberPath(const MemberPath& other, DDS::MemberId id)
  : ids(other.ids)
  {
    ids.push_back(id);
  }

  MemberPath& id(DDS::MemberId id)
  {
    ids.push_back(id);
    return *this;
  }

  size_t level() const { return ids.size(); }

  DDS::ReturnCode_t get_member_from_type(
    DDS::DynamicType_ptr type, DDS::DynamicTypeMember_var& member);

  /**
   * DynamicData itself holds the access methods to its members, so in order to
   * access them, we have to give both the containing Dynamic Data and the
   * member ID.
   */
  DDS::ReturnCode_t get_member_from_data(
    DDS::DynamicData_ptr data, DDS::DynamicData_var& container, DDS::MemberId& member_id);
};
typedef OPENDDS_VECTOR(MemberPath) MemberPathVec;

OpenDDS_Dcps_Export DDS::ReturnCode_t get_keys(DDS::DynamicType_ptr type, MemberPathVec& paths);
OpenDDS_Dcps_Export DDS::ReturnCode_t key_count(DDS::DynamicType_ptr type, size_t& count);

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
#endif
