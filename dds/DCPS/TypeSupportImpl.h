/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TYPESUPPORTIMPL_H
#define OPENDDS_DCPS_TYPESUPPORTIMPL_H

#include "dcps_export.h"
#include "Definitions.h"
#include "LocalObject.h"
#include "Serializer.h"
#include "SafetyProfileStreams.h"
#include "XTypes/TypeObject.h"
#include "XTypes/TypeLookupService.h"

#include <dds/DdsDynamicDataC.h>
#include <dds/DdsDcpsTypeSupportExtC.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MetaStruct;

template <typename Message> struct DDSTraits;

template <typename Message> struct MarshalTraits;

class OpenDDS_Dcps_Export SerializedSizeBound {
public:
  SerializedSizeBound()
  : bounded_(false)
  , bound_(0)
  {
  }

  SerializedSizeBound(size_t bound)
  : bounded_(true)
  , bound_(bound)
  {
  }

  operator bool() const
  {
    return bounded_;
  }

  size_t get() const
  {
    OPENDDS_ASSERT(bounded_);
    return bound_;
  }

  OPENDDS_STRING to_string() const
  {
    return bounded_ ? to_dds_string(bound_) : "<unbounded>";
  }

private:
  bool bounded_;
  size_t bound_;
};

class OpenDDS_Dcps_Export TypeSupportImpl
  : public virtual LocalObject<TypeSupport> {
public:
  TypeSupportImpl() {}

  virtual ~TypeSupportImpl();

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual const MetaStruct& getMetaStructForType() = 0;
#endif

  virtual DDS::ReturnCode_t register_type(DDS::DomainParticipant_ptr participant,
                                          const char* type_name);

  virtual DDS::ReturnCode_t unregister_type(DDS::DomainParticipant_ptr participant,
                                            const char* type_name);

  virtual const char* name() const = 0;

  /// NOTE: This one implements the IDL TypeSupport method so it returns a CORBA String
  /// that the caller must take ownership of.
  virtual char* get_type_name();

  virtual size_t key_count() const = 0;
  bool has_dcps_key()
  {
    return key_count();
  }

  virtual SerializedSizeBound serialized_size_bound(const Encoding& encoding) const = 0;
  virtual SerializedSizeBound key_only_serialized_size_bound(const Encoding& encoding) const = 0;

  /// Returns the extensibility of just the topic type.
  virtual Extensibility base_extensibility() const = 0;
  /// Between the topic type and its nested types, return the extensibility
  /// that is furthest right in (final, appenable, mutable).
  virtual Extensibility max_extensibility() const = 0;

  virtual const XTypes::TypeIdentifier& getMinimalTypeIdentifier() const = 0;
  virtual const XTypes::TypeMap& getMinimalTypeMap() const = 0;
  virtual const XTypes::TypeIdentifier& getCompleteTypeIdentifier() const = 0;
  virtual const XTypes::TypeMap& getCompleteTypeMap() const = 0;

  void to_type_info(XTypes::TypeInformation& type_info) const;

  void add_types(const XTypes::TypeLookupService_rch& tls) const;
  void populate_dependencies(const XTypes::TypeLookupService_rch& tls) const;

protected:
#ifndef OPENDDS_SAFETY_PROFILE
  DDS::DynamicType_ptr get_type_from_type_lookup_service();
#endif

private:
  static const ACE_CDR::Long TYPE_INFO_DEPENDENT_COUNT_NOT_PROVIDED;

  void to_type_info_i(XTypes::TypeIdentifierWithDependencies& ti_with_deps,
                      const XTypes::TypeIdentifier& ti,
                      const XTypes::TypeMap& type_map) const;

  void populate_dependencies_i(const XTypes::TypeLookupService_rch& tls,
                               XTypes::EquivalenceKind ek) const;

#ifndef OPENDDS_SAFETY_PROFILE
  XTypes::TypeLookupService_rch type_lookup_service_;
#endif

  OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(TypeSupportImpl)
};

template <typename NativeType>
class TypeSupportImpl_T : public TypeSupportImpl {
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
    return get_type_from_type_lookup_service();
  }
#endif
};

template <typename T>
struct TypeSupportInitializer {
  TypeSupportInitializer()
    : ts_(new T)
  {
    ts_->register_type(0, "");
  }

  ~TypeSupportInitializer()
  {
    T* const t = dynamic_cast<T*>(ts_.in());
    ts_->unregister_type(0, t ? t->name() : 0);
  }

  typename T::_var_type ts_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
