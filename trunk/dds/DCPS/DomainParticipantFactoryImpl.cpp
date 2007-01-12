// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h"
#include  "DomainParticipantFactoryImpl.h"
#include  "DomainParticipantImpl.h"
#include  "dds/DdsDcpsInfoC.h"
#include  "Service_Participant.h"
#include  "Qos_Helper.h"
#include  "tao/debug.h"

namespace TAO
{
  namespace DCPS
  {
    // Implementation skeleton constructor
    DomainParticipantFactoryImpl::DomainParticipantFactoryImpl (void)
    : default_participant_qos_ (TheServiceParticipant->initial_DomainParticipantQos ())
    {
    }
      
    // Implementation skeleton destructor
    DomainParticipantFactoryImpl::~DomainParticipantFactoryImpl (void)
    {
    }
      
    ::DDS::DomainParticipant_ptr 
    DomainParticipantFactoryImpl::create_participant (
        ::DDS::DomainId_t domainId,
        const ::DDS::DomainParticipantQos & qos,
        ::DDS::DomainParticipantListener_ptr a_listener
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      if (! Qos_Helper::valid (qos))
        {
          ACE_ERROR ((LM_ERROR, 
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                      ACE_TEXT("invalid qos.\n")));
          return ::DDS::DomainParticipant::_nil();
        }

      if (! Qos_Helper::consistent (qos))
        {
          ACE_ERROR ((LM_ERROR,  
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                      ACE_TEXT("inconsistent qos.\n")));
          return ::DDS::DomainParticipant::_nil();
        }

      DCPSInfo_var repo = TheServiceParticipant->get_repository ();

      RepoId dp_id = 0;

      ACE_TRY  
        { 
          dp_id = repo->add_domain_participant (domainId, 
                                                qos 
                                                ACE_ENV_ARG_PARAMETER);
          ACE_TRY_CHECK;
        }
      ACE_CATCH (CORBA::SystemException, sysex)
        {
          ACE_PRINT_EXCEPTION (sysex, 
                              "ERROR: System Exception"
                              " in DomainParticipantFactoryImpl::create_participant");
          return ::DDS::DomainParticipant::_nil();
        }
      ACE_CATCH (CORBA::UserException, userex)
        {
          ACE_PRINT_EXCEPTION (userex, 
                              "ERROR: User Exception"
                              " in DomainParticipantFactoryImpl::create_participant");
          return ::DDS::DomainParticipant::_nil();
        }
      ACE_ENDTRY;

      if (dp_id == 0)
        {
          ACE_ERROR ((LM_ERROR, 
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                      ACE_TEXT("add_domain_participant returned invalid id.\n")));
          return ::DDS::DomainParticipant::_nil ();
        }

      DomainParticipantImpl* dp;
      
      ACE_NEW_RETURN (dp,
                      DomainParticipantImpl(domainId, dp_id, qos, a_listener),
                      ::DDS::DomainParticipant::_nil ());

      ::DDS::DomainParticipant_ptr dp_obj 
        = servant_to_reference (dp ACE_ENV_ARG_PARAMETER); 
      
      ACE_CHECK_RETURN(::DDS::DomainParticipant::_nil ());

      // Give ownership to poa. 
      dp->_remove_ref ();

      if (CORBA::is_nil (dp_obj))
        {
          ACE_ERROR ((LM_ERROR, 
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                      ACE_TEXT("nil DomainParticipant.\n")));
          return ::DDS::DomainParticipant::_nil ();
        }


      // Set the participant object reference before enable since it's
      // needed for the built in topics during enable.
      dp->set_object_reference (dp_obj);

      // There is no qos policy in the DomainParticipantFactory, the DomainParticipant
      // defaults to enabled.
      dp->enable (ACE_ENV_SINGLE_ARG_PARAMETER);
      ACE_CHECK_RETURN(::DDS::DomainParticipant::_nil ());

      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, 
                        tao_mon,
                        this->participants_protector_,
                        ::DDS::DomainParticipant::_nil ());
      
      Participant_Pair pair (dp, dp_obj, NO_DUP);

      DPMap_Entry* entry;
      if (participants_.find (domainId, entry) == -1) 
        {
          DPSet set;

          if (set.insert (pair) == -1)
            {
              ACE_ERROR ((LM_ERROR, 
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                          ACE_TEXT(" %p.\n"),
                          ACE_TEXT("insert"))); 
              return ::DDS::DomainParticipant::_nil ();
            }

          if (participants_.bind (domainId, set)  == -1) 
            {
              ACE_ERROR ((LM_ERROR, 
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                          ACE_TEXT(" %p.\n"),
                          ACE_TEXT("bind"))); 
              return ::DDS::DomainParticipant::_nil ();
            }
        }
      else 
        {
          if (entry->int_id_.insert (pair) == -1)
            {
              ACE_ERROR ((LM_ERROR, 
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                          ACE_TEXT(" %p.\n"),
                          ACE_TEXT("insert")));
              return ::DDS::DomainParticipant::_nil ();
            }
        }
      
