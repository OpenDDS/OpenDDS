/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_SUPPORT_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_SUPPORT_H

#ifndef OPENDDS_SAFETY_PROFILE

#include <dds/DdsDynamicTypeSupportC.h>
#include <dds/DdsDcpsTypeSupportExtC.h>

#include <dds/DCPS/TypeSupportImpl.h>

#include "DynamicSample.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {
  class DynamicTypeSupport;
}

namespace OpenDDS {
  namespace DCPS {
    class DataReaderImpl;
    class ReceivedDataElement;
  }

  namespace XTypes {
    struct DynamicSequenceAdapter {
      explicit DynamicSequenceAdapter(DDS::DynamicDataSeq& seq)
        : seq_(seq)
      {}

      ACE_CDR::ULong max_slots() const { return seq_.maximum(); }
      void internal_set_length(ACE_CDR::ULong len) { seq_.length(len); }

      void assign_sample(ACE_CDR::ULong i, const DynamicSample& d)
      {
        seq_[i] = d.dynamic_data();
      }

      void assign_ptr(ACE_CDR::ULong, const DCPS::ReceivedDataElement*) {}
      void set_loaner(DCPS::DataReaderImpl*) {}
      void increment_references() {}

      DDS::DynamicDataSeq& seq_;
    };
  }

  namespace DCPS {
    template <>
    struct DDSTraits<XTypes::DynamicSample> {
      typedef XTypes::DynamicSample MessageType;
      typedef DDS::DynamicDataSeq MessageSequenceType;
      typedef XTypes::DynamicSequenceAdapter MessageSequenceAdapterType;
      typedef DDS::DynamicTypeSupport TypeSupportType;
      typedef DDS::DynamicDataWriter DataWriterType;
      typedef DDS::DynamicDataReader DataReaderType;
      typedef XTypes::DynamicSample::KeyLessThan LessThanType;
      typedef DCPS::KeyOnly<const XTypes::DynamicSample> KeyOnlyType;
      static const char* type_name() { return "Dynamic"; } // used for logging
    };

    template <>
    struct MarshalTraits<XTypes::DynamicSample> {
      static bool to_message_block(ACE_Message_Block&, const XTypes::DynamicSample&) { return false; }
      static bool from_message_block(XTypes::DynamicSample&, const ACE_Message_Block&) { return false; }
      static Extensibility extensibility() { return APPENDABLE; }
      static Extensibility max_extensibility_level() { return APPENDABLE; }
    };

    bool operator>>(Serializer& strm, XTypes::DynamicSample& sample);
    bool operator>>(Serializer& strm, const KeyOnly<XTypes::DynamicSample>& sample);
  }
}

namespace DDS {

  typedef DynamicTypeSupport* DynamicTypeSupport_ptr;

  typedef TAO_Objref_Var_T<DynamicTypeSupport> DynamicTypeSupport_var;

  class OpenDDS_Dcps_Export DynamicTypeSupport
    : public virtual OpenDDS::DCPS::TypeSupportImpl
    , public virtual DynamicTypeSupportInterf
  {
  public:
    typedef DynamicTypeSupport_ptr _ptr_type;
    typedef DynamicTypeSupport_var _var_type;

    explicit DynamicTypeSupport(DynamicType_ptr type)
      : TypeSupportImpl(type)
      , name_(type->get_name())
    {
    }

    virtual ~DynamicTypeSupport() {}

    const char* name() const
    {
      return name_.in();
    }

    size_t key_count() const;
    bool is_dcps_key(const char* field) const;

    void representations_allowed_by_type(DataRepresentationIdSeq& seq);

    OpenDDS::DCPS::Extensibility base_extensibility() const;
    OpenDDS::DCPS::Extensibility max_extensibility() const;

    OpenDDS::DCPS::SerializedSizeBound serialized_size_bound(const OpenDDS::DCPS::Encoding&) const
    {
      return OpenDDS::DCPS::SerializedSizeBound();
    }

    OpenDDS::DCPS::SerializedSizeBound key_only_serialized_size_bound(
      const OpenDDS::DCPS::Encoding&) const
    {
      return OpenDDS::DCPS::SerializedSizeBound();
    }

    DataWriter_ptr create_datawriter();
    DataReader_ptr create_datareader();
#ifndef OPENDDS_NO_MULTI_TOPIC
    DataReader_ptr create_multitopic_datareader();
#endif

    const OpenDDS::XTypes::TypeIdentifier& getMinimalTypeIdentifier() const;
    const OpenDDS::XTypes::TypeMap& getMinimalTypeMap() const;
    const OpenDDS::XTypes::TypeIdentifier& getCompleteTypeIdentifier() const;
    const OpenDDS::XTypes::TypeMap& getCompleteTypeMap() const;

    const OpenDDS::XTypes::TypeInformation* preset_type_info() const;

    DynamicType_ptr get_type()
    {
      return DynamicType::_duplicate(type_);
    }

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    const OpenDDS::DCPS::MetaStruct& getMetaStructForType() const;
#endif

    CORBA::Boolean _is_a(const char* type_id)
    {
      return DynamicTypeSupportInterf::_is_a(type_id);
    }

    const char* _interface_repository_id() const
    {
      return DynamicTypeSupportInterf::_interface_repository_id();
    }

    CORBA::Boolean marshal(TAO_OutputCDR&)
    {
      return false;
    }

    static DynamicTypeSupport_ptr _duplicate(DynamicTypeSupport_ptr obj);

  protected:
    CORBA::String_var name_;
  };
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

TAO_BEGIN_VERSIONED_NAMESPACE_DECL
namespace TAO {
  template <>
  struct OpenDDS_Dcps_Export Objref_Traits<DDS::DynamicTypeSupport> {
    static DDS::DynamicTypeSupport_ptr duplicate(DDS::DynamicTypeSupport_ptr p);
    static void release(DDS::DynamicTypeSupport_ptr p);
    static DDS::DynamicTypeSupport_ptr nil();
    static CORBA::Boolean marshal(const DDS::DynamicTypeSupport_ptr p, TAO_OutputCDR & cdr);
  };
}
TAO_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_SUPPORT_H
