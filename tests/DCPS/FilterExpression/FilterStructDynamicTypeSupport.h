#include <dds/DCPS/TypeSupportImpl.h>

// FilterStruct.idl includes DdsDcpsInfrastructure.idl which doesn't have -Gxtypes-complete

struct DummyTypeSupport : OpenDDS::DCPS::TypeSupportImpl {

  // DDS::TypeSupport
  DDS::ReturnCode_t register_type(DDS::DomainParticipant*, const char*) { return DDS::RETCODE_UNSUPPORTED; }
  char* get_type_name() { return CORBA::string_dup("TBTD"); }
  DDS::DynamicType* get_type() { return static_cast<const DummyTypeSupport*>(this)->get_type(); }

  // OpenDDS::DCPS::TypeSupport
  DDS::DataWriter* create_datawriter() { return 0; }
  DDS::DataReader* create_datareader() { return 0; }
  DDS::DataReader* create_multitopic_datareader() { return 0; }
  bool has_dcps_key() { return false; }
  DDS::ReturnCode_t unregister_type(DDS::DomainParticipant*, const char*) { return DDS::RETCODE_UNSUPPORTED; }
  void representations_allowed_by_type(DDS::DataRepresentationIdSeq&) {}

  // TypeSupportImpl
  const OpenDDS::DCPS::MetaStruct& getMetaStructForType() const;
  const char* name() const { return "TBTD"; }
  DDS::DynamicType* get_type() const;
  size_t key_count() const { return 0; }
  bool is_dcps_key(const char*) const { return false; }
  OpenDDS::DCPS::SerializedSizeBound serialized_size_bound(const OpenDDS::DCPS::Encoding& encoding) const;
  OpenDDS::DCPS::SerializedSizeBound key_only_serialized_size_bound(const OpenDDS::DCPS::Encoding& encoding) const;
  OpenDDS::DCPS::Extensibility base_extensibility() const;
  OpenDDS::DCPS::Extensibility max_extensibility() const;
  OpenDDS::XTypes::TypeIdentifier& getMinimalTypeIdentifier() const;
  const OpenDDS::XTypes::TypeMap& getMinimalTypeMap() const;
  const OpenDDS::XTypes::TypeIdentifier& getCompleteTypeIdentifier() const;
  const OpenDDS::XTypes::TypeMap& getCompleteTypeMap() const;
};

