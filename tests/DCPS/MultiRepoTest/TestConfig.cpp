
#include "TestConfig.h"
#include "TestException.h"
#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_stdlib.h"
// #include "ace/Log_Msg.h"

#include "dds/DCPS/StaticIncludes.h"

namespace { // anonymous namespace for file scope.
  //
  // Default values.
  //
  const bool  TC_DEFAULT_VERBOSE = false;
  enum {      TC_DEFAULT_SAMPLES = 10};
  enum {      TC_DEFAULT_SAMPLE_INTERVAL = 0};
  const char* TC_DEFAULT_TYPE    = "Foo";

} // end of anonymous namespace.

TestConfig::~TestConfig()
{
}

TestConfig::TestConfig( int argc, ACE_TCHAR** argv, char** envp)
 : verbose_( TC_DEFAULT_VERBOSE),
   samples_( TC_DEFAULT_SAMPLES),
   sample_interval_(TC_DEFAULT_SAMPLE_INTERVAL),
   typeName_( TC_DEFAULT_TYPE)
{
  ACE_UNUSED_ARG(envp);

  // Default mapping.
  this->domainRepoMap_[ -1] = -1; // MAGIC: -1 == ANY_DOMAIN, -1 == DEFAULT_REPO

  ACE_Arg_Shifter parser( argc, argv);
  while( parser.is_anything_left()) {
    const ACE_TCHAR* currentArg = 0;
    if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-Address")))) {
      this->transportAddressName_ = currentArg;
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-TypeName")))) {
      this->typeName_ = ACE_TEXT_ALWAYS_CHAR(currentArg);
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-WriterDomain")))) {
      long domain = ACE_OS::atoi( currentArg);
      this->publisherDomain_.push_back( domain);
      this->domainRepoMap_[ domain] = static_cast<long>(this->infoRepoIor_.size() - 1);
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-ReaderDomain")))) {
      long domain = ACE_OS::atoi( currentArg);
      this->subscriberDomain_.push_back( domain);
      this->domainRepoMap_[ domain] = static_cast<long>(this->infoRepoIor_.size() - 1);
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-WriterTopic")))) {
      this->writerTopicName_.push_back(ACE_TEXT_ALWAYS_CHAR(currentArg));
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-ReaderTopic")))) {
      this->readerTopicName_.push_back(ACE_TEXT_ALWAYS_CHAR(currentArg));
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-InfoRepo")))) {
      this->infoRepoIor_.push_back(ACE_TEXT_ALWAYS_CHAR(currentArg));
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-Samples")))) {
      this->samples_ = ACE_OS::atoi( currentArg);
      parser.consume_arg();
    }
    else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-SampleInterval")))) {
      this->sample_interval_ = ACE_OS::atoi( currentArg);
      parser.consume_arg();
    } else if( 0 != (currentArg = parser.get_the_parameter(ACE_TEXT("-v")))) {
      this->verbose_ = true;

    } else {
      parser.ignore_arg();
    }
  }
#if 0
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: Configured with: verbose       = %b\n")
    ACE_TEXT("                                          type          = %C\n")
    ACE_TEXT("                                          address       = %s\n")
    ACE_TEXT("                                          repository    = %C\n")
    ACE_TEXT("                                          reader topic  = %C\n")
    ACE_TEXT("                                          writer topic  = %C\n")
    ACE_TEXT("                                          reader domain = %d\n")
    ACE_TEXT("                                          writer domain = %d\n"),
    this->verbose_, this->typeName_,
    this->transportAddressName_, this->infoRepoIor_,
    this->readerTopicName_, this->writerTopicName_,
    this->subscriberDomain_, this->publisherDomain_
  ));
#endif
}

bool&
TestConfig::verbose()
{
  return this->verbose_;
}

bool
TestConfig::verbose() const
{
  return this->verbose_;
}

int&
TestConfig::samples()
{
  return this->samples_;
}

int
TestConfig::samples() const
{
  return this->samples_;
}

int&
TestConfig::sample_interval()
{
  return this->sample_interval_;
}

int
TestConfig::sample_interval() const
{
  return this->sample_interval_;
}

std::string&
TestConfig::typeName()
{
  return this->typeName_;
}

std::string
TestConfig::typeName() const
{
  return this->typeName_;
}

std::string&
TestConfig::infoRepoIor( int index)
{
  if( (index < 0) || (unsigned(index) >= this->infoRepoIor_.size())) {
    throw OutOfRangeException();

  } else {
    return this->infoRepoIor_[index];
  }
}

std::string
TestConfig::infoRepoIor( int index) const
{
  if( (index < 0) || (unsigned(index) >= this->infoRepoIor_.size())) {
    throw OutOfRangeException();

  } else {
    return this->infoRepoIor_[index];
  }
}

ACE_TString&
TestConfig::transportAddressName()
{
  return this->transportAddressName_;
}

ACE_TString
TestConfig::transportAddressName() const
{
  return this->transportAddressName_;
}

std::string&
TestConfig::readerTopicName( int index)
{
  if( (index < 0) || (unsigned(index) >= this->readerTopicName_.size())) {
    throw OutOfRangeException();

  } else {
    return this->readerTopicName_[index];
  }
}

std::string
TestConfig::readerTopicName( int index) const
{
  if( (index < 0) || (unsigned(index) >= this->readerTopicName_.size())) {
    throw OutOfRangeException();

  } else {
    return this->readerTopicName_[index];
  }
}

std::string&
TestConfig::writerTopicName( int index)
{
  if( (index < 0) || (unsigned(index) >= this->writerTopicName_.size())) {
    throw OutOfRangeException();

  } else {
    return this->writerTopicName_[index];
  }
}

std::string
TestConfig::writerTopicName( int index) const
{
  if( (index < 0) || (unsigned(index) >= this->writerTopicName_.size())) {
    throw OutOfRangeException();

  } else {
    return this->writerTopicName_[index];
  }
}

long&
TestConfig::subscriberDomain( int index)
{
  if( (index < 0) || (unsigned(index) >= this->subscriberDomain_.size())) {
    throw OutOfRangeException();

  } else {
    return this->subscriberDomain_[index];
  }
}

long
TestConfig::subscriberDomain( int index) const
{
  if( (index < 0) || (unsigned(index) >= this->subscriberDomain_.size())) {
    throw OutOfRangeException();

  } else {
    return this->subscriberDomain_[index];
  }
}

long&
TestConfig::publisherDomain( int index)
{
  if( (index < 0) || (unsigned(index) >= this->publisherDomain_.size())) {
    throw OutOfRangeException();

  } else {
    return this->publisherDomain_[index];
  }
}

long
TestConfig::publisherDomain( int index) const
{
  if( (index < 0) || (unsigned(index) >= this->publisherDomain_.size())) {
    throw OutOfRangeException();

  } else {
    return this->publisherDomain_[index];
  }
}

long&
TestConfig::domainToRepo( int domain)
{
  DomainRepoMap::iterator where = this->domainRepoMap_.find( domain);
  if( where == this->domainRepoMap_.end()) {
    return this->domainRepoMap_[ -1]; // MAGIC: -1 == ANY_DOMAIN

  } else {
    return where->second;
  }
}

long
TestConfig::domainToRepo( int domain) const
{
  DomainRepoMap::const_iterator where = this->domainRepoMap_.find( domain);
  if( where == this->domainRepoMap_.end()) {
    return -1; // MAGIC: -1 == DEFAULT_REPO

  } else {
    return where->second;
  }
}

