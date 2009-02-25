// -*- C++ -*-
//
// $Id$
#ifndef PUBLICATION_H
#define PUBLICATION_H

#include "Process.h"

#include <dds/DdsDcpsPublicationC.h>
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

    /// Virtual destructor.
    virtual ~Publication();

    //
    // Task_Base methods.
    //

    virtual int open(void*);
    virtual int svc();
    virtual int close( u_long flags = 0);


    /// Thread control.
    void start();
    void stop();

    /// Resource management.
    void enable(
      ::DDS::DomainParticipant_ptr participant,
      ::DDS::Topic_ptr             topic
    );

    /// State access
    int messages() const;

    /// Access to the writer internal status values.
    ::DDS::StatusCondition_ptr get_statuscondition();

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

    /// The writer for the publication.
    ::DDS::DataWriter_var writer_;

    /// Lock for synchronizing access to the methods.
    ACE_SYNCH_MUTEX lock_;
};

} // End of namespace Test

#endif /* PUBLICATION_H */

