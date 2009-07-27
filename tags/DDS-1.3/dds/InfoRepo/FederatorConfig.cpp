// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "dds/DCPS/debug.h"
#include "FederatorConfig.h"
#include "FederatorC.h"
#include "ace/Configuration.h"
#include "ace/Configuration_Import_Export.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include <algorithm>

#if !defined (__ACE_INLINE__)
# include "FederatorConfig.inl"
#endif /* ! __ACE_INLINE__ */

namespace { // Anonymous namespace for file scope

  // FederationDomain key value.
  const ACE_TCHAR FEDERATION_DOMAIN_KEY[] = ACE_TEXT("FederationDomain");

  // FederationId key value.
  const ACE_TCHAR FEDERATION_ID_KEY[] = ACE_TEXT("FederationId");

  // FederationId key value.
  const ACE_TCHAR FEDERATION_PORT_KEY[] = ACE_TEXT("FederationPort");

  /// Define an argument copying functor.
  class ArgCopier {
    public:
      /// Identify the action to take on the next argument.
      enum Action { COPY, FILENAME, IDVALUE, IORVALUE };

      /// Construct with a target pointer array.
      ArgCopier( OpenDDS::Federator::Config* config);

      /// The Functor function operator.
      void operator()( ACE_TCHAR* arg);

    private:
      /// The configuration object.
      OpenDDS::Federator::Config* config_;

      /// How to treat the next argument.
      Action action_;
  };

  ArgCopier::ArgCopier( OpenDDS::Federator::Config* config)
   : config_( config),
     action_( COPY)
  {
  }

  void
  ArgCopier::operator()( ACE_TCHAR* arg)
  {
    // Search for command line arguments to process rather than copy.
    if( ::OpenDDS::Federator::Config::FEDERATOR_CONFIG_OPTION == arg) {
      // Configuration file option, filename value is next arg.
      this->action_ = FILENAME;
      return;

    } else if( ::OpenDDS::Federator::Config::FEDERATOR_ID_OPTION == arg) {
      // Federation Id option, Id value is next arg.
      this->action_ = IDVALUE;
      return;

    } else if( ::OpenDDS::Federator::Config::FEDERATE_WITH_OPTION == arg) {
      // Federate with option, IOR is next arg.
      this->action_ = IORVALUE;
      return;
    }

    // Process unrecognized arguments and all values.
    switch( this->action_) {
      case FILENAME:
        // Store the configuration file name.
        this->config_->configFile() = arg;
        break;

      case IDVALUE:
        // Capture the federation Id.
        this->config_->federationId() = ACE_OS::atoi( arg);
        break;

      case IORVALUE:
        // Capture the IOR to federate with.
        this->config_->federateIor() = arg;
        break;

      case COPY:
        // Copy other args verbatim.
        this->config_->argv()[ this->config_->argc()++] = arg;
        break;
    }
    this->action_ = COPY;
  }

} // End of anonymous namespace

namespace OpenDDS { namespace Federator {

const tstring
Config::FEDERATOR_CONFIG_OPTION(ACE_TEXT("-FederatorConfig"));

const tstring
Config::FEDERATOR_ID_OPTION(ACE_TEXT("-FederationId"));

const tstring
Config::FEDERATE_WITH_OPTION(ACE_TEXT("-FederateWith"));

Config::Config( int argc, ACE_TCHAR** argv)
 : argc_( 0),
   federationId_( NIL_REPOSITORY),
   federationDomain_( DEFAULT_FEDERATIONDOMAIN),
   federationPort_( -1)
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Federator::Config::Config()\n")
    ));
  }

  // Setup the internal storage.
  this->argv_ = new ACE_TCHAR*[ argc];

  // Process the federation arguments.  Copy the uninteresting arguments verbatim.
  ArgCopier argCopier( this);
  std::for_each( &argv[0], &argv[ argc], argCopier);

  // Read and process any configuration file.
  this->processFile();
}

Config::~Config()
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Federator::Config::~FederatorConfig()\n")
    ));
  }

  // We prwn this
  delete [] this->argv_;
}

void
Config::processFile()
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Federator::Config::process()\n")
    ));
  }

  if( this->configFile_.empty()) {
    // No filename, no processing.
    return;
  }

  // Grab a spot to stick the configuration.
  ACE_Configuration_Heap heap;
  if( 0 != heap.open()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("unable to open configuration heap.\n")
    ));
    return;
  }

  // Import the file into our shiny new spot.
  ACE_Ini_ImpExp import( heap);
  if( 0 != import.import_config(this->configFile_.c_str())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("unable to import configuration file.\n")
    ));
    return;
  }

  // Configuration file format:
  //
  //   FederationDomain = <number>                       (REQUIRED)
  //   FederationId     = <number>                       (REQUIRED)
  //   FederationPort   = <number>                       (REQUIRED)
  //

  // Grab the common configuration settings.
  const ACE_Configuration_Section_Key &root = heap.root_section();

  // Federation Domain value - REQUIRED
  ACE_TString federationDomainString;
  if( 0 != heap.get_string_value( root, FEDERATION_DOMAIN_KEY, federationDomainString)) {
    ACE_ERROR(( LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("Unable to obtain value for FederationDomain in root section\n")
    ));
    return;
  }

  // Convert to numeric repository key value.
  this->federationDomain_ = ACE_OS::atoi( federationDomainString.c_str());
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t)   FederationDomain == %d\n"),
      this->federationDomain_
    ));
  }

  // Federation Id value - REQUIRED
  ACE_TString federationIdString;
  if( 0 != heap.get_string_value( root, FEDERATION_ID_KEY, federationIdString)) {
    ACE_ERROR(( LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("Unable to obtain value for FederationId in root section\n")
    ));
    return;
  }

  // Convert to numeric repository key value.
  RepoKey idValue = ACE_OS::atoi( federationIdString.c_str());

  // Allow the command line to override the file value.
  if( this->federationId_ != NIL_REPOSITORY) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t)   FederationId == %d from file ")
      ACE_TEXT("overridden by value %d from command line.\n"),
      idValue,
      this->federationId_
    ));

  } else {
    this->federationId_ = idValue;
    if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t)   FederationId == %d\n"),
        this->federationId_
      ));
    }
  }

  // Federation port value - REQUIRED
  ACE_TString federationPortString;
  if( 0 != heap.get_string_value( root, FEDERATION_PORT_KEY, federationPortString)) {
    ACE_ERROR(( LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("Unable to obtain value for FederationPort in root section\n")
    ));
    return;
  }

  // Convert to numeric repository key value.
  this->federationPort_ = ACE_OS::atoi( federationPortString.c_str());
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t)   FederationPort == %d\n"),
      this->federationPort_
    ));
  }

}

}} // End namespace OpenDDS::Federator

