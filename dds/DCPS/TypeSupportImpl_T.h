#ifndef dds_DCPS_TypeSupportImpl_h
#define dds_DCPS_TypeSupportImpl_h

#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "dds/DCPS/MultiTopicDataReader_T.h"
#include "dds/DCPS/DataWriterImpl_T.h"
#include "dds/DCPS/DataReaderImpl_T.h"
#include "dcps_export.h"

namespace OpenDDS {
  namespace DCPS {

/** Servant for TypeSupport interface of Traits::MessageType data type.
 *
 * See the DDS specification, OMG formal/04-12-02, for a description of
 * this interface.
 *
 */
  template <typename MessageType>
  class
#if ( __GNUC__ == 4 && __GNUC_MINOR__ == 1)
    OpenDDS_Dcps_Export
#endif
    TypeSupportImpl_T
    : public virtual OpenDDS::DCPS::LocalObject<typename DDSTraits<MessageType>::TypeSupportType>
    , public virtual OpenDDS::DCPS::TypeSupportImpl
  {
  public:
    typedef DDSTraits<MessageType> TraitsType;
    typedef typename TraitsType::TypeSupportType TypeSupportType;
    typedef typename OpenDDS::DCPS::LocalObject<TypeSupportType>::_var_type _var_type;
    typedef typename OpenDDS::DCPS::LocalObject<TypeSupportType>::_ptr_type _ptr_type;

    TypeSupportImpl_T() { }
    virtual ~TypeSupportImpl_T() { }

    virtual ::DDS::DataWriter_ptr create_datawriter()
    {
      typedef DataWriterImpl_T<MessageType> DataWriterImplType;

      DataWriterImplType* writer_impl;
      ACE_NEW_RETURN(writer_impl,
                     DataWriterImplType(),
                     ::DDS::DataWriter::_nil());

      return writer_impl;
    }

    virtual ::DDS::DataReader_ptr create_datareader()
    {
      typedef DataReaderImpl_T<MessageType> DataReaderImplType;

      DataReaderImplType* reader_impl = 0;
      ACE_NEW_RETURN(reader_impl,
                     DataReaderImplType(),
                     ::DDS::DataReader::_nil());

      return reader_impl;
    }

#ifndef OPENDDS_NO_MULTI_TOPIC
    virtual ::DDS::DataReader_ptr create_multitopic_datareader()
    {
      typedef DataReaderImpl_T<MessageType> DataReaderImplType;
      return new OpenDDS::DCPS::MultiTopicDataReader_T<MessageType, DataReaderImplType>;
    }
#endif

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    virtual const OpenDDS::DCPS::MetaStruct& getMetaStructForType()
    {
      return OpenDDS::DCPS::getMetaStruct<MessageType>();
    }
#endif

    virtual bool has_dcps_key()
    {
      return TraitsType::gen_has_key(MessageType());
    }

    static typename TraitsType::TypeSupportType::_ptr_type _narrow(CORBA::Object_ptr obj) {
      return TypeSupportType::_narrow(obj);
    }
  };

  }
}

#endif /* dds_DCPS_TypeSupportImpl_h */
