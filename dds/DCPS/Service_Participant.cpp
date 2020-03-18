/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "WaitSet.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "debug.h"
#include "Service_Participant.h"
#include "BuiltInTopicUtils.h"
#include "DataDurabilityCache.h"
#include "GuidConverter.h"
#include "MonitorFactory.h"
#include "ConfigUtils.h"
#include "RecorderImpl.h"
#include "ReplayerImpl.h"
#include "NetworkConfigMonitor.h"
#include "NetworkConfigModifier.h"
#include "LinuxNetworkConfigMonitor.h"
#include "StaticDiscovery.h"
#if defined(OPENDDS_SECURITY)
#include "security/framework/SecurityRegistry.h"
#endif

#include "ace/config.h"
#include "ace/Singleton.h"
#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"
#include "ace/Configuration_Import_Export.h"
#include "ace/Service_Config.h"
#include "ace/Argv_Type_Converter.h"
#include "ace/Auto_Ptr.h"
#include "ace/Sched_Params.h"
#include "ace/Malloc_Allocator.h"
#include "ace/OS_NS_unistd.h"

#ifdef OPENDDS_SAFETY_PROFILE
#include <stdio.h> // <cstdio> after FaceCTS bug 623 is fixed
#else
#include <fstream>
#endif

#if !defined (__ACE_INLINE__)
#include "Service_Participant.inl"
#endif /* __ACE_INLINE__ */

namespace {

void set_log_file_name(const char* fname)
{
#ifdef OPENDDS_SAFETY_PROFILE
  ACE_LOG_MSG->msg_ostream(fopen(fname, "a"), true);
#else
  std::ofstream* output_stream = new std::ofstream(fname, ios::app);
  if (output_stream->bad()) {
    delete output_stream;
  } else {
    ACE_LOG_MSG->msg_ostream(output_stream, true);
  }
#endif
  ACE_LOG_MSG->clr_flags(ACE_Log_Msg::STDERR | ACE_Log_Msg::LOGGER);
  ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
}


void set_log_verbose(unsigned long verbose_logging)
{
  // Code copied from TAO_ORB_Core::init() in
  // TAO version 1.6a_p13.

  typedef void (ACE_Log_Msg::*PTMF)(u_long);
  PTMF flagop = &ACE_Log_Msg::set_flags;
  u_long value;

  switch (verbose_logging)
    {
    case 0:
      flagop = &ACE_Log_Msg::clr_flags;
      value = ACE_Log_Msg::VERBOSE | ACE_Log_Msg::VERBOSE_LITE;
      break;
    case 1:
      value = ACE_Log_Msg::VERBOSE_LITE; break;
    default:
      value = ACE_Log_Msg::VERBOSE; break;
    }

  (ACE_LOG_MSG->*flagop)(value);
}


}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

int Service_Participant::zero_argc = 0;

const size_t DEFAULT_NUM_CHUNKS = 20;

const size_t DEFAULT_CHUNK_MULTIPLIER = 10;

const int DEFAULT_FEDERATION_RECOVERY_DURATION       = 900; // 15 minutes in seconds.
const int DEFAULT_FEDERATION_INITIAL_BACKOFF_SECONDS = 1;   // Wait only 1 second.
const int DEFAULT_FEDERATION_BACKOFF_MULTIPLIER      = 2;   // Exponential backoff.
const int DEFAULT_FEDERATION_LIVELINESS              = 60;  // 1 minute hearbeat.

const int BIT_LOOKUP_DURATION_MSEC = 2000;

static ACE_TString config_fname(ACE_TEXT(""));

static const ACE_TCHAR DEFAULT_REPO_IOR[] = ACE_TEXT("file://repo.ior");

static const char DEFAULT_PERSISTENT_DATA_DIR[] = "OpenDDS-durable-data-dir";

static const ACE_TCHAR COMMON_SECTION_NAME[] = ACE_TEXT("common");
static const ACE_TCHAR DOMAIN_SECTION_NAME[] = ACE_TEXT("domain");
static const ACE_TCHAR REPO_SECTION_NAME[]   = ACE_TEXT("repository");
static const ACE_TCHAR RTPS_SECTION_NAME[]   = ACE_TEXT("rtps_discovery");

static bool got_debug_level = false;
static bool got_use_rti_serialization = false;
static bool got_info = false;
static bool got_chunks = false;
static bool got_chunk_association_multiplier = false;
static bool got_liveliness_factor = false;
static bool got_bit_transport_port = false;
static bool got_bit_transport_ip = false;
static bool got_bit_lookup_duration_msec = false;
static bool got_global_transport_config = false;
static bool got_bit_flag = false;

#if defined(OPENDDS_SECURITY)
static bool got_security_flag = false;
static bool got_security_debug = false;
static bool got_security_fake_encryption = false;
#endif

static bool got_publisher_content_filter = false;
static bool got_transport_debug_level = false;
static bool got_pending_timeout = false;
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
static bool got_persistent_data_dir = false;
#endif
static bool got_default_discovery = false;
#ifndef DDS_DEFAULT_DISCOVERY_METHOD
# ifdef OPENDDS_SAFETY_PROFILE
#  define DDS_DEFAULT_DISCOVERY_METHOD Discovery::DEFAULT_RTPS
# else
#  define DDS_DEFAULT_DISCOVERY_METHOD Discovery::DEFAULT_REPO
# endif
#endif
static bool got_log_fname = false;
static bool got_log_verbose = false;
static bool got_default_address = false;
static bool got_bidir_giop = false;
static bool got_monitor = false;

Service_Participant::Service_Participant()
  :
#ifndef OPENDDS_SAFETY_PROFILE
    ORB_argv_(false /*substitute_env_args*/),
#endif
    reactor_task_(false),
    defaultDiscovery_(DDS_DEFAULT_DISCOVERY_METHOD),
    n_chunks_(DEFAULT_NUM_CHUNKS),
    association_chunk_multiplier_(DEFAULT_CHUNK_MULTIPLIER),
    liveliness_factor_(80),
    bit_transport_port_(0),
    bit_enabled_(
#ifdef DDS_HAS_MINIMUM_BIT
      false
#else
      true
#endif
    ),
#if defined(OPENDDS_SECURITY)
    security_enabled_(false),
#endif
    bit_lookup_duration_msec_(BIT_LOOKUP_DURATION_MSEC),
    global_transport_config_(ACE_TEXT("")),
    monitor_factory_(0),
    federation_recovery_duration_(DEFAULT_FEDERATION_RECOVERY_DURATION),
    federation_initial_backoff_seconds_(DEFAULT_FEDERATION_INITIAL_BACKOFF_SECONDS),
    federation_backoff_multiplier_(DEFAULT_FEDERATION_BACKOFF_MULTIPLIER),
    federation_liveliness_(DEFAULT_FEDERATION_LIVELINESS),
#if defined OPENDDS_SAFETY_PROFILE && defined ACE_HAS_ALLOC_HOOKS
    pool_size_(1024*1024*16),
    pool_granularity_(8),
#endif
    scheduler_(-1),
    priority_min_(0),
    priority_max_(0),
    publisher_content_filter_(true),
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    persistent_data_dir_(DEFAULT_PERSISTENT_DATA_DIR),
#endif
    bidir_giop_(true),
    monitor_enabled_(false),
    shut_down_(false),
    shutdown_listener_(0),
    default_configuration_file_(ACE_TEXT(""))
{
  initialize();
}

Service_Participant::~Service_Participant()
{
  shutdown();

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               "%T (%P|%t) Service_Participant::~Service_Participant()\n"));
  }
}

Service_Participant*
Service_Participant::instance()
{
  // Hide the template instantiation to prevent multiple instances
  // from being created.

  return ACE_Singleton<Service_Participant, ACE_SYNCH_MUTEX>::instance();
}

ACE_Reactor_Timer_Interface*
Service_Participant::timer()
{
  return reactor_task_.get_reactor();
}

ACE_Reactor*
Service_Participant::reactor()
{
  return reactor_task_.get_reactor();
}

ACE_thread_t
Service_Participant::reactor_owner() const
{
  return reactor_task_.get_reactor_owner();
}

