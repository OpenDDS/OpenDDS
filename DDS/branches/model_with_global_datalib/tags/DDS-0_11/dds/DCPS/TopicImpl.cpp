// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h"
#include  "TopicImpl.h"
#include  "Qos_Helper.h"
#include  "Definitions.h"
#include  "Service_Participant.h"


namespace TAO
{
  namespace DCPS
  {
    // Implementation skeleton constructor
    TopicImpl::TopicImpl (const RepoId                   topic_id,
                          const char*                    topic_name,
                          const char*                    type_name,
                          POA_TAO::DCPS::TypeSupport_ptr type_support,
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
          if (enabled_.value())
            {
              if (! Qos_Helper::changeable (qos_, qos))
                {
                  return ::DDS::RETCODE_IMMUTABLE_POLICY;
                }
            }
          if (! (qos_ == qos))
            {
              qos_ = qos;
              // TBD - when there are changable QoS then we
              //       need to tell the DCPSInfo/repo about
              //       the changes in Qos.
              // repo->set_qos(qos_);
            }
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
      //note: OK to duplicate  and reference_to_servant a nil object ref
      listener_ = ::DDS::TopicListener::_duplicate(a_listener);
      fast_listener_
        = reference_to_servant< ::POA_DDS::TopicListener,
                                ::DDS::TopicListener_ptr >
          (listener_.in ());
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


    ::DDS::StatusKindMask
    TopicImpl::get_status_changes (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      // TBD - Currently no supported QoS allow changes
      // so it always returns 0 (No changes).
      return 0;
    }

    RepoId
    TopicImpl::get_id () const
    {
      return id_;
    }


  } // namespace DCPS
} // namespace TAO
