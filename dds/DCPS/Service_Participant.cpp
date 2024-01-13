/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> //Only the _pch include should start with DCPS/

#include "Service_Participant.h"

#include "Logging.h"
#include "WaitSet.h"
#include "transport/framework/TransportRegistry.h"
#include "debug.h"
#include "BuiltInTopicUtils.h"
#include "DataDurabilityCache.h"
#include "GuidConverter.h"
#include "MonitorFactory.h"
#include "RecorderImpl.h"
#include "ReplayerImpl.h"
#include "LinuxNetworkConfigMonitor.h"
#include "DefaultNetworkConfigMonitor.h"
#include "StaticDiscovery.h"
#include "ThreadStatusManager.h"
#include "Qos_Helper.h"
#include "../Version.h"
#ifdef OPENDDS_SECURITY
#  include "security/framework/SecurityRegistry.h"
#endif

#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>
#include <ace/Configuration.h>
#include <ace/Configuration_Import_Export.h>
#include <ace/Malloc_Allocator.h>
#include <ace/OS_NS_ctype.h>
#include <ace/OS_NS_sys_utsname.h>
#include <ace/OS_NS_unistd.h>
#include <ace/Reactor.h>
#include <ace/Sched_Params.h>
#include <ace/Select_Reactor.h>
#include <ace/Service_Config.h>
#include <ace/Singleton.h>
#include <ace/Version.h>
#include <ace/config.h>

#include <cstring>
#ifdef OPENDDS_SAFETY_PROFILE
#  include <stdio.h> // <cstdio> after FaceCTS bug 623 is fixed
#else
#  include <fstream>
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

static const ACE_TCHAR DEFAULT_REPO_IOR[] = ACE_TEXT("file://repo.ior");

static const ACE_TCHAR DOMAIN_SECTION_NAME[] = ACE_TEXT("domain");
static const ACE_TCHAR REPO_SECTION_NAME[]   = ACE_TEXT("repository");
static const ACE_TCHAR RTPS_SECTION_NAME[]   = ACE_TEXT("rtps_discovery");

namespace {

String toupper(const String& x)
{
  String retval;
  for (String::const_iterator pos = x.begin(), limit = x.end(); pos != limit; ++pos) {
    retval.push_back(ACE_OS::ace_toupper(*pos));
  }
  return retval;
}

}

Service_Participant::Service_Participant()
  :
#ifndef OPENDDS_SAFETY_PROFILE
  ORB_argv_(false /*substitute_env_args*/),
#endif
  time_source_()
  , reactor_task_(false)
  , monitor_factory_(0)
  , priority_min_(0)
  , priority_max_(0)
  , shut_down_(false)
  , network_interface_address_topic_(make_rch<InternalTopic<NetworkInterfaceAddress> >())
  , config_topic_(make_rch<InternalTopic<ConfigPair> >())
  , config_store_(make_rch<ConfigStoreImpl>(config_topic_))
  , config_reader_(make_rch<InternalDataReader<ConfigPair> >(DataReaderQosBuilder().reliability_reliable().durability_transient_local()))
  , config_reader_listener_(make_rch<ConfigReaderListener>(ref(*this)))
  , pending_timeout_(0,0) // Can't use OPENDDS_COMMON_DCPS_PENDING_TIMEOUT_default due to initialization order.
#ifdef DDS_DEFAULT_DISCOVERY_METHOD
  , default_discovery_(DDS_DEFAULT_DISCOVERY_METHOD)
#else
# ifdef OPENDDS_SAFETY_PROFILE
  , default_discovery_(Discovery::DEFAULT_RTPS)
# else
  , default_discovery_(Discovery::DEFAULT_REPO)
# endif
#endif
{
  config_topic_->connect(config_reader_);
  initialize();
}

Service_Participant::~Service_Participant()
{
  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Service_Participant::~Service_Participant\n"));
  }

  {
    ACE_GUARD(ACE_Thread_Mutex, guard, factory_lock_);
    if (dp_factory_servant_) {
      const size_t count = dp_factory_servant_->participant_count();
      if (count > 0 && log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: Service_Participant::~Service_Participant: "
          "There are %B remaining domain participant(s). "
          "It is recommended to delete them before shutdown.\n",
          count));
      }

      const DDS::ReturnCode_t cleanup_status = dp_factory_servant_->delete_all_participants();
      if (cleanup_status) {
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: Service_Participant::~Service_Participant: "
            "delete_all_participants returned %C\n",
            retcode_to_string(cleanup_status)));
        }
      }
    }
  }

  const DDS::ReturnCode_t shutdown_status = shutdown();
  if (shutdown_status != DDS::RETCODE_OK && shutdown_status != DDS::RETCODE_ALREADY_DELETED) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: Service_Participant::~Service_Participant: "
        "shutdown returned %C\n",
        retcode_to_string(shutdown_status)));
    }
  }

  config_topic_->disconnect(config_reader_);
}

Service_Participant*
Service_Participant::instance()
{
  // Hide the template instantiation to prevent multiple instances
  // from being created.

  return ACE_Singleton<Service_Participant, ACE_SYNCH_MUTEX>::instance();
}

const TimeSource&
Service_Participant::time_source() const
{
  return time_source_;
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

ReactorInterceptor_rch
Service_Participant::interceptor() const
{
  return reactor_task_.interceptor();
}

JobQueue_rch
Service_Participant::job_queue() const
{
  return job_queue_;
}

DDS::ReturnCode_t Service_Participant::shutdown()
{
  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Service_Participant::shutdown\n"));
  }

  if (shut_down_) {
    return DDS::RETCODE_ALREADY_DELETED;
  }

  if (monitor_factory_) {
    monitor_factory_->deinitialize();
    monitor_factory_ = 0;
  }

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, factory_lock_, DDS::RETCODE_OUT_OF_RESOURCES);
    if (dp_factory_servant_) {
      const size_t count = dp_factory_servant_->participant_count();
      if (count > 0) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Service_Participant::shutdown: "
            "there are %B domain participant(s) that must be deleted before shutdown can occur\n",
            count));
        }
        return DDS::RETCODE_PRECONDITION_NOT_MET;
      }
    }
  }

  if (shutdown_listener_) {
    shutdown_listener_->notify_shutdown();
  }

  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  try {
    TransportRegistry::instance()->release();
    {
      ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, factory_lock_, DDS::RETCODE_OUT_OF_RESOURCES);

      shut_down_ = true;

      dp_factory_servant_.reset();

      domainRepoMap_.clear();

      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, network_config_monitor_lock_,
          DDS::RETCODE_OUT_OF_RESOURCES);
        if (network_config_monitor_) {
          network_config_monitor_->close();
          network_config_monitor_->disconnect(network_interface_address_topic_);
          network_config_monitor_.reset();
        }
      }

      domain_ranges_.clear();

      reactor_task_.stop();

      discoveryMap_.clear();

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
      transient_data_cache_.reset();
      persistent_data_cache_.reset();
#endif

      discovery_types_.clear();
    }
    TransportRegistry::close();
#ifdef OPENDDS_SECURITY
    OpenDDS::Security::SecurityRegistry::close();
