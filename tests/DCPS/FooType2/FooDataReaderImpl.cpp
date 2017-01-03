// -*- C++ -*-
//



#include "FooDataReaderImpl.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DdsDcpsDomainC.h"


FooDataReaderImpl::FooDataReaderImpl (void)
#ifdef WIN32
: pos_ (0)
#endif
  {
  }

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
  {
    ACE_UNUSED_ARG (received_data);
    ACE_UNUSED_ARG (info_seq);
    ACE_UNUSED_ARG (max_samples);
    ACE_UNUSED_ARG (sample_states);
    ACE_UNUSED_ARG (view_states);
    ACE_UNUSED_ARG (instance_states);
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
  {
    ACE_UNUSED_ARG (received_data);
    ACE_UNUSED_ARG (info_seq);
    ACE_UNUSED_ARG (max_samples);
    ACE_UNUSED_ARG (sample_states);
    ACE_UNUSED_ARG (view_states);
    ACE_UNUSED_ARG (instance_states);

    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::read_next_sample (
    Foo & received_data,
    ::DDS::SampleInfo & sample_info
  )
  {

    ACE_UNUSED_ARG (received_data);
    ACE_UNUSED_ARG (sample_info);

    FILE* fp =
      ACE_OS::fopen ("Foo.txt", ACE_LIB_TEXT("r"));
    if (fp == 0)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("(%P|%t) ")
                         ACE_TEXT("FooDataReaderImpl::read_next_sample, ")
                         ACE_TEXT("Unable to open Foo.txt for reading: %p\n"),
                         "fopen"),
                         ::DDS::RETCODE_ERROR);
    }

    if( ACE_OS::fsetpos( fp, &pos_ ) != 0 )
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT("(%P|%t) ")
                         ACE_TEXT("FooDataReaderImpl::read_next_sample, ")
                         ACE_TEXT("Unable to open Foo.txt for reading: %p\n"),
                         "fsetpos"),
                         ::DDS::RETCODE_ERROR);
    }

    fscanf (fp, "%d %f %f\n", &received_data.key, &received_data.x, &received_data.y);
    fgetpos( fp, &pos_ );
    ACE_OS::fclose (fp);

    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::take_next_sample (
    Foo & received_data,
    ::DDS::SampleInfo & sample_info
  )
  {
    ACE_UNUSED_ARG (received_data);
    ACE_UNUSED_ARG (sample_info);
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
  {
    ACE_UNUSED_ARG (received_data);
    ACE_UNUSED_ARG (info_seq);
    ACE_UNUSED_ARG (max_samples);
    ACE_UNUSED_ARG (a_handle);
    ACE_UNUSED_ARG (sample_states);
    ACE_UNUSED_ARG (view_states);
    ACE_UNUSED_ARG (instance_states);

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
  {
    ACE_UNUSED_ARG (received_data);
    ACE_UNUSED_ARG (info_seq);
    ACE_UNUSED_ARG (max_samples);
    ACE_UNUSED_ARG (a_handle);
    ACE_UNUSED_ARG (sample_states);
    ACE_UNUSED_ARG (view_states);
    ACE_UNUSED_ARG (instance_states);
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
  {
    ACE_UNUSED_ARG (received_data);
    ACE_UNUSED_ARG (info_seq);
    ACE_UNUSED_ARG (max_samples);
    ACE_UNUSED_ARG (a_handle);
    ACE_UNUSED_ARG (sample_states);
    ACE_UNUSED_ARG (view_states);
    ACE_UNUSED_ARG (instance_states);
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
  {
    ACE_UNUSED_ARG (received_data);
    ACE_UNUSED_ARG (info_seq);
    ACE_UNUSED_ARG (max_samples);
    ACE_UNUSED_ARG (a_handle);
    ACE_UNUSED_ARG (sample_states);
    ACE_UNUSED_ARG (view_states);
    ACE_UNUSED_ARG (instance_states);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::return_loan (
    FooSeq & received_data,
    ::DDS::SampleInfoSeq & info_seq
  )
  {
    ACE_UNUSED_ARG (received_data);
    ACE_UNUSED_ARG (info_seq);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

::DDS::ReturnCode_t FooDataReaderImpl::get_key_value (
    Foo & key_holder,
    ::DDS::InstanceHandle_t handle
  )
  {
    ACE_UNUSED_ARG (key_holder);
    ACE_UNUSED_ARG (handle);
    // Add your implementation here
    return ::DDS::RETCODE_OK;
  }

