// -*- C++ -*-
// ============================================================================
/**
 *  @file   FooDataReaderImpl.h
 *
 *
 *
 */
// ============================================================================



#ifndef FOODATAREADERIMPL_H_
#define FOODATAREADERIMPL_H_

#include "FooTypeS.h"
#include "footype_export.h"
#include "dds/DCPS/DataReaderImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */



/**
 * @class TAO_DDS_DCPSInfo_i
 *
 * @brief Implementation of the DCPSInfo
 *
 */
class FooType_Export FooDataReaderImpl : public virtual POA_FooDataReader,
  public virtual OpenDDS::DCPS::DataReaderImpl
{
public:
  FooDataReaderImpl (void);

  virtual ~FooDataReaderImpl (void);

  virtual ::DDS::ReturnCode_t read (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
    );

  virtual ::DDS::ReturnCode_t take (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
    );

  virtual ::DDS::ReturnCode_t read_next_sample (
      Foo & received_data,
      ::DDS::SampleInfo & sample_info
    );

  virtual ::DDS::ReturnCode_t take_next_sample (
      Foo & received_data,
      ::DDS::SampleInfo & sample_info
    );

  virtual ::DDS::ReturnCode_t read_instance (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::InstanceHandle_t a_handle,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
    );

  virtual ::DDS::ReturnCode_t take_instance (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::InstanceHandle_t a_handle,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
    );

  virtual ::DDS::ReturnCode_t read_next_instance (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::InstanceHandle_t a_handle,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
    );

  virtual ::DDS::ReturnCode_t take_next_instance (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::InstanceHandle_t a_handle,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
    );

  virtual ::DDS::ReturnCode_t return_loan (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq
    );

  virtual ::DDS::ReturnCode_t get_key_value (
      Foo & key_holder,
      ::DDS::InstanceHandle_t handle
    );

  private:
    fpos_t pos_;
};


#endif /* FOODATAREADERIMPL_H_  */
