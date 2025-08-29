/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_UTILS_H
#define OPENDDS_DCPS_XTYPES_UTILS_H

#include <dds/DCPS/Definitions.h>

#if !OPENDDS_CONFIG_SAFETY_PROFILE
#  include <dds/DCPS/Serializer.h>
#  include <dds/DCPS/Sample.h>
#  include <dds/DCPS/DCPS_Utils.h>
#  include <dds/DCPS/debug.h>

#  include <dds/DdsDynamicDataC.h>

#  include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class TypeLookupService;

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

  MemberPathParser(const char* a_path)
    : pos(0)
    , left(a_path ? std::strlen(a_path) : 0)
    , path(left > 0 ? a_path : 0)
    , error(false)
    , in_subscript(false)
  {
  }

  MemberPathParser(const DCPS::String& a_path)
    : pos(0)
    , left(a_path.size())
    , path(left > 0 ? a_path.c_str() : 0)
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

  bool get_index(DDS::UInt32 &index);
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
OpenDDS_Dcps_Export bool is_key(DDS::DynamicType_ptr type, const char* field);
OpenDDS_Dcps_Export DDS::ReturnCode_t key_count(DDS::DynamicType_ptr type, size_t& count);

OpenDDS_Dcps_Export DDS::ReturnCode_t less_than(
  bool& result, DDS::DynamicData_ptr a, DDS::DynamicData_ptr b, Filter filter);
OpenDDS_Dcps_Export DDS::ReturnCode_t key_less_than(
  bool& result, DDS::DynamicData_ptr a, DDS::DynamicData_ptr b);
OpenDDS_Dcps_Export DDS::ReturnCode_t compare_members(
  int& result, DDS::DynamicData_ptr a, DDS::DynamicData_ptr b, DDS::MemberId id);

OpenDDS_Dcps_Export DDS::ReturnCode_t get_member_type(
  DDS::DynamicType_var& member_type, DDS::DynamicType_ptr container_type, DDS::MemberId id);

inline DDS::ReturnCode_t get_member_type(
  DDS::DynamicType_var& member_type, DDS::DynamicData_ptr container, DDS::MemberId id)
{
  DDS::DynamicType_var container_type = container->type();
  return get_member_type(member_type, container_type, id);
}

OpenDDS_Dcps_Export DDS::ReturnCode_t get_uint_value(
  DDS::UInt64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind);
OpenDDS_Dcps_Export DDS::ReturnCode_t set_uint_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, DDS::TypeKind kind, DDS::UInt64 value);
OpenDDS_Dcps_Export DDS::ReturnCode_t get_int_value(
  DDS::Int64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind);
OpenDDS_Dcps_Export DDS::ReturnCode_t set_int_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, DDS::TypeKind kind, DDS::Int64 value);

OpenDDS_Dcps_Export DDS::UInt32 bound_total(DDS::TypeDescriptor_var descriptor);

OpenDDS_Dcps_Export DDS::ReturnCode_t bitmask_bound(
  DDS::DynamicType_ptr type, DDS::TypeKind& bound_kind);
OpenDDS_Dcps_Export DDS::ReturnCode_t get_bitmask_value(
  DDS::UInt64& value, DDS::DynamicType_ptr type, DDS::DynamicData_ptr src, DDS::MemberId id);

OpenDDS_Dcps_Export DDS::ReturnCode_t enum_bound(DDS::DynamicType_ptr enum_type, DDS::TypeKind& bound_kind);

OpenDDS_Dcps_Export DDS::ReturnCode_t get_enum_value(
  DDS::Int32& value, DDS::DynamicType_ptr enum_type, DDS::DynamicData_ptr src, DDS::MemberId id);