#endif
  } catch (const CORBA::Exception& ex) {
    if (log_level >= LogLevel::Error) {
      ex._tao_print_exception("ERROR: Service_Participant::shutdown");
    }
    rc = DDS::RETCODE_ERROR;
  }

  return rc;
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
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, factory_lock_, 0);

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

      String config_fname = config_store_->get(OPENDDS_COMMON_DCPS_CONFIG_FILE,
                                               OPENDDS_COMMON_DCPS_CONFIG_FILE_default);
      const String default_configuration_file = config_store_->get(OPENDDS_DEFAULT_CONFIGURATION_FILE,
                                                                   OPENDDS_DEFAULT_CONFIGURATION_FILE_default);

      if (config_fname.empty() && !default_configuration_file.empty()) {
        config_fname = default_configuration_file;
      }

      if (config_fname.empty()) {
        if (log_level >= LogLevel::Info) {
          ACE_DEBUG((LM_INFO,
                     "(%P|%t) INFO: Service_Participant::get_domain_participant_factory: "
                     "no configuration file specified.\n"));
        }

      } else {
        // Convenient way to run tests in a different place from ini files.
        const char* const config_dir = ACE_OS::getenv("OPENDDS_CONFIG_DIR");
        if (config_dir && config_dir[0]) {
          String new_path = config_dir;
          new_path += ACE_DIRECTORY_SEPARATOR_CHAR_A;
          new_path += config_fname;
          config_fname = new_path;
        }

        // Load configuration only if the configuration file exists.
        FILE* const in = ACE_OS::fopen(config_fname.c_str(), ACE_TEXT("r"));
        if (!in) {
          if (log_level >= LogLevel::Error) {
            ACE_ERROR((LM_ERROR,
                       "(%P|%t) ERROR: Service_Participant::get_domain_participant_factory: "
                       "could not find config file \"%s\": %p\n",
                       config_fname.c_str(), ACE_TEXT("fopen")));
          }
          return DDS::DomainParticipantFactory::_nil();

        } else {
          ACE_OS::fclose(in);

          if (log_level >= LogLevel::Info) {
            ACE_DEBUG((LM_INFO,
                       "(%P|%t) INFO: Service_Participant::get_domain_participant_factory: "
                       "Going to load configuration from <%s>\n",
                       config_fname.c_str()));
          }

          if (this->load_configuration(config_fname) != 0) {
            if (log_level >= LogLevel::Error) {
              ACE_ERROR((LM_ERROR,
                         "(%P|%t) ERROR: Service_Participant::get_domain_participant_factory: "
                         "load_configuration() failed.\n"));
            }
            return DDS::DomainParticipantFactory::_nil();
          }
        }
      }

      config_reader_listener_->on_data_available(config_reader_);

#if OPENDDS_POOL_ALLOCATOR
      // For non-FACE tests, configure pool
      configure_pool();
#endif

      if (log_level >= LogLevel::Info) {
        ACE_DEBUG((LM_INFO, "(%P|%t) Service_Participant::get_domain_participant_factory: "
          "This is OpenDDS " OPENDDS_VERSION " using ACE " ACE_VERSION "\n"));

        ACE_DEBUG((LM_INFO, "(%P|%t) Service_Participant::get_domain_participant_factory: "
          "log_level: %C DCPS_debug_level: %u\n", log_level.get_as_string(), DCPS_debug_level));

        ACE_utsname uname;
        if (ACE_OS::uname(&uname) != -1) {
          ACE_DEBUG((LM_INFO, "(%P|%t) Service_Participant::get_domain_participant_factory: "
            "machine: %C, %C platform: %C, %C, %C\n",
            uname.nodename, uname.machine, uname.sysname, uname.release, uname.version));
        }

        ACE_DEBUG((LM_INFO, "(%P|%t) Service_Participant::get_domain_participant_factory: "
          "compiler: %C version %d.%d.%d\n",
          ACE::compiler_name(), ACE::compiler_major_version(), ACE::compiler_minor_version(), ACE::compiler_beta_version()));
      }

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

      reactor_task_.open_reactor_task(0,
                                      &thread_status_manager_,
                                      "Service_Participant");

      job_queue_ = make_rch<JobQueue>(reactor_task_.get_reactor());

      const bool monitor_enabled = config_store_->get_boolean(OPENDDS_COMMON_DCPS_MONITOR,
                                                              OPENDDS_COMMON_DCPS_MONITOR_default);

      if (monitor_enabled) {
#if !defined(ACE_AS_STATIC_LIBS)
        ACE_TString directive = ACE_TEXT("dynamic OpenDDS_Monitor Service_Object * OpenDDS_monitor:_make_MonitorFactoryImpl()");
        ACE_Service_Config::process_directive(directive.c_str());
#endif
        this->monitor_factory_ =
          ACE_Dynamic_Service<MonitorFactory>::instance ("OpenDDS_Monitor");

        if (this->monitor_factory_ == 0) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("ERROR: Service_Participant::get_domain_participant_factory, ")
                     ACE_TEXT("Unable to enable monitor factory.\n")));
        }
      }

      if (this->monitor_factory_ == 0) {
        // Use the stubbed factory
        MonitorFactory::service_initialize();
        this->monitor_factory_ =
          ACE_Dynamic_Service<MonitorFactory>::instance ("OpenDDS_Monitor_Default");
      }
      if (monitor_enabled) {
        this->monitor_factory_->initialize();
      }

      this->monitor_.reset(this->monitor_factory_->create_sp_monitor(this));
    }

#if defined OPENDDS_LINUX_NETWORK_CONFIG_MONITOR
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) Service_Participant::get_domain_participant_factory: Creating LinuxNetworkConfigMonitor\n"));
    }
    network_config_monitor_ = make_rch<LinuxNetworkConfigMonitor>(reactor_task_.interceptor());
#elif defined(OPENDDS_NETWORK_CONFIG_MODIFIER)
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) Service_Participant::get_domain_participant_factory: Creating NetworkConfigModifier\n"));
    }
    network_config_monitor_ = make_rch<NetworkConfigModifier>();
#else
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) Service_Participant::get_domain_participant_factory: Creating DefaultNetworkConfigMonitor\n"));
    }
    network_config_monitor_ = make_rch<DefaultNetworkConfigMonitor>();
#endif

    network_config_monitor_->connect(network_interface_address_topic_);
    if (!network_config_monitor_->open()) {
      bool open_failed = false;
#ifdef OPENDDS_NETWORK_CONFIG_MODIFIER
      if (DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG,
                   "(%P|%t) Service_Participant::get_domain_participant_factory: Creating NetworkConfigModifier\n"));
      }
      network_config_monitor_->disconnect(network_interface_address_topic_);
      network_config_monitor_ = make_rch<NetworkConfigModifier>();
      network_config_monitor_->connect(network_interface_address_topic_);
      if (!network_config_monitor_->open()) {
        open_failed = true;
      }
#else
      open_failed = true;
#endif
      if (open_failed) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Service_Participant::get_domain_participant_factory: Could not open network config monitor\n"));
        }
        network_config_monitor_->close();
        network_config_monitor_->disconnect(network_interface_address_topic_);
        network_config_monitor_.reset();
      }
    }
  }

  return DDS::DomainParticipantFactory::_duplicate(dp_factory_servant_.in());
}

