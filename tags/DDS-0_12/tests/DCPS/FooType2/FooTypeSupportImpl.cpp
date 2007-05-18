// -*- C++ -*-
//
// $Id$

#include "FooTypeSupportImpl.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/Service_Participant.h"

#include "FooDataWriterImpl.h"
#include "FooDataReaderImpl.h"

// Implementation skeleton constructor
FooTypeSupportImpl::FooTypeSupportImpl (void)
  {
  }

// Implementation skeleton destructor
FooTypeSupportImpl::~FooTypeSupportImpl (void)
  {
  }

::DDS::ReturnCode_t FooTypeSupportImpl::register_type (
    ::DDS::DomainParticipant_ptr participant,
    const char * type_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ::DDS::DomainId_t domain = 0;
    domain = participant->get_domain_id();

    ::DDS::ReturnCode_t registered =
      ::TAO::DCPS::Registered_Data_Types->register_type(domain,
                                                        type_name,
                                                        this);

    return registered;
  }

::TAO::DCPS::DataWriterRemote_ptr FooTypeSupportImpl::create_datawriter (
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    FooDataWriterImpl* writer_impl;
    ACE_NEW_RETURN(writer_impl,
                    FooDataWriterImpl(),
                    ::TAO::DCPS::DataWriterRemote::_nil());


    ::TAO::DCPS::DataWriterRemote_ptr writer_obj
        = ::TAO::DCPS::servant_to_reference<TAO::DCPS::DataWriterRemote,
                                            FooDataWriterImpl,
                                            TAO::DCPS::DataWriterRemote_ptr>
              (writer_impl);

    return writer_obj;
  }

::TAO::DCPS::DataReaderRemote_ptr FooTypeSupportImpl::create_datareader (
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    FooDataReaderImpl* reader_impl;
    ACE_NEW_RETURN(reader_impl,
                    FooDataReaderImpl(),
                    ::TAO::DCPS::DataReaderRemote::_nil());


    ::TAO::DCPS::DataReaderRemote_ptr reader_obj
        = ::TAO::DCPS::servant_to_reference<TAO::DCPS::DataReaderRemote,
                                            FooDataReaderImpl,
                                            TAO::DCPS::DataReaderRemote_ptr>
              (reader_impl);

    return reader_obj;
  }


