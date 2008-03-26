// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "FederatorConfig.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

namespace { // Anonymous namespace for file scope

  /// Define an argument copying functor.
  class ArgCopier {
    public:
      /// Construct with a target pointer array.
      ArgCopier( char** target_);

      /// The Functor function operator.
      void operator()( char* arg);

    private:
      /// The copy target.
      char** target_;

      /// Ugly argument value switch.
      bool fileArg_;
  };

  ArgCopier::ArgCopier( char** target)
   : target_( target),
     fileArg_( false)
  {
  }

  void
  ArgCopier::operator()( char* arg)
  {
    // Skip the file argument and its value.
    if( ::OpenDDS::Federator::Config::FEDERATOR_CONFIG_OPTION == arg) {
      this->fileArg_ = true;
      return;
    }

    if( this->fileArg_ != true) {
      *target_++ = arg;
    }
    this->fileArg_ = false;
  }

} // End of anonymous namespace

namespace OpenDDS { namespace Federator {

const std::string
Config::FEDERATOR_CONFIG_OPTION( "-FederatorConfig");

Config::Config( int argc, char** argv)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorConfig::FederatorConfig()\n")
  ));

  // Setup the internal storage and management.
  //
  // N.B. We will be adding two arguments and their respective values,
  //      but we will also be removing the configuration file option and
  //      its value for a total of 2 additional arguments.  If there is
  //      no configuration file option, we will not be removing any
  //      arguments, but neither will we be adding any.
  //
  this->argc_     = argc;
  this->argvSize_ = this->argc_ + 2;
  this->argv_     = new char*[ this->argvSize_];

  // Copy the existing arguments verbatim.
  ArgCopier argCopier( this->argv_);
  std::for_each( &argv[0], &argv[ argc], argCopier);
}

Config::~Config()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorConfig::~FederatorConfig()\n")
  ));

  // We own this memory.
  delete [] this->argv_;
}

void
Config::configFile( const std::string /* arg */)
{
  /// @TODO: Implement the file parse and argument value formation.
}

int&
Config::argc()
{
  return this->argc_;
}

int
Config::argc() const
{
  return this->argc_;
}

char**&
Config::argv()
{
  return this->argv_;
}

char**
Config::argv() const
{
  return this->argv_;
}

}} // End namespace OpenDDS::Federator