int Service_Participant::parse_args(int& argc, ACE_TCHAR* argv[])
{
  // Process logging options first, so they are in effect if we need to log
  // while processing other options.
  ACE_Arg_Shifter log_arg_shifter(argc, argv);
  while (log_arg_shifter.is_anything_left()) {
    const ACE_TCHAR* currentArg = 0;

    if ((currentArg = log_arg_shifter.get_the_parameter(ACE_TEXT("-ORBLogFile"))) != 0) {
      config_store_->set_string(OPENDDS_COMMON_ORB_LOG_FILE, ACE_TEXT_ALWAYS_CHAR(currentArg));
      config_reader_listener_->on_data_available(config_reader_);
      log_arg_shifter.consume_arg();

    } else if ((currentArg = log_arg_shifter.get_the_parameter(ACE_TEXT("-ORBVerboseLogging"))) != 0) {
      config_store_->set_string(OPENDDS_COMMON_ORB_VERBOSE_LOGGING, ACE_TEXT_ALWAYS_CHAR(currentArg));
      config_reader_listener_->on_data_available(config_reader_);
      log_arg_shifter.consume_arg();

    } else {
      log_arg_shifter.ignore_arg();
    }
  }

  ACE_Arg_Shifter arg_shifter(argc, argv);
  while (arg_shifter.is_anything_left()) {

    const String current = ACE_TEXT_ALWAYS_CHAR(arg_shifter.get_current());
    if (toupper(current.substr(0, 5)) == "-DCPS" || toupper(current.substr(0, 11)) == "-FEDERATION") {
      arg_shifter.consume_arg();
      if (!arg_shifter.is_anything_left()) {
        break;
      }
      const String key = "OPENDDS_COMMON" + current;
      if (arg_shifter.is_parameter_next()) {
        config_store_->set_string(key.c_str(), ACE_TEXT_ALWAYS_CHAR(arg_shifter.get_current()));
        config_reader_listener_->on_data_available(config_reader_);
        arg_shifter.consume_arg();
      } else {
        arg_shifter.ignore_arg();
      }
    } else if (current.substr(0, 8) == "-OpenDDS") {
      arg_shifter.consume_arg();
      if (!arg_shifter.is_anything_left()) {
        break;
      }
      const String key = "OPENDDS_" + current.substr(8);
      if (arg_shifter.is_parameter_next()) {
        config_store_->set_string(key.c_str(), ACE_TEXT_ALWAYS_CHAR(arg_shifter.get_current()));
        config_reader_listener_->on_data_available(config_reader_);
        arg_shifter.consume_arg();
      } else {
        arg_shifter.ignore_arg();
      }
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
  initial_TransportPriorityQosPolicy_ = TransportPriorityQosPolicyBuilder();
  initial_LifespanQosPolicy_ = LifespanQosPolicyBuilder();

  initial_DurabilityQosPolicy_ = DurabilityQosPolicyBuilder();

  initial_DurabilityServiceQosPolicy_ = DurabilityServiceQosPolicyBuilder();

  initial_PresentationQosPolicy_.access_scope = DDS::INSTANCE_PRESENTATION_QOS;
  initial_PresentationQosPolicy_.coherent_access = false;
  initial_PresentationQosPolicy_.ordered_access = false;

  initial_DeadlineQosPolicy_ = DeadlineQosPolicyBuilder();

  initial_LatencyBudgetQosPolicy_ = LatencyBudgetQosPolicyBuilder();

  initial_OwnershipQosPolicy_ = OwnershipQosPolicyBuilder();
  initial_OwnershipStrengthQosPolicy_ = OwnershipStrengthQosPolicyBuilder();

  initial_LivelinessQosPolicy_ = LivelinessQosPolicyBuilder();

  initial_TimeBasedFilterQosPolicy_ = TimeBasedFilterQosPolicyBuilder();

  initial_ReliabilityQosPolicy_ = ReliabilityQosPolicyBuilder();

  initial_DestinationOrderQosPolicy_ = DestinationOrderQosPolicyBuilder();

  initial_HistoryQosPolicy_ = HistoryQosPolicyBuilder();

  initial_ResourceLimitsQosPolicy_ = ResourceLimitsQosPolicyBuilder();

  initial_EntityFactoryQosPolicy_.autoenable_created_entities = true;

  initial_WriterDataLifecycleQosPolicy_ = WriterDataLifecycleQosPolicyBuilder();

  // Will get interpreted based on how the type was annotated.
  initial_DataRepresentationQosPolicy_.value.length(0);

  initial_ReaderDataLifecycleQosPolicy_ = ReaderDataLifecycleQosPolicyBuilder();

  initial_TypeConsistencyEnforcementQosPolicy_ = TypeConsistencyEnforcementQosPolicyBuilder();

  initial_DomainParticipantQos_.user_data = initial_UserDataQosPolicy_;
  initial_DomainParticipantQos_.entity_factory = initial_EntityFactoryQosPolicy_;
  initial_DomainParticipantFactoryQos_.entity_factory = initial_EntityFactoryQosPolicy_;

  initial_TopicQos_ = TopicQosBuilder();

  initial_DataWriterQos_ = DataWriterQosBuilder();

  initial_PublisherQos_.presentation = initial_PresentationQosPolicy_;
  initial_PublisherQos_.partition = initial_PartitionQosPolicy_;
  initial_PublisherQos_.group_data = initial_GroupDataQosPolicy_;
  initial_PublisherQos_.entity_factory = initial_EntityFactoryQosPolicy_;

  initial_DataReaderQos_ = DataReaderQosBuilder();

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
  const String scheduler_str = config_store_->get(OPENDDS_COMMON_SCHEDULER,
                                                 OPENDDS_COMMON_SCHEDULER_default);

  suseconds_t usec = config_store_->get_int32(OPENDDS_COMMON_SCHEDULER_SLICE,
                                             OPENDDS_COMMON_SCHEDULER_SLICE_default);
  if (usec < 0) {
    usec = 0;
  }
  const TimeDuration scheduler_quantum(0, usec);

  if (scheduler_str.length() == 0) {
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

    if (scheduler_str == "SCHED_RR") {
      ace_scheduler    = ACE_SCHED_RR;

    } else if (scheduler_str == "SCHED_FIFO") {
      ace_scheduler    = ACE_SCHED_FIFO;

    } else if (scheduler_str == "SCHED_OTHER") {
      ace_scheduler    = ACE_SCHED_OTHER;

    } else {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Service_Participant::initializeScheduling() - ")
                 ACE_TEXT("unrecognized scheduling policy: %C, set to SCHED_OTHER.\n"),
                 scheduler_str.c_str()));
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
      scheduler_quantum.value());

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
      this->scheduler(-1);
      ace_scheduler    = ACE_SCHED_OTHER;

    } else if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Service_Participant::initializeScheduling() - ")
                 ACE_TEXT("scheduling policy set to %C.\n"),
                 scheduler_str.c_str()));
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
                                  bool attach_participant,
                                  bool overwrite)
{
  return set_repo_ior(ACE_Wide_To_Ascii(ior).char_rep(), key, attach_participant, overwrite);
}
#endif

bool
Service_Participant::set_repo_ior(const char* ior,
                                  Discovery::RepoKey key,
                                  bool attach_participant,
                                  bool overwrite)
{
  if (key == "-1") {
    key = Discovery::DEFAULT_REPO;
  }

  // Create the repository.
  config_store_->set((String("OPENDDS_REPOSITORY_") + key).c_str(), String("@") + key);
  const String k = String("OPENDDS_REPOSITORY_") + key + "_RepositoryIor";
  if (overwrite) {
    config_store_->set(k.c_str(), ior);
  }
  config_reader_listener_->on_data_available(config_reader_);

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Service_Participant::set_repo_ior: Repo[%C] == %C\n"),
               key.c_str(), ior));
  }

  const OPENDDS_STRING repo_type = ACE_TEXT_ALWAYS_CHAR(REPO_SECTION_NAME);
  if (!discovery_types_.count(repo_type)) {
    // Re-use a transport registry function to attempt a dynamic load of the
    // library that implements the 'repo_type' (InfoRepoDiscovery)
    TheTransportRegistry->load_transport_lib(repo_type);
  }

  if (discovery_types_.count(repo_type)) {
    ACE_Configuration_Heap cf;
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

bool
Service_Participant::use_bidir_giop() const
{
  return config_store_->get_boolean(OPENDDS_COMMON_DCPS_BIDIR_GIOP, OPENDDS_COMMON_DCPS_BIDIR_GIOP_default);
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
  typedef std::pair<Discovery_rch, GUID_t> DiscRepoPair;
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
              GUID_t id = (*current)->get_id();
              repoList.push_back(std::make_pair(disc_iter->second, id));

              if (DCPS_debug_level > 0) {
                ACE_DEBUG((LM_DEBUG,
                           ACE_TEXT("(%P|%t) Service_Participant::set_repo_domain: ")
                           ACE_TEXT("participant %C attached to Repo[ %C].\n"),
                           LogGuid(id).c_str(),
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
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Service_Participant::set_repo_domain: ")
                 ACE_TEXT("(%d of %d) attaching domain %d participant %C to Repo[ %C].\n"),
                 (1+index), repoList.size(), domain,
                 LogGuid(repoList[ index].second).c_str(),
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
    if (current != this->discoveryMap_.end() && current->second->active()) {

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
    if (current != this->discoveryMap_.end()) {
      ++current;
    }
  }

  // If we reach here, we have exceeded the total recovery time
  // specified.
  OPENDDS_ASSERT(recoveryFailedTime.is_zero());
}

void
Service_Participant::set_default_discovery(const Discovery::RepoKey& key)
{
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, cached_config_mutex_);
    default_discovery_ = key;
  }
  config_store_->set_string(OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY, key.c_str());
}

Discovery::RepoKey
Service_Participant::get_default_discovery()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, cached_config_mutex_, OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY_default);
  return default_discovery_;
}

