// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "FederatorConfig.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#if !defined (__ACE_INLINE__)
# include "FederatorConfig.inl"
#endif /* ! __ACE_INLINE__ */

namespace { // Anonymous namespace for file scope

  /// Define an argument copying functor.
  class ArgCopier {
    public:
      /// Construct with a target pointer array.
      ArgCopier( char** target_, std::string& configFile);

      /// The Functor function operator.
      void operator()( char* arg);

    private:
      /// The copy target.
      char** target_;

      /// The configuration filename target.
      std::string& configFile_;

      /// Ugly argument value switch.
      bool fileArg_;
  };

  ArgCopier::ArgCopier( char** target, std::string& configFile)
   : target_( target),
     configFile_( configFile),
     fileArg_( false)
  {
  }

  void
  ArgCopier::operator()( char* arg)
  {
    // Skip the file argument and its value.
    if( ::OpenDDS::Federator::Config::FEDERATOR_CONFIG_OPTION == arg) {
      // Configuration file option, filename value is next arg.
      this->fileArg_ = true;
      return;
    }

    if( this->fileArg_ == true) {
      // Store the configuration file name, but don't copy the arg.
      this->configFile_ = arg;

    } else {
      // Copy other args verbatim.
      *this->target_++ = arg;
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

  // Setup the internal storage.
  //
  // N.B. We will be adding two arguments and their respective values,
  //      but we will also be removing the configuration file option and
  //      its value for a total of 2 additional slots.  If there is no
  //      configuration file option, we will not be removing any
  //      arguments, but neither will we be adding any.
  //
  this->argc_ = argc;
  this->argv_ = new char*[ argc + 2];

  // Copy the existing arguments verbatim.
  ArgCopier argCopier( this->argv_, this->configFile_);
  std::for_each( &argv[0], &argv[ argc], argCopier);

  // Form the new argument list.
  this->process();
}

Config::~Config()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorConfig::~FederatorConfig()\n")
  ));

  // We prwn this
  delete [] this->argv_;
}

void
Config::process()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorConfig::process()\n")
  ));

  if( this->configFile_.empty()) {
    // No filename, no processing.
    return;
  }

  /// @TODO: Implement the file parse and argument value formation.
}

}} // End namespace OpenDDS::Federator

