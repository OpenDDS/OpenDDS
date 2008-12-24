// -*- C++ -*-
//
// $Id$
#include "Writer.h"
#include "PublicationProfile.h"
#include "TestTypeSupportC.h"

#include <sstream>

namespace Test {

Writer::Writer(
  ::DDS::DataWriter_ptr writer,
  const PublicationProfile& profile,
  bool verbose

) : writer_( ::DDS::DataWriter::_duplicate( writer)),
    profile_( profile),
    verbose_( verbose),
    done_( false)
{
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
      this->profile_.name().c_str()
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
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Writer::svc() - ")
    ACE_TEXT("processing publication %s.\n"),
    this->profile_.name().c_str()
  ));

  int count = 1;
  while( this->done_ == false) {
    Test::DataDataWriter_var dataWriter
      = Test::DataDataWriter::_narrow( this->writer_.in());
    Test::Data sample;
    sample.priority = this->profile_.priority();
    sample.seq      = count++;
    sample.buffer.length( this->profile_.messageSize());

    dataWriter->write( sample, DDS::HANDLE_NIL);

    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Writer::svc() - ")
        ACE_TEXT("publication %s wrote sample %d at priority %d.\n"),
        this->profile_.name().c_str(),
        count,
        this->profile_.priority()
      ));
    }

    // Wait before sending the next message.
    ACE_Time_Value interval( 0, this->profile_.interval());
    ACE_OS::sleep( interval);
  }

  if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Writer::svc() - ")
      ACE_TEXT("publication %s honoring termination request, ")
      ACE_TEXT("stopping thread.\n"),
      this->profile_.name().c_str()
    ));
  }
  return 0;
}

} // End of namespace Test