Discovery_rch
Service_Participant::get_discovery(const DDS::DomainId_t domain)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->maps_lock_, Discovery_rch());

  // Default to the Default InfoRepo-based discovery unless the user has
  // changed defaultDiscovery_ using the API or config file
  Discovery::RepoKey repo = get_default_discovery();
  bool in_range = false;
  const Discovery::RepoKey instance_name = get_discovery_template_instance_name(domain);
  DomainRange dr_inst("");

  RepoKeyDiscoveryMap::const_iterator location;

  // Find if this domain has a repo key (really a discovery key)
  // mapped to it.
  DomainRepoMap::const_iterator where = this->domainRepoMap_.find(domain);
  if (where != this->domainRepoMap_.end()) {
    repo = where->second;
  } else {
    // Is domain part of a DomainRange template?
    in_range = get_domain_range_info(domain, dr_inst);
  }

  // check to see if this domain has a discovery template
  // and if the template instance has already been loaded.
  if (!in_range && is_discovery_template(repo)) {
    location = this->discoveryMap_.find(instance_name);
    if (location == this->discoveryMap_.end()) {
      if (configure_discovery_template(domain, repo)) {
        repo = instance_name;
      }
    }
  }

  location = this->discoveryMap_.find(repo);

  if (location == this->discoveryMap_.end()) {
    if (in_range) {
      const int ret = configure_domain_range_instance(domain);

      // return the newly configured domain and return it
      if (!ret) {
        return this->discoveryMap_[instance_name];
      } else {
        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::get_discovery: ")
                     ACE_TEXT("failed attempt to set RTPS discovery for domain range %d.\n"),
                     domain));
        }

        return Discovery_rch();
      }
    } else if ((repo == Discovery::DEFAULT_REPO) ||
        (repo == "-1")) {
      // Set the default repository IOR if it hasn't already happened
      // by this point.  This is why this can't be const.
      bool ok = this->set_repo_ior(DEFAULT_REPO_IOR, Discovery::DEFAULT_REPO, true, false);

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
      cf.open_section(cf.root_section(), RTPS_SECTION_NAME, true /*create*/, k);

      int status = load_discovery_configuration(cf, RTPS_SECTION_NAME);

      if (status != 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::get_Discovery ")
                   ACE_TEXT("failed attempt to load default RTPS discovery for domain %d.\n"),
                   domain));

        return Discovery_rch();
      }

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

void
Service_Participant::federation_recovery_duration(int duration)
{
  config_store_->set_int32(OPENDDS_COMMON_FEDERATION_RECOVERY_DURATION, duration);
}

int
Service_Participant::federation_recovery_duration() const
{
  return config_store_->get_int32(OPENDDS_COMMON_FEDERATION_RECOVERY_DURATION,
                                 OPENDDS_COMMON_FEDERATION_RECOVERY_DURATION_default);
}

void
Service_Participant::federation_initial_backoff_seconds(int value)
{
  config_store_->set_int32(OPENDDS_COMMON_FEDERATION_INITIAL_BACKOFF_SECONDS, value);
}

int
Service_Participant::federation_initial_backoff_seconds() const
{
  return config_store_->get_int32(OPENDDS_COMMON_FEDERATION_INITIAL_BACKOFF_SECONDS,
                                 OPENDDS_COMMON_FEDERATION_INITIAL_BACKOFF_SECONDS_default);
}

void
Service_Participant::federation_backoff_multiplier(int value)
{
  config_store_->set_int32(OPENDDS_COMMON_FEDERATION_BACKOFF_MULTIPLIER, value);
}

int
Service_Participant::federation_backoff_multiplier() const
{
  return config_store_->get_int32(OPENDDS_COMMON_FEDERATION_BACKOFF_MULTIPLIER,
                                 OPENDDS_COMMON_FEDERATION_BACKOFF_MULTIPLIER_default);
}

void
Service_Participant::federation_liveliness(int value)
{
  config_store_->set_int32(OPENDDS_COMMON_FEDERATION_LIVELINESS_DURATION, value);
}

int
Service_Participant::federation_liveliness() const
{
  return config_store_->get_int32(OPENDDS_COMMON_FEDERATION_LIVELINESS_DURATION, OPENDDS_COMMON_FEDERATION_LIVELINESS_DURATION_default);
}

void
Service_Participant::scheduler(long value)
{
  // Using a switch results in a compilation error since THR_SCHED_DEFAULT could be THR_SCHED_RR or THR_SCHED_FIFO.
  if (value == THR_SCHED_DEFAULT) {
    config_store_->set(OPENDDS_COMMON_SCHEDULER, "SCHED_OTHER");
  } else if (value == THR_SCHED_RR) {
    config_store_->set(OPENDDS_COMMON_SCHEDULER, "SCHED_RR");
  } else if (value == THR_SCHED_FIFO) {
    config_store_->set(OPENDDS_COMMON_SCHEDULER, "SCHED_FIFO");
  } else {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING,
                 "(%P|%t) WARNING: Service_Participant::scheduler: cannot translate scheduler value %d\n",
                 value));
    }
    config_store_->set(OPENDDS_COMMON_SCHEDULER, "");
  }
}

long
Service_Participant::scheduler() const
{
  const String str = config_store_->get(OPENDDS_COMMON_SCHEDULER, "");
  if (str == "SCHED_RR") {
    return THR_SCHED_RR;
  } else if (str == "SCHED_FIFO") {
    return THR_SCHED_FIFO;
  } else if (str == "SCHED_OTHER") {
    return THR_SCHED_DEFAULT;
  }

  return -1;
}

void
Service_Participant::publisher_content_filter(bool flag)
{
  config_store_->set_boolean(OPENDDS_COMMON_DCPS_PUBLISHER_CONTENT_FILTER, flag);
}

bool
Service_Participant::publisher_content_filter() const
{
  return config_store_->get_boolean(OPENDDS_COMMON_DCPS_PUBLISHER_CONTENT_FILTER,
                                   OPENDDS_COMMON_DCPS_PUBLISHER_CONTENT_FILTER_default);
}

TimeDuration
Service_Participant::pending_timeout() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, cached_config_mutex_, OPENDDS_COMMON_DCPS_PENDING_TIMEOUT_default);
  return pending_timeout_;
}

void Service_Participant::pending_timeout(const TimeDuration& value)
{
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, cached_config_mutex_);
    pending_timeout_ = value;
  }
  config_store_->set(OPENDDS_COMMON_DCPS_PENDING_TIMEOUT, value, ConfigStoreImpl::Format_IntegerSeconds);
}

MonotonicTimePoint
Service_Participant::new_pending_timeout_deadline() const
{
  const TimeDuration pt = pending_timeout();
  return pt.is_zero() ?
    MonotonicTimePoint() : MonotonicTimePoint::now() + pt;
}


OPENDDS_STRING
Service_Participant::bit_transport_ip() const
{
  return config_store_->get(OPENDDS_COMMON_DCPS_BIT_TRANSPORT_IP_ADDRESS,
                           OPENDDS_COMMON_DCPS_BIT_TRANSPORT_IP_ADDRESS_default);
}

int
Service_Participant::bit_transport_port() const
{
  return config_store_->get_int32(OPENDDS_COMMON_DCPS_BIT_TRANSPORT_PORT,
                                 OPENDDS_COMMON_DCPS_BIT_TRANSPORT_PORT_default);
}

void
Service_Participant::bit_transport_port(int port)
{
  config_store_->set_int32(OPENDDS_COMMON_DCPS_BIT_TRANSPORT_PORT, port);
}

int
Service_Participant::bit_lookup_duration_msec() const
{
  return config_store_->get_int32(OPENDDS_COMMON_DCPS_BIT_LOOKUP_DURATION_MSEC, OPENDDS_COMMON_DCPS_BIT_LOOKUP_DURATION_MSEC_default);
}

void
Service_Participant::bit_lookup_duration_msec(int msec)
{
  config_store_->set_int32(OPENDDS_COMMON_DCPS_BIT_LOOKUP_DURATION_MSEC, msec);
}

