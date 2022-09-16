#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_IMPL_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "TypeDescriptorImpl.h"
#include "MemberDescriptorImpl.h"

#include <dds/DCPS/RcHandle_T.h>
#include <dds/DCPS/RcObject.h>
#include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

// This will eventually be replaced by a map.
class DynamicTypeMembersByNameImpl : public DDS::DynamicTypeMembersByName  {
private:
  typedef OPENDDS_MAP(OpenDDS::DCPS::String, DDS::DynamicTypeMember_var) MapType;
public:
  typedef MapType::const_iterator const_iterator;

  const_iterator find(const OpenDDS::DCPS::String& key) const
  {
    return map_.find(key);
  }

  const_iterator begin() const
  {
    return map_.begin();
  }

  const_iterator end() const
  {
    return map_.end();
  }

  void insert(const MapType::value_type& value)
  {
    map_.insert(value);
  }

  size_t size() const
  {
    return map_.size();
  }

  void clear()
  {
    map_.clear();
  }

private:
  MapType map_;
};

// This will eventually be replaced by a map.
class DynamicTypeMembersByIdImpl : public DDS::DynamicTypeMembersById {
private:
  typedef OPENDDS_MAP(MemberId, DDS::DynamicTypeMember_var) MapType;
public:
  typedef MapType::const_iterator const_iterator;

  const_iterator find(MemberId key) const
  {
    return map_.find(key);
  }

  const_iterator begin() const
  {
    return map_.begin();
  }

  const_iterator end() const
  {
    return map_.end();
  }

  void insert(const MapType::value_type& value)
  {
    map_.insert(value);
  }

  size_t size() const
  {
    return map_.size();
  }

  void clear()
  {
    map_.clear();
  }

private:
  MapType map_;
};

class OpenDDS_Dcps_Export DynamicTypeImpl : public DDS::DynamicType
{
public:
  DynamicTypeImpl();
  ~DynamicTypeImpl();

  void set_descriptor(DDS::TypeDescriptor* descriptor);
  DDS::ReturnCode_t get_descriptor(DDS::TypeDescriptor*& descriptor);
  char* get_name();
  TypeKind get_kind();
  DDS::ReturnCode_t get_member_by_name(DDS::DynamicTypeMember_ptr& member, const char* name);
  DDS::ReturnCode_t get_all_members_by_name(DDS::DynamicTypeMembersByName_ptr& member);
  DDS::ReturnCode_t get_member(DDS::DynamicTypeMember_ptr& member, MemberId id);
  DDS::ReturnCode_t get_all_members(DDS::DynamicTypeMembersById_ptr& member);
  ACE_CDR::ULong get_member_count();
  DDS::ReturnCode_t get_member_by_index(DDS::DynamicTypeMember_ptr& member, ACE_CDR::ULong index);
  CORBA::ULong get_annotation_count();
  DDS::ReturnCode_t get_annotation(DDS::AnnotationDescriptor*& descriptor, CORBA::ULong idx);
  CORBA::ULong get_verbatim_text_count();
  DDS::ReturnCode_t get_verbatim_text(DDS::VerbatimTextDescriptor*& descriptor, CORBA::ULong idx);
  bool equals(DDS::DynamicType_ptr other);
  void insert_dynamic_member(DDS::DynamicTypeMember_ptr dtm);
  void clear();
 private:
  DynamicTypeMembersByNameImpl member_by_name_;
  DynamicTypeMembersByIdImpl member_by_id_;
  typedef OPENDDS_VECTOR(DDS::DynamicTypeMember_var) DynamicTypeMembersByIndex;
  DynamicTypeMembersByIndex member_by_index_;
  DDS::TypeDescriptor_var descriptor_;
};

OpenDDS_Dcps_Export DDS::DynamicType_var get_base_type(DDS::DynamicType_ptr type);
bool test_equality(DDS::DynamicType_ptr lhs, DDS::DynamicType_ptr rhs, DynamicTypePtrPairSeen& dt_ptr_pair);
bool test_equality(DynamicTypeMembersByNameImpl* lhs, DynamicTypeMembersByNameImpl* rhs, DynamicTypePtrPairSeen& dt_ptr_pair);
bool test_equality(DynamicTypeMembersByIdImpl* lhs, DynamicTypeMembersByIdImpl* rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif  /* OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_IMPL_H */