void
Service_Participant::shutdown()
{
  // When we are already shutdown just let the shutdown be a noop
  if (shut_down_) {
    return;
  }

  if (shutdown_listener_) {
    shutdown_listener_->notify_shutdown();
  }

  shut_down_ = true;
  try {
    TransportRegistry::instance()->release();
    {
      ACE_GUARD(TAO_SYNCH_MUTEX, guard, this->factory_lock_);

      if (dp_factory_servant_)
        dp_factory_servant_->cleanup();
      dp_factory_servant_.reset();

      domainRepoMap_.clear();

      {
        ACE_GUARD(ACE_Thread_Mutex, guard, network_config_monitor_lock_);
        if (network_config_monitor_) {
          network_config_monitor_->close();
          network_config_monitor_.reset();
        }
      }

      reactor_task_.stop();

      discoveryMap_.clear();

  #ifndef OPENDDS_NO_PERSISTENCE_PROFILE
      transient_data_cache_.reset();
      persistent_data_cache_.reset();
  #endif

      discovery_types_.clear();
      monitor_factory_ = 0;
    }
    TransportRegistry::close();
#if defined(OPENDDS_SECURITY)
    OpenDDS::Security::SecurityRegistry::close();
#endif
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: Service_Participant::shutdown");
  }
}

#ifdef ACE_USES_WCHAR
DDS::DomainParticipantFactory_ptr
Service_Participant::get_domain_participant_factory(int &argc,
                                                    char *argv[])
{
  ACE_Argv_Type_Converter converter(argc, argv);
  return get_domain_participant_factory(converter.get_argc(),
                                        converter.get_TCHAR_argv());
}
#endif

DDS::DomainParticipantFactory_ptr
Service_Participant::get_domain_participant_factory(int &argc,
                                                    ACE_TCHAR *argv[])
{
  if (!dp_factory_servant_) {
    ACE_GUARD_RETURN(TAO_SYNCH_MUTEX,
                     guard,
                     this->factory_lock_,
                     DDS::DomainParticipantFactory::_nil());

    shut_down_ = false;
    if (!dp_factory_servant_) {
      // This used to be a call to ORB_init().  Since the ORB is now managed
      // by InfoRepoDiscovery, just save the -ORB* args for later use.
      // The exceptions are -ORBLogFile and -ORBVerboseLogging, which
      // are processed by the service participant. This allows log control
      // even if an ORB is not being used.
#ifndef OPENDDS_SAFETY_PROFILE
      ORB_argv_.add(ACE_TEXT("unused_arg_0"));
#endif
      /* NOTE ABOUT ADDING NEW OPTIONS HERE ==================================
       *
       * The argument parsing here is simple. It will match substrings of
       * options even if that isn't the whole option. For example; If
       * "-DCPSSecurity" is checked before "-DCPSSecurityDebug" and
       * "-DCPSSecurityDebug" is passed, it will match "-DCPSSecurity". Check
       * to make sure the order is correct.
       *
       * TODO/TBD: Create or make use of a stricter command line argument
       * parsing method/library. Something where we can define the format of
       * the argument and it will handle it better. Maybe could be integrated
       * into the config parsing, which has most of these options.
       */
      ACE_Arg_Shifter shifter(argc, argv);
      while (shifter.is_anything_left()) {
        if (shifter.cur_arg_strncasecmp(ACE_TEXT("-ORBLogFile")) == 0) {
          shifter.ignore_arg();
        } else if (shifter.cur_arg_strncasecmp(ACE_TEXT("-ORBVerboseLogging")) == 0) {
          shifter.ignore_arg();
        } else if (shifter.cur_arg_strncasecmp(ACE_TEXT("-ORB")) < 0) {
          shifter.ignore_arg();
        } else {
#ifndef OPENDDS_SAFETY_PROFILE
          ORB_argv_.add(shifter.get_current());
#endif
          shifter.consume_arg();
          if (shifter.is_parameter_next()) {
#ifndef OPENDDS_SAFETY_PROFILE
            ORB_argv_.add(shifter.get_current(), true /*quote_arg*/);
#endif
            shifter.consume_arg();
          }
        }
      }

      if (parse_args(argc, argv) != 0) {
        return DDS::DomainParticipantFactory::_nil();
      }

      if (config_fname.is_empty() && !default_configuration_file_.is_empty()) {
        config_fname = default_configuration_file_;
      }

      if (config_fname.is_empty()) {
        if (DCPS_debug_level) {
          ACE_DEBUG((LM_NOTICE,
                     ACE_TEXT("(%P|%t) NOTICE: not using file configuration - no configuration ")
                     ACE_TEXT("file specified.\n")));
        }

      } else {
        // Load configuration only if the configuration
        // file exists.
        FILE* in = ACE_OS::fopen(config_fname.c_str(),
                                 ACE_TEXT("r"));

        if (!in) {
          ACE_DEBUG((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: not using file configuration - ")
                     ACE_TEXT("can not open \"%s\" for reading. %p\n"),
                     config_fname.c_str(), ACE_TEXT("fopen")));

        } else {
          ACE_OS::fclose(in);

          if (this->load_configuration() != 0) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: Service_Participant::get_domain_participant_factory: ")
                       ACE_TEXT("load_configuration() failed.\n")));
            return DDS::DomainParticipantFactory::_nil();
          }
        }
      }

#if defined OPENDDS_SAFETY_PROFILE && defined ACE_HAS_ALLOC_HOOKS
      // For non-FACE tests, configure pool
      configure_pool();
#endif

      // Establish the default scheduling mechanism and
      // priority here.  Sadly, the ORB is already
      // initialized so we have no influence over its
      // scheduling or thread priority(ies).

      /// @TODO: Move ORB initialization to after the
      ///        configuration file is processed and the
      ///        initial scheduling policy and priority are
      ///        established.
      this->initializeScheduling();

      dp_factory_servant_ = make_rch<DomainParticipantFactoryImpl>();

      reactor_task_.open(0);

      if (this->monitor_enabled_) {
#if !defined(ACE_AS_STATIC_LIBS)
        ACE_TString directive = ACE_TEXT("dynamic OpenDDS_Monitor Service_Object * OpenDDS_monitor:_make_MonitorFactoryImpl()");
        ACE_Service_Config::process_directive(directive.c_str());
#endif
        this->monitor_factory_ =
          ACE_Dynamic_Service<MonitorFactory>::instance ("OpenDDS_Monitor");

        if (this->monitor_factory_ == 0) {
          if (this->monitor_enabled_) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("ERROR: Service_Participant::get_domain_participant_factory, ")
                       ACE_TEXT("Unable to enable monitor factory.\n")));
          }
        }
      }

      if (this->monitor_factory_ == 0) {
        // Use the stubbed factory
        MonitorFactory::service_initialize();
        this->monitor_factory_ =
          ACE_Dynamic_Service<MonitorFactory>::instance ("OpenDDS_Monitor_Default");
      }
      if (this->monitor_enabled_) {
        this->monitor_factory_->initialize();
      }

      this->monitor_.reset(this->monitor_factory_->create_sp_monitor(this));
    }
  }

  return DDS::DomainParticipantFactory::_duplicate(dp_factory_servant_.in());
}

