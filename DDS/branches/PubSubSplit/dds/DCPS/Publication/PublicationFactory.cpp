// -*- C++ -*-
//
// $Id$
#include "Publication_pch.h"
#include "PublicationFactory.h"
#include "PublisherImpl.h"

namespace
{
  struct RegistrationGuard
  {
    RegistrationGuard(TAO::DCPS::PublisherFactory* factory)
    {
      TAO::DCPS::setPublisherFactory(factory);
    }

    ~RegistrationGuard()
    {
      TAO::DCPS::setPublisherFactory(0);
    }
  };

  static TAO::DCPS::PublicationFactory publicationFactory;
  static RegistrationGuard registrationGuard(&publicationFactory);
}

TAO::DCPS::PublisherImpl*
TAO::DCPS::PublicationFactory::make_publisher(
  const ::DDS::PublisherQos&   qos,
  ::DDS::PublisherListener_ptr a_listener,
  DomainParticipantImpl*       participant,
  ::DDS::DomainParticipant_ptr participant_objref
  )
{
  PublisherImpl* pub = 0;
  ACE_NEW_RETURN(pub,
                PublisherImpl(qos,
                              a_listener,
                              participant,
                              participant_objref),
                0);
  return pub;
}
