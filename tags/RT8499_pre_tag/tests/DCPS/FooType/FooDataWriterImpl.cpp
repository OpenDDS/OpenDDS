// -*- C++ -*-
//
// $Id$


#include "FooDataWriterImpl.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/Service_Participant.h"

#include "Foo_Singleton_Transport.h"


// Implementation skeleton constructor
FooDataWriterImpl::FooDataWriterImpl (void)
  {
  }
  
// Implementation skeleton destructor
FooDataWriterImpl::~FooDataWriterImpl (void)
  {
  }
  
::DDS::InstanceHandle_t FooDataWriterImpl::_cxx_register (
    const Foo & instance_data
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(instance_data);
    // Add your implementation here
    return 0;
  }
  
::DDS::InstanceHandle_t FooDataWriterImpl::register_w_timestamp (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle,
    const ::DDS::Time_t & source_timestamp
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(instance_data);
    ACE_UNUSED_ARG(handle);
    ACE_UNUSED_ARG(source_timestamp);
    // Add your implementation here
    return 0;
  }
  
::DDS::ReturnCode_t FooDataWriterImpl::unregister (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(instance_data);
    ACE_UNUSED_ARG(handle);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }
  
::DDS::ReturnCode_t FooDataWriterImpl::unregister_w_timestamp (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle,
    const ::DDS::Time_t & source_timestamp
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(instance_data);
    ACE_UNUSED_ARG(handle);
    ACE_UNUSED_ARG(source_timestamp);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }
  
::DDS::ReturnCode_t FooDataWriterImpl::write (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(handle);
    // Add your implementation here

    return Foo_Singleton_Transport->set_foo(instance_data);
  }
  
::DDS::ReturnCode_t FooDataWriterImpl::write_w_timestamp (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle,
    const ::DDS::Time_t & source_timestamp
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(instance_data);
    ACE_UNUSED_ARG(handle);
    ACE_UNUSED_ARG(source_timestamp);
    // Add your implementation here
    //TBD add handling of timestamp
    return Foo_Singleton_Transport->set_foo(instance_data);
  }
  
::DDS::ReturnCode_t FooDataWriterImpl::dispose (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t instance_handle
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(instance_data);
    ACE_UNUSED_ARG(instance_handle);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }
  
::DDS::ReturnCode_t FooDataWriterImpl::dispose_w_timestamp (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t instance_handle,
    const ::DDS::Time_t & source_timestamp
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(instance_data);
    ACE_UNUSED_ARG(instance_handle);
    ACE_UNUSED_ARG(source_timestamp);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }
  
::DDS::ReturnCode_t FooDataWriterImpl::get_key_value (
    Foo & key_holder,
    ::DDS::InstanceHandle_t handle
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(key_holder);
    ACE_UNUSED_ARG(handle);
    // Add your implementation here
    //key_holder = key;
    return ::DDS::RETCODE_OK;
  }