#ifdef OPENDDS_SECURITY
bool
Service_Participant::get_security() const
{
  return config_store_->get_boolean(OPENDDS_COMMON_DCPS_SECURITY, OPENDDS_COMMON_DCPS_SECURITY_default);
}

void
Service_Participant::set_security(bool b)
{
  config_store_->set_boolean(OPENDDS_COMMON_DCPS_SECURITY, b);
}
#endif

bool
Service_Participant::get_BIT() const
{
  return config_store_->get_boolean(OPENDDS_COMMON_DCPS_BIT, OPENDDS_COMMON_DCPS_BIT_default);
}

void
Service_Participant::set_BIT(bool b)
{
  config_store_->set_boolean(OPENDDS_COMMON_DCPS_BIT, b);
}

NetworkAddress
Service_Participant::default_address() const
{
  return config_store_->get(OPENDDS_COMMON_DCPS_DEFAULT_ADDRESS,
                            OPENDDS_COMMON_DCPS_DEFAULT_ADDRESS_default,
                            ConfigStoreImpl::Format_No_Port,
                            ConfigStoreImpl::Kind_IPV4);
}

size_t
Service_Participant::n_chunks() const
{
  return config_store_->get_uint32(OPENDDS_COMMON_DCPS_CHUNKS, OPENDDS_COMMON_DCPS_CHUNKS_default);
}

void
Service_Participant::n_chunks(size_t chunks)
{
  config_store_->set_uint32(OPENDDS_COMMON_DCPS_CHUNKS, static_cast<DDS::UInt32>(chunks));
}

size_t
Service_Participant::association_chunk_multiplier() const
{
  return config_store_->get_uint32(OPENDDS_COMMON_DCPS_CHUNK_ASSOCIATION_MULTIPLIER,
                                  config_store_->get_uint32(OPENDDS_COMMON_DCPS_CHUNK_ASSOCIATION_MUTLTIPLIER,
                                                           OPENDDS_COMMON_DCPS_CHUNK_ASSOCIATION_MULTIPLIER_default));
}

void
Service_Participant::association_chunk_multiplier(size_t multiplier)
{
  config_store_->set_uint32(OPENDDS_COMMON_DCPS_CHUNK_ASSOCIATION_MULTIPLIER, static_cast<DDS::UInt32>(multiplier));
}

void
Service_Participant::liveliness_factor(int factor)
{
  config_store_->set_int32(OPENDDS_COMMON_DCPS_LIVELINESS_FACTOR, factor);
}

int
Service_Participant::liveliness_factor() const
{
  return config_store_->get_int32(OPENDDS_COMMON_DCPS_LIVELINESS_FACTOR,
                                 OPENDDS_COMMON_DCPS_LIVELINESS_FACTOR_default);
}

void
Service_Participant::register_discovery_type(const char* section_name,
                                             Discovery::Config* cfg)
{
  discovery_types_[section_name].reset(cfg);
}

int
Service_Participant::load_configuration(const String& config_fname)
{
  ACE_Configuration_Heap cf;
  int status = 0;

  if ((status = cf.open()) != 0)
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("open() returned %d\n"),
                      status),
                     -1);

  ACE_Ini_ImpExp import(cf);
  status = import.import_config(ACE_TEXT_CHAR_TO_TCHAR(config_fname.c_str()));

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("import_config () returned %d\n"),
                      status),
                     -1);
  } else {
    status = this->load_configuration(cf, ACE_TEXT_CHAR_TO_TCHAR(config_fname.c_str()));
  }

  return status;
}

int
Service_Participant::load_configuration(
  ACE_Configuration_Heap& config,
  const ACE_TCHAR* filename)
{
  process_section(*config_store_, config_reader_, config_reader_listener_, "OPENDDS", config, config.root_section(), ACE_TEXT_ALWAYS_CHAR(filename), false);

  // Register static discovery.
  add_discovery(static_rchandle_cast<Discovery>(StaticDiscovery::instance()));

  // load any discovery configuration templates before rtps discovery
  // this will populate the domain_range_templates_
  int status = load_domain_ranges();

  if (status != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::load_configuration: "
                 "load_domain_ranges() returned %d\n",
                 status));
    }
    return -1;
  }

  // Domain config is loaded after Discovery (see below). Since the domain
  // could be a domain_range that specifies the DiscoveryTemplate, check
  // for config templates before loading any config information.

  // load any rtps_discovery templates
  status = load_discovery_templates();

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_domain_range_configuration() returned %d\n"),
                      status),
                     -1);
  }

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

  // load any transport configuration templates before the transport config
  status = TransportRegistry::instance()->load_transport_templates();

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_transport_templates() returned %d\n"),
                      status),
                     -1);
  }

  status = TransportRegistry::instance()->load_transport_configuration(
             ACE_TEXT_ALWAYS_CHAR(filename), config);
  const String global_transport_config = config_store_->get(OPENDDS_COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG,
                                                           OPENDDS_COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG_default);
  if (!global_transport_config.empty()) {
    TransportConfig_rch config = TransportRegistry::instance()->get_config(global_transport_config);
    if (config) {
      TransportRegistry::instance()->global_config(config);
    } else if (TheTransportRegistry->config_has_transport_template(global_transport_config)) {
      if (DCPS_debug_level > 0) {
        // This is not an error.
        ACE_DEBUG((LM_NOTICE,
                   ACE_TEXT("(%P|%t) NOTICE: Service_Participant::load_configuration ")
                   ACE_TEXT("DCPSGlobalTransportConfig %C is a transport_template\n"),
                   global_transport_config.c_str()));
      }
    } else {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                        ACE_TEXT("Unable to locate specified global transport config: %C\n"),
                        global_transport_config.c_str()),
                       -1);
    }
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
Service_Participant::load_domain_configuration(ACE_Configuration_Heap& cf,
                                               const ACE_TCHAR* filename)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key domain_sect;

  if (cf.open_section(root, DOMAIN_SECTION_NAME, false, domain_sect) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any domain (sub)section. The code default configuration will be used.
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: Service_Participant::load_domain_configuration(): ")
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
Service_Participant::load_domain_ranges()
{
  const DCPS::ConfigStoreImpl::StringList sections = config_store_->get_section_names("OPENDDS_DOMAIN_RANGE");

  // Loop through the [DomainRange/*] sections
  for (DCPS::ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    DomainRange range_element(*pos);

    if (range_element.parse_domain_range() != 0) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Service_Participant::load_domain_ranges: "
                   "Error parsing %C section.\n",
                   range_element.config_prefix().c_str()));
      }
      return -1;
    }

    domain_ranges_.push_back(range_element);
  }

  return 0;
}

