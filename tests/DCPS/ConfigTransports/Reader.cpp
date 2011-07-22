#include "Reader.h"
#include "common.h"
#include "../common/TestSupport.h"

#include "ace/Log_Msg.h"

Reader::Reader(const Factory& f, DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::DataReaderListener_ptr listener) :
        dpf(DDS::DomainParticipantFactory::_duplicate(factory)),
        dp(DDS::DomainParticipant::_duplicate(participant)),
        topic(f.topic(dp.in())),
        sub(f.subscriber(dp.in())),
        reader_(f.reader(sub.in(), topic.in(), listener)) { }

Reader::Reader(const Factory& f, DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::Subscriber_ptr subscriber, DDS::DataReaderListener_ptr listener) :
        dpf(DDS::DomainParticipantFactory::_duplicate(factory)),
        dp(DDS::DomainParticipant::_duplicate(participant)),
        topic(f.topic(dp.in())),
        sub(DDS::Subscriber::_duplicate(subscriber)),
        reader_(f.reader(sub.in(), topic.in(), listener)) { }

Reader::~Reader()
{
  // Clean up subscriber objects
  sub->delete_contained_entities();
  dp->delete_subscriber(sub.in());
  dp->delete_topic(topic.in());
  dpf->delete_participant(dp.in());
}

