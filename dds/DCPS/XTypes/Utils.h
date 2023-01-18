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

enum Filter {
  Filter_All,
  Filter_Keys,
  Filter_NestedKeys,
  Filter_NonKeys
};

OpenDDS_Dcps_Export DDS::ReturnCode_t get_values(
  DDS::DynamicType_ptr type, MemberPathVec& paths, Filter filter);
OpenDDS_Dcps_Export DDS::ReturnCode_t get_keys(DDS::DynamicType_ptr type, MemberPathVec& paths);
OpenDDS_Dcps_Export DDS::ReturnCode_t key_count(DDS::DynamicType_ptr type, size_t& count);

OpenDDS_Dcps_Export DDS::ReturnCode_t less_than(
  bool& result, DDS::DynamicData_ptr a, DDS::DynamicData_ptr b, Filter filter);
OpenDDS_Dcps_Export DDS::ReturnCode_t key_less_than(
  bool& result, DDS::DynamicData_ptr a, DDS::DynamicData_ptr b);

OpenDDS_Dcps_Export DDS::ReturnCode_t get_member_type(
  DDS::DynamicType_var& type, DDS::DynamicData_ptr data, DDS::MemberId id);

OpenDDS_Dcps_Export bool is_int(DDS::TypeKind tk);
OpenDDS_Dcps_Export bool is_uint(DDS::TypeKind tk);

OpenDDS_Dcps_Export DDS::ReturnCode_t get_uint_value(
  CORBA::UInt64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind);
OpenDDS_Dcps_Export DDS::ReturnCode_t set_uint_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, DDS::TypeKind kind, CORBA::UInt64 value);
OpenDDS_Dcps_Export DDS::ReturnCode_t get_int_value(
  CORBA::Int64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind);
OpenDDS_Dcps_Export DDS::ReturnCode_t set_int_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, DDS::TypeKind kind, CORBA::Int64 value);

OpenDDS_Dcps_Export DDS::ReturnCode_t bitmask_bound(
  DDS::DynamicType_ptr type, CORBA::UInt64& bound_max, DDS::TypeKind& bound_kind);
OpenDDS_Dcps_Export DDS::ReturnCode_t get_bitmask_value(
  CORBA::UInt64& value, DDS::DynamicType_ptr type, DDS::DynamicData_ptr src, DDS::MemberId id);

OpenDDS_Dcps_Export DDS::ReturnCode_t enum_bound(DDS::DynamicType_ptr type, DDS::TypeKind& bound_kind);

OpenDDS_Dcps_Export DDS::ReturnCode_t get_enum_value(
  CORBA::Int32& value, DDS::DynamicType_ptr type, DDS::DynamicData_ptr src, DDS::MemberId id);
inline DDS::ReturnCode_t get_enum_value(
  CORBA::Int32& value, DDS::DynamicData_ptr src, DDS::MemberId id)
{
  DDS::DynamicType_var type;
  const DDS::ReturnCode_t rc = get_member_type(type, src, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return get_enum_value(value, type, src, id);
}

OpenDDS_Dcps_Export DDS::ReturnCode_t set_enum_value(
  DDS::DynamicType_ptr type, DDS::DynamicData_ptr dest, DDS::MemberId id, CORBA::Int32 value);
inline DDS::ReturnCode_t set_enum_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, CORBA::Int32 value)
{
  DDS::DynamicType_var type;
  const DDS::ReturnCode_t rc = get_member_type(type, dest, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return set_enum_value(type, dest, id, value);
}

OpenDDS_Dcps_Export DDS::ReturnCode_t set_enum_value(
  DDS::DynamicType_ptr type, DDS::DynamicData_ptr dest, DDS::MemberId id, const char* enumeral_name);
inline DDS::ReturnCode_t set_enum_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, const char* enumeral_name)
{
  DDS::DynamicType_var type;
  const DDS::ReturnCode_t rc = get_member_type(type, dest, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return set_enum_value(type, dest, id, enumeral_name);
}

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
#endif