int
Service_Participant::parse_args(int &argc, ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left()) {
    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSDebugLevel"))) != 0) {
      set_DCPS_debug_level(ACE_OS::atoi(currentArg));
      arg_shifter.consume_arg();
      got_debug_level = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSInfoRepo"))) != 0) {
      this->set_repo_ior(currentArg, Discovery::DEFAULT_REPO);
      arg_shifter.consume_arg();
      got_info = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSRTISerialization"))) != 0) {
      if (ACE_OS::atoi(currentArg) == 0) {
        ACE_ERROR((LM_WARNING,
          ACE_TEXT("(%P|%t) WARNING: Service_Participant::parse_args ")
          ACE_TEXT("Argument ignored: DCPSRTISerialization is required to be enabled\n")));
      }
      arg_shifter.consume_arg();
      got_use_rti_serialization = true;
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSChunks"))) != 0) {
      n_chunks_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_chunks = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSChunkAssociationMultiplier"))) != 0) {
      association_chunk_multiplier_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_chunk_association_multiplier = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSConfigFile"))) != 0) {
      config_fname = currentArg;
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSLivelinessFactor"))) != 0) {
      liveliness_factor_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_liveliness_factor = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBitTransportPort"))) != 0) {
      /// No need to guard this insertion as we are still single
      /// threaded here.
      this->bit_transport_port_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_bit_transport_port = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBitTransportIPAddress"))) != 0) {
      /// No need to guard this insertion as we are still single
      /// threaded here.
      this->bit_transport_ip_ = currentArg;
      arg_shifter.consume_arg();
      got_bit_transport_ip = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBitLookupDurationMsec"))) != 0) {
      bit_lookup_duration_msec_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_bit_lookup_duration_msec = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSGlobalTransportConfig"))) != 0) {
      global_transport_config_ = currentArg;
      arg_shifter.consume_arg();
      got_global_transport_config = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBit"))) != 0) {
      bit_enabled_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_bit_flag = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSTransportDebugLevel"))) != 0) {
      OpenDDS::DCPS::Transport_debug_level = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_transport_debug_level = true;

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSPersistentDataDir"))) != 0) {
      this->persistent_data_dir_ = ACE_TEXT_ALWAYS_CHAR(currentArg);
      arg_shifter.consume_arg();
      got_persistent_data_dir = true;
#endif

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSPendingTimeout"))) != 0) {
      pending_timeout_ = TimeDuration(ACE_OS::atoi(currentArg));
      arg_shifter.consume_arg();
      got_pending_timeout = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSPublisherContentFilter"))) != 0) {
      this->publisher_content_filter_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_publisher_content_filter = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSDefaultDiscovery"))) != 0) {
      this->defaultDiscovery_ = ACE_TEXT_ALWAYS_CHAR(currentArg);
      arg_shifter.consume_arg();
      got_default_discovery = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBidirGIOP"))) != 0) {
      bidir_giop_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_bidir_giop = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-FederationRecoveryDuration"))) != 0) {
      this->federation_recovery_duration_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-FederationInitialBackoffSeconds"))) != 0) {
      this->federation_initial_backoff_seconds_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-FederationBackoffMultiplier"))) != 0) {
      this->federation_backoff_multiplier_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-FederationLivelinessDuration"))) != 0) {
      this->federation_liveliness_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-ORBLogFile"))) != 0) {
      set_log_file_name(ACE_TEXT_ALWAYS_CHAR(currentArg));
      arg_shifter.consume_arg();
      got_log_fname = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-ORBVerboseLogging"))) != 0) {
      set_log_verbose(ACE_OS::atoi(currentArg));
      arg_shifter.consume_arg();
      got_log_verbose = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSDefaultAddress"))) != 0) {
      this->default_address_ = ACE_TEXT_ALWAYS_CHAR(currentArg);
      arg_shifter.consume_arg();
      got_default_address = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSMonitor"))) != 0) {
      this->monitor_enabled_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_monitor = true;

#if defined(OPENDDS_SECURITY)
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSSecurityDebugLevel"))) != 0) {
      security_debug.set_debug_level(ACE_OS::atoi(currentArg));
      arg_shifter.consume_arg();
      got_security_debug = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSSecurityDebug"))) != 0) {
      security_debug.parse_flags(currentArg);
      arg_shifter.consume_arg();
      got_security_debug = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSSecurityFakeEncryption"))) != 0) {
      security_debug.fake_encryption = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_security_fake_encryption = true;

    // Must be last "-DCPSSecurity*" option, see comment above this arg parsing loop
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSSecurity"))) != 0) {
      security_enabled_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_security_flag = true;

#endif

    } else {
      arg_shifter.ignore_arg();
    }
  }

  // Indicates successful parsing of the command line
  return 0;
}

void
Service_Participant::initialize()
{
  //NOTE: in the future these initial values may be configurable
  //      (to override the Specification's default values
  //       hmm - I guess that would be OK since the user
  //       is overriding them.)
  initial_TransportPriorityQosPolicy_.value = 0;
  initial_LifespanQosPolicy_.duration.sec = DDS::DURATION_INFINITE_SEC;
  initial_LifespanQosPolicy_.duration.nanosec = DDS::DURATION_INFINITE_NSEC;

  initial_DurabilityQosPolicy_.kind = DDS::VOLATILE_DURABILITY_QOS;

  initial_DurabilityServiceQosPolicy_.service_cleanup_delay.sec =
    DDS::DURATION_ZERO_SEC;
  initial_DurabilityServiceQosPolicy_.service_cleanup_delay.nanosec =
    DDS::DURATION_ZERO_NSEC;
  initial_DurabilityServiceQosPolicy_.history_kind =
    DDS::KEEP_LAST_HISTORY_QOS;
  initial_DurabilityServiceQosPolicy_.history_depth = 1;
  initial_DurabilityServiceQosPolicy_.max_samples =
    DDS::LENGTH_UNLIMITED;
  initial_DurabilityServiceQosPolicy_.max_instances =
    DDS::LENGTH_UNLIMITED;
  initial_DurabilityServiceQosPolicy_.max_samples_per_instance =
    DDS::LENGTH_UNLIMITED;

  initial_PresentationQosPolicy_.access_scope = DDS::INSTANCE_PRESENTATION_QOS;
  initial_PresentationQosPolicy_.coherent_access = false;
  initial_PresentationQosPolicy_.ordered_access = false;

  initial_DeadlineQosPolicy_.period.sec = DDS::DURATION_INFINITE_SEC;
  initial_DeadlineQosPolicy_.period.nanosec = DDS::DURATION_INFINITE_NSEC;

  initial_LatencyBudgetQosPolicy_.duration.sec = DDS::DURATION_ZERO_SEC;
  initial_LatencyBudgetQosPolicy_.duration.nanosec = DDS::DURATION_ZERO_NSEC;

  initial_OwnershipQosPolicy_.kind = DDS::SHARED_OWNERSHIP_QOS;
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  initial_OwnershipStrengthQosPolicy_.value = 0;
#endif

  initial_LivelinessQosPolicy_.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
  initial_LivelinessQosPolicy_.lease_duration.sec = DDS::DURATION_INFINITE_SEC;
  initial_LivelinessQosPolicy_.lease_duration.nanosec = DDS::DURATION_INFINITE_NSEC;

  initial_TimeBasedFilterQosPolicy_.minimum_separation.sec = DDS::DURATION_ZERO_SEC;
  initial_TimeBasedFilterQosPolicy_.minimum_separation.nanosec = DDS::DURATION_ZERO_NSEC;

  initial_ReliabilityQosPolicy_.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  initial_ReliabilityQosPolicy_.max_blocking_time.sec = DDS::DURATION_INFINITE_SEC;
  initial_ReliabilityQosPolicy_.max_blocking_time.nanosec = DDS::DURATION_INFINITE_NSEC;

  initial_DestinationOrderQosPolicy_.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

  initial_HistoryQosPolicy_.kind = DDS::KEEP_LAST_HISTORY_QOS;
  initial_HistoryQosPolicy_.depth = 1;

  initial_ResourceLimitsQosPolicy_.max_samples = DDS::LENGTH_UNLIMITED;
  initial_ResourceLimitsQosPolicy_.max_instances = DDS::LENGTH_UNLIMITED;
  initial_ResourceLimitsQosPolicy_.max_samples_per_instance = DDS::LENGTH_UNLIMITED;

  initial_EntityFactoryQosPolicy_.autoenable_created_entities = true;

  initial_WriterDataLifecycleQosPolicy_.autodispose_unregistered_instances = true;

  initial_ReaderDataLifecycleQosPolicy_.autopurge_nowriter_samples_delay.sec = DDS::DURATION_INFINITE_SEC;
  initial_ReaderDataLifecycleQosPolicy_.autopurge_nowriter_samples_delay.nanosec = DDS::DURATION_INFINITE_NSEC;
  initial_ReaderDataLifecycleQosPolicy_.autopurge_disposed_samples_delay.sec = DDS::DURATION_INFINITE_SEC;
  initial_ReaderDataLifecycleQosPolicy_.autopurge_disposed_samples_delay.nanosec = DDS::DURATION_INFINITE_NSEC;

  initial_DomainParticipantQos_.user_data = initial_UserDataQosPolicy_;
  initial_DomainParticipantQos_.entity_factory = initial_EntityFactoryQosPolicy_;
  initial_DomainParticipantFactoryQos_.entity_factory = initial_EntityFactoryQosPolicy_;

  initial_TopicQos_.topic_data = initial_TopicDataQosPolicy_;
  initial_TopicQos_.durability = initial_DurabilityQosPolicy_;
  initial_TopicQos_.durability_service = initial_DurabilityServiceQosPolicy_;
  initial_TopicQos_.deadline = initial_DeadlineQosPolicy_;
  initial_TopicQos_.latency_budget = initial_LatencyBudgetQosPolicy_;
  initial_TopicQos_.liveliness = initial_LivelinessQosPolicy_;
  initial_TopicQos_.reliability = initial_ReliabilityQosPolicy_;
  initial_TopicQos_.destination_order = initial_DestinationOrderQosPolicy_;
  initial_TopicQos_.history = initial_HistoryQosPolicy_;
  initial_TopicQos_.resource_limits = initial_ResourceLimitsQosPolicy_;
  initial_TopicQos_.transport_priority = initial_TransportPriorityQosPolicy_;
  initial_TopicQos_.lifespan = initial_LifespanQosPolicy_;
  initial_TopicQos_.ownership = initial_OwnershipQosPolicy_;

  initial_DataWriterQos_.durability = initial_DurabilityQosPolicy_;
  initial_DataWriterQos_.durability_service = initial_DurabilityServiceQosPolicy_;
  initial_DataWriterQos_.deadline = initial_DeadlineQosPolicy_;
  initial_DataWriterQos_.latency_budget = initial_LatencyBudgetQosPolicy_;
  initial_DataWriterQos_.liveliness = initial_LivelinessQosPolicy_;
  initial_DataWriterQos_.reliability = initial_ReliabilityQosPolicy_;
  initial_DataWriterQos_.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  initial_DataWriterQos_.reliability.max_blocking_time.sec = 0;
  initial_DataWriterQos_.reliability.max_blocking_time.nanosec = 100000000;
  initial_DataWriterQos_.destination_order = initial_DestinationOrderQosPolicy_;
  initial_DataWriterQos_.history = initial_HistoryQosPolicy_;
  initial_DataWriterQos_.resource_limits = initial_ResourceLimitsQosPolicy_;
  initial_DataWriterQos_.transport_priority = initial_TransportPriorityQosPolicy_;
  initial_DataWriterQos_.lifespan = initial_LifespanQosPolicy_;
  initial_DataWriterQos_.user_data = initial_UserDataQosPolicy_;
  initial_DataWriterQos_.ownership = initial_OwnershipQosPolicy_;
#ifdef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  initial_DataWriterQos_.ownership_strength.value = 0;
#else
  initial_DataWriterQos_.ownership_strength = initial_OwnershipStrengthQosPolicy_;
#endif
  initial_DataWriterQos_.writer_data_lifecycle = initial_WriterDataLifecycleQosPolicy_;

  initial_PublisherQos_.presentation = initial_PresentationQosPolicy_;
  initial_PublisherQos_.partition = initial_PartitionQosPolicy_;
  initial_PublisherQos_.group_data = initial_GroupDataQosPolicy_;
  initial_PublisherQos_.entity_factory = initial_EntityFactoryQosPolicy_;

  initial_DataReaderQos_.durability = initial_DurabilityQosPolicy_;
  initial_DataReaderQos_.deadline = initial_DeadlineQosPolicy_;
  initial_DataReaderQos_.latency_budget = initial_LatencyBudgetQosPolicy_;
  initial_DataReaderQos_.liveliness = initial_LivelinessQosPolicy_;
  initial_DataReaderQos_.reliability = initial_ReliabilityQosPolicy_;
  initial_DataReaderQos_.destination_order = initial_DestinationOrderQosPolicy_;
  initial_DataReaderQos_.history = initial_HistoryQosPolicy_;
  initial_DataReaderQos_.resource_limits = initial_ResourceLimitsQosPolicy_;
  initial_DataReaderQos_.user_data = initial_UserDataQosPolicy_;
  initial_DataReaderQos_.time_based_filter = initial_TimeBasedFilterQosPolicy_;
  initial_DataReaderQos_.ownership = initial_OwnershipQosPolicy_;
  initial_DataReaderQos_.reader_data_lifecycle = initial_ReaderDataLifecycleQosPolicy_;

  initial_SubscriberQos_.presentation = initial_PresentationQosPolicy_;
  initial_SubscriberQos_.partition = initial_PartitionQosPolicy_;
  initial_SubscriberQos_.group_data = initial_GroupDataQosPolicy_;
  initial_SubscriberQos_.entity_factory = initial_EntityFactoryQosPolicy_;
}

