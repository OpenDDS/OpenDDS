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

inline bool dynamic_type_is_valid(DDS::DynamicType_ptr type)
{
  DDS::TypeDescriptor_var td;
  return type && type->get_descriptor(td) == DDS::RETCODE_OK && td;
}

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

class OpenDDS_Dcps_Export MemberNamePath {
public:
  typedef OPENDDS_VECTOR(DCPS::String) MemberNameVec;
  MemberNameVec member_names;

  MemberNamePath()
  {
  }

  MemberNamePath(const char* member_name)
  {
    add(member_name);
  }

  MemberNamePath& add(const DCPS::String& member_name)
  {
    member_names.push_back(member_name);
    return *this;
  }

  DCPS::String join() const
  {
    DCPS::String result;
    for (MemberNameVec::const_iterator it = member_names.begin(); it != member_names.end(); ++it) {
      if (it != member_names.begin()) {
        result += ".";
      }
      result += *it;
    }
    return result;
  }

  DDS::ReturnCode_t resolve(DDS::DynamicType_ptr type, MemberPath& member_path) const;

  DDS::ReturnCode_t get_member_from_type(
    DDS::DynamicType_ptr type, DDS::DynamicTypeMember_var& member) const
  {
    MemberPath member_path;
    const DDS::ReturnCode_t rc = resolve(type, member_path);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    return member_path.get_member_from_type(type, member);
  }

  DDS::ReturnCode_t get_member_from_data(
    DDS::DynamicData_ptr data, DDS::DynamicData_var& container, DDS::MemberId& member_id) const
  {
    DDS::DynamicType_var type = data->type();
    MemberPath member_path;
    const DDS::ReturnCode_t rc = resolve(type, member_path);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    return member_path.get_member_from_data(data, container, member_id);
  }
};

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

OpenDDS_Dcps_Export bool is_int(DDS::TypeKind tk);
OpenDDS_Dcps_Export bool is_uint(DDS::TypeKind tk);

OpenDDS_Dcps_Export DDS::ReturnCode_t get_uint_value(
  CORBA::UInt64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind);
OpenDDS_Dcps_Export DDS::ReturnCode_t get_int_value(
  CORBA::Int64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind);

OpenDDS_Dcps_Export DDS::ReturnCode_t bitmask_bound(
  DDS::DynamicType_ptr type, CORBA::UInt64& bound_max, DDS::TypeKind& bound_kind);
OpenDDS_Dcps_Export DDS::ReturnCode_t get_bitmask_value(
  CORBA::UInt64& value, DDS::DynamicType_ptr type, DDS::DynamicData_ptr src, DDS::MemberId id);

OpenDDS_Dcps_Export DDS::ReturnCode_t enum_bound(DDS::DynamicType_ptr type, DDS::TypeKind& bound_kind);
OpenDDS_Dcps_Export DDS::ReturnCode_t get_enum_value(
  CORBA::Int32& value, DDS::DynamicType_ptr type, DDS::DynamicData_ptr src, DDS::MemberId id);

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
#endif
