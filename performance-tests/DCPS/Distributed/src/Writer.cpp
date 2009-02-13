// -*- C++ -*-
//
// $Id$
#include "Writer.h"
#include "TestTypeSupportC.h"

#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/Qos_Helper.h"

#include <sstream>

/// Control the spew.
namespace { enum { BE_REALLY_VERBOSE = 0};}

namespace Test {

Writer::Writer(
  ::DDS::DataWriter_ptr writer,
  // const PublicationProfile& profile,
  bool verbose

) : writer_( ::DDS::DataWriter::_duplicate( writer)),
    // profile_( profile),
    verbose_( verbose),
    done_( false),
    messages_( 0)
{
  OpenDDS::DCPS::DataWriterImpl* writerImpl
    = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>( this->writer_.in());
  if( writerImpl) {
    ::OpenDDS::DCPS::GUID_t id = writerImpl->get_publication_id();
    ::OpenDDS::DCPS::GuidConverter converter( id);
    this->publicationId_ = static_cast<long>( converter);

  } else {
    this->publicationId_ = -1;
  }

}

Writer::~Writer()
{
  // Terminate cleanly.
  this->stop();
  this->wait();
}

int
Writer::open( void*)
{
  int result = activate (THR_NEW_LWP | THR_JOINABLE, 1);
  if( result != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Writer::open() - ")
      ACE_TEXT("failed to activate the %s publication thread.\n"),
      "BOB"// this->profile_.name().c_str()
    ));
  }
  return result;
}

int
Writer::close( u_long /* flags */)
{
  return 0;
}

void
Writer::start()
{
  this->open( 0);
}

void
Writer::stop()
{
  // This is the single location where this member is written.
  this->done_ = true;
}

int
Writer::messages() const
{
  return this->messages_;
}

int
Writer::svc ()
{
#if 0
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Writer::svc() - ")
    ACE_TEXT("processing publication %s.\n"),
    "BOB"// this->profile_.name().c_str()
  ));

  int count = 0;
  while( this->done_ == false) {
    Test::DataDataWriter_var dataWriter
      = Test::DataDataWriter::_narrow( this->writer_.in());
    Test::Data sample;
    sample.priority = this->profile_.priority();
    sample.seq      = ++count;
    sample.pid      = this->publicationId_;
    sample.buffer.length( this->profile_.messageSize());

    ACE_Time_Value  start = ACE_OS::gettimeofday ();
    DDS::Duration_t stamp = ::OpenDDS::DCPS::time_value_to_duration( start);
    sample.sec     = stamp.sec;
    sample.nanosec = stamp.nanosec;

    dataWriter->write( sample, DDS::HANDLE_NIL);
    ++this->messages_;

    // Determine the interval to next message here so it can be mentioned
    // in the diagnostic messsage.
    ACE_Time_Value interval( 0, this->profile_.interval());

    if( this->verbose_ && BE_REALLY_VERBOSE) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Writer::svc() - ")
        ACE_TEXT("publication %s wrote sample %d at priority %d, ")
        ACE_TEXT("waiting %d microseconds to send next one.\n"),
        this->profile_.name().c_str(),
        count,
        this->profile_.priority(),
        interval.usec()
      ));
    }

    // Wait the remainder of the interval before sending the next message.
    ACE_Time_Value now = ACE_OS::gettimeofday ();
    interval -= (now - start);
    if( interval > ACE_Time_Value::zero) {
      ACE_OS::sleep( interval);
    }
  }

  if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Writer::svc() - ")
      ACE_TEXT("publication %s honoring termination request, ")
      ACE_TEXT("stopping thread.\n"),
      this->profile_.name().c_str()
    ));
  }
#endif
  return 0;
}

} // End of namespace Test

