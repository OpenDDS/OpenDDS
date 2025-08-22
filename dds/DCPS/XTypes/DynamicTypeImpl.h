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

class OpenDDS_Dcps_Export DynamicTypeImpl : public DDS::DynamicType {
public:
  DynamicTypeImpl();
  ~DynamicTypeImpl();

  void set_descriptor(DDS::TypeDescriptor* descriptor);
  DDS::ReturnCode_t get_descriptor(DDS::TypeDescriptor*& descriptor);
  char* get_name();
  TypeKind get_kind();
  DDS::ReturnCode_t get_member_by_name(DDS::DynamicTypeMember_ptr& member, const char* name);

#if OPENDDS_CONFIG_IDL_MAP
  DDS::ReturnCode_t get_all_members_by_name(DDS::DynamicTypeMembersByName& members);
  DDS::ReturnCode_t get_all_members(DDS::DynamicTypeMembersById& members);
#else
  DDS::ReturnCode_t get_all_members_by_name(DDS::DynamicTypeMembersByName*& members);
  DDS::ReturnCode_t get_all_members(DDS::DynamicTypeMembersById*& members);
#endif

  DDS::ReturnCode_t get_member(DDS::DynamicTypeMember_ptr& member, MemberId id);
  ACE_CDR::ULong get_member_count();
  DDS::ReturnCode_t get_member_by_index(DDS::DynamicTypeMember_ptr& member, ACE_CDR::ULong index);
  CORBA::ULong get_annotation_count();
  DDS::ReturnCode_t get_annotation(DDS::AnnotationDescriptor*& descriptor, CORBA::ULong idx);
  CORBA::ULong get_verbatim_text_count();
  DDS::ReturnCode_t get_verbatim_text(DDS::VerbatimTextDescriptor*& descriptor, CORBA::ULong idx);
  bool equals(DDS::DynamicType_ptr other);
  void insert_dynamic_member(DDS::DynamicTypeMember_ptr dtm);
  void clear();

  void set_minimal_type_identifier(const TypeIdentifier& ti)
  {
    minimal_ti_ = ti;
  }

  const TypeIdentifier& get_minimal_type_identifier() const
  {
    return minimal_ti_;
  }

  void set_minimal_type_map(const TypeMap& tm)
  {
    minimal_tm_ = tm;
  }

  const TypeMap& get_minimal_type_map() const
  {
    return minimal_tm_;
  }

  void set_complete_type_identifier(const TypeIdentifier& ti)
  {
    complete_ti_ = ti;
  }

  const TypeIdentifier& get_complete_type_identifier() const
  {
    return complete_ti_;
  }

  void set_complete_type_map(const TypeMap& tm)
  {
    complete_tm_ = tm;
  }

  const TypeMap& get_complete_type_map() const
  {
    return complete_tm_;
  }

  void set_preset_type_info(const TypeInformation& type_info)
  {
    preset_type_info_ = type_info;
    preset_type_info_set_ = true;
  }

  const TypeInformation* get_preset_type_info() const
  {
    return preset_type_info_set_ ? &preset_type_info_ : 0;
  }

#if OPENDDS_CONFIG_IDL_MAP
  typedef DDS::DynamicTypeMembersByName DynamicTypeMembersByName;
  typedef DDS::DynamicTypeMembersById DynamicTypeMembersById;
#else
  typedef OPENDDS_MAP(DCPS::String, DDS::DynamicTypeMember_var) DynamicTypeMembersByName;
  typedef OPENDDS_MAP(DDS::MemberId, DDS::DynamicTypeMember_var) DynamicTypeMembersById;
#endif

private:
  DynamicTypeMembersByName members_by_name_;
  DynamicTypeMembersById members_by_id_;

  typedef OPENDDS_VECTOR(DDS::DynamicTypeMember_var) DynamicTypeMembersByIndex;
  DynamicTypeMembersByIndex members_by_index_;

  DDS::TypeDescriptor_var descriptor_;
  TypeIdentifier minimal_ti_;
  TypeMap minimal_tm_;
  TypeIdentifier complete_ti_;
  TypeMap complete_tm_;
  bool preset_type_info_set_;
  TypeInformation preset_type_info_;

  friend bool test_equality(DDS::DynamicType_ptr lhs, DDS::DynamicType_ptr rhs, DynamicTypePtrPairSeen& dt_ptr_pair);
  static bool test_equality_i(DynamicTypeMembersByName* lhs, DynamicTypeMembersByName* rhs, DynamicTypePtrPairSeen& dt_ptr_pair);
  static bool test_equality_i(DynamicTypeMembersById* lhs, DynamicTypeMembersById* rhs, DynamicTypePtrPairSeen& dt_ptr_pair);
};

bool test_equality(DDS::DynamicType_ptr lhs, DDS::DynamicType_ptr rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

OpenDDS_Dcps_Export DDS::DynamicType_var get_base_type(DDS::DynamicType_ptr type);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif  /* OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_IMPL_H */
