#ifndef dds_DCPS_TypeSupportImpl_h
#define dds_DCPS_TypeSupportImpl_h

#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "dds/DCPS/MultiTopicDataReader_T.h"
#include "dds/DCPS/DataWriterImpl_T.h"
#include "dds/DCPS/DataReaderImpl_T.h"

namespace OpenDDS {

/** Servant for TypeSupport interface of Traits::MessageType data type.
 *
 * See the DDS specification, OMG formal/04-12-02, for a description of
 * this interface.
 *
 */
  template <typename Traits>
  class TypeSupportImpl
    : public virtual OpenDDS::DCPS::LocalObject<typename Traits::TypeSupportType>
    , public virtual OpenDDS::DCPS::TypeSupportImpl
  {
  public:
    typedef Traits TraitsType;
    typedef typename Traits::MessageType MessageType;
    typedef typename Traits::TypeSupportType TypeSupportType;

    TypeSupportImpl() { }
    virtual ~TypeSupportImpl() { }

    virtual ::DDS::DataWriter_ptr create_datawriter()
    {
      typedef DataWriterImpl<Traits> DataWriterImplType;

      DataWriterImplType* writer_impl;
      ACE_NEW_RETURN(writer_impl,
                     DataWriterImplType(),
                     ::DDS::DataWriter::_nil());

      return writer_impl;
    }

    virtual ::DDS::DataReader_ptr create_datareader()
    {
      typedef DataReaderImpl<Traits> DataReaderImplType;

      DataReaderImplType* reader_impl = 0;
      ACE_NEW_RETURN(reader_impl,
                     DataReaderImplType(),
                     ::DDS::DataReader::_nil());

      return reader_impl;
    }

#ifndef OPENDDS_NO_MULTI_TOPIC
    virtual ::DDS::DataReader_ptr create_multitopic_datareader()
    {
      typedef DataReaderImpl<Traits> DataReaderImplType;
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
      return Traits::gen_has_key(MessageType());
    }

    static _ptr_type _narrow(CORBA::Object_ptr obj) {
      return TypeSupportType::_narrow(obj);
    }
  };

}

#endif /* dds_DCPS_TypeSupportImpl_h */
