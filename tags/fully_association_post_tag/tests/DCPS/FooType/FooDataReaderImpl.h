// -*- C++ -*-
// ============================================================================
/**
 *  @file   DCPSInfo_i.h
 *
 *  $Id$
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
  public virtual TAO::DCPS::DataReaderImpl
{
public:
  virtual ::DDS::ReturnCode_t enable_specific (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
        )) { return ::DDS::RETCODE_OK;};

  //Constructor 
  FooDataReaderImpl (void);
  
  //Destructor 
  virtual ~FooDataReaderImpl (void);
  

  virtual ::DDS::ReturnCode_t read (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t take (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t read_next_sample (
      Foo & received_data,
      ::DDS::SampleInfo & sample_info
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t take_next_sample (
      Foo & received_data,
      ::DDS::SampleInfo & sample_info
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t read_instance (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::InstanceHandle_t a_handle,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t take_instance (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::InstanceHandle_t a_handle,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t read_next_instance (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::InstanceHandle_t a_handle,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t take_next_instance (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq,
      CORBA::Long max_samples,
      ::DDS::InstanceHandle_t a_handle,
      ::DDS::SampleStateMask sample_states,
      ::DDS::ViewStateMask view_states,
      ::DDS::InstanceStateMask instance_states
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t return_loan (
      FooSeq & received_data,
      ::DDS::SampleInfoSeq & info_seq
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t get_key_value (
      Foo & key_holder,
      ::DDS::InstanceHandle_t handle
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

};


#endif /* FOODATAREADERIMPL_H_  */
