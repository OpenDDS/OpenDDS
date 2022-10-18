/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"
#include "dds/DCPS/debug.h"
#include "FederatorConfig.h"
#include "FederatorC.h"
#include "ace/Configuration.h"
#include "ace/Configuration_Import_Export.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_strings.h"
#include "ace/OS_NS_sys_time.h"

#include <algorithm>

#if !defined (__ACE_INLINE__)
# include "FederatorConfig.inl"
#endif /* !__ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace {

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
  ArgCopier(OpenDDS::Federator::Config* config);

  /// The Functor function operator.
  void operator()(ACE_TCHAR* arg);

private:
  /// The configuration object.
  OpenDDS::Federator::Config* config_;

  /// How to treat the next argument.
  Action action_;
};

ArgCopier::ArgCopier(OpenDDS::Federator::Config* config)
  : config_(config),
    action_(COPY)
{

} // namespace

void
ArgCopier::operator()(ACE_TCHAR* arg)
{
  // Search for command line arguments to process rather than copy.
  if (::OpenDDS::Federator::Config::FEDERATOR_CONFIG_OPTION == arg) {
    // Configuration file option, filename value is next arg.
    this->action_ = FILENAME;
    return;

  } else if (::OpenDDS::Federator::Config::FEDERATOR_ID_OPTION == arg) {
    // Federation Id option, Id value is next arg.
    this->action_ = IDVALUE;
    return;

  } else if (::OpenDDS::Federator::Config::FEDERATE_WITH_OPTION == arg) {
    // Federate with option, IOR is next arg.
    this->action_ = IORVALUE;
    return;
  }

  // Process unrecognized arguments and all values.
  switch (this->action_) {
  case FILENAME:
    // Store the configuration file name.
    this->config_->configFile(arg);
    break;

  case IDVALUE:
    // Capture the federation Id.
    this->config_->federationId().id(ACE_OS::atoi(arg));
    break;

  case IORVALUE:
    // Capture the IOR to federate with.
    this->config_->federateIor(arg);
    break;

  case COPY:
    // Copy other args verbatim.
    this->config_->addArg(arg);
    break;
  }

  this->action_ = COPY;
}

int random_id()
{
  ACE_UINT64 msec;
  ACE_OS::gettimeofday().msec(msec);
  ACE_OS::srand((unsigned int)msec);
  const int r = ACE_OS::rand();
  return r;
}

void hash_endpoint(::CORBA::Long& hash, const char* const endpoint, const size_t len)
{
  std::string toprint(endpoint, len);
  if (len > 0)
  {
    for (size_t i = 0; i < len; i++)
    {
      hash = 31 * hash + endpoint[i];
    }
  }
}

#if defined (ACE_USES_WCHAR)
void hash_endpoint(::CORBA::Long& hash, const wchar_t* const endpoint, const size_t len)
{
  // treat the wchar string as a double length string
  hash_endpoint(hash, reinterpret_cast<const char*>(endpoint), len * 2);
}
#endif

void hash_endpoints(::CORBA::Long& hash, const ACE_TCHAR* const endpoints_str)
{
  const ACE_TCHAR* delim = ACE_TEXT(";");
  const size_t len = ACE_OS::strlen(endpoints_str);
  const ACE_TCHAR* curr = endpoints_str;
  while (curr < endpoints_str + len) {
    const ACE_TCHAR* next = ACE_OS::strstr(curr, delim);
    if (next == 0)
      next = endpoints_str + len;
    hash_endpoint(hash, curr, (next - curr));
    curr = next + 1;
  }
}

