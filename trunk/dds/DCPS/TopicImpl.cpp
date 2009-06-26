// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TopicImpl.h"
#include "Qos_Helper.h"
#include "RepoIdConverter.h"
#include "Definitions.h"
#include "Service_Participant.h"
#include "DomainParticipantImpl.h"


namespace OpenDDS
{
  namespace DCPS
  {
    // Implementation skeleton constructor
    TopicImpl::TopicImpl (const RepoId                   topic_id,
                          const char*                    topic_name,
                          const char*                    type_name,
                          OpenDDS::DCPS::TypeSupport_ptr type_support,
                          const ::DDS::TopicQos &        qos,
                          ::DDS::TopicListener_ptr       a_listener,
                          ::DDS::DomainParticipant_ptr   participant)
      : TopicDescriptionImpl(topic_name,
                             type_name,
                             type_support,
                             participant),
        qos_(qos),
        // must default because create_topic does not take a mask
        // like set_listener does. - ?discrepency in the DDS spec.
        listener_mask_(DEFAULT_STATUS_KIND_MASK),
        listener_(::DDS::TopicListener::_duplicate(a_listener)),
        fast_listener_ (0),
        id_(topic_id),
        entity_refs_(0)
    {
      inconsistent_topic_status_.total_count = 0;
      inconsistent_topic_status_.total_count_change = 0;
    }


    // Implementation skeleton destructor
    TopicImpl::~TopicImpl (void)
    {
    }


    ::DDS::ReturnCode_t
    TopicImpl::set_qos (
        const ::DDS::TopicQos & qos
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      if (Qos_Helper::valid(qos) && Qos_Helper::consistent(qos))
        {
          if (qos_ == qos)
            return ::DDS::RETCODE_OK;

          if (enabled_.value())
            {
              if (! Qos_Helper::changeable (qos_, qos))
                {
                  return ::DDS::RETCODE_IMMUTABLE_POLICY;
                }
              else 
                {
                  qos_ = qos;
                  DomainParticipantImpl* part = dynamic_cast<DomainParticipantImpl*> (this->participant_);

                  try
                  {
                    DCPSInfo_var repo = TheServiceParticipant->get_repository(part->get_domain_id());
                    CORBA::Boolean status 
                      = repo->update_topic_qos(this->id_, part->get_domain_id(), part->get_id(), qos_);

                    if (status == 0)
                    {
                      ACE_ERROR_RETURN ((LM_ERROR,
                        ACE_TEXT("(%P|%t) TopicImpl::set_qos, ")
                        ACE_TEXT("failed on compatiblity check. \n")),
                        ::DDS::RETCODE_ERROR);
                    }
                  }
                  catch (const CORBA::SystemException& sysex)
                  {
                    sysex._tao_print_exception (
                      "ERROR: System Exception"
                      " in TopicImpl::set_qos");
                    return ::DDS::RETCODE_ERROR;
                  }
                  catch (const CORBA::UserException& userex)
                  {
                    userex._tao_print_exception (
                      "ERROR:  Exception"
                      " in TopicImpl::set_qos");
                    return ::DDS::RETCODE_ERROR;
                  }
                }
            }
          else 
            qos_ = qos;

          return ::DDS::RETCODE_OK;
        }
      else
        {
          return ::DDS::RETCODE_INCONSISTENT_POLICY;
        }
    }


    void
    TopicImpl::get_qos (
        ::DDS::TopicQos & qos
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      qos = qos_;
    }


    ::DDS::ReturnCode_t
    TopicImpl::set_listener (
        ::DDS::TopicListener_ptr a_listener,
        ::DDS::StatusKindMask mask
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      listener_mask_ = mask;
      //note: OK to duplicate  a nil object ref
      listener_ = ::DDS::TopicListener::_duplicate(a_listener);
      fast_listener_ = listener_.in ();
      return ::DDS::RETCODE_OK;
    }


    ::DDS::TopicListener_ptr
    TopicImpl::get_listener (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      return ::DDS::TopicListener::_duplicate(listener_.in ());
    }


    ::DDS::InconsistentTopicStatus
    TopicImpl::get_inconsistent_topic_status (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      ::DDS::InconsistentTopicStatus status;
      status = inconsistent_topic_status_;
      return status;
    }


    ::DDS::ReturnCode_t
    TopicImpl::enable (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      //TDB - check if factory is enables and then enable all entities
      // (don't need to do it for now because
      //  entity_factory.autoenable_created_entities is always = 1)
      return this->set_enabled ();
    }


    RepoId
    TopicImpl::get_id () const
    {
      return id_;
    }

    DDS::InstanceHandle_t
    TopicImpl::get_instance_handle()
      ACE_THROW_SPEC ((CORBA::SystemException))
    {
      RepoIdConverter converter(id_);
      return DDS::InstanceHandle_t(converter);
    }

  } // namespace DCPS
} // namespace OpenDDS
