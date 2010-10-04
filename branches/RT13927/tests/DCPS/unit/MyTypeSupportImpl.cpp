// -*- C++ -*-
//
// $Id$

#include "MyTypeSupportImpl.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"

#include <stdexcept>

// Implementation skeleton constructor
MyTypeSupportImpl::MyTypeSupportImpl (void)
  {
  }

// Implementation skeleton destructor
MyTypeSupportImpl::~MyTypeSupportImpl (void)
  {
  }

DDS::ReturnCode_t
MyTypeSupportImpl::register_type (
    ::DDS::DomainParticipant_ptr participant,
    const char * type_name
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  CORBA::String_var tn;
  if (type_name == 0 || type_name[0] == '\0')
     tn = this->get_type_name ();
  else
     tn = CORBA::string_dup (type_name);

  return ::OpenDDS::DCPS::Registered_Data_Types->register_type(participant,
                                                           tn.in (),
                                                           this);
}


char *
MyTypeSupportImpl::get_type_name (
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  return CORBA::string_dup (this->_interface_repository_id());
}


::DDS::DataWriter_ptr MyTypeSupportImpl::create_datawriter (
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    MyDataWriterImpl* writer_impl;
    ACE_NEW_RETURN(writer_impl,
                   MyDataWriterImpl(),
                   ::DDS::DataWriter::_nil());

    return writer_impl;
  }

::DDS::DataReader_ptr MyTypeSupportImpl::create_datareader (
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    MyDataReaderImpl* reader_impl;
    ACE_NEW_RETURN(reader_impl,
                   MyDataReaderImpl(),
                   ::DDS::DataReader::_nil());

    return reader_impl;
  }


#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
::DDS::DataReader_ptr
MyTypeSupportImpl::create_multitopic_datareader()
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  return NULL;
}

const OpenDDS::DCPS::MetaStruct&
MyTypeSupportImpl::getMetaStructForType()
{
  throw std::runtime_error("unimplemented");
}

DDS::ReturnCode_t MyDataReaderImpl::read_generic(
  OpenDDS::DCPS::DataReaderImpl::GenericBundle&, DDS::SampleStateMask,
  DDS::ViewStateMask, DDS::InstanceStateMask)
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::InstanceHandle_t MyDataReaderImpl::lookup_instance_generic(const void*)
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t MyDataReaderImpl::read_instance_generic(void*&,
  DDS::SampleInfo&, DDS::InstanceHandle_t, DDS::SampleStateMask,
  DDS::ViewStateMask, DDS::InstanceStateMask)
{
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t MyDataReaderImpl::read_next_instance_generic(void*&,
  DDS::SampleInfo&, DDS::InstanceHandle_t, DDS::SampleStateMask,
  DDS::ViewStateMask, DDS::InstanceStateMask)
{
  return DDS::RETCODE_UNSUPPORTED;
}

#endif
