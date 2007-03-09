// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_PUBLISHER_FACTORY_H
#define TAO_DDS_DCPS_PUBLISHER_FACTORY_H

#include "dcps_export.h"
#include "dds/DCPS/DomainParticipantImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO
{
  namespace DCPS
  {

    class PublisherImpl;

    class TAO_DdsDcps_Export PublisherFactory
    {
    public:
      virtual ~PublisherFactory() {}

      virtual PublisherImpl* make_publisher(
        const ::DDS::PublisherQos&   qos,
        ::DDS::PublisherListener_ptr a_listener,
        DomainParticipantImpl*       participant,
        ::DDS::DomainParticipant_ptr participant_objref
        ) = 0;
    };

    PublisherFactory& getPublisherFactory();
    void setPublisherFactory(PublisherFactory* factory);

  } // namespace  ::DDS
} // namespace TAO

#endif /* TAO_DDS_DCPS_PUBLISHER_FACTORY_H  */