::CORBA::Long hash_endpoints(int argc, ACE_TCHAR** argv)
{
  ::CORBA::Long hash = 0;
  bool found = false;
  for (int i = 0; i < argc - 1; ++i) {
    if (ACE_OS::strncasecmp(argv[i], ACE_TEXT("-ORB"), ACE_OS::strlen(ACE_TEXT("-ORB"))) == 0 &&
        (ACE_OS::strcasecmp(ACE_TEXT("-ORBEndpoint"), argv[i]) == 0 ||
         ACE_OS::strcasecmp(ACE_TEXT("-ORBListenEndpoints"), argv[i]) == 0 ||
         ACE_OS::strcasecmp(ACE_TEXT("-ORBLaneEndpoint"), argv[i]) == 0 ||
         ACE_OS::strcasecmp(ACE_TEXT("-ORBLaneListenEndpoints"), argv[i]) == 0)) {
      const ACE_TCHAR* enpoints = argv[++i];
      hash_endpoints(hash, enpoints);
      found = true;
    }
  }
  if (!found) {
    hash = random_id();
  }
  return hash;
}

} // End of anonymous namespace

namespace OpenDDS {
namespace Federator {

const tstring
Config::FEDERATOR_CONFIG_OPTION(ACE_TEXT("-FederatorConfig"));

const tstring
Config::FEDERATOR_ID_OPTION(ACE_TEXT("-FederationId"));

const tstring
Config::FEDERATE_WITH_OPTION(ACE_TEXT("-FederateWith"));

Config::Config(int argc, ACE_TCHAR** argv)
  : argc_(0),
    federationId_(hash_endpoints(argc, argv)),
    federationDomain_(DEFAULT_FEDERATIONDOMAIN),
    federationPort_(-1)
{
  if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) INFO: Federator::Config::Config()\n")));
  }

  // Setup the internal storage.
  this->argv_ = new ACE_TCHAR*[argc + 1](); // argv_[argc] == 0

  // Process the federation arguments.  Copy the uninteresting arguments verbatim.
  ArgCopier argCopier(this);
  std::for_each(&argv[0], &argv[ argc], argCopier);

  // Read and process any configuration file.
  this->processFile();
}

Config::~Config()
{
  if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) INFO: Federator::Config::~FederatorConfig()\n")));
  }

  // We prwn this
  delete [] this->argv_;
}

void
Config::processFile()
{
  if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) INFO: Federator::Config::process()\n")));
  }

  if (this->configFile_.empty()) {
    // No filename, no processing.
    return;
  }

  // Grab a spot to stick the configuration.
  ACE_Configuration_Heap heap;

  if (0 != heap.open()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
               ACE_TEXT("unable to open configuration heap.\n")));
    return;
  }

  // Import the file into our shiny new spot.
  ACE_Ini_ImpExp import(heap);

  if (0 != import.import_config(this->configFile_.c_str())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
               ACE_TEXT("unable to import configuration file.\n")));
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

  if (0 != heap.get_string_value(root, FEDERATION_DOMAIN_KEY, federationDomainString)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
               ACE_TEXT("Unable to obtain value for FederationDomain in root section\n")));
    return;
  }

  // Convert to numeric repository key value.
  this->federationDomain_ = ACE_OS::atoi(federationDomainString.c_str());

  if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t)   FederationDomain == %d\n"),
               this->federationDomain_));
  }

  // Federation Id value - REQUIRED
  ACE_TString federationIdString;

  if (0 != heap.get_string_value(root, FEDERATION_ID_KEY, federationIdString)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
               ACE_TEXT("Unable to obtain value for FederationId in root section\n")));
    return;
  }

  // Convert to numeric repository key value.
  RepoKey idValue = ACE_OS::atoi(federationIdString.c_str());

  // Allow the command line to override the file value.
  if (this->federationId_.overridden()) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t)   FederationId == %d from file ")
               ACE_TEXT("overridden by value %d from command line.\n"),
               idValue,
               this->federationId_.id()));

  } else {
    this->federationId_.id(idValue);

    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t)   FederationId == %d\n"),
                 this->federationId_.id()));
    }
  }

  // Federation port value - REQUIRED
  ACE_TString federationPortString;

  if (0 != heap.get_string_value(root, FEDERATION_PORT_KEY, federationPortString)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Federator::Config::process - ")
               ACE_TEXT("Unable to obtain value for FederationPort in root section\n")));
    return;
  }

  // Convert to numeric repository key value.
  this->federationPort_ = ACE_OS::atoi(federationPortString.c_str());

  if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t)   FederationPort == %d\n"),
               this->federationPort_));
  }
}

} // namespace Federator
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
