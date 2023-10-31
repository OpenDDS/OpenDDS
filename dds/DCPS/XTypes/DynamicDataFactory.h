/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_FACTORY_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_FACTORY_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/LocalObject.h>

#  include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {

class DynamicDataFactory;
typedef DynamicDataFactory* DynamicDataFactory_ptr;

typedef TAO_Objref_Var_T<DynamicDataFactory> DynamicDataFactory_var;

class OpenDDS_Dcps_Export DynamicDataFactory
  : public OpenDDS::DCPS::LocalObject<DynamicDataFactoryInterf>
{
public:
  typedef DynamicDataFactory_ptr _ptr_type;
  typedef DynamicDataFactory_var _var_type;

  DynamicDataFactory()
  {
  }

  virtual ~DynamicDataFactory() {}

  static DynamicDataFactory_ptr get_instance();

  static ReturnCode_t delete_instance();

  DynamicData_ptr create_data(DynamicType_ptr type);

  ReturnCode_t delete_data(DynamicData_ptr /*data*/)
  {
    return RETCODE_OK;
  }

  CORBA::Boolean _is_a(const char* type_id)
  {
    return DynamicDataFactoryInterf::_is_a(type_id);
  }

  const char* _interface_repository_id() const
  {
    return DynamicDataFactoryInterf::_interface_repository_id();
  }

  CORBA::Boolean marshal(TAO_OutputCDR&)
  {
    return false;
  }

  static DynamicDataFactory_ptr _duplicate(DynamicDataFactory_ptr obj);
};

} // namespace DDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#  ifndef DDS_DYNAMICDATAFACTORY__TRAITS_
#    define DDS_DYNAMICDATAFACTORY__TRAITS_
TAO_BEGIN_VERSIONED_NAMESPACE_DECL
namespace TAO {
template<>
struct OpenDDS_Dcps_Export Objref_Traits<DDS::DynamicDataFactory> {
  static DDS::DynamicDataFactory_ptr duplicate(DDS::DynamicDataFactory_ptr p);
  static void release(DDS::DynamicDataFactory_ptr p);
  static DDS::DynamicDataFactory_ptr nil();
  static CORBA::Boolean marshal(const DDS::DynamicDataFactory_ptr p, TAO_OutputCDR & cdr);
};
} // namespace TAO
TAO_END_VERSIONED_NAMESPACE_DECL
#  endif

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_FACTORY_H
