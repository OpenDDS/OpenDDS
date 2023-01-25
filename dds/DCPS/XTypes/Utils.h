/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_UTILS_H
#define OPENDDS_DCPS_XTYPES_UTILS_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/Serializer.h>

#  include <dds/DdsDynamicDataC.h>

#  include <cstring>

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

/**
 * Iterate over parts of a member name path that can be the name of a
 * field inside a function or an array element nested multiple levels inside a
 * type.
 *
 * Examples:
 *   "member_a"
 *   "member_b[2]"
 *   "member_b[2].x"
 *   "[2]" (If the DynamicData is indexed or keyed)
 */
class OpenDDS_Dcps_Export MemberPathParser {
public:
  size_t pos;
  size_t left;
  const char* path;
  bool error;
  bool in_subscript;
  DCPS::String subpath;

  MemberPathParser(const char* path)
    : pos(0)
    , left(path ? std::strlen(path) : 0)
    , path(left > 0 ? path : 0)
    , error(false)
    , in_subscript(false)
  {
  }

  MemberPathParser(const DCPS::String& path)
    : pos(0)
    , left(path.size())
    , path(left > 0 ? path.c_str() : 0)
    , error(false)
    , in_subscript(false)
  {
  }

  bool consume(size_t by);

  /**
   * Put the next member name or subscript value in subpath and which kind in
   * in_subscript. Returns false when the end of the path is reached or there
   * was an error in which case error is set to true.
   */
  bool get_next_subpath();

  bool get_index(CORBA::UInt32 &index);
};

/**
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

  DDS::ReturnCode_t resolve_string_path(
    DDS::DynamicType_ptr type, const DCPS::String &path);

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