int Service_Participant::configure_domain_range_instance(DDS::DomainId_t domainId)
{
  Discovery::RepoKey name = get_discovery_template_instance_name(domainId);

  if (discoveryMap_.find(name) == discoveryMap_.end()) {
    // create a cf that has [rtps_discovery/name+domainId]
    // copy sections adding customization
    DomainRange dr_inst("");

    if (get_domain_range_info(domainId, dr_inst)) {
      ACE_Configuration_Heap dcf;
      dcf.open();
      const ACE_Configuration_Section_Key& root = dcf.root_section();

      // set the transport_config_name
      domain_to_transport_name_map_[domainId] = dr_inst.transport_config_name(config_store_);

      // create domain instance
      ACE_Configuration_Section_Key dsect;
      dcf.open_section(root, DOMAIN_SECTION_NAME, true /* create */, dsect);
      ACE_Configuration_Section_Key dsub_sect;
      dcf.open_section(dsect, ACE_TEXT_CHAR_TO_TCHAR(to_dds_string(domainId).c_str()), true /* create */, dsub_sect);
      dcf.set_string_value(dsub_sect, ACE_TEXT("DiscoveryConfig"), ACE_TEXT_CHAR_TO_TCHAR(name.c_str()));
      const DCPS::ConfigStoreImpl::StringMap domain_info = dr_inst.domain_info(config_store_);
      for (DCPS::ConfigStoreImpl::StringMap::const_iterator it = domain_info.begin();
           it != domain_info.end();
           ++it) {
        if (it->first != "DISCOVERY_TEMPLATE") {
          dcf.set_string_value(dsub_sect, ACE_TEXT_CHAR_TO_TCHAR(it->first.c_str()), ACE_TEXT_CHAR_TO_TCHAR(it->second.c_str()));
        }
        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::")
                     ACE_TEXT("configure_domain_range_instance(): adding %C=%C\n"),
                     it->first.c_str(), it->second.c_str()));
        }
      }

      String cfg_name;
      if (get_transport_base_config_name(domainId, cfg_name)) {
        if (TransportRegistry::instance()->config_has_transport_template(cfg_name)) {
          // create transport instance add default transport config
          TransportRegistry::instance()->create_transport_template_instance(domainId, cfg_name);
          const OPENDDS_STRING config_instance_name = TransportRegistry::instance()->get_config_instance_name(domainId);
          dcf.set_string_value(dsub_sect, ACE_TEXT("DefaultTransportConfig"),
                               ACE_TEXT_CHAR_TO_TCHAR(config_instance_name.c_str()));
          if (DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) Service_Participant::")
                       ACE_TEXT("configure_domain_range_instance(): setting DefaultTransportConfig=%C\n"),
                       config_instance_name.c_str()));
          }
        }
      } else {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: Service_Participant::")
                          ACE_TEXT("configure_domain_range_instance(): ")
                          ACE_TEXT("transport config not found for domain %d\n"),
                          domainId),
                         -1);
      }

      //create matching discovery instance
      ACE_Configuration_Section_Key sect;
      dcf.open_section(root, RTPS_SECTION_NAME, true /* create */, sect);
      ACE_Configuration_Section_Key sub_sect;
      dcf.open_section(sect, ACE_TEXT_CHAR_TO_TCHAR(name.c_str()), true, sub_sect);

      ValueMap discovery_settings;
      if (process_customizations(domainId, dr_inst.discovery_template_name(config_store_), discovery_settings)) {
        for (ValueMap::const_iterator ds_it = discovery_settings.begin(); ds_it != discovery_settings.end(); ++ds_it) {
          dcf.set_string_value(sub_sect, ACE_TEXT_CHAR_TO_TCHAR(ds_it->first.c_str()), ACE_TEXT_CHAR_TO_TCHAR(ds_it->second.c_str()));
        }
      }

      // load discovery
      int status = this->load_discovery_configuration(dcf, RTPS_SECTION_NAME);

      if (status != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: Service_Participant::configure_domain_range_instance(): ")
                          ACE_TEXT("load_discovery_configuration() returned %d\n"),
                          status),
                         -1);
      }

      // load domain config
      status = this->load_domain_configuration(dcf, 0);

      if (status != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: Service_Participant::configure_domain_range_instance(): ")
                          ACE_TEXT("load_domain_configuration() returned %d\n"),
                          status),
                         -1);
      }

      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Service_Participant::configure_domain_range_instance(): ")
                 ACE_TEXT("configure domain %d.\n"),
                 domainId));
      }
    }

  } else {
    // > 9 to limit number of messages.
    if (DCPS_debug_level > 9) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Service_Participant::configure_domain_range_instance(): ")
                 ACE_TEXT("domain %d already configured.\n"),
                 domainId));
    }
  }
  return 0;
}


bool
Service_Participant::belongs_to_domain_range(DDS::DomainId_t domainId) const
{
  for (OPENDDS_VECTOR(DomainRange)::const_iterator i = domain_ranges_.begin(); i != domain_ranges_.end(); ++i) {
    if (i->belongs_to_domain_range(domainId)) {
      return true;
    }
  }

  return false;
}

bool
Service_Participant::get_transport_base_config_name(DDS::DomainId_t domainId, String& name) const
{
  const String global_transport_config = config_store_->get(OPENDDS_COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG,
                                                            OPENDDS_COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG_default);
  OPENDDS_MAP(DDS::DomainId_t, OPENDDS_STRING)::const_iterator it = domain_to_transport_name_map_.find(domainId);
  if ( it != domain_to_transport_name_map_.end()) {
    name = it->second;
    return true;
  } else if (!global_transport_config.empty()) {
    name = global_transport_config;
    return true;
  } else {
    return false;
  }
}

int
Service_Participant::load_discovery_configuration(ACE_Configuration_Heap& cf,
                                                  const ACE_TCHAR* section_name)
{
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key sect;
  if (cf.open_section(root, section_name, false, sect) == 0) {

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
                        ACE_TEXT("load_discovery_configuration(): ")
                        ACE_TEXT("Unable to load libraries for %s\n"),
                        section_name),
                       -1);
    }
  }
  return 0;
}

int
Service_Participant::configure_discovery_template(DDS::DomainId_t domainId, const OPENDDS_STRING& discovery_name)
{
  ValueMap discovery_settings;
  if (process_customizations(domainId, discovery_name, discovery_settings)) {
    Discovery::RepoKey name = get_discovery_template_instance_name(domainId);

    if (discoveryMap_.find(name) == discoveryMap_.end()) {
      ACE_Configuration_Heap dcf;
      dcf.open();
      const ACE_Configuration_Section_Key& root = dcf.root_section();

      //create discovery instance
      ACE_Configuration_Section_Key sect;
      dcf.open_section(root, RTPS_SECTION_NAME, true /* create */, sect);
      ACE_Configuration_Section_Key sub_sect;
      dcf.open_section(sect, ACE_TEXT_CHAR_TO_TCHAR(name.c_str()), true, sub_sect);

      for (ValueMap::const_iterator ds_it = discovery_settings.begin(); ds_it != discovery_settings.end(); ++ds_it) {
        dcf.set_string_value(sub_sect, ACE_TEXT_CHAR_TO_TCHAR(ds_it->first.c_str()), ACE_TEXT_CHAR_TO_TCHAR(ds_it->second.c_str()));
        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::configure_discovery_template(): ")
                     ACE_TEXT("setting %C = %C\n"),
                     ds_it->first.c_str(), ds_it->second.c_str()));
        }
      }

      // load discovery
      int status = this->load_discovery_configuration(dcf, RTPS_SECTION_NAME);

      if (status != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: Service_Participant::configure_discovery_template(): ")
                          ACE_TEXT("load_discovery_configuration() returned %d\n"),
                          status),
                         -1);
      }
    } else {
      // already configured. not necessarily an error
      if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Discovery config %C already exists\n"),
                     name.c_str()));
        }

    }
  } else {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::configure_discovery_template(): ")
                      ACE_TEXT("process_customizations() returned false\n")),
                     -1);
  }

  return 0;
}


int
Service_Participant::load_discovery_templates()
{
  const DCPS::ConfigStoreImpl::StringList sections = config_store_->get_section_names("OPENDDS_RTPS_DISCOVERY");

  // Loop through the [rtps_discovery/*] sections
  for (DCPS::ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    discovery_infos_.push_back(DiscoveryInfo(*pos));
  }

  // return 0 even if no templates were loaded
  return 0;
}

int Service_Participant::DomainRange::parse_domain_range()
{
  const std::size_t dash_pos = name_.find("-", 0);

  if (dash_pos == std::string::npos || dash_pos == name_.length() - 1) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_domain_range: "
                 "'-' is missing from %C in %C section.\n",
                 name_.c_str(),
                 config_prefix_.c_str()));
    }
    return -1;
  }

  if (!convertToInteger(name_.substr(0, dash_pos), range_start_)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_domain_range: "
                 "Illegal integer value for start %C from %C in %C section.\n",
                 name_.substr(0, dash_pos).c_str(),
                 name_.c_str(),
                 config_prefix_.c_str()));
    }
    return -1;
  }
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) DEBUG: Service_Participant::parse_domain_range: "
               "%C range_start %d\n",
               config_prefix_.c_str(),
               range_start_));
  }

  if (!convertToInteger(name_.substr(dash_pos + 1), range_end_)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_domain_range: "
                 "Illegal integer value for end %C from %C in %C section.\n",
                 name_.substr(0, dash_pos).c_str(),
                 name_.c_str(),
                 config_prefix_.c_str()));
    }
    return -1;
  }

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) DEBUG: Service_Participant::parse_domain_range: "
               "%C range_end %d\n",
               config_prefix_.c_str(),
               range_end_));
  }

  if (range_end_ < range_start_) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_domain_range: "
                 "Range end %d is less than range start %d in %C section.\n",
                 range_end_,
                 range_start_,
                 config_prefix_.c_str()));
    }
    return -1;
  }

  return 0;
}

