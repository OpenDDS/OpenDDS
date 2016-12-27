// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include <ace/Task.h>

namespace Test {

class Writer : public ACE_Task_Base {
  public:
    /// Construct with a writer and a profile.
    Writer(
      ::DDS::DataWriter_ptr writer,
      int                   index,
      bool                  verbose = false
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
    int status() const;

    /// DataWriter access.
    ::DDS::ReturnCode_t wait_for_acks( const ::DDS::Duration_t& delay);

  private:
    /// DataWriter to write with.
    ::DDS::DataWriter_var writer_;

    /// This writers index in the writer container.  Used to distinguish
    /// this writer from the others.
    int index_;

    /// Verbosity indication.
    bool verbose_;

    /// Completion indicator.
    bool done_;

    /// Count of messages sent by this publication.
    int messages_;

    /// Status of execution.
    int status_;

    /// Lock for synchronizing access to the methods.
    ACE_SYNCH_MUTEX lock_;
};

} // End of namespace Test

#endif /* WRITER_H */

