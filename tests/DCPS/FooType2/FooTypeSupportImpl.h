// -*- C++ -*-
// ============================================================================
/**
 *  @file   FooTypeSupportImpl.h
 *
 *
 *
 */
// ============================================================================


#ifndef FOOTYPESUPPORTIMPL_H_
#define FOOTYPESUPPORTIMPL_H_

#include "FooTypeS.h"
#include "footype_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

/**
 * @class FooTypeSupportImpl
 *
 * @brief An implementation of a TypeSupport
 *
 */
class FooType_Export FooTypeSupportImpl : public virtual POA_FooTypeSupport
{
public:
  FooTypeSupportImpl (void);

  virtual ~FooTypeSupportImpl (void);

  virtual ::DDS::ReturnCode_t register_type (
      ::DDS::DomainParticipant_ptr participant,
      const char * type_name
    );

  virtual ::OpenDDS::DCPS::DataWriterRemote_ptr create_datawriter (
    );

  virtual ::OpenDDS::DCPS::DataReaderRemote_ptr create_datareader (
    );
};



#endif /* FOOTYPESUPPORTIMPL_H_  */
