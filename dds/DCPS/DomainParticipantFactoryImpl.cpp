// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DomainParticipantFactoryImpl.h"
#include "DomainParticipantImpl.h"
#include "dds/DdsDcpsInfoC.h"
#include "Service_Participant.h"
#include "Qos_Helper.h"
#include "Util.h"
#include "tao/debug.h"

namespace OpenDDS
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

      DCPSInfo_var repo = TheServiceParticipant->get_repository ( domainId);
      if ( CORBA::is_nil( repo.in()))
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                      ACE_TEXT("no repository found for domainId: %d.\n"), domainId));
          return ::DDS::DomainParticipant::_nil();
        }

      RepoId dp_id = 0;

      try
        {
          dp_id = repo->add_domain_participant (domainId,
                                                qos);
        }
      catch (const CORBA::SystemException& sysex)
        {
          sysex._tao_print_exception (
            "ERROR: System Exception"
            " in DomainParticipantFactoryImpl::create_participant");
          return ::DDS::DomainParticipant::_nil();
        }
      catch (const CORBA::UserException& userex)
        {
          userex._tao_print_exception (
            "ERROR: User Exception"
            " in DomainParticipantFactoryImpl::create_participant");
          return ::DDS::DomainParticipant::_nil();
        }

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

      ::DDS::DomainParticipant_ptr dp_obj(dp);


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
      dp->set_object_reference (dp_obj); //xxx no change

      // There is no qos policy in the DomainParticipantFactory, the DomainParticipant
      // defaults to enabled.
      dp->enable ();

      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                        tao_mon,
                        this->participants_protector_,
                        ::DDS::DomainParticipant::_nil ());

      // the Pair will also act as a guard against leaking the
      // new DomainParticipantImpl (NO_DUP, so this takes over mem)
      Participant_Pair pair (dp, dp_obj, NO_DUP);

      DPSet* entry;
      if (find(participants_, domainId, entry) == -1)
        {
          DPSet set;

          if (OpenDDS::DCPS::insert(set, pair) == -1)
            {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                          ACE_TEXT(" %p.\n"),
                          ACE_TEXT("insert")));
              return ::DDS::DomainParticipant::_nil ();
            }

            if (bind(participants_, domainId, set)  == -1)
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
          if (OpenDDS::DCPS::insert(*entry, pair) == -1)
            {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("DomainParticipantFactoryImpl::create_participant, ")
                          ACE_TEXT(" %p.\n"),
                          ACE_TEXT("insert")));
              return ::DDS::DomainParticipant::_nil ();
            }
        }
//xxx still ref_count = 1

      return ::DDS::DomainParticipant::_duplicate(dp_obj); //xxx still 2  (obj 3->4)
    } //xxx obj 4->3


    ::DDS::ReturnCode_t
    DomainParticipantFactoryImpl::delete_participant (
        ::DDS::DomainParticipant_ptr a_participant
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
//xxx rc = 4
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
        = dynamic_cast<DomainParticipantImpl*> (a_participant);

      //xxx servant rc = 4 (servant::DP::Entity::ServantBase::ref_count_
      if (the_servant->is_clean () == 0)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                            ACE_TEXT("The participant(repo_id=%d) is not empty.\n"),
                            the_servant->get_id ()),
                            ::DDS::RETCODE_PRECONDITION_NOT_MET);
        }

      ::DDS::DomainId_t domain_id = the_servant->get_domain_id ();
      RepoId dp_id = the_servant->get_id ();

      DPSet* entry;
      if (find(participants_, domain_id, entry) == -1)
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

          DPSet* entry;
          if (find(participants_, domain_id, entry) == -1)
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
                = the_servant->delete_contained_entities ();

//xxx still rc=4
              if (result != ::DDS::RETCODE_OK)
                {
                  return result;
                }

              Participant_Pair pair (the_servant, a_participant, DUP);

              if (OpenDDS::DCPS::remove(*entry, pair) == -1)
                {
                  ACE_ERROR_RETURN ((LM_ERROR,
                                    ACE_TEXT("(%P|%t) ERROR: ")
                                    ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                                    ACE_TEXT(" %p.\n"),
                                    ACE_TEXT("remove")),
                                    ::DDS::RETCODE_ERROR);
                }
//xxx now obj rc=5 and servant rc=4
              if (entry->empty())
                {
                  if (unbind(participants_, domain_id) == -1)
                    {
                      ACE_ERROR_RETURN ((LM_ERROR,
                                        ACE_TEXT("(%P|%t) ERROR: ")
                                        ACE_TEXT("DomainParticipantFactoryImpl::delete_participant, ")
                                        ACE_TEXT(" %p.\n"),
                                        ACE_TEXT("unbind")),
                                        ::DDS::RETCODE_ERROR);
                    }
                }
            } //xxx now obj rc = 4
        }//xxx now obj rc = 3

      DCPSInfo_var repo = TheServiceParticipant->get_repository( domain_id);

      try
        {
          repo->remove_domain_participant (domain_id,
                                           dp_id);
        }
      catch (const CORBA::SystemException& sysex)
        {
          sysex._tao_print_exception (
            "ERROR: System Exception"
            " in DomainParticipantFactoryImpl::delete_participant");
          return ::DDS::RETCODE_ERROR;
        }
      catch (const CORBA::UserException& userex)
        {
          userex._tao_print_exception (
            "ERROR: User Exception"
            " in DomainParticipantFactoryImpl::delete_participant");
          return ::DDS::RETCODE_ERROR;
        }

      return ::DDS::RETCODE_OK;
    }


    ::DDS::DomainParticipant_ptr
    DomainParticipantFactoryImpl::lookup_participant (
        ::DDS::DomainId_t domainId
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                        tao_mon,
                        this->participants_protector_,
                        ::DDS::DomainParticipant::_nil ());

      DPSet* entry;
      if (find(participants_, domainId, entry) == -1)
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
          return ::DDS::DomainParticipant::_duplicate ((*(entry->begin ())).obj_.in ());
        }
    }

    ::DDS::ReturnCode_t
    DomainParticipantFactoryImpl::set_default_participant_qos (
        const ::DDS::DomainParticipantQos & qos
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

      for (DPMap::iterator mapIter = participants_.begin();
        mapIter != participants_.end();
        ++mapIter)
      {
        for (DPSet::iterator iter = mapIter->second.begin();
          iter != mapIter->second.end();
          ++iter)
        {
          ::DDS::ReturnCode_t result = delete_participant ((*iter).obj_.in ());
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
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      return TheParticipantFactory;
    }


 } // namespace DCPS
} // namespace OpenDDS
