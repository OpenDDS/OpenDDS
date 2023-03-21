#ifndef OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_IMPL_H
#define OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "TypeObject.h"

#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/RcHandle_T.h>
#include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export TypeDescriptorImpl
  : public virtual OBV_DDS::TypeDescriptor
  , public virtual CORBA::DefaultValueRefCountBase
{
public:
  TypeDescriptorImpl();
  ~TypeDescriptorImpl();

  CORBA::Boolean equals(DDS::TypeDescriptor*);
  DDS::ReturnCode_t copy_from(DDS::TypeDescriptor*);
  CORBA::Boolean is_consistent();

  CORBA::ValueBase* _copy_value();
  CORBA::TypeCode_ptr _tao_type() const;

private:
  CORBA::Boolean _tao_marshal__DDS_TypeDescriptor(TAO_OutputCDR&, TAO_ChunkInfo&) const;
  CORBA::Boolean _tao_unmarshal__DDS_TypeDescriptor(TAO_InputCDR&, TAO_ChunkInfo&);
};

typedef std::pair<const DDS::DynamicType*, const DDS::DynamicType*> DynamicTypePtrPair;
typedef OPENDDS_SET(DynamicTypePtrPair) DynamicTypePtrPairSeen;

bool test_equality(DDS::TypeDescriptor* lhs,
                   DDS::TypeDescriptor* rhs,
                   DynamicTypePtrPairSeen& dt_ptr_pair);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif  /* OPENDDS_DCPS_XTYPES_TYPE_DESCRIPTOR_IMPL_H */
