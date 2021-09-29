#include "MyTypeSupportImpl.h"

#include <dds/DCPS/Registered_Data_Types.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/DataWriterImpl.h>
#include <dds/DCPS/DataReaderImpl.h>

#include <dds/DdsDcpsDomainC.h>

#include <stdexcept>

MyTypeSupportImpl::MyTypeSupportImpl()
{
}

MyTypeSupportImpl::~MyTypeSupportImpl()
{
}

DDS::ReturnCode_t MyTypeSupportImpl::register_type(
  DDS::DomainParticipant* participant,
  const char* type_name)
{
  CORBA::String_var tn;
  if (type_name == 0 || type_name[0] == '\0') {
   tn = get_type_name();
  } else {
   tn = CORBA::string_dup(type_name);
  }

  return Registered_Data_Types->register_type(participant, tn.in(), this);
}


DDS::ReturnCode_t MyTypeSupportImpl::unregister_type (
  DDS::DomainParticipant* participant,
  const char* type_name)
{
  if (type_name == 0 || type_name[0] == '\0') {
   return DDS::RETCODE_BAD_PARAMETER;
  } else {
   return Registered_Data_Types->unregister_type(participant, type_name, this);
  }
}


char* MyTypeSupportImpl::get_type_name()
{
  return CORBA::string_dup(this->_interface_repository_id());
}


::DDS::DataWriter_ptr MyTypeSupportImpl::create_datawriter()
{
  MyDataWriterImpl* writer_impl;
  ACE_NEW_RETURN(writer_impl, MyDataWriterImpl(), ::DDS::DataWriter::_nil());

  return writer_impl;
}

::DDS::DataReader_ptr MyTypeSupportImpl::create_datareader()
{
  MyDataReaderImpl* reader_impl;
  ACE_NEW_RETURN(reader_impl, MyDataReaderImpl(), ::DDS::DataReader::_nil());

  return reader_impl;
}

void MyTypeSupportImpl::representations_allowed_by_type(
  DDS::DataRepresentationIdSeq& seq)
{
  seq.length(0); // Let OpenDDS Default
}

#ifndef OPENDDS_NO_MULTI_TOPIC
::DDS::DataReader_ptr MyTypeSupportImpl::create_multitopic_datareader()
{
  return 0;
}
#endif

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

const OpenDDS::DCPS::MetaStruct&
MyTypeSupportImpl::getMetaStructForType()
{
  throw std::runtime_error("unimplemented");
}

DDS::ReturnCode_t MyDataReaderImpl::read_generic(
  OpenDDS::DCPS::DataReaderImpl::GenericBundle&, DDS::SampleStateMask,
  DDS::ViewStateMask, DDS::InstanceStateMask, bool)
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