      // Increase ref count when the servant is referenced by the 
      // map.
      dp->_add_ref ();

      return ::DDS::DomainParticipant::_duplicate(dp_obj);
    }
      

    ::DDS::ReturnCode_t 
    DomainParticipantFactoryImpl::delete_participant (
        ::DDS::DomainParticipant_ptr a_participant
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {

      if (CORBA::is_nil(a_participant))
        {
          ACE_ERROR_RETURN ((LM_ERROR, 
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                            ACE_TEXT("Nil participant.\n")),
                            ::DDS::RETCODE_BAD_PARAMETER);
        }
      
      // The servant's ref count should be 2 at this point, one referenced
      // by the poa and the other referenced by the map.
      DomainParticipantImpl* the_servant 
        = reference_to_servant<DomainParticipantImpl, ::DDS::DomainParticipant_ptr> 
            (a_participant ACE_ENV_ARG_PARAMETER);
      ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);
      
      if (the_servant->is_clean () == 0)
        {
          ACE_ERROR_RETURN ((LM_ERROR, 
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                            ACE_TEXT("The participant(repo_id=%d) is not empty.\n"),
                            the_servant->get_id ()),
                            ::DDS::RETCODE_PRECONDITION_NOT_MET);
        }

      ::DDS::DomainId_t domain_id = the_servant->get_domain_id (ACE_ENV_SINGLE_ARG_PARAMETER);
      RepoId dp_id = the_servant->get_id (ACE_ENV_SINGLE_ARG_PARAMETER);

      DPMap_Entry* entry;
      if (participants_.find (domain_id, entry) == -1) 
        {
          ACE_ERROR_RETURN ((LM_ERROR, 
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                            ACE_TEXT("%p domain_id=%d dp_id=%d.\n"),
                            ACE_TEXT("find"), domain_id, dp_id),
                            ::DDS::RETCODE_ERROR);
        }
      else 
        {
          ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, 
                            tao_mon,
                            this->participants_protector_,
                            ::DDS::RETCODE_ERROR);

          DPMap_Entry* entry;
          if (participants_.find (domain_id, entry) == -1) 
            {
              ACE_ERROR_RETURN ((LM_ERROR, 
                                ACE_TEXT("(%P|%t) ERROR: ")
                                ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                                ACE_TEXT(" %p domain_id=%d dp_id=%d\n"),
                                ACE_TEXT("find"), 
                                domain_id, 
                                dp_id),
                                ::DDS::RETCODE_ERROR);
            }
          else 
            {
              ::DDS::ReturnCode_t result 
                = the_servant->delete_contained_entities (ACE_ENV_SINGLE_ARG_PARAMETER);
                  
              ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);

              if (result != ::DDS::RETCODE_OK)
                {
                  return result;
                }

              Participant_Pair pair (the_servant, a_participant, DUP);

              if (entry->int_id_.remove (pair) == -1) 
                {
                  ACE_ERROR_RETURN ((LM_ERROR, 
                                    ACE_TEXT("(%P|%t) ERROR: ")
                                    ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                                    ACE_TEXT(" %p.\n"),
                                    ACE_TEXT("remove")), 
                                    ::DDS::RETCODE_ERROR);
                }
              
              if (entry->int_id_.is_empty ())
                {
                  if (participants_.unbind (entry->ext_id_) == -1)
                    {
                      ACE_ERROR_RETURN ((LM_ERROR, 
                                        ACE_TEXT("(%P|%t) ERROR: ")
                                        ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                                        ACE_TEXT(" %p.\n"),
                                        ACE_TEXT("unbind")), 
                                        ::DDS::RETCODE_ERROR);
                    }
                }
            }
        }

      DCPSInfo_var repo = TheServiceParticipant->get_repository();

      ACE_TRY  
        { 
          repo->remove_domain_participant (domain_id, 
                                           dp_id 
                                           ACE_ENV_ARG_PARAMETER);
          ACE_TRY_CHECK;
        }
      ACE_CATCH (CORBA::SystemException, sysex)
        {
          ACE_PRINT_EXCEPTION (sysex, 
                              "ERROR: System Exception"
                              " in DomainParticipantFactoryImpl::delete_participant");
          return ::DDS::RETCODE_ERROR;
        }
      ACE_CATCH (CORBA::UserException, userex)
        {
          ACE_PRINT_EXCEPTION (userex, 
                              "ERROR: User Exception"
                              " in DomainParticipantFactoryImpl::delete_participant");
          return ::DDS::RETCODE_ERROR;
        }
      ACE_ENDTRY;

      // Decrease ref count when the servant is removed from map.
      the_servant->_remove_ref ();  
      
      deactivate_object < ::DDS::DomainParticipant_ptr > (a_participant);

      return ::DDS::RETCODE_OK;
    }

      
    ::DDS::DomainParticipant_ptr
    DomainParticipantFactoryImpl::lookup_participant (
        ::DDS::DomainId_t domainId
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, 
                        tao_mon,
                        this->participants_protector_,
                        ::DDS::DomainParticipant::_nil ());

      DPMap_Entry* entry;
      if (participants_.find (domainId, entry) == -1) 
        {
          if (DCPS_debug_level >= 1) 
            {
              ACE_DEBUG ((LM_DEBUG, 
                          ACE_TEXT("(%P|%t) ")
                          ACE_TEXT("DomainParticipantFactoryImpl::lookup_participant, ")
                          ACE_TEXT(" not found for domain %d.\n"),
                          domainId));
            }
          return ::DDS::DomainParticipant::_nil ();
        }
      else 
        {
          // No specification about which participant will return. We just return the first
          // object.
          // Note: We are not duplicate the object ref, so a delete call is not needed.
          return ::DDS::DomainParticipant::_duplicate ((*(entry->int_id_.begin ())).obj_.in ());
        }
    }
      
    ::DDS::ReturnCode_t
    DomainParticipantFactoryImpl::set_default_participant_qos (
        const ::DDS::DomainParticipantQos & qos
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      if (Qos_Helper::valid (qos) 
          && Qos_Helper::consistent (qos)) 
        {
          default_participant_qos_ = qos;
          return ::DDS::RETCODE_OK;
        }
      else 
        {
          return ::DDS::RETCODE_INCONSISTENT_POLICY;
        }
    }
      
    void DomainParticipantFactoryImpl::get_default_participant_qos (
        ::DDS::DomainParticipantQos & qos
        ACE_ENV_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      qos = default_participant_qos_;
    }
      
    ::DDS::ReturnCode_t
    DomainParticipantFactoryImpl::delete_contained_participants ()
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, 
                        tao_mon,
                        this->participants_protector_,
                        ::DDS::RETCODE_ERROR);

      DPMap_Iterator mapIter(participants_);
      
      for (DPMap_Entry* entry = 0; 
          mapIter.next (entry) != 0; 
          mapIter.advance ())
        {
          
          DPSet_Iterator setIter(entry->int_id_);
          for (Participant_Pair* e = 0; 
              setIter.next (e) != 0; 
              setIter.advance ())
            {
              ::DDS::ReturnCode_t result = delete_participant ((*e).obj_.in ());
              if (result != ::DDS::RETCODE_OK)
                {
                  return result;  
                }          
            }
        }

      return ::DDS::RETCODE_OK;
    }

    ::DDS::DomainParticipantFactory_ptr 
    DomainParticipantFactoryImpl::get_instance (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      return TheParticipantFactory;
    }


 } // namespace DCPS
} // namespace TAO


#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

template class ACE_Unbounded_Set <::DDS::DomainParticipant_ptr>;
template class ACE_Unbounded_Set_Iterator <::DDS::DomainParticipant_ptr>;
template class ACE_Hash_Map_Manager<::DDS::DomainId_t, DPSet, ACE_NULL_SYNCH>;  
template class ACE_Hash_Map_Iterator <::DDS::DomainId_t, DPSet, ACE_NULL_SYNCH>;
template class ACE_Hash_Map_Entry<::DDS::DomainId_t, DPSet>;


#elif defined(ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate ACE_Unbounded_Set <::DDS::DomainParticipant_ptr>;
#pragma instantiate ACE_Unbounded_Set_Iterator <::DDS::DomainParticipant_ptr>;
#pragma instantiate ACE_Hash_Map_Manager<::DDS::DomainId_t, DPSet, ACE_NULL_SYNCH>;  
#pragma instantiate ACE_Hash_Map_Iterator <::DDS::DomainId_t, DPSet, ACE_NULL_SYNCH>;
#pragma instantiate ACE_Hash_Map_Entry<::DDS::DomainId_t, DPSet>;

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