String
Service_Participant::DomainRange::discovery_template_name(RcHandle<ConfigStoreImpl> config_store) const
{
  return config_store->get(config_key("DiscoveryTemplate").c_str(), "");
}

String
Service_Participant::DomainRange::transport_config_name(RcHandle<ConfigStoreImpl> config_store) const
{
  const String global_transport_config = config_store->get(OPENDDS_COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG,
                                                           OPENDDS_COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG_default);
  return config_store->get(config_key("DefaultTransportConfig").c_str(), global_transport_config);
}

DCPS::ConfigStoreImpl::StringMap
Service_Participant::DomainRange::domain_info(RcHandle<ConfigStoreImpl> config_store) const
{
  return config_store->get_section_values(config_prefix_);
}

DCPS::ConfigStoreImpl::StringMap
Service_Participant::DiscoveryInfo::customizations(RcHandle<ConfigStoreImpl> config_store) const
{
  if (config_store->has(config_key("Customization").c_str())) {
    const String c = config_store->get(config_key("Customization").c_str(), "");
    if (!c.empty()) {
      return config_store->get_section_values("OPENDDS_CUSTOMIZATION_" + c);
    }
  }

  return DCPS::ConfigStoreImpl::StringMap();
}

DCPS::ConfigStoreImpl::StringMap
Service_Participant::DiscoveryInfo::disc_info(RcHandle<ConfigStoreImpl> config_store) const
{
  return config_store->get_section_values(config_prefix_);
}

bool
Service_Participant::has_domain_range() const
{
  return !domain_ranges_.empty();
}

bool
Service_Participant::get_domain_range_info(const DDS::DomainId_t id, DomainRange& inst)
{
  if (has_domain_range()) {
    for (OPENDDS_VECTOR(DomainRange)::iterator it = domain_ranges_.begin();
         it != domain_ranges_.end(); ++it) {
      if (it->belongs_to_domain_range(id)) {
        inst = *it;

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) DEBUG: Service_Participant::get_domain_range_info: "
                     "Domain %d is in %C\n",
                     id, it->config_prefix().c_str()));
        }

        return true;
      }
    }
  }
  return false;
}

bool
Service_Participant::process_customizations(DDS::DomainId_t id, const OPENDDS_STRING& discovery_name, ValueMap& customs)
{
  // get the discovery info
  OPENDDS_VECTOR(DiscoveryInfo)::const_iterator dit;
  for (dit = discovery_infos_.begin(); dit != discovery_infos_.end(); ++dit) {
    if (discovery_name == dit->discovery_name()) {
      break;
    }
  }

  if (dit != discovery_infos_.end()) {
    // add discovery info to customs
    const DCPS::ConfigStoreImpl::StringMap disc_info = dit->disc_info(config_store_);
    for (DCPS::ConfigStoreImpl::StringMap::const_iterator i = disc_info.begin(); i != disc_info.end(); ++i) {
      customs[i->first] = i->second;
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Service_Participant::")
                   ACE_TEXT("process_customizations(): adding config %C=%C\n"),
                   i->first.c_str(), i->second.c_str()));
      }
    }

    // update customs valuemap with any customizations
    const DCPS::ConfigStoreImpl::StringMap customizations = dit->customizations(config_store_);
    for (ValueMap::const_iterator i = customizations.begin(); i != customizations.end(); ++i) {
      if (i->first == "INTEROP_MULTICAST_OVERRIDE" && i->second == "AddDomainId") {
        DCPS::ConfigStoreImpl::StringMap::const_iterator pos2 = customs.find("INTEROP_MULTICAST_OVERRIDE");
        if (pos2 == customs.end()) {
          pos2 = customs.find("InteropMulticastOverride");
        }
        OPENDDS_STRING addr = pos2 != customs.end() ? pos2->second : "";
        size_t pos = addr.find_last_of(".");
        if (pos != OPENDDS_STRING::npos) {
          OPENDDS_STRING custom = addr.substr(pos + 1);
          int val = 0;
          if (!convertToInteger(custom, val)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: Service_Participant::")
                              ACE_TEXT("process_customizations(): ")
                              ACE_TEXT("could not convert %C to integer\n"),
                              custom.c_str()),
                             false);
          }
          val += id;
          addr = addr.substr(0, pos);
          addr += "." + to_dds_string(val);
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: Service_Participant::")
                            ACE_TEXT("process_customizations(): ")
                            ACE_TEXT("could not AddDomainId for %s\n"),
                            customs["InteropMulticastOverride"].c_str()),
                           false);
        }

        customs["InteropMulticastOverride"] = addr;
      }
    }
  }

  return true;
}

Discovery::RepoKey
Service_Participant::get_discovery_template_instance_name(const DDS::DomainId_t id)
{
  OpenDDS::DCPS::Discovery::RepoKey configured_name = "rtps_template_instance_";
  configured_name += to_dds_string(id);
  return configured_name;
}

bool
Service_Participant::is_discovery_template(const OPENDDS_STRING& name)
{
  OPENDDS_VECTOR(DiscoveryInfo)::const_iterator i;
  for (i = discovery_infos_.begin(); i != discovery_infos_.end(); ++i) {
    if (i->discovery_name() == name && !i->customizations(config_store_).empty()) {
      return true;
    }
  }

  return false;
}

