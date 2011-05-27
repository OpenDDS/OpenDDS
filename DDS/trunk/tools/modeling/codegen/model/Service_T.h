#ifndef SERVICE_H
#define SERVICE_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "CopyQos.h"
#include "Entities.h"
#include "DefaultInstanceTraits.h"
#include "dds/DdsDcpsC.h"
#include "dds/DCPS/Service_Participant.h"

#include <vector>

namespace OpenDDS { namespace Model {

  class Application;

  template< typename ModelName, class InstanceTraits = DefaultInstanceTraits>
  class Service : public CopyQos, public Entities, public InstanceTraits {
    public:
      typedef typename ModelName::Data Data;

      typedef typename ModelName::Participants Participants;
      typedef typename ModelName::Types        Types;
      typedef typename ModelName::Topics       Topics;
      typedef typename ModelName::ContentFilteredTopics   ContentFilteredTopics;
      typedef typename ModelName::MultiTopics             MultiTopics;
      typedef typename ModelName::Publishers   Publishers;
      typedef typename ModelName::Subscribers  Subscribers;
      typedef typename ModelName::DataWriters  DataWriters;
      typedef typename ModelName::DataReaders  DataReaders;

      Service(const Application& application, int& argc, ACE_TCHAR* argv[]);
      ~Service();

      ///{ @name DDS API Entity accessors.
      DDS::DomainParticipant_var participant(typename Participants::Values participant);
      DDS::TopicDescription_var topic(typename Participants::Values participant,
                                      typename Topics::Values       topic);
      DDS::Publisher_var publisher(typename Publishers::Values publisher);
      DDS::Subscriber_var subscriber(typename Subscribers::Values subscriber);
      DDS::DataWriter_var writer(typename DataWriters::Values writer);
      DDS::DataReader_var reader(typename DataReaders::Values reader);
      ///}

    private:
      ///{ @name Entity Creation.
      DDS::DomainParticipant* createParticipant(
             typename Participants::Values participant
           );
      void createTopicDescription(
             typename Participants::Values participant,
             typename Topics::Values       topic
           );
      void createTopic(
             typename Participants::Values participant,
             typename Topics::Values       topic
           );
      void createContentFilteredTopic(
             typename Participants::Values          participant,
             typename Topics::Values                topic,
             typename ContentFilteredTopics::Values cfTopic
           );
      void createMultiTopic(
             typename Participants::Values participant,
             typename Topics::Values       topic,
             typename MultiTopics::Values  multiTopic
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
      ///}

      ///{ @name Delegate Callbacks
      virtual void copyPublicationQos(
                     unsigned int        which,
                     DDS::DataWriterQos& writerQos
                   );
      virtual void copySubscriptionQos(
                     unsigned int        which,
                     DDS::DataReaderQos& readerQos
                   );
      ///}

      const Application& application_;
      Data modelData_;

      ///{@ The vectors are indexed by the corresponding enumeration,
      ///   for example Participants::Values for participants_, except
      ///   for the special cases (vectors-of-vectors) which are noted.
      std::vector<DDS::DomainParticipant*>       participants_;
      std::vector<std::vector<bool> >            types_;  // [part][type]
      std::vector<std::vector<DDS::TopicDescription*> > topics_; // [part][topic]
      std::vector<DDS::Publisher*>               publishers_;
      std::vector<DDS::Subscriber*>              subscribers_;
      std::vector<DDS::DataWriter*>              writers_;
      std::vector<DDS::DataReader*>              readers_;
      ///}
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

