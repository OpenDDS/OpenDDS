#ifndef TESTMONITOR_H
#define TESTMONITOR_H

#include "TestConfig.h"
#include "ForwardingListener.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include <vector>
#include <map>

/**
 * @brief Test subsystem to emulate an independent DCPS implementation.
 *
 * This class is intended to be used in order to test the ability to have
 * multiple DCPS implementations, with separate and distinct InfoRepo
 * repositories, play well together.  This includes the ability to
 * operate independently without interfereing with each other as well as
 * allowing processes to simultaneously participate in one or more of
 * these implementations.
 *
 * This implementation is entirely reactive.  That is, it will wait to
 * receive data to which it subscribes and directly republish that data.
 * There is no application defined active object within this subsystem.
 */
class TestMonitor {
  public:
    /// Construct from command line.
    TestMonitor( int argc, ACE_TCHAR** argv, char** envp);

    virtual ~TestMonitor();

    /// Main actions occur here.
    void run();

  private:
    /// Map to hold unique participants.
    typedef std::map< ::DDS::DomainId_t, ::DDS::DomainParticipant_var> ParticipantMap;

    /// Map repository keys to transport implementations.
    typedef std::map< OpenDDS::DCPS::Discovery::RepoKey,
                      OpenDDS::DCPS::TransportImpl_rch>            RepoToTransportMap;

    /// Configuration information.
    TestConfig config_;

    /// Unique participants.
    ParticipantMap participants_;

    /// Subscriber participants.
    std::vector< ::DDS::DomainParticipant_var> subscriberParticipant_;

    /// Publisher participants.
    std::vector< ::DDS::DomainParticipant_var> publisherParticipant_;

    /// Subscribers.
    std::vector< ::DDS::Subscriber_var> subscriber_;

    /// Publishers.
    std::vector< ::DDS::Publisher_var> publisher_;

    /// Reader Topics.
    std::vector< ::DDS::Topic_var> readerTopic_;

    /// Writer Topics.
    std::vector< ::DDS::Topic_var> writerTopic_;

    /// DataReaders.
    std::vector< ::DDS::DataReader_var> dataReader_;

    /// DataWriters.
    std::vector< ::DDS::DataWriter_var> dataWriter_;

    /// Listeners.
    std::vector< ::DDS::DataReaderListener_var> listener_;

    /// Forwarders.
    std::vector< ForwardingListenerImpl*> forwarder_;

    /// Data transport.
    RepoToTransportMap transport_;
};

#endif // TESTMONITOR_H