void
Service_Participant::initializeScheduling()
{
  //
  // Establish the scheduler if specified.
  //
  if (this->schedulerString_.length() == 0) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: Service_Participant::intializeScheduling() - ")
                 ACE_TEXT("no scheduling policy specified, not setting policy.\n")));
    }

  } else {
    //
    // Translate the scheduling policy to a usable value.
    //
    int ace_scheduler = ACE_SCHED_OTHER;
    this->scheduler_  = THR_SCHED_DEFAULT;

    if (this->schedulerString_ == ACE_TEXT("SCHED_RR")) {
      this->scheduler_ = THR_SCHED_RR;
      ace_scheduler    = ACE_SCHED_RR;

    } else if (this->schedulerString_ == ACE_TEXT("SCHED_FIFO")) {
      this->scheduler_ = THR_SCHED_FIFO;
      ace_scheduler    = ACE_SCHED_FIFO;

    } else if (this->schedulerString_ == ACE_TEXT("SCHED_OTHER")) {
      this->scheduler_ = THR_SCHED_DEFAULT;
      ace_scheduler    = ACE_SCHED_OTHER;

    } else {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Service_Participant::initializeScheduling() - ")
                 ACE_TEXT("unrecognized scheduling policy: %s, set to SCHED_OTHER.\n"),
                 this->schedulerString_.c_str()));
    }

    //
    // Attempt to set the scheduling policy.
    //
#ifdef ACE_WIN32
    ACE_DEBUG((LM_NOTICE,
               ACE_TEXT("(%P|%t) NOTICE: Service_Participant::initializeScheduling() - ")
               ACE_TEXT("scheduling is not implemented on Win32.\n")));
    ACE_UNUSED_ARG(ace_scheduler);
#else
    ACE_Sched_Params params(
      ace_scheduler,
      ACE_Sched_Params::priority_min(ace_scheduler),
      ACE_SCOPE_THREAD,
      schedulerQuantum_.value());

    if (ACE_OS::sched_params(params) != 0) {
      if (ACE_OS::last_error() == EPERM) {
        ACE_DEBUG((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: Service_Participant::initializeScheduling() - ")
                   ACE_TEXT("user is not superuser, requested scheduler not set.\n")));

      } else {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::initializeScheduling() - ")
                   ACE_TEXT("sched_params failed: %m.\n")));
      }

      // Reset the scheduler value(s) if we did not succeed.
      this->scheduler_ = -1;
      ace_scheduler    = ACE_SCHED_OTHER;

    } else if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Service_Participant::initializeScheduling() - ")
                 ACE_TEXT("scheduling policy set to %s(%d).\n"),
                 this->schedulerString_.c_str()));
    }

    //
    // Setup some scheduler specific information for later use.
    //
    this->priority_min_ = ACE_Sched_Params::priority_min(ace_scheduler, ACE_SCOPE_THREAD);
    this->priority_max_ = ACE_Sched_Params::priority_max(ace_scheduler, ACE_SCOPE_THREAD);
#endif // ACE_WIN32
  }
}

#ifdef DDS_HAS_WCHAR
bool
Service_Participant::set_repo_ior(const wchar_t* ior,
                                  Discovery::RepoKey key,
                                  bool attach_participant)
{
  return set_repo_ior(ACE_Wide_To_Ascii(ior).char_rep(), key, attach_participant);
}
#endif

bool
Service_Participant::set_repo_ior(const char* ior,
                                  Discovery::RepoKey key,
                                  bool attach_participant)
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Service_Participant::set_repo_ior: Repo[ %C] == %C\n"),
               key.c_str(), ior));
  }

  // This is a global used for the bizzare commandline/configfile
  // processing done for this class.
  got_info = true;

  if (key == "-1") {
    key = Discovery::DEFAULT_REPO;
  }

  const OPENDDS_STRING repo_type = ACE_TEXT_ALWAYS_CHAR(REPO_SECTION_NAME);
  if (!discovery_types_.count(repo_type)) {
    // Re-use a transport registry function to attempt a dynamic load of the
    // library that implements the 'repo_type' (InfoRepoDiscovery)
    TheTransportRegistry->load_transport_lib(repo_type);
  }

  if (discovery_types_.count(repo_type)) {
    ACE_Configuration_Heap cf;
    cf.open();
    ACE_Configuration_Section_Key sect_key;
    ACE_TString section = REPO_SECTION_NAME;
    section += ACE_TEXT('\\');
    section += ACE_TEXT_CHAR_TO_TCHAR(key.c_str());
    cf.open_section(cf.root_section(), section.c_str(), 1 /*create*/, sect_key);
    cf.set_string_value(sect_key, ACE_TEXT("RepositoryIor"),
                        ACE_TEXT_CHAR_TO_TCHAR(ior));

    discovery_types_[repo_type]->discovery_config(cf);

    this->remap_domains(key, key, attach_participant);
    return true;
  }

  ACE_ERROR_RETURN((LM_ERROR,
                    ACE_TEXT("(%P|%t) Service_Participant::set_repo_ior ")
                    ACE_TEXT("ERROR - no discovery type registered for ")
                    ACE_TEXT("InfoRepoDiscovery\n")),
                   false);
}

