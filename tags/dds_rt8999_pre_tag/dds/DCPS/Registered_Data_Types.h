// -*- C++ -*-
//
// $Id$
#ifndef REGISTERED_DATA_TYPES_H_
#define REGISTERED_DATA_TYPES_H_

#include "dcps_export.h"
#include "dds/DdsDcpsInfrastructureS.h"
#include "dds/DdsDcpsTopicS.h"
#include "tao/TAO_Singleton.h"
#include "dds/DdsDcpsTypeSupportTaoS.h"

#include "ace/Hash_Map_Manager_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO
{
  namespace DCPS
  {
    typedef ACE_Hash_Map_Manager<ACE_CString, POA_TAO::DCPS::TypeSupport_ptr, ACE_SYNCH_RECURSIVE_MUTEX> TypeSupportHash;
    typedef ACE_Hash_Map_Manager<void*, TypeSupportHash*, ACE_SYNCH_RECURSIVE_MUTEX> DomainHash;

    /**
    * A singleton class that keeps track of the registered DDS data types
    * local to this process.
    * Data types are split into separate domains.
    */
    class TAO_DdsDcps_Export Data_Types_Register
    {
      friend class TAO_Singleton<Data_Types_Register, TAO_SYNCH_MUTEX>;

    public:

      /**
       * Register a type.
       *
       * @note This class takes Ownership of the memory pointed to by the_type
       *  when this method returns RETCODE_OK  It does this by calling
       *  _add_ref() on the TypeSupport_ptr
       *
       * @returns RETCODE_OK if the type_name is unique to the domain participant
       *         or the type_name is already registered to the_type.
       *         Otherwise returns RETCODE_ERROR
       */ 
      ::DDS::ReturnCode_t register_type (::DDS::DomainParticipant_ptr domain_participant,
                                         ACE_CString type_name,
                                         POA_TAO::DCPS::TypeSupport_ptr the_type);

      /**
       * Find a data type by its type name.
       * @note This class retains Ownership of the memory returned
       * @returns a pointer to the memory registered to the 
       *         type_name
       *         Otherwise returns TypeSupport::_nil()
       */
      POA_TAO::DCPS::TypeSupport_ptr lookup(::DDS::DomainParticipant_ptr domain_participant,
                                      ACE_CString type_name);
    private:
      Data_Types_Register(void);
      ~Data_Types_Register(void);

      DomainHash domains_;
    };

    typedef TAO_Singleton<Data_Types_Register, TAO_SYNCH_MUTEX> DATA_TYPES_REGISTER;

    #if (__GNUC__ > 3)
    #else
    TAO_DDSDCPS_SINGLETON_DECLARE (::TAO_Singleton, Data_Types_Register, TAO_SYNCH_MUTEX)  
    #endif

    #define Registered_Data_Types DATA_TYPES_REGISTER::instance()

  }
}


#endif /* REGISTERED_DATA_TYPES_H_  */
