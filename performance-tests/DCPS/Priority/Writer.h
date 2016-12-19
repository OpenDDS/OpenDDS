// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include <ace/Task.h>

namespace Test {

class PublicationProfile;

class Writer : public ACE_Task_Base {
  public:
    /// Construct with a writer and a profile.
    Writer(
      ::DDS::DataWriter_ptr     writer,
      const PublicationProfile& profile,
      bool                      verbose = false
    );

    virtual ~Writer();

    //
    // Task_Base methods.
    //

    virtual int open(void*);
    virtual int svc();
    virtual int close( u_long flags = 0);


    /// Thread control.
    void start();
    void stop();

    /// State access
    int messages() const;

  private:
    /// DataWriter to write with.
    ::DDS::DataWriter_var writer_;

    /// Hashed publication GUID_t value for this writer.
    long publicationId_;

    /// Profile of this publication.
    const PublicationProfile& profile_;

    /// Verbosity indication.
    bool verbose_;

    /// Completion indicator.
    bool done_;

    /// Count of messages sent by this publication.
    int messages_;

    /// Lock for synchronizing access to the methods.
    ACE_SYNCH_MUTEX lock_;
};

} // End of namespace Test

#endif /* WRITER_H */

