// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "dds/DCPS/debug.h"
#include "FederatorConfig.h"
#include "ace/Configuration.h"
#include "ace/Configuration_Import_Export.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#if !defined (__ACE_INLINE__)
# include "FederatorConfig.inl"
#endif /* ! __ACE_INLINE__ */

// Define the following in some way to do what it says.  Default is not to.
// #define ENFORCE_PREFERRED_INTERFACES

namespace { // Anonymous namespace for file scope

  // Configuration file 'interface' section name.
  const ACE_TCHAR INTERFACE_SECTION_NAME[] = ACE_TEXT("interface");

  // FederationId key value.
  const ACE_TCHAR FEDERATION_ID_KEY[] = ACE_TEXT("FederationId");

  // [interface] Hostname key value.
  const ACE_TCHAR HOSTNAME_KEY[] = ACE_TEXT("Hostname");

  // [interface] Route key value.
  const ACE_TCHAR ROUTE_KEY[] = ACE_TEXT("Route");

  // Preferred interfaces option.
  const char* PREFERRED_OPTION = "-ORBPreferredInterfaces";

  // Enforcement argument and value.
  const char* ENFORCE_OPTION = "-ORBEnforcePreferredInterfaces";
  const char* ENFORCE_VALUE  = "yes";

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
    // Handle the file argument and its value.
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
 : federationId_( 0)
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Federator::Config::Config()\n")
    ));
  }

  // Remove one option/value pair, add one option/value pair.
  int additionalSlots = 0;

#ifdef ENFORCE_PREFERRED_INTERFACES
  // Remove one option/value pair, add two option/value pairs.
  additionalSlots = 2;
#endif /* ENFORCE_PREFERRED_INTERFACES */

  // Setup the internal storage.
  //
  // N.B. We will be adding two arguments and their respective values,
  //      but we will also be removing the configuration file option and
  //      its value for a total of 2 additional slots.  If there is no
  //      configuration file option, we will not be removing any
  //      arguments, but neither will we be adding any.
  //
  this->argc_ = argc;
  this->argv_ = new char*[ argc + additionalSlots];

  // Copy the existing arguments verbatim.
  ArgCopier argCopier( this->argv_, this->configFile_);
  std::for_each( &argv[0], &argv[ argc], argCopier);

  // Read and process any configuration file.
  this->processFile();

  // Form any preferred interface option value.
  this->buildInterfaceList();
  if( this->interfaceList_.size() > 0) {
    this->argv_[ this->argc_++] = const_cast<char*>( PREFERRED_OPTION);
    this->argv_[ this->argc_++] = const_cast<char*>( this->interfaceList_.c_str());

    if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) INFO: Federator::Config - added option: ")
        ACE_TEXT("'%s %s' to command line arguments.\n"),
        PREFERRED_OPTION,
        this->interfaceList_.c_str()
      ));
    }
  }

#ifdef ENFORCE_PREFERRED_INTERFACES
  this->argv_[ this->argc_++] = const_cast<char*>( ENFORCE_OPTION);
  this->argv_[ this->argc_++] = const_cast<char*>( ENFORCE_VALUE);

  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Federator::Config - added option: ")
      ACE_TEXT("'%s %s' to command line arguments.\n"),
      ENFORCE_OPTION,
      ENFORCE_VALUE
    ));
  }
#endif /* ENFORCE_PREFERRED_INTERFACES */
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
  if( 0 != import.import_config( this->configFile_.c_str())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
      ACE_TEXT("unable to import configuration file.\n")
    ));
    return;
  }

  // Configuration file format:
  //
  //   FederationId = <number>                         (REQUIRED)
  //
  //   [interface/<name>]                              (0 or MORE)
  //   Hostname = <remote hostname or dotted decimal>  (REQUIRED)
  //   Route    = <interface>                          (REQUIRED)
  //

  const ACE_Configuration_Section_Key &root = heap.root_section();
  ACE_Configuration_Section_Key interfaceKey;
  if( heap.open_section (root, INTERFACE_SECTION_NAME, 0, interfaceKey) != 0) {
    if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any interface (sub)section.  No routes will be generated.
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) INFO: Federator::Config::process - ")
        ACE_TEXT("failed to open [%s] section.\n"),
        INTERFACE_SECTION_NAME
      ));
    }
    return;

  } else {
    // Grab the common configuration settings.
    ACE_TString federationIdString;
    if( 0 != heap.get_string_value( root, FEDERATION_ID_KEY, federationIdString)) {
      ACE_ERROR(( LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
        ACE_TEXT("Unable to obtain value for FederationId in root section\n")
      ));
      return;
    }

    this->federationId_ = ACE_OS::atoi( federationIdString.c_str());
    if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t)   FederationId == %d\n"),
        this->federationId_
      ));
    }

    // Now iterate through the [interface] sections and process them.
    ACE_TString sectionName;
    for( int index = 0;
         (0 == heap.enumerate_sections( interfaceKey, index, sectionName));
         ++index) {
      if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG(( LM_DEBUG, ACE_TEXT("(%P|%t) INFO: Examining section: %s\n"), sectionName.c_str()));
      }

      ACE_Configuration_Section_Key sectionKey;
      if( 0 != heap.open_section( interfaceKey, sectionName.c_str(), 0, sectionKey)) {
        ACE_ERROR(( LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
          ACE_TEXT("Unable to open [%s] section.\n"),
          sectionName.c_str()
        ));
        continue;
      }

      ACE_TString hostnameString;
      if( 0 != heap.get_string_value( sectionKey, HOSTNAME_KEY, hostnameString)) {
        ACE_ERROR(( LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
          ACE_TEXT("Unable to obtain value for Hostname in [%s] section\n"),
          sectionName.c_str()));
        continue;
      }

      ACE_TString routeString;
      if( 0 != heap.get_string_value( sectionKey, ROUTE_KEY, routeString)) {
        ACE_ERROR(( LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
          ACE_TEXT("Unable to obtain value for Route in [%s] section\n"),
          sectionName.c_str()));
        continue;
      }

      // Convert to C-string representation to allow the conversion to
      // standard string values in the mapping.
      this->route_[ hostnameString.c_str()] = routeString.c_str();
    }
  }

}

void
Config::buildInterfaceList()
{
  std::string separator;
  for( HostToRouteMap::iterator current = this->route_.begin();
       current != this->route_.end();
       ++current) {
    this->interfaceList_ += separator
                         +  current->first  + ":" +  current->second;
    separator = ",";
  }
}

}} // End namespace OpenDDS::Federator

