#ifndef TESTSYSTEM_H
#define TESTSYSTEM_H

#include "TestConfig.h"
#include "ForwardingListener.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"

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
class TestSystem {
  public:
    /// Construct from command line.
    TestSystem( int argc, ACE_TCHAR** argv, char** envp);

    virtual ~TestSystem();

    /// Main actions occur here.
    void run();

  private:
    /// Configuration information.
    TestConfig config_;

    /// Subscriber DomainParticipant.
    ::DDS::DomainParticipant_var subscriberParticipant_;

    /// Publisher DomainParticipant.
    ::DDS::DomainParticipant_var publisherParticipant_;

    /// Subscriber.
    ::DDS::Subscriber_var subscriber_;

    /// Publisher.
    ::DDS::Publisher_var publisher_;

    /// Reader Topic.
    ::DDS::Topic_var readerTopic_;

    /// Writer Topic.
    ::DDS::Topic_var writerTopic_;

    /// DataReader.
    ::DDS::DataReader_var dataReader_;

    /// DataWriter.
    ::DDS::DataWriter_var dataWriter_;

    /// Data receiver.
    ::DDS::DataReaderListener_var listener_;

    /// Data transport.
    OpenDDS::DCPS::TransportImpl_rch transport_;
};

#endif // TESTSYSTEM_H

