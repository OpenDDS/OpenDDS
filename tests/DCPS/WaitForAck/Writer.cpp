// -*- C++ -*-
//
#include "Writer.h"
#include "TestTypeSupportC.h"

#include "dds/DCPS/DataWriterImpl.h"

#include "ace/OS_NS_unistd.h"

#include <sstream>

/// Control the spew.
namespace { enum { BE_REALLY_VERBOSE = 0};}

namespace Test {

Writer::Writer(
  ::DDS::DataWriter_ptr writer,
  int index,
  bool verbose

) : writer_(::DDS::DataWriter::_duplicate(writer)),
    index_(index),
    verbose_(verbose),
    done_(false),
    messages_(0),
    status_(0)
{
}

Writer::~Writer()
{
  // Terminate cleanly.
  stop();
  wait();
}

int
Writer::status() const
{
  return status_;
}

int
Writer::open(void*)
{
  int result = activate (THR_NEW_LWP | THR_JOINABLE, 1);
  if (result != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Writer::open() - ")
      ACE_TEXT("failed to activate the publication thread %d.\n"),
      index_
    ));
  }
  return result;
}

int
Writer::close(u_long /* flags */)
{
  return 0;
}

void
Writer::start()
{
  open(0);
}

void
Writer::stop()
{
  // This is the single location where this member is written.
  done_ = true;
}

int
Writer::messages() const
{
  return messages_;
}

::DDS::ReturnCode_t
Writer::wait_for_acks(const ::DDS::Duration_t& delay)
{
  return writer_->wait_for_acknowledgments(delay);
}

int
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Writer::svc() - ")
    ACE_TEXT("processing publication %d.\n"),
    index_
  ));

  int count = 0;
  Test::DataDataWriter_var dataWriter
    = Test::DataDataWriter::_narrow(writer_.in());
  while (done_.value() == false) {
    Test::Data sample;
    sample.pid = index_;
    sample.seq = ++count;

    dataWriter->write(sample, DDS::HANDLE_NIL);
    ++messages_;

    ACE_Time_Value interval(0, 1000); // 1 mS == 1000 samples per second.

    if (verbose_ && BE_REALLY_VERBOSE) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Writer::svc() - ")
        ACE_TEXT("publication %d wrote sample %d, ")
        ACE_TEXT("waiting %d microseconds to send next one.\n"),
        index_,
        count,
        interval.usec()
      ));
    }

    // Wait before sending the next message.
    ACE_OS::sleep(interval);
  }

  ::DDS::Duration_t shutdownDelay = { 30, 0 }; // Wait for up to 30 seconds.
  ::DDS::ReturnCode_t result
    = writer_->wait_for_acknowledgments(shutdownDelay);
  if (result != ::DDS::RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) ERROR: Writer::svc() - ")
      ACE_TEXT("publication %d wait failed with code: %d while terminating.\n"),
      index_,
      result
    ));
    ++status_;
  }

  if (verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Writer::svc() - ")
      ACE_TEXT("publication %d honoring termination request, ")
      ACE_TEXT("stopping thread.\n"),
      index_
    ));
  }
  return 0;
}

} // End of namespace Test
