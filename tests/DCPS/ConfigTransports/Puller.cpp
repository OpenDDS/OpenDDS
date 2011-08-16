#include "Puller.h"
#include "Factory.h"
#include "../common/TestSupport.h"

#include "ace/Log_Msg.h"

Puller::Puller(const Factory& f,
               const DDS::DomainParticipantFactory_var& factory,
               const DDS::DomainParticipant_var& participant,
               const DDS::DataReaderListener_var& listener) :
        dpf(factory),
        dp(participant),
        topic(f.topic(dp)),
        sub(f.subscriber(dp)),
        reader_(f.reader(sub, topic, listener))
{
}


Puller::Puller(const Factory& f,
               const DDS::DomainParticipantFactory_var& factory,
               const DDS::DomainParticipant_var& participant,
               const DDS::Subscriber_var& subscriber,
               const DDS::DataReaderListener_var& listener) :
        dpf(factory),
        dp(participant),
        topic(f.topic(dp)),
        sub(subscriber),
        reader_(f.reader(sub, topic, listener))
{
}


Puller::~Puller()
{
  // Clean up and shut down DDS objects
  sub->delete_contained_entities();
  dp->delete_contained_entities();
  dpf->delete_participant(dp.in());
}

