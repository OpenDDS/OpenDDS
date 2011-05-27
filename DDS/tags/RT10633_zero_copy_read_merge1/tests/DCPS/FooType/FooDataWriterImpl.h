// -*- C++ -*-
// ============================================================================
/**
 *  @file   FooDataWriterImpl.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================



#ifndef FOODATAWRITERIMPL_H_
#define FOODATAWRITERIMPL_H_

#include "FooTypeS.h"
#include "footype_export.h"
#include "dds/DCPS/DataWriterImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


/**
 * @class FooDataWriterImpl
 *
 * @brief Implementation of the FooDataWriter
 *
 */
class FooType_Export FooDataWriterImpl : public virtual POA_FooDataWriter,
                                         public virtual TAO::DCPS::DataWriterImpl
{
public:
  virtual ::DDS::ReturnCode_t enable_specific (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
        )) {return ::DDS::RETCODE_OK;};

  //Constructor
  FooDataWriterImpl (void);

  //Destructor
  virtual ~FooDataWriterImpl (void);


  virtual ::DDS::InstanceHandle_t _cxx_register (
      const Foo & instance_data
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::InstanceHandle_t register_w_timestamp (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t handle,
      const ::DDS::Time_t & source_timestamp
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t unregister (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t handle
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t unregister_w_timestamp (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t handle,
      const ::DDS::Time_t & source_timestamp
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t write (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t handle
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t write_w_timestamp (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t handle,
      const ::DDS::Time_t & source_timestamp
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t dispose (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t instance_handle
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t dispose_w_timestamp (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t instance_handle,
      const ::DDS::Time_t & source_timestamp
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::DDS::ReturnCode_t get_key_value (
      Foo & key_holder,
      ::DDS::InstanceHandle_t handle
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));
};



#endif /* FOODATAWRITERIMPL_H_  */
