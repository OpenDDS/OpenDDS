// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Registered_Data_Types.h"

#include "tao/TAO_Singleton.h"

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
      ACE_Unbounded_Set<TAO::DCPS::TypeSupport_ptr> typeSupports;

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

          ACE_Unbounded_Set<TAO::DCPS::TypeSupport_ptr>::ITERATOR typesIter =
            typeSupports.begin();
          ACE_Unbounded_Set<TAO::DCPS::TypeSupport_ptr>::ITERATOR typesEnd =
            typeSupports.end();

          while (typesEnd != typesIter)
            {
              TAO::DCPS::TypeSupport_ptr type = *typesIter;
              ++typesIter;
              // if there are no more references then it will be deleted
              type->_remove_ref();
            }

          typeSupports.reset();
        } /* if (0 < domains_.current_size() ) */

    }

    Data_Types_Register*
    Data_Types_Register::instance (void)
    {
      // Hide the template instantiation to prevent multiple instances
      // from being created.

      return
	TAO_Singleton<Data_Types_Register, TAO_SYNCH_MUTEX>::instance ();
    }



    ::DDS::ReturnCode_t Data_Types_Register::register_type (
      ::DDS::DomainParticipant_ptr domain_participant,
      ACE_CString type_name,
      TAO::DCPS::TypeSupport_ptr the_type)
    {
      ::DDS::ReturnCode_t retCode = ::DDS::RETCODE_ERROR;
      TypeSupportHash*  supportHash = NULL;

      if (0 == domains_.find(reinterpret_cast <void*> (domain_participant), supportHash))
        {
          int lookup = supportHash->bind(type_name, the_type);

          if (0 == lookup)
            {
              the_type->_add_ref();
              retCode = ::DDS::RETCODE_OK;
            }
          else if (1 == lookup)
            {
              TAO::DCPS::TypeSupport_ptr currentType = NULL;
              if ( 0 == supportHash->find(type_name, currentType) )
                {
                  // Allow different TypeSupport instances of the same TypeSupport
                  // type register with the same type name in the same
                  // domain pariticipant. The second (and subsequent) registrations
                  // will be ignored.
                  CORBA::String_var the_type_name = the_type->get_type_name();
                  CORBA::String_var current_type_name = currentType->get_type_name();
                  if (ACE_OS::strcmp (the_type_name.in (), current_type_name.in ()) == 0)
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

          if (0 == domains_.bind(reinterpret_cast<void*> (domain_participant), supportHash))
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


    TAO::DCPS::TypeSupport_ptr Data_Types_Register::lookup(
      ::DDS::DomainParticipant_ptr domain_participant,
      ACE_CString type_name)
    {
      TAO::DCPS::TypeSupport_ptr typeSupport = 0;

      TypeSupportHash*  supportHash = NULL;

      if (0 == domains_.find(reinterpret_cast<void*> (domain_participant), supportHash))
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

template class ACE_Hash_Map_Manager<ACE_CString, TAO::DCPS::TypeSupport_ptr, ACE_SYNCH_RECURSIVE_MUTEX>;
template class ACE_Hash_Map_Iterator <ACE_CString, TAO::DCPS::TypeSupport_ptr, ACE_SYNCH_RECURSIVE_MUTEX>;
template class ACE_Hash_Map_Entry<ACE_CString, TAO::DCPS::TypeSupport_ptr>;

template class ACE_Hash_Map_Manager<::DDS::DomainId_t, TypeSupportHash*, ACE_NULL_SYNCH>;
template class ACE_Hash_Map_Iterator <::DDS::DomainId_t, TypeSupportHash*, ACE_NULL_SYNCH>;
template class ACE_Hash_Map_Entry<::DDS::DomainId_t, TypeSupportHash*>;

template class ACE_Node<TAO::DCPS::TypeSupport_ptr>;
template class ACE_Unbounded_Set<TAO::DCPS::TypeSupport_ptr>;
template class ACE_Unbounded_Set_Iterator<TAO::DCPS::TypeSupport_ptr>;

#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate TAO_Singleton<Data_Types_Register, TAO_SYNCH_MUTEX>

#pragma instantiate ACE_Hash_Map_Manager<ACE_CString, TAO::DCPS::TypeSupport_ptr, ACE_SYNCH_RECURSIVE_MUTEX>;
#pragma instantiate ACE_Hash_Map_Iterator <ACE_CString, TAO::DCPS::TypeSupport_ptr, ACE_SYNCH_RECURSIVE_MUTEX>;
#pragma instantiate ACE_Hash_Map_Entry<ACE_CString, TAO::DCPS::TypeSupport_ptr>;

#pragma instantiate ACE_Hash_Map_Manager<::DDS::DomainId_t, TypeSupportHash*, ACE_SYNCH_RECURSIVE_MUTEX>;
#pragma instantiate ACE_Hash_Map_Iterator <::DDS::DomainId_t, TypeSupportHash*, ACE_SYNCH_RECURSIVE_MUTEX>;
#pragma instantiate ACE_Hash_Map_Entry<::DDS::DomainId_t, TypeSupportHash*>;

#pragma instantiate ACE_Node<TAO::DCPS::TypeSupport_ptr>
#pragma instantiate ACE_Unbounded_Set<TAO::DCPS::TypeSupport_ptr>
#pragma instantiate ACE_Unbounded_Set_Iterator<TAO::DCPS::TypeSupport_ptr>

#endif /*ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
