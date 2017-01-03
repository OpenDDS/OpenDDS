#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include "TestConfig.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"

class DataReaderListenerImpl;

/**
 * @brief Subscriber to receive data
 */
class Subscriber {
  public:
    /// Construct from command line.
    Subscriber( int argc, ACE_TCHAR** argv, char** envp);

    virtual ~Subscriber();

    /// Main actions occur here.
    void run();

  private:
    /// Configuration information.
    TestConfig config_;

    /// Participant
    ::DDS::DomainParticipant_var participant_;

    /// Subscriber
    ::DDS::Subscriber_var subscriber_;

    /// Topic
    ::DDS::Topic_var topic_;

    /// DataReader
    ::DDS::DataReader_var dataReader_;

    /// Listener servant used for synchronization
    DataReaderListenerImpl* sync_;

    /// Listener
    ::DDS::DataReaderListener_var listener_;

    /// Data transport
    OpenDDS::DCPS::TransportImpl_rch transport_;
};

#endif // SUBSCRIBER_H

