// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h"

#include  "Registered_Data_Types.h"
#include "ace/Unbounded_Set.h"
#include "ace/SString.h"

namespace TAO
{
  namespace DCPS
  {

    Data_Types_Register::Data_Types_Register(void)
    {
    }


    Data_Types_Register::~Data_Types_Register(void)
    {
      TypeSupportHash*  supportHash;
      ACE_Unbounded_Set<POA_TAO::DCPS::TypeSupport_ptr> typeSupports;

      if (0 < domains_.current_size() )
        {
          DomainHash::ITERATOR domainIter = domains_.begin();
          DomainHash::ITERATOR domainEnd = domains_.end();

          while (domainIter != domainEnd)
            {
              supportHash = (*domainIter).int_id_;
              ++domainIter;

              if (0 < supportHash->current_size() )
                {
                  TypeSupportHash::ITERATOR supportIter = supportHash->begin();
                  TypeSupportHash::ITERATOR supportEnd = supportHash->end();

                  while (supportIter != supportEnd)
                    {
                      // ignore the error of adding a duplicate pointer.
                      // this done to handle a pointer having been registered
                      // to multiple names.
                      typeSupports.insert( (*supportIter).int_id_);
                      ++supportIter;

                    } /* while (supportIter != supportEnd) */

                } /* if (0 < supportHash->current_size() ) */

              supportHash->unbind_all();
              delete supportHash;

            } /* while (domainIter != domainEnd) */

          domains_.unbind_all();

          ACE_Unbounded_Set<POA_TAO::DCPS::TypeSupport_ptr>::ITERATOR typesIter = 
            typeSupports.begin();
          ACE_Unbounded_Set<POA_TAO::DCPS::TypeSupport_ptr>::ITERATOR typesEnd = 
            typeSupports.end();

          while (typesEnd != typesIter)
            {
              POA_TAO::DCPS::TypeSupport_ptr type = *typesIter;
              ++typesIter;
              // if there are no more references then it will be deleted
              type->_remove_ref();
            }

          typeSupports.reset();
        } /* if (0 < domains_.current_size() ) */

    }


    ::DDS::ReturnCode_t Data_Types_Register::register_type (
      ::DDS::DomainParticipant_ptr domain_participant,
      ACE_CString type_name,
      POA_TAO::DCPS::TypeSupport_ptr the_type)
    {
      ::DDS::ReturnCode_t retCode = ::DDS::RETCODE_ERROR;
      TypeSupportHash*  supportHash;

      if (0 == domains_.find(reinterpret_cast <CORBA::Long> (domain_participant), supportHash))
        {
          int lookup = supportHash->bind(type_name, the_type);

          if (0 == lookup)
            {
              the_type->_add_ref();
              retCode = ::DDS::RETCODE_OK;
            }
          else if (1 == lookup)
            {
              POA_TAO::DCPS::TypeSupport_ptr currentType;
              if ( 0 == supportHash->find(type_name, currentType) )
                {
                  if (the_type == currentType)
                    {
                      retCode = ::DDS::RETCODE_OK;
                    } /* if (the_type == currentType) */
                } /* if ( 0 == supportHash->find(type_name, currentType) ) */
            } /* else if (1 == lookup) */
        }
      else
        {
          // new domain id!
          supportHash = new TypeSupportHash;

          if (0 == domains_.bind(reinterpret_cast<CORBA::Long> (domain_participant), supportHash))
            {
              if (0 == supportHash->bind(type_name, the_type))
                {
                  the_type->_add_ref();
                  retCode = ::DDS::RETCODE_OK;
                }
            }
          else
            {
              delete supportHash;
            } /* if (0 == domains_.bind(domain_participant, supportHash)) */
        } /* if (0 == domains_.find(domain_participant, supportHash)) */

      return retCode;
    }


    POA_TAO::DCPS::TypeSupport_ptr Data_Types_Register::lookup(
      ::DDS::DomainParticipant_ptr domain_participant,
      ACE_CString type_name)
    {
      POA_TAO::DCPS::TypeSupport_ptr typeSupport = 0;

      TypeSupportHash*  supportHash;

      if (0 == domains_.find(reinterpret_cast<CORBA::Long> (domain_participant), supportHash))
        {
          if (0 != supportHash->find(type_name, typeSupport))
            {
              // reassign to nil to make sure that there was no partial
              // assignment in the find.
              typeSupport = 0;
            }
        }
      return typeSupport;
    }

  }

}



// gcc on AIX needs explicit instantiation of the singleton templates
#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION) || (defined (__GNUC__) && defined (_AIX))

template class TAO_Singleton<Data_Types_Register, TAO_SYNCH_MUTEX>;

template class ACE_Hash_Map_Manager<ACE_CString, POA_TAO::DCPS::TypeSupport_ptr, ACE_SYNCH_RECURSIVE_MUTEX>;  
template class ACE_Hash_Map_Iterator <ACE_CString, POA_TAO::DCPS::TypeSupport_ptr, ACE_SYNCH_RECURSIVE_MUTEX>;
template class ACE_Hash_Map_Entry<ACE_CString, POA_TAO::DCPS::TypeSupport_ptr>;

template class ACE_Hash_Map_Manager<::DDS::DomainId_t, TypeSupportHash*, ACE_NULL_SYNCH>;  
template class ACE_Hash_Map_Iterator <::DDS::DomainId_t, TypeSupportHash*, ACE_NULL_SYNCH>;
template class ACE_Hash_Map_Entry<::DDS::DomainId_t, TypeSupportHash*>;

template class ACE_Node<POA_TAO::DCPS::TypeSupport_ptr>;
template class ACE_Unbounded_Set<POA_TAO::DCPS::TypeSupport_ptr>;
template class ACE_Unbounded_Set_Iterator<POA_TAO::DCPS::TypeSupport_ptr>;

#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate TAO_Singleton<Data_Types_Register, TAO_SYNCH_MUTEX>

#pragma instantiate ACE_Hash_Map_Manager<ACE_CString, POA_TAO::DCPS::TypeSupport_ptr, ACE_SYNCH_RECURSIVE_MUTEX>;  
#pragma instantiate ACE_Hash_Map_Iterator <ACE_CString, POA_TAO::DCPS::TypeSupport_ptr, ACE_SYNCH_RECURSIVE_MUTEX>;
#pragma instantiate ACE_Hash_Map_Entry<ACE_CString, POA_TAO::DCPS::TypeSupport_ptr>;

#pragma instantiate ACE_Hash_Map_Manager<::DDS::DomainId_t, TypeSupportHash*, ACE_SYNCH_RECURSIVE_MUTEX>;  
#pragma instantiate ACE_Hash_Map_Iterator <::DDS::DomainId_t, TypeSupportHash*, ACE_SYNCH_RECURSIVE_MUTEX>;
#pragma instantiate ACE_Hash_Map_Entry<::DDS::DomainId_t, TypeSupportHash*>;

#pragma instantiate ACE_Node<POA_TAO::DCPS::TypeSupport_ptr>
#pragma instantiate ACE_Unbounded_Set<POA_TAO::DCPS::TypeSupport_ptr>
#pragma instantiate ACE_Unbounded_Set_Iterator<POA_TAO::DCPS::TypeSupport_ptr>

#endif /*ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */

