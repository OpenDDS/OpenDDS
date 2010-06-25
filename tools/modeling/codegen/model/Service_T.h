#ifndef SERVICE_H
#define SERVICE_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Delegate.h"
#include "dds/DdsDcpsC.h"

namespace OpenDDS { namespace Model {

  template< typename ModelName>
  class Service {
    public:
      typedef typename ModelName::Data         Data;
      typedef typename ModelName::Participants Participants;
      typedef typename ModelName::Types        Types;
      typedef typename ModelName::Topics       Topics;
      typedef typename ModelName::Publishers   Publishers;
      typedef typename ModelName::Subscribers  Subscribers;
      typedef typename ModelName::DataWriters  DataWriters;
      typedef typename ModelName::DataReaders  DataReaders;
      typedef typename ModelName::Transports   Transports;

      Service();
      ~Service();

      ///{ @name DDS API Entity accessors.
      DDS::DomainParticipant_var participant( typename Participants::Values participant);
      DDS::Topic_var topic( typename Participants::Values participant,
                            typename Topics::Values       topic
                     );
      DDS::Publisher_var publisher( typename Publishers::Values publisher);
      DDS::Subscriber_var subscriber( typename Subscribers::Values subscriber);
      DDS::DataWriter_var writer( typename DataWriters::Values writer);
      DDS::DataReader_var reader( typename DataReaders::Values reader);
      ///}

      ///( @name Lifetime management.
      void init( int argc = 0, char** argv = 0);
      void fini();
      ///}

    private:
      ///{ @name Entity Creation.
      void createParticipant(
             typename Participants::Values participant
           );
      void createTopic(
             typename Participants::Values participant,
             typename Topics::Values       topic
           );
      void createPublisher(
             typename Publishers::Values publisher
           );
      void createSubscriber(
             typename Subscribers::Values subscriber
           );
      void createPublication(
             typename DataWriters::Values writer
           );
      void createSubscription(
             typename DataReaders::Values reader
           );
      void createTransport(
             typename Transports::Values transport
           );
      ///}

      Data     modelData_;
      Delegate delegate_;

      // Basic array containers since we only allow access using the
      // defined enumeration values.

      DDS::DomainParticipant*       participants_[ Participants::LAST_INDEX];
      bool                          types_[        Participants::LAST_INDEX]
                                          [        Types::LAST_INDEX];
      DDS::Topic*                   topics_[       Participants::LAST_INDEX]
                                           [       Topics::LAST_INDEX];
      DDS::Publisher*               publishers_[   Publishers::LAST_INDEX];
      DDS::Subscriber*              subscribers_[  Subscribers::LAST_INDEX];
      DDS::DataWriter*              writers_[      DataWriters::LAST_INDEX];
      DDS::DataReader*              readers_[      DataReaders::LAST_INDEX];
      OpenDDS::DCPS::TransportImpl* transports_[   Transports::LAST_INDEX];
  };

} } // End namespace OpenDDS::Model

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "Service_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("Service_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#include /**/ "ace/post.h"

#endif /* SERVICE_H */

