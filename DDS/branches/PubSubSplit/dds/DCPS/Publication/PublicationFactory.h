// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_PUBLICATION_FACTORY_H
#define TAO_DDS_DCPS_PUBLICATION_FACTORY_H

#include "publication_export.h"
#include "dds/DCPS/PublisherFactory.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO
{
  namespace DCPS
  {
    class TAO_DdsDcps_Publication_Export PublicationFactory
      : public PublisherFactory
    {
    public:
      virtual ~PublicationFactory() {}

      virtual PublisherImpl* make_publisher(
        const ::DDS::PublisherQos&   qos,
        ::DDS::PublisherListener_ptr a_listener,
        DomainParticipantImpl*       participant,
        ::DDS::DomainParticipant_ptr participant_objref
        );
    };

  } // namespace  ::DDS
} // namespace TAO

#endif /* TAO_DDS_DCPS_PUBLICATION_FACTORY_H  */