void
Service_Participant::remap_domains(Discovery::RepoKey oldKey,
                                   Discovery::RepoKey newKey,
                                   bool attach_participant)
{
  // Search the mappings for any domains mapped to this repository.
  OPENDDS_VECTOR(DDS::DomainId_t) domainList;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->maps_lock_);

    for (DomainRepoMap::const_iterator current = this->domainRepoMap_.begin();
         current != this->domainRepoMap_.end();
         ++current) {
      if (current->second == oldKey) {
        domainList.push_back(current->first);
      }
    }
  }

  // Remap the domains that were attached to this repository.
  for (unsigned int index = 0; index < domainList.size(); ++index) {
    // For mapped domains, attach their participants by setting the
    // mapping again.
    this->set_repo_domain(domainList[ index], newKey, attach_participant);
  }
}

void
Service_Participant::set_repo_domain(const DDS::DomainId_t domain,
                                     Discovery::RepoKey key,
                                     bool attach_participant)
{
  typedef std::pair<Discovery_rch, RepoId> DiscRepoPair;
  OPENDDS_VECTOR(DiscRepoPair) repoList;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->maps_lock_);
    DomainRepoMap::const_iterator where = this->domainRepoMap_.find(domain);

    if (key == "-1") {
      key = Discovery::DEFAULT_REPO;
    }

    if ((where == this->domainRepoMap_.end()) || (where->second != key)) {
      // Only assign entries into the map when they change the
      // contents.
      this->domainRepoMap_[ domain] = key;

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Service_Participant::set_repo_domain: ")
                   ACE_TEXT("Domain[ %d] = Repo[ %C].\n"),
                   domain, key.c_str()));
      }
    }

    //
    // Make sure that we mark each DomainParticipant for this domain
    // using this repository as attached to this repository.
    //
    // @TODO: Move this note into user documentation.
    // N.B. Calling set_repo() or set_repo_ior() will result in this
    //      code executing again with the new repository.  It is best
    //      to call those routines first when making changes.
    //

    // No servant means no participant.  No worries.
    if (this->dp_factory_servant_) {
      // Map of domains to sets of participants.
      const DomainParticipantFactoryImpl::DPMap& participants
      = this->dp_factory_servant_->participants();

      // Extract the set of participants for the current domain.
      DomainParticipantFactoryImpl::DPMap::const_iterator
      which  = participants.find(domain);

      if (which != participants.end()) {
        // Extract the repository to attach this domain to.
        RepoKeyDiscoveryMap::const_iterator disc_iter = this->discoveryMap_.find(key);

        if (disc_iter != this->discoveryMap_.end()) {
          for (DomainParticipantFactoryImpl::DPSet::const_iterator
               current  = which->second.begin();
               current != which->second.end();
               ++current) {
            try {
              // Attach each DomainParticipant in this domain to this
              // repository.
              RepoId id = (*current)->get_id();
              repoList.push_back(std::make_pair(disc_iter->second, id));

              if (DCPS_debug_level > 0) {
                GuidConverter converter(id);
                ACE_DEBUG((LM_DEBUG,
                           ACE_TEXT("(%P|%t) Service_Participant::set_repo_domain: ")
                           ACE_TEXT("participant %C attached to Repo[ %C].\n"),
                           OPENDDS_STRING(converter).c_str(),
                           key.c_str()));
              }

            } catch (const CORBA::Exception& ex) {
              ex._tao_print_exception(
                "ERROR: Service_Participant::set_repo_domain: failed to attach repository - ");
              return;
            }
          }
        }
      }
    }
  } // End of GUARD scope.

  // Make all of the remote calls after releasing the lock.
  for (unsigned int index = 0; index < repoList.size(); ++index) {
    if (DCPS_debug_level > 0) {
      GuidConverter converter(repoList[ index].second);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Service_Participant::set_repo_domain: ")
                 ACE_TEXT("(%d of %d) attaching domain %d participant %C to Repo[ %C].\n"),
                 (1+index), repoList.size(), domain,
                 OPENDDS_STRING(converter).c_str(),
                 key.c_str()));
    }

    if (attach_participant)
    {
      repoList[ index].first->attach_participant(domain, repoList[ index].second);
    }
  }
}

void
Service_Participant::repository_lost(Discovery::RepoKey key)
{
  // Find the lost repository.
  RepoKeyDiscoveryMap::iterator initialLocation = this->discoveryMap_.find(key);
  RepoKeyDiscoveryMap::iterator current         = initialLocation;

  if (current == this->discoveryMap_.end()) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: Service_Participant::repository_lost: ")
               ACE_TEXT("lost repository %C was not present, ")
               ACE_TEXT("finding another anyway.\n"),
               key.c_str()));

  } else {
    // Start with the repository *after* the lost one.
    ++current;
  }

  // Calculate the bounding end time for attempts.
  const TimeDuration td(federation_recovery_duration());
  const MonotonicTimePoint recoveryFailedTime(MonotonicTimePoint::now() + td);

  // Backoff delay.
  int backoff = this->federation_initial_backoff_seconds();

  // Keep trying until the total recovery time specified is exceeded.
  while (recoveryFailedTime > MonotonicTimePoint::now()) {

    // Wrap to the beginning at the end of the list.
    if (current == this->discoveryMap_.end()) {
      // Continue to traverse the list.
      current = this->discoveryMap_.begin();
    }

    // Handle reaching the lost repository by waiting before trying
    // again.
    if (current == initialLocation) {
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Service_Participant::repository_lost: ")
                   ACE_TEXT("waiting %d seconds to traverse the ")
                   ACE_TEXT("repository list another time ")
                   ACE_TEXT("for lost key %C.\n"),
                   backoff,
                   key.c_str()));
      }

      // Wait to traverse the list and try again.
      ACE_OS::sleep(backoff);

      // Exponentially backoff delay.
      backoff *= this->federation_backoff_multiplier();

      // Don't increment current to allow us to reattach to the
      // original repository if it is restarted.
    }

    // Check the availability of the current repository.
    if (current->second->active()) {

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Service_Participant::repository_lost: ")
                   ACE_TEXT("replacing repository %C with %C.\n"),
                   key.c_str(),
                   current->first.c_str()));
      }

      // If we reach here, the validate_connection() call succeeded
      // and the repository is reachable.
      this->remap_domains(key, current->first);

      // Now we are done.  This is the only non-failure exit from
      // this method.
      return;

    } else {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Service_Participant::repository_lost: ")
                 ACE_TEXT("repository %C was not available to replace %C, ")
                 ACE_TEXT("looking for another.\n"),
                 current->first.c_str(),
                 key.c_str()));
    }

    // Move to the next candidate repository.
    ++current;
  }

  // If we reach here, we have exceeded the total recovery time
  // specified.
  OPENDDS_ASSERT(recoveryFailedTime.is_zero());
}

void
Service_Participant::set_default_discovery(const Discovery::RepoKey& key)
{
  this->defaultDiscovery_ = key;
}

Discovery::RepoKey
Service_Participant::get_default_discovery()
{
  return this->defaultDiscovery_;
}