inline DDS::ReturnCode_t get_enum_value(
  DDS::Int32& value, DDS::DynamicData_ptr src, DDS::MemberId id)
{
  DDS::DynamicType_var enum_type;
  const DDS::ReturnCode_t rc = get_member_type(enum_type, src, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return get_enum_value(value, enum_type, src, id);
}

OpenDDS_Dcps_Export DDS::ReturnCode_t set_enum_value(
  DDS::DynamicType_ptr enum_type, DDS::DynamicData_ptr dest, DDS::MemberId id, DDS::Int32 value);
inline DDS::ReturnCode_t set_enum_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, DDS::Int32 value)
{
  DDS::DynamicType_var enum_type;
  const DDS::ReturnCode_t rc = get_member_type(enum_type, dest, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return set_enum_value(enum_type, dest, id, value);
}

OpenDDS_Dcps_Export DDS::ReturnCode_t set_enum_value(
  DDS::DynamicType_ptr enum_type, DDS::DynamicData_ptr dest, DDS::MemberId id,
  const char* enumeral_name);
inline DDS::ReturnCode_t set_enum_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, const char* enumeral_name)
{
  DDS::DynamicType_var enum_type;
  const DDS::ReturnCode_t rc = get_member_type(enum_type, dest, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return set_enum_value(enum_type, dest, id, enumeral_name);
}

OpenDDS_Dcps_Export DDS::ReturnCode_t get_enumerator_name(
  DDS::String8_var& name, DDS::Int32 value, DDS::DynamicType_ptr type);
OpenDDS_Dcps_Export DDS::ReturnCode_t get_enumerator_value(
  DDS::Int32& value, const char* name, DDS::DynamicType_ptr type);

OpenDDS_Dcps_Export DDS::ReturnCode_t copy_member(
  DDS::DynamicData_ptr dest, DDS::MemberId dest_id,
  DDS::DynamicData_ptr src, DDS::MemberId src_id);
OpenDDS_Dcps_Export DDS::ReturnCode_t copy(DDS::DynamicData_ptr dest, DDS::DynamicData_ptr src);

DDS::ReturnCode_t get_selected_union_branch(
  DDS::DynamicType_var union_type, DDS::Int32 disc, bool& found_selected_member,
  DDS::MemberDescriptor_var& selected_md);
bool has_explicit_keys(DDS::DynamicType* dt);

inline bool exclude_member(DCPS::Sample::Extent ext, bool is_key, bool has_explicit_keys)
{
  // see Fields::Iterator and explicit_keys_only() in opendds_idl's dds_generator.h
  const bool explicit_keys_only = ext == DCPS::Sample::KeyOnly || (ext == DCPS::Sample::NestedKeyOnly && has_explicit_keys);
  return explicit_keys_only && !is_key;
}

inline DCPS::Sample::Extent nested(DCPS::Sample::Extent ext)
{
  return ext == DCPS::Sample::KeyOnly ? DCPS::Sample::NestedKeyOnly : ext;
}

// Convert the index vector to an element in a multi-dimensional array into a flat index.
// See description for ARRAY_TYPE in XTypes 1.3, page 139.
OpenDDS_Dcps_Export DDS::ReturnCode_t flat_index(CORBA::ULong& flat_idx, const DDS::BoundSeq& idx_vec,
                                                 const DDS::BoundSeq& dims);

inline bool check_rc_from_get(DDS::ReturnCode_t rc, DDS::MemberId id, DDS::TypeKind tk,
                              const char* fn_name, DCPS::LogLevel::Value log_thres = DCPS::LogLevel::Notice)
{
  if (rc != DDS::RETCODE_OK && rc != DDS::RETCODE_NO_DATA) {
    if (DCPS::log_level >= log_thres) {
      ACE_ERROR((DCPS::LogLevel::to_priority(log_thres), "(%P|%t) %C: %C: Failed to get %C member ID %u: %C\n",
                 DCPS::LogLevel::to_string(log_thres), fn_name,
                 XTypes::typekind_to_string(tk), id, DCPS::retcode_to_string(rc)));
    }
    return false;
  }
  return true;
}

/**
 * Walk the TypeObject graph starting at top_level looking for all TypeIdentifiers
 * that match enum_type and replacing each of those with a new TypeIdentifier that
 * represents the orignal enum_type with certain enumerators (represented by the
 * values sequence) removed.  All newly-generated TypeObjects are inserted into
 * the type_map with their respective TypeIdentifiers.
 * The returned TypeIdentifier represents the modified top_level.
 */
OpenDDS_Dcps_Export
TypeIdentifier remove_enumerators(const TypeIdentifier& top_level,
                                  const TypeIdentifier& enum_type,
                                  const Sequence<DDS::Int32>& values,
                                  const TypeLookupService& lookup,
                                  TypeMap& type_map,
                                  TypeIdentifier* modified_enum = 0);

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
#endif
