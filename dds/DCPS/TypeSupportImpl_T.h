/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TYPESUPPORTIMPL_T_H
#define OPENDDS_DCPS_TYPESUPPORTIMPL_T_H

#include "debug.h"
#include "DCPS_Utils.h"
#include "LocalObject.h"
#include "TypeSupportImpl.h"
#include "XTypes/DynamicDataAdapter.h"
#include "XTypes/DynamicDataFactory.h"
#include "XTypes/Utils.h"

#include <dds/DdsDynamicDataC.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename NativeType>
class TypeSupportImpl_T
  : public virtual LocalObject<typename DDSTraits<NativeType>::TypeSupportType>
  , public TypeSupportImpl
{
public:
  typedef DDSTraits<NativeType> TraitsType;
  typedef MarshalTraits<NativeType> MarshalTraitsType;

  const char* name() const
  {
    return TraitsType::type_name();
  }

  size_t key_count() const
  {
    return TraitsType::key_count();
  }

  bool is_dcps_key(const char* fieldname) const
  {
    return TraitsType::is_key(fieldname);
  }

  void representations_allowed_by_type(DDS::DataRepresentationIdSeq& seq)
  {
    MarshalTraitsType::representations_allowed_by_type(seq);
  }

  Extensibility base_extensibility() const
  {
    return MarshalTraitsType::extensibility();
  }

  Extensibility max_extensibility() const
  {
    return MarshalTraitsType::max_extensibility_level();
  }

  SerializedSizeBound serialized_size_bound(const Encoding& encoding) const
  {
    return MarshalTraitsType::serialized_size_bound(encoding);
  }

  SerializedSizeBound key_only_serialized_size_bound(const Encoding& encoding) const
  {
    return MarshalTraitsType::key_only_serialized_size_bound(encoding);
  }

#ifndef OPENDDS_SAFETY_PROFILE
  DDS::DynamicType_ptr get_type()
  {
    get_type_from_type_lookup_service();
    return TypeSupportImpl::get_type();
  }

  /// The IDL is `NativeType create_sample(DDS::DynamicData src)`, but in the
  /// C++ mapping this can return NativeType* or just NativeType depending on if
  /// the type is "variable length". If it has something like a sequence then
  /// it's variable length and returns a pointer else it's fixed and returns on
  /// the stack. opendds_idl will wrap this in the correct form.
  DDS::ReturnCode_t create_sample_rc(NativeType& dst, DDS::DynamicData_ptr src)
  {
    OpenDDS::DCPS::set_default(dst);
#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
    DDS::DynamicType_var type = get_type();
    DDS::DynamicData_var dst_dd = XTypes::get_dynamic_data_adapter<NativeType>(type, dst);
    const DDS::ReturnCode_t rc = XTypes::copy(dst_dd, src);
    if (rc != DDS::RETCODE_OK && log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: TypeSupportImpl_T::create_sample_rc: "
        "failed to copy from DynamicData: %C\n", retcode_to_string(rc)));
    }
    return rc;
#  else
    ACE_UNUSED_ARG(dst);
    ACE_UNUSED_ARG(src);
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: TypeSupportImpl_T::create_sample_rc: "
        "OpenDDS built without DynamicDataAdapter support\n"));
    }
    return DDS::RETCODE_UNSUPPORTED;
#  endif
  }

  DDS::ReturnCode_t create_dynamic_sample_rc(DDS::DynamicData_ptr& dst, const NativeType& src)
  {
    CORBA::release(dst);
    DDS::DynamicType_var type = get_type();
    dst = DDS::DynamicDataFactory::get_instance()->create_data(type);
#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
    DDS::DynamicData_var src_dd = XTypes::get_dynamic_data_adapter<NativeType>(type, src);
    const DDS::ReturnCode_t rc = XTypes::copy(dst, src_dd);
    if (rc != DDS::RETCODE_OK && log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: TypeSupportImpl_T::create_dynamic_sample_rc: "
        "failed to copy to DynamicData: %C\n", retcode_to_string(rc)));
    }
    return rc;
#  else
    ACE_UNUSED_ARG(dst);
    ACE_UNUSED_ARG(src);
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: TypeSupportImpl_T::create_dynamic_sample_rc: "
        "OpenDDS built without DynamicDataAdapter support\n"));
    }
    return DDS::RETCODE_UNSUPPORTED;
#  endif
  }

  DDS::DynamicData_ptr create_dynamic_sample(const NativeType& src)
  {
    DDS::DynamicData_var dst;
    create_dynamic_sample_rc(dst, src);
    return dst;
  }
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