Discovery_rch
Service_Participant::get_discovery(const DDS::DomainId_t domain)
{
  // Default to the Default InfoRepo-based discovery unless the user has
  // changed defaultDiscovery_ using the API or config file
  Discovery::RepoKey repo = defaultDiscovery_;

  // Find if this domain has a repo key (really a discovery key)
  // mapped to it.
  DomainRepoMap::const_iterator where = this->domainRepoMap_.find(domain);
  if (where != this->domainRepoMap_.end()) {
    repo = where->second;
  }

  RepoKeyDiscoveryMap::const_iterator location = this->discoveryMap_.find(repo);

  if (location == this->discoveryMap_.end()) {
    if ((repo == Discovery::DEFAULT_REPO) ||
        (repo == "-1")) {
      // Set the default repository IOR if it hasn't already happened
      // by this point.  This is why this can't be const.
      bool ok = this->set_repo_ior(DEFAULT_REPO_IOR, Discovery::DEFAULT_REPO);

      if (!ok) {
        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::get_discovery: ")
                     ACE_TEXT("failed attempt to set default IOR for domain %d.\n"),
                     domain));
        }

      } else {
        // Found the default!
        if (DCPS_debug_level > 4) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::get_discovery: ")
                     ACE_TEXT("returning default repository for domain %d.\n"),
                     domain));
        }

      }
      return this->discoveryMap_[Discovery::DEFAULT_REPO];

    } else if (repo == Discovery::DEFAULT_RTPS) {

      ACE_Configuration_Heap cf;
      cf.open();
      ACE_Configuration_Section_Key k;
      cf.open_section(cf.root_section(), RTPS_SECTION_NAME, 1 /*create*/, k);
      this->load_discovery_configuration(cf, RTPS_SECTION_NAME);

      // Try to find it again
      location = this->discoveryMap_.find(Discovery::DEFAULT_RTPS);

      if (location == this->discoveryMap_.end()) {
        // Unable to load DEFAULT_RTPS
        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::get_discovery: ")
                     ACE_TEXT("failed attempt to set default RTPS discovery for domain %d.\n"),
                     domain));
        }

        return Discovery_rch();

      } else {
        // Found the default!
        if (DCPS_debug_level > 4) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::get_discovery: ")
                     ACE_TEXT("returning default RTPS discovery for domain %d.\n"),
                     domain));
        }

        return location->second;
      }

    } else {
      // Non-default repositories _must_ be loaded by application.
      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Service_Participant::get_discovery: ")
                   ACE_TEXT("repository for domain %d was not set.\n"),
                   domain));
      }

      return Discovery_rch();
    }
  }

  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Service_Participant::get_discovery: ")
               ACE_TEXT("returning repository for domain %d, repo %C.\n"),
               domain, repo.c_str()));
  }

  return location->second;
}

OPENDDS_STRING
Service_Participant::bit_transport_ip() const
{
  return ACE_TEXT_ALWAYS_CHAR(this->bit_transport_ip_.c_str());
}

int
Service_Participant::bit_transport_port() const
{
  return this->bit_transport_port_;
}

void
Service_Participant::bit_transport_port(int port)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->maps_lock_);
  this->bit_transport_port_ = port;
  got_bit_transport_port = true;
}

int
Service_Participant::bit_lookup_duration_msec() const
{
  return bit_lookup_duration_msec_;
}

void
Service_Participant::bit_lookup_duration_msec(int sec)
{
  bit_lookup_duration_msec_ = sec;
  got_bit_lookup_duration_msec = true;
}

size_t
Service_Participant::n_chunks() const
{
  return n_chunks_;
}

void
Service_Participant::n_chunks(size_t chunks)
{
  n_chunks_ = chunks;
  got_chunks = true;
}

size_t
Service_Participant::association_chunk_multiplier() const
{
  return association_chunk_multiplier_;
}

void
Service_Participant::association_chunk_multiplier(size_t multiplier)
{
  association_chunk_multiplier_ = multiplier;
  got_chunk_association_multiplier = true;
}

void
Service_Participant::liveliness_factor(int factor)
{
  liveliness_factor_ = factor;
  got_liveliness_factor = true;
}

int
Service_Participant::liveliness_factor() const
{
  return liveliness_factor_;
}

void
Service_Participant::register_discovery_type(const char* section_name,
                                             Discovery::Config* cfg)
{
  discovery_types_[section_name].reset(cfg);
}

int
Service_Participant::load_configuration()
{
  int status = 0;

  if ((status = this->cf_.open()) != 0)
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("open() returned %d\n"),
                      status),
                     -1);

  ACE_Ini_ImpExp import(this->cf_);
  status = import.import_config(config_fname.c_str());

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("import_config () returned %d\n"),
                      status),
                     -1);
  } else {
    status = this->load_configuration(this->cf_, config_fname.c_str());
  }
  return status;
}

int
Service_Participant::load_configuration(
  ACE_Configuration_Heap& config,
  const ACE_TCHAR* filename)
{
  int status = 0;

  status = this->load_common_configuration(config, filename);

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_common_configuration () returned %d\n"),
                      status),
                     -1);
  }

  // Register static discovery.
  this->add_discovery(static_rchandle_cast<Discovery>(StaticDiscovery::instance()));

  status = this->load_discovery_configuration(config, RTPS_SECTION_NAME);

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_discovery_configuration() returned %d\n"),
                      status),
                     -1);
  }

  status = this->load_discovery_configuration(config, REPO_SECTION_NAME);

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_discovery_configuration() returned %d\n"),
                      status),
                     -1);
  }

  status = TransportRegistry::instance()->load_transport_configuration(
             ACE_TEXT_ALWAYS_CHAR(filename), config);
  if (this->global_transport_config_ != ACE_TEXT("")) {
    TransportConfig_rch config = TransportRegistry::instance()->get_config(
      ACE_TEXT_ALWAYS_CHAR(this->global_transport_config_.c_str()));
    if (!config) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                        ACE_TEXT("Unable to locate specified global transport config: %s\n"),
                        this->global_transport_config_.c_str()),
                       -1);
    }
    TransportRegistry::instance()->global_config(config);
  }

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_transport_configuration () returned %d\n"),
                      status),
                     -1);
  }

  // Needs to be loaded after the [rtps_discovery/*] and [repository/*]
  // sections to allow error reporting on bad discovery config names.
  // Also loaded after the transport configuration so that
  // DefaultTransportConfig within [domain/*] can use TransportConfig objects.
  status = this->load_domain_configuration(config, filename);

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_domain_configuration () returned %d\n"),
                      status),
                     -1);
  }

  // Needs to be loaded after transport configs and instances and domains.
  try {
    status = StaticDiscovery::instance()->load_configuration(config);

    if (status != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
        ACE_TEXT("load_discovery_configuration() returned %d\n"),
        status),
        -1);
    }
  } catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in Service_Participant::load_configuration: "
      "trying to load_discovery_configuration()");
    return -1;
  }

  return 0;
}

int
Service_Participant::load_common_configuration(ACE_Configuration_Heap& cf,
                                               const ACE_TCHAR* filename)
{
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key sect;

  if (cf.open_section(root, COMMON_SECTION_NAME, 0, sect) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // a common section. The code default configuration will be used.
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: Service_Participant::load_common_configuration ")
                 ACE_TEXT("failed to open section %s\n"),
                 COMMON_SECTION_NAME));
    }

    return 0;

  } else {
    const ACE_TCHAR* message =
      ACE_TEXT("(%P|%t) NOTICE: using %s value from command option (overrides value if it's in config file)\n");

    if (got_debug_level) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSDebugLevel")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSDebugLevel"), DCPS_debug_level, int)
    }

    if (got_info) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSInfoRepo")));
    } else {
      ACE_TString value;
      GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("DCPSInfoRepo"), value)
      if (!value.empty()) {
        this->set_repo_ior(value.c_str(), Discovery::DEFAULT_REPO);
      }
    }

    if (got_use_rti_serialization) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSRTISerialization")));
    } else {
      bool should_use = true;
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSRTISerialization"), should_use, bool)
      if (!should_use) {
        ACE_ERROR((LM_WARNING,
          ACE_TEXT("(%P|%t) WARNING: Service_Participant::load_common_configuration ")
          ACE_TEXT("Argument ignored: DCPSRTISerialization is required to be enabled\n")));
      }
    }

    if (got_chunks) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSChunks")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSChunks"), this->n_chunks_, size_t)
    }

    if (got_chunk_association_multiplier) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSChunkAssociationMutltiplier")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSChunkAssociationMutltiplier"), this->association_chunk_multiplier_, size_t)
    }

    if (got_bit_transport_port) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSBitTransportPort")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSBitTransportPort"), this->bit_transport_port_, int)
    }

    if (got_bit_transport_ip) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSBitTransportIPAddress")));
    } else {
      GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("DCPSBitTransportIPAddress"), this->bit_transport_ip_)
    }

    if (got_liveliness_factor) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSLivelinessFactor")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSLivelinessFactor"), this->liveliness_factor_, int)
    }

    if (got_bit_lookup_duration_msec) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSBitLookupDurationMsec")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSBitLookupDurationMsec"), this->bit_lookup_duration_msec_, int)
    }

    if (got_global_transport_config) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSGlobalTransportConfig")));
    } else {
      GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("DCPSGlobalTransportConfig"), this->global_transport_config_);
      if (this->global_transport_config_ == ACE_TEXT("$file")) {
        // When the special string of "$file" is used, substitute the file name
        this->global_transport_config_ = filename;
      }
    }

    if (got_bit_flag) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSBit")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSBit"), this->bit_enabled_, int)
    }

