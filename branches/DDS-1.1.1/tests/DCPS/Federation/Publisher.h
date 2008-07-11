#ifndef TESTMONITOR_H
#define TESTMONITOR_H

#include "TestConfig.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include <vector>
#include <map>

/**
 * @brief Publish data
 */
class Publisher {
  public:
    /// Construct from command line.
    Publisher( int argc, char** argv, char** envp);

    /// Virtual destructor.
    virtual ~Publisher();

    /// Main actions occur here.
    void run();

  private:
    /// Configuration information.
    TestConfig config_;

    /// Participant.
    ::DDS::DomainParticipant_var participant_;

    /// Publisher.
    ::DDS::Publisher_var publisher_;

    /// Topic.
    ::DDS::Topic_var topic_;

    /// DataWriter.
    ::DDS::DataWriter_var dataWriter_;

    /// Data transport.
    OpenDDS::DCPS::TransportImpl_rch transport_;
};

#endif // TESTMONITOR_H

