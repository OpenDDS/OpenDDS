// -*- C++ -*-
//
// $Id$



#include "FooDataReaderImpl.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DdsDcpsDomainC.h"
#include "Foo_Singleton_Transport.h"





// Implementation skeleton constructor
FooDataReaderImpl::FooDataReaderImpl (void)
  {
  }

// Implementation skeleton destructor
FooDataReaderImpl::~FooDataReaderImpl (void)
  {
  }

::DDS::ReturnCode_t FooDataReaderImpl::read (
    FooSeq & received_data,
    ::DDS::SampleInfoSeq & info_seq,
    CORBA::Long max_samples,
    ::DDS::SampleStateMask sample_states,
    ::DDS::ViewStateMask view_states,
    ::DDS::InstanceStateMask instance_states
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(received_data);
    ACE_UNUSED_ARG(info_seq);
    ACE_UNUSED_ARG(max_samples);
    ACE_UNUSED_ARG(sample_states);
    ACE_UNUSED_ARG(view_states);
    ACE_UNUSED_ARG(instance_states);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::take (
    FooSeq & received_data,
    ::DDS::SampleInfoSeq & info_seq,
    CORBA::Long max_samples,
    ::DDS::SampleStateMask sample_states,
    ::DDS::ViewStateMask view_states,
    ::DDS::InstanceStateMask instance_states
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(received_data);
    ACE_UNUSED_ARG(info_seq);
    ACE_UNUSED_ARG(max_samples);
    ACE_UNUSED_ARG(sample_states);
    ACE_UNUSED_ARG(view_states);
    ACE_UNUSED_ARG(instance_states);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::read_next_sample (
    Foo & received_data,
    ::DDS::SampleInfo & sample_info
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(sample_info);
    // Add your implementation here
    received_data = Foo_Singleton_Transport->get_foo();

    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::take_next_sample (
    Foo & received_data,
    ::DDS::SampleInfo & sample_info
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(sample_info);
    // Add your implementation here
    received_data = Foo_Singleton_Transport->get_foo();

    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::read_instance (
    FooSeq & received_data,
    ::DDS::SampleInfoSeq & info_seq,
    CORBA::Long max_samples,
    ::DDS::InstanceHandle_t a_handle,
    ::DDS::SampleStateMask sample_states,
    ::DDS::ViewStateMask view_states,
    ::DDS::InstanceStateMask instance_states
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(received_data);
    ACE_UNUSED_ARG(info_seq);
    ACE_UNUSED_ARG(max_samples);
    ACE_UNUSED_ARG(a_handle);
    ACE_UNUSED_ARG(sample_states);
    ACE_UNUSED_ARG(view_states);
    ACE_UNUSED_ARG(instance_states);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::take_instance (
    FooSeq & received_data,
    ::DDS::SampleInfoSeq & info_seq,
    CORBA::Long max_samples,
    ::DDS::InstanceHandle_t a_handle,
    ::DDS::SampleStateMask sample_states,
    ::DDS::ViewStateMask view_states,
    ::DDS::InstanceStateMask instance_states
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(received_data);
    ACE_UNUSED_ARG(info_seq);
    ACE_UNUSED_ARG(max_samples);
    ACE_UNUSED_ARG(a_handle);
    ACE_UNUSED_ARG(sample_states);
    ACE_UNUSED_ARG(view_states);
    ACE_UNUSED_ARG(instance_states);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::read_next_instance (
    FooSeq & received_data,
    ::DDS::SampleInfoSeq & info_seq,
    CORBA::Long max_samples,
    ::DDS::InstanceHandle_t a_handle,
    ::DDS::SampleStateMask sample_states,
    ::DDS::ViewStateMask view_states,
    ::DDS::InstanceStateMask instance_states
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(received_data);
    ACE_UNUSED_ARG(info_seq);
    ACE_UNUSED_ARG(max_samples);
    ACE_UNUSED_ARG(a_handle);
    ACE_UNUSED_ARG(sample_states);
    ACE_UNUSED_ARG(view_states);
    ACE_UNUSED_ARG(instance_states);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::take_next_instance (
    FooSeq & received_data,
    ::DDS::SampleInfoSeq & info_seq,
    CORBA::Long max_samples,
    ::DDS::InstanceHandle_t a_handle,
    ::DDS::SampleStateMask sample_states,
    ::DDS::ViewStateMask view_states,
    ::DDS::InstanceStateMask instance_states
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(received_data);
    ACE_UNUSED_ARG(info_seq);
    ACE_UNUSED_ARG(max_samples);
    ACE_UNUSED_ARG(a_handle);
    ACE_UNUSED_ARG(sample_states);
    ACE_UNUSED_ARG(view_states);
    ACE_UNUSED_ARG(instance_states);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::return_loan (
    FooSeq & received_data,
    ::DDS::SampleInfoSeq & info_seq
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(received_data);
    ACE_UNUSED_ARG(info_seq);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::get_key_value (
    Foo & key_holder,
    ::DDS::InstanceHandle_t handle
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(key_holder);
    ACE_UNUSED_ARG(handle);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