#if OPENDDS_POOL_ALLOCATOR
void
Service_Participant::configure_pool()
{
  const size_t pool_size = config_store_->get_uint32(OPENDDS_COMMON_POOL_SIZE,
                                                    OPENDDS_COMMON_POOL_SIZE_default);
  const size_t pool_granularity = config_store_->get_uint32(OPENDDS_COMMON_POOL_GRANULARITY,
                                                          OPENDDS_COMMON_POOL_GRANULARITY_default);
  if (pool_size) {
    SafetyProfilePool::instance()->configure_pool(pool_size, pool_granularity);
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
      ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, factory_lock_, 0);

      if (!this->transient_data_cache_) {
        this->transient_data_cache_.reset(new DataDurabilityCache(kind));
      }
    }

    cache = this->transient_data_cache_.get();

  } else if (kind == DDS::PERSISTENT_DURABILITY_QOS) {
    {
      ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, factory_lock_, 0);

      try {
        if (!this->persistent_data_cache_) {
          const String persistent_data_dir =
            config_store_->get(OPENDDS_COMMON_DCPS_PERSISTENT_DATA_DIR,
                              OPENDDS_COMMON_DCPS_PERSISTENT_DATA_DIR_default);
          this->persistent_data_cache_.reset(new DataDurabilityCache(kind, persistent_data_dir));
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
Service_Participant::set_shutdown_listener(RcHandle<ShutdownListener> listener)
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
  config_store_->set_string(OPENDDS_DEFAULT_CONFIGURATION_FILE, ACE_TEXT_ALWAYS_CHAR(path));
}

ThreadStatusManager& Service_Participant::get_thread_status_manager()
{
  return thread_status_manager_;
}

ACE_Thread_Mutex& Service_Participant::get_static_xtypes_lock()
{
  return xtypes_lock_;
}

#ifdef OPENDDS_NETWORK_CONFIG_MODIFIER
NetworkConfigModifier* Service_Participant::network_config_modifier()
{
  return dynamic_cast<NetworkConfigModifier*>(network_config_monitor_.get());
}
#endif

DDS::Duration_t
Service_Participant::bit_autopurge_nowriter_samples_delay() const
{
  return config_store_->get_duration(OPENDDS_COMMON_BIT_AUTOPURGE_NOWRITER_SAMPLES_DELAY,
                                    OPENDDS_COMMON_BIT_AUTOPURGE_NOWRITER_SAMPLES_DELAY_default);
}

void
Service_Participant::bit_autopurge_nowriter_samples_delay(const DDS::Duration_t& delay)
{
  config_store_->set_duration(OPENDDS_COMMON_BIT_AUTOPURGE_NOWRITER_SAMPLES_DELAY, delay);
}

DDS::Duration_t
Service_Participant::bit_autopurge_disposed_samples_delay() const
{
  return config_store_->get_duration(OPENDDS_COMMON_BIT_AUTOPURGE_DISPOSED_SAMPLES_DELAY,
                                    OPENDDS_COMMON_BIT_AUTOPURGE_DISPOSED_SAMPLES_DELAY_default);
}

void
Service_Participant::bit_autopurge_disposed_samples_delay(const DDS::Duration_t& delay)
{
  config_store_->set_duration(OPENDDS_COMMON_BIT_AUTOPURGE_DISPOSED_SAMPLES_DELAY, delay);
}

XTypes::TypeInformation
Service_Participant::get_type_information(DDS::DomainParticipant_ptr participant,
                                          const DDS::BuiltinTopicKey_t& key) const
{
  DomainParticipantImpl* participant_servant = dynamic_cast<DomainParticipantImpl*>(participant);
  if (participant_servant) {
    XTypes::TypeLookupService_rch tls = participant_servant->get_type_lookup_service();
    if (tls) {
      return tls->get_type_info(key);
    }
  }

  return XTypes::TypeInformation();
}

#ifndef OPENDDS_SAFETY_PROFILE
DDS::ReturnCode_t Service_Participant::get_dynamic_type(DDS::DynamicType_var& type,
  DDS::DomainParticipant_ptr participant, const DDS::BuiltinTopicKey_t& key) const
{
  return dynamic_cast<DomainParticipantImpl*>(participant)->get_dynamic_type(type, key);
}
#endif

XTypes::TypeObject
Service_Participant::get_type_object(DDS::DomainParticipant_ptr participant,
                                     const XTypes::TypeIdentifier& ti) const
{
  DomainParticipantImpl* participant_servant = dynamic_cast<DomainParticipantImpl*>(participant);
  if (participant_servant) {
    XTypes::TypeLookupService_rch tls = participant_servant->get_type_lookup_service();
    if (tls) {
      return tls->get_type_object(ti);
    }
  }

  return XTypes::TypeObject();
}

Service_Participant::TypeObjectEncoding
Service_Participant::type_object_encoding() const
{
  String encoding_str = "Normal";
  config_store_->get(OPENDDS_COMMON_DCPS_TYPE_OBJECT_ENCODING, encoding_str);

  struct NameValue {
    const char* name;
    TypeObjectEncoding value;
  };
  static const NameValue entries[] = {
    {"Normal", Encoding_Normal},
    {"WriteOldFormat", Encoding_WriteOldFormat},
    {"ReadOldFormat", Encoding_ReadOldFormat},
  };
  for (size_t i = 0; i < sizeof entries / sizeof entries[0]; ++i) {
    if (0 == std::strcmp(entries[i].name, encoding_str.c_str())) {
      return entries[i].value;
    }
  }
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Service_Participant::type_object_encoding: "
             "invalid encoding %C\n", encoding_str.c_str()));

  return Encoding_Normal;
}

void Service_Participant::type_object_encoding(TypeObjectEncoding encoding)
{
  switch (encoding) {
  case Encoding_Normal:
    config_store_->set_string(OPENDDS_COMMON_DCPS_TYPE_OBJECT_ENCODING, "Normal");
    break;
  case Encoding_WriteOldFormat:
    config_store_->set_string(OPENDDS_COMMON_DCPS_TYPE_OBJECT_ENCODING, "WriteOldFormat");
    break;
  case Encoding_ReadOldFormat:
    config_store_->set_string(OPENDDS_COMMON_DCPS_TYPE_OBJECT_ENCODING, "ReadOldFormat");
    break;
  }
}

void
Service_Participant::type_object_encoding(const char* encoding)
{
  config_store_->set_string(OPENDDS_COMMON_DCPS_TYPE_OBJECT_ENCODING, encoding);
}

unsigned int
Service_Participant::printer_value_writer_indent() const
{
  return config_store_->get_uint32(OPENDDS_COMMON_PRINTER_VALUE_WRITER_INDENT,
                                  OPENDDS_COMMON_PRINTER_VALUE_WRITER_INDENT_default);
}

void
Service_Participant::printer_value_writer_indent(unsigned int value)
{
  config_store_->set_uint32(OPENDDS_COMMON_PRINTER_VALUE_WRITER_INDENT, value);
}

void
Service_Participant::ConfigReaderListener::on_data_available(InternalDataReader_rch reader)
{
  InternalDataReader<ConfigPair>::SampleSequence samples;
  InternalSampleInfoSequence infos;
  reader->read(samples, infos, DDS::LENGTH_UNLIMITED,
               DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& p = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      if (p.key() == OPENDDS_COMMON_ORB_LOG_FILE) {
        set_log_file_name(p.value().c_str());
      } else if (p.key() == OPENDDS_COMMON_ORB_VERBOSE_LOGGING) {
        set_log_verbose(ACE_OS::atoi(p.value().c_str()));
      } else if (p.key() == OPENDDS_COMMON_DCPS_DEBUG_LEVEL) {
        set_DCPS_debug_level(ACE_OS::atoi(p.value().c_str()));
      } else if (p.key() == OPENDDS_COMMON_DCPSRTI_SERIALIZATION) {
        if (ACE_OS::atoi(p.value().c_str()) == 0 && log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ConfigReaderListener::on_data_available: ")
                     ACE_TEXT("Argument ignored: DCPSRTISerialization is required to be enabled\n")));
        }
      } else if (p.key() == OPENDDS_COMMON_DCPS_TRANSPORT_DEBUG_LEVEL) {
        OpenDDS::DCPS::Transport_debug_level = ACE_OS::atoi(p.value().c_str());
      } else if (p.key() == OPENDDS_COMMON_DCPS_THREAD_STATUS_INTERVAL) {
        service_participant_.thread_status_manager_.thread_status_interval(TimeDuration(ACE_OS::atoi(p.value().c_str())));
#ifdef OPENDDS_SECURITY
      } else if (p.key() == OPENDDS_COMMON_DCPS_SECURITY_DEBUG_LEVEL) {
        security_debug.set_debug_level(ACE_OS::atoi(p.value().c_str()));
      } else if (p.key() == OPENDDS_COMMON_DCPS_SECURITY_DEBUG) {
        security_debug.parse_flags(p.value().c_str());
      } else if (p.key() == OPENDDS_COMMON_DCPS_SECURITY_FAKE_ENCRYPTION) {
        security_debug.fake_encryption = ACE_OS::atoi(p.value().c_str());
#endif
      } else if (p.key() == OPENDDS_COMMON_DCPS_LOG_LEVEL) {
        log_level.set_from_string(p.value().c_str());
      } else if (p.key() == OPENDDS_COMMON_DCPS_PENDING_TIMEOUT) {
        ACE_GUARD(ACE_Thread_Mutex, guard, service_participant_.cached_config_mutex_);
        service_participant_.pending_timeout_ =
          service_participant_.config_store_->get(OPENDDS_COMMON_DCPS_PENDING_TIMEOUT,
                                                  OPENDDS_COMMON_DCPS_PENDING_TIMEOUT_default,
                                                  ConfigStoreImpl::Format_IntegerSeconds);
      } else if (p.key() == OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY) {
        ACE_GUARD(ACE_Thread_Mutex, guard, service_participant_.cached_config_mutex_);
        service_participant_.default_discovery_ =
          service_participant_.config_store_->get(OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY,
                                                  OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY_default);
      } else if (p.key() == OPENDDS_CONFIG_DEBUG_LOGGING) {
        const bool flag = service_participant_.config_store_->get_boolean(OPENDDS_CONFIG_DEBUG_LOGGING,
                                                                          OPENDDS_CONFIG_DEBUG_LOGGING_default);
        service_participant_.config_store_->debug_logging = flag;
      }
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
