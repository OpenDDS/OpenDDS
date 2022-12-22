#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_IMPL_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "TypeObject.h"

#include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export DynamicTypeMemberImpl : public DDS::DynamicTypeMember {
public:
  DynamicTypeMemberImpl();
  ~DynamicTypeMemberImpl();
  DDS::ReturnCode_t set_descriptor(DDS::MemberDescriptor* descriptor);
  DDS::ReturnCode_t get_descriptor(DDS::MemberDescriptor*& descriptor);
  CORBA::ULong get_annotation_count();
  DDS::ReturnCode_t get_annotation(DDS::AnnotationDescriptor*& descriptor, CORBA::ULong idx);
  CORBA::ULong get_verbatim_text_count();
  DDS::ReturnCode_t get_verbatim_text(DDS::VerbatimTextDescriptor*& descriptor, CORBA::ULong idx);
  bool equals(DDS::DynamicTypeMember_ptr other);
  MemberId get_id();
  char* get_name();

private:
  DDS::MemberDescriptor_var descriptor_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif  /* OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_MEMBER_IMPL_H */