#if defined(OPENDDS_SECURITY)
    if (got_security_flag) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSSecurity")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSSecurity"), this->security_enabled_, int)
    }

    if (got_security_debug) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSSecurityDebug or DCPSSecurityDebugLevel")));
    } else {
      const ACE_TCHAR* debug_name = ACE_TEXT("DCPSSecurityDebug");
      const ACE_TCHAR* debug_level_name = ACE_TEXT("DCPSSecurityDebugLevel");
      bool got_value = false;
      ACE_TString debug_level_value;
      if (cf.get_string_value(sect, debug_level_name, debug_level_value) == -1) {
        ACE_TString debug_value;
        if (cf.get_string_value(sect, debug_name, debug_value) != -1) {
          if (debug_value != ACE_TEXT("")) {
            got_value = true;
            security_debug.parse_flags(debug_value.c_str());
          }
        }
      } else if (debug_level_value != ACE_TEXT("")) {
        got_value = true;
        security_debug.set_debug_level(ACE_OS::atoi(debug_level_value.c_str()));
      }
      if (!got_value && OpenDDS::DCPS::Transport_debug_level > 0) {
        ACE_DEBUG((LM_NOTICE,
          ACE_TEXT("(%P|%t) NOTICE: DCPSSecurityDebug and DCPSSecurityDebugLevel ")
          ACE_TEXT("are not defined in config file or are blank - using code default.\n")));
      }
    }

    if (got_security_fake_encryption) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSSecurityFakeEncryption")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSSecurityFakeEncryption"), security_debug.fake_encryption, int)
    }
#endif

    if (got_transport_debug_level) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSTransportDebugLevel")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSTransportDebugLevel"), OpenDDS::DCPS::Transport_debug_level, int)
    }

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
    if (got_persistent_data_dir) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSPersistentDataDir")));
    } else {
      ACE_TString value;
      GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("DCPSPersistentDataDir"), value)
      this->persistent_data_dir_ = ACE_TEXT_ALWAYS_CHAR(value.c_str());
    }
#endif

    if (got_pending_timeout) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSPendingTimeout")));
    } else {
      int timeout = 0;
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSPendingTimeout"), timeout, int)
      pending_timeout_ = TimeDuration(timeout);
    }

    if (got_publisher_content_filter) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSPublisherContentFilter")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSPublisherContentFilter"),
        this->publisher_content_filter_, bool)
    }

    if (got_default_discovery) {
      ACE_Configuration::VALUETYPE type;
      if (cf.find_value(sect, ACE_TEXT("DCPSDefaultDiscovery"), type) != -1) {
        ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSDefaultDiscovery")));
      }
    } else {
      GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("DCPSDefaultDiscovery"),
        this->defaultDiscovery_);
    }

    if (got_bidir_giop) {
      ACE_Configuration::VALUETYPE type;
      if (cf.find_value(sect, ACE_TEXT("DCPSBidirGIOP"), type) != -1) {
        ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSBidirGIOP")));
      }
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSBidirGIOP"), bidir_giop_, bool)
    }

    ACE_Configuration::VALUETYPE type;
    if (got_log_fname) {
      if (cf.find_value(sect, ACE_TEXT("ORBLogFile"), type) != -1) {
        ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("ORBLogFile")));
      }
    } else {
      OPENDDS_STRING log_fname;
      GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("ORBLogFile"), log_fname);
      if (!log_fname.empty()) {
        set_log_file_name(log_fname.c_str());
      }
    }

    if (got_log_verbose) {
      if (cf.find_value(sect, ACE_TEXT("ORBVerboseLogging"), type) != -1) {
        ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("ORBVerboseLogging")));
      }
    } else {
      unsigned long verbose_logging = 0;
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("ORBVerboseLogging"), verbose_logging, unsigned long);
      set_log_verbose(verbose_logging);
    }

    if (got_default_address) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSDefaultAddress")));
    } else {
      GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("DCPSDefaultAddress"), this->default_address_)
    }

    if (got_monitor) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("DCPSMonitor")));
    } else {
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("DCPSMonitor"), monitor_enabled_, bool)
    }

    // These are not handled on the command line.
    GET_CONFIG_VALUE(cf, sect, ACE_TEXT("FederationRecoveryDuration"), this->federation_recovery_duration_, int)
    GET_CONFIG_VALUE(cf, sect, ACE_TEXT("FederationInitialBackoffSeconds"), this->federation_initial_backoff_seconds_, int)
    GET_CONFIG_VALUE(cf, sect, ACE_TEXT("FederationBackoffMultiplier"), this->federation_backoff_multiplier_, int)
    GET_CONFIG_VALUE(cf, sect, ACE_TEXT("FederationLivelinessDuration"), this->federation_liveliness_, int)

#if defined OPENDDS_SAFETY_PROFILE && defined ACE_HAS_ALLOC_HOOKS
    GET_CONFIG_VALUE(cf, sect, ACE_TEXT("pool_size"), pool_size_, size_t)
    GET_CONFIG_VALUE(cf, sect, ACE_TEXT("pool_granularity"), pool_granularity_, size_t)
#endif

    //
    // Establish the scheduler if specified.
    //
    GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("scheduler"), this->schedulerString_)

    suseconds_t usec(0);

    GET_CONFIG_VALUE(cf, sect, ACE_TEXT("scheduler_slice"), usec, suseconds_t)

    if (usec > 0) {
      schedulerQuantum_ = TimeDuration(0, usec);
    }
  }

  return 0;
}

int
Service_Participant::load_domain_configuration(ACE_Configuration_Heap& cf,
                                               const ACE_TCHAR* filename)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key domain_sect;

  if (cf.open_section(root, DOMAIN_SECTION_NAME, 0, domain_sect) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any domain (sub)section. The code default configuration will be used.
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: Service_Participant::load_domain_configuration ")
                 ACE_TEXT("failed to open [%s] section - using code default.\n"),
                 DOMAIN_SECTION_NAME));
    }

    return 0;

  } else {
    // Ensure there are no properties in this section
    ValueMap vm;
    if (pullValues(cf, domain_sect, vm) > 0) {
      // There are values inside [domain]
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration(): ")
                        ACE_TEXT("domain sections must have a subsection name\n")),
                       -1);
    }
    // Process the subsections of this section (the individual domains)
    KeyList keys;
    if (processSections(cf, domain_sect, keys) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration(): ")
                        ACE_TEXT("too many nesting layers in the [domain] section.\n")),
                       -1);
    }

    // Loop through the [domain/*] sections
    for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
      OPENDDS_STRING domain_name = it->first;

      ValueMap values;
      pullValues(cf, it->second, values);
      DDS::DomainId_t domainId = -1;
      Discovery::RepoKey repoKey;
      OPENDDS_STRING perDomainDefaultTportConfig;
      for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
        OPENDDS_STRING name = it->first;
        if (name == "DomainId") {
          OPENDDS_STRING value = it->second;
          if (!convertToInteger(value, domainId)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration(): ")
                              ACE_TEXT("Illegal integer value for DomainId (%C) in [domain/%C] section.\n"),
                              value.c_str(), domain_name.c_str()),
                             -1);
          }
          if (DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) [domain/%C]: DomainId == %d\n"),
                       domain_name.c_str(), domainId));
          }
        } else if (name == "DomainRepoKey") {
          // We will still process this for backward compatibility, but
          // it can now be replaced by "DiscoveryConfig=REPO:<key>"
          repoKey = it->second;
          if (repoKey == "-1") {
            repoKey = Discovery::DEFAULT_REPO;
          }

          if (DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) [domain/%C]: DomainRepoKey == %C\n"),
                       domain_name.c_str(), repoKey.c_str()));
          }
        } else if (name == "DiscoveryConfig") {
          repoKey = it->second;

        } else if (name == "DefaultTransportConfig") {
          if (it->second == "$file") {
            // When the special string of "$file" is used, substitute the file name
            perDomainDefaultTportConfig = ACE_TEXT_ALWAYS_CHAR(filename);

          } else {
            perDomainDefaultTportConfig = it->second;
          }

        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration(): ")
                            ACE_TEXT("Unexpected entry (%C) in [domain/%C] section.\n"),
                            name.c_str(), domain_name.c_str()),
                           -1);
        }
      }

      if (domainId == -1) {
        // DomainId parameter is not set, try using the domain name as an ID
        if (!convertToInteger(domain_name, domainId)) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration(): ")
                            ACE_TEXT("Missing DomainId value in [domain/%C] section.\n"),
                            domain_name.c_str()),
                           -1);
        }
      }

      if (!perDomainDefaultTportConfig.empty()) {
        TransportRegistry* const reg = TransportRegistry::instance();
        TransportConfig_rch tc = reg->get_config(perDomainDefaultTportConfig);
        if (tc.is_nil()) {
          ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration(): ")
            ACE_TEXT("Unknown transport config %C in [domain/%C] section.\n"),
            perDomainDefaultTportConfig.c_str(), domain_name.c_str()), -1);
        } else {
          reg->domain_default_config(domainId, tc);
        }
      }

      // Check to see if the specified discovery configuration has been defined
      if (!repoKey.empty()) {
        if ((repoKey != Discovery::DEFAULT_REPO) &&
            (repoKey != Discovery::DEFAULT_RTPS) &&
            (repoKey != Discovery::DEFAULT_STATIC) &&
            (this->discoveryMap_.find(repoKey) == this->discoveryMap_.end())) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration(): ")
                            ACE_TEXT("Specified configuration (%C) not found.  Referenced in [domain/%C] section.\n"),
                            repoKey.c_str(), domain_name.c_str()),
                           -1);
        }
        this->set_repo_domain(domainId, repoKey);
      }
    }
  }

  return 0;
}

