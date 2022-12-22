#ifndef OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_IMPL_H
#define OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "TypeObject.h"

#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/RcHandle_T.h>
#include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export MemberDescriptorImpl
  : public virtual OBV_DDS::MemberDescriptor
  , public virtual CORBA::DefaultValueRefCountBase
{
public:
  MemberDescriptorImpl();
  ~MemberDescriptorImpl();
  MemberDescriptorImpl(const char* a_name, bool a_is_key)
  {
    name(a_name);
    id(0);
    index(0);
    try_construct_kind(DDS::USE_DEFAULT);
    is_key(a_is_key);
    is_optional(false);
    is_must_understand(false);
    is_shared(false);
    is_default_label(false);
  }

  bool equals(DDS::MemberDescriptor* other);
  DDS::ReturnCode_t copy_from(DDS::MemberDescriptor*);
  ::CORBA::Boolean is_consistent();

  CORBA::ValueBase* _copy_value();
  CORBA::TypeCode_ptr _tao_type() const;

private:
  ::CORBA::Boolean _tao_marshal__DDS_MemberDescriptor(TAO_OutputCDR &, TAO_ChunkInfo &) const;
  ::CORBA::Boolean _tao_unmarshal__DDS_MemberDescriptor(TAO_InputCDR &, TAO_ChunkInfo &);
};

typedef std::pair<const DDS::DynamicType*, const DDS::DynamicType*> DynamicTypePtrPair;
typedef OPENDDS_SET(DynamicTypePtrPair) DynamicTypePtrPairSeen;

bool test_equality(DDS::MemberDescriptor* lhs, DDS::MemberDescriptor* rhs, DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#else

namespace DDS {

class MemberDescriptor {
public:
  // a_name should be shallow-copyable, e.g., a string literal.
  MemberDescriptor(const char* a_name, bool a_is_key)
    : name_(a_name)
    , is_key_(a_is_key)
  { }

  const char* name() const { return name_; }
  bool is_key() const { return is_key_; }

private:
  const char* name_;
  bool is_key_;
};

}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

typedef DDS::MemberDescriptor MemberDescriptorImpl;

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif  /* OPENDDS_DCPS_XTYPES_MEMBER_DESCRIPTOR_IMPL_H */
