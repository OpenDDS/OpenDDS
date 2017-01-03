// -*- C++ -*-
//
#ifndef PUBLICATION_H
#define PUBLICATION_H

#include "Process.h"

#include "TestTypeSupportC.h"
#include <ace/Task.h>

#include <string>

namespace Test {

struct PublicationProfile;

class Publication : public ACE_Task_Base {
  public:
    /// Construct with a profile.
    Publication(
      const char* name,
      PublicationProfile* profile,
      bool verbose = false
    );

    virtual ~Publication();

    /// @name Task_Base interfaces.
    /// @{
    virtual int open(void*);
    virtual int svc();
    virtual int close( u_long flags = 0);
    /// @}

    /// @name Thread control.
    /// @{
    void start();
    void stop();
    /// @}

    /// Resource management.
    void enable(
      ::DDS::DomainParticipant_ptr participant,
      ::DDS::Topic_ptr             topic
    );

    /// @name State access
    /// @{
    int messages() const;
    int timeouts() const;
    double duration() const;
    int associations() const;
    int missing_associations() const;
    bool ready() const;
    /// @}

    /// @name DataWriter interfaces.
    /// @{
    ::DDS::StatusCondition_ptr get_statuscondition();
    ::DDS::DataWriterListener_ptr get_listener();
    ::DDS::ReturnCode_t set_listener(
                          ::DDS::DataWriterListener_ptr a_listener,
                          ::DDS::StatusMask mask
                        );
    /// @}

    // Publish a data sample from an external source.
    void write( const Test::Data& sample);

  private:
    /// Name of this publication.
    std::string name_;

    /// Profile configuring this publication.
    PublicationProfile* profile_;

    /// Verbosity indication.
    bool verbose_;

    /// Completion indicator.
    bool done_;

    /// Resource status.
    bool enabled_;

    /// Count of messages sent by this publication.
    int messages_;

    /// Count of write timeouts encountered during publishing.
    int timeouts_;

    /// Duration of sending samples.
    double duration_;

    /// The writer for the publication.
    Test::DataDataWriter_var writer_;

    /// The publisher for the publication.
    ::DDS::Publisher_var publisher_;

    /// Lock for synchronizing access to the methods.
    ACE_SYNCH_MUTEX lock_;
};

} // End of namespace Test

#endif /* PUBLICATION_H */