int
Service_Participant::load_discovery_configuration(ACE_Configuration_Heap& cf,
                                                  const ACE_TCHAR* section_name)
{
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key sect;
  if (cf.open_section(root, section_name, 0, sect) == 0) {

    const OPENDDS_STRING sect_name = ACE_TEXT_ALWAYS_CHAR(section_name);
    DiscoveryTypes::iterator iter =
      this->discovery_types_.find(sect_name);

    if (iter == this->discovery_types_.end()) {
      // See if we can dynamically load the required libraries
      TheTransportRegistry->load_transport_lib(sect_name);
      iter = this->discovery_types_.find(sect_name);
    }

    if (iter != this->discovery_types_.end()) {
      // discovery code is loaded, process options
      return iter->second->discovery_config(cf);
    } else {
      // No discovery code can be loaded, report an error
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: Service_Participant::")
                        ACE_TEXT("load_discovery_configuration ")
                        ACE_TEXT("Unable to load libraries for %s\n"),
                        section_name),
                       -1);
    }
  }
  return 0;
}

#if defined OPENDDS_SAFETY_PROFILE && defined ACE_HAS_ALLOC_HOOKS
void
Service_Participant::configure_pool()
{
  if (pool_size_) {
    SafetyProfilePool::instance()->configure_pool(pool_size_, pool_granularity_);
    SafetyProfilePool::instance()->install();
  }
}
#endif

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
DataDurabilityCache *
Service_Participant::get_data_durability_cache(
  DDS::DurabilityQosPolicy const & durability)
{
  DDS::DurabilityQosPolicyKind const kind =
    durability.kind;

  DataDurabilityCache * cache = 0;

  if (kind == DDS::TRANSIENT_DURABILITY_QOS) {
    {
      ACE_GUARD_RETURN(TAO_SYNCH_MUTEX,
                       guard,
                       this->factory_lock_,
                       0);

      if (!this->transient_data_cache_) {
        this->transient_data_cache_.reset(new DataDurabilityCache(kind));
      }
    }

    cache = this->transient_data_cache_.get();

  } else if (kind == DDS::PERSISTENT_DURABILITY_QOS) {
    {
      ACE_GUARD_RETURN(TAO_SYNCH_MUTEX,
                       guard,
                       this->factory_lock_,
                       0);

      try {
        if (!this->persistent_data_cache_) {
          this->persistent_data_cache_.reset(new DataDurabilityCache(kind,
                                                                     this->persistent_data_dir_));
        }

      } catch (const std::exception& ex) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: Service_Participant::get_data_durability_cache ")
                     ACE_TEXT("failed to create PERSISTENT cache, falling back on ")
                     ACE_TEXT("TRANSIENT behavior: %C\n"), ex.what()));
        }

        this->persistent_data_cache_.reset(new DataDurabilityCache(DDS::TRANSIENT_DURABILITY_QOS));
      }
    }

    cache = this->persistent_data_cache_.get();
  }

  return cache;
}
#endif

void
Service_Participant::add_discovery(Discovery_rch discovery)
{
  if (discovery) {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->maps_lock_);
    this->discoveryMap_[discovery->key()] = discovery;
  }
}

void
Service_Participant::set_shutdown_listener(ShutdownListener* listener)
{
  shutdown_listener_ = listener;
}

const Service_Participant::RepoKeyDiscoveryMap&
Service_Participant::discoveryMap() const
{
  return this->discoveryMap_;
}

const Service_Participant::DomainRepoMap&
Service_Participant::domainRepoMap() const
{
  return this->domainRepoMap_;
}

Recorder_ptr
Service_Participant::create_recorder(DDS::DomainParticipant_ptr participant,
                                     DDS::Topic_ptr a_topic,
                                     const DDS::SubscriberQos& subscriber_qos,
                                     const DDS::DataReaderQos& datareader_qos,
                                     const RecorderListener_rch& a_listener)
{
  DomainParticipantImpl* participant_servant = dynamic_cast<DomainParticipantImpl*>(participant);
  if (participant_servant)
    return participant_servant->create_recorder(a_topic, subscriber_qos, datareader_qos, a_listener, 0);
  return 0;
}

DDS::ReturnCode_t
Service_Participant::delete_recorder(Recorder_ptr recorder)
{
  DDS::ReturnCode_t ret = DDS::RETCODE_ERROR;
  RecorderImpl* impl = dynamic_cast<RecorderImpl*>(recorder);
  if (impl){
    ret = impl->cleanup();
    impl->participant()->delete_recorder(recorder);
  }
  return ret;
}

Replayer_ptr
Service_Participant::create_replayer(DDS::DomainParticipant_ptr participant,
                                     DDS::Topic_ptr a_topic,
                                     const DDS::PublisherQos& publisher_qos,
                                     const DDS::DataWriterQos& datawriter_qos,
                                     const ReplayerListener_rch& a_listener)
{
  ACE_DEBUG((LM_DEBUG, "Service_Participant::create_replayer\n"));
  DomainParticipantImpl* participant_servant = dynamic_cast<DomainParticipantImpl*>(participant);
  if (participant_servant)
    return participant_servant->create_replayer(a_topic, publisher_qos, datawriter_qos, a_listener, 0);
  return 0;
}

DDS::ReturnCode_t
Service_Participant::delete_replayer(Replayer_ptr replayer)
{
  DDS::ReturnCode_t ret = DDS::RETCODE_ERROR;
  ReplayerImpl* impl = static_cast<ReplayerImpl*>(replayer);
  if (impl) {
    ret = impl->cleanup();
    impl->participant()->delete_replayer(replayer);
  }
  return ret;
}

DDS::Topic_ptr Service_Participant::create_typeless_topic(
  DDS::DomainParticipant_ptr participant,
  const char* topic_name,
  const char* type_name,
  bool type_has_keys,
  const DDS::TopicQos& qos,
  DDS::TopicListener_ptr a_listener,
  DDS::StatusMask mask)
{
  DomainParticipantImpl* participant_servant = dynamic_cast<DomainParticipantImpl*>(participant);
  if (!participant_servant) {
    return 0;
  }
  return participant_servant->create_typeless_topic(topic_name, type_name, type_has_keys, qos, a_listener, mask);
}

void Service_Participant::default_configuration_file(const ACE_TCHAR* path)
{
  default_configuration_file_ = path;
}

NetworkConfigMonitor_rch Service_Participant::network_config_monitor()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, network_config_monitor_lock_, NetworkConfigMonitor_rch());

  if (!network_config_monitor_) {
#ifdef OPENDDS_LINUX_NETWORK_CONFIG_MONITOR
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
               "%T (%P|%t) Service_Participant::network_config_monitor(). Creating LinuxNetworkConfigMonitor\n"));
    }
    network_config_monitor_ = make_rch<LinuxNetworkConfigMonitor>(reactor_task_.interceptor());
#elif defined(OPENDDS_NETWORK_CONFIG_MODIFIER)
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
               "%T (%P|%t) Service_Participant::network_config_monitor(). Creating NetworkConfigModifier\n"));
    }
    network_config_monitor_ = make_rch<NetworkConfigModifier>();
#endif

    if (network_config_monitor_ && !network_config_monitor_->open()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Service_Participant::network_config_monitor could not open network config monitor\n ")));
      network_config_monitor_->close();
      network_config_monitor_.reset();
    }
  }

  return network_config_monitor_;
}

#ifdef OPENDDS_NETWORK_CONFIG_MODIFIER
NetworkConfigModifier* Service_Participant::network_config_modifier()
{
  return dynamic_cast<NetworkConfigModifier*>(network_config_monitor().get());
}
#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
