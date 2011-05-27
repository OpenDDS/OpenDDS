// -*- C++ -*-
// ============================================================================
/**
 *  @file   FooTypeSupportImpl.h
 *
 *  $Id$
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
class FooType_Export FooTypeSupportImpl 
: public virtual POA_FooTypeSupport,
  public virtual PortableServer::RefCountServantBase
{
public:
  //Constructor 
  FooTypeSupportImpl (void);
  
  //Destructor 
  virtual ~FooTypeSupportImpl (void);
  

  virtual ::DDS::ReturnCode_t register_type (
      ::DDS::DomainParticipant_ptr participant,
      const char * type_name
      ACE_ENV_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual
  char * get_type_name (
      ACE_ENV_SINGLE_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::TAO::DCPS::DataWriterRemote_ptr create_datawriter (
      ACE_ENV_SINGLE_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual ::TAO::DCPS::DataReaderRemote_ptr create_datareader (
      ACE_ENV_SINGLE_ARG_DECL
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));
};



#endif /* FOOTYPESUPPORTIMPL_H_  */
