/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> //Only the _pch include should start with DCPS/

#include "Service_Participant.h"

#include "BuiltInTopicUtils.h"
#include "DataDurabilityCache.h"
#include "DefaultNetworkConfigMonitor.h"
#include "GuidConverter.h"
#include "LinuxNetworkConfigMonitor.h"
#include "Logging.h"
#include "MonitorFactory.h"
#include "Qos_Helper.h"
#include "RecorderImpl.h"
#include "ReplayerImpl.h"
#include "StaticDiscovery.h"
#include "ThreadStatusManager.h"
#include "WaitSet.h"
#include "debug.h"

#include "transport/framework/TransportRegistry.h"

#include <dds/OpenDDSConfigWrapper.h>

#include "../Version.h"
#if OPENDDS_CONFIG_SECURITY
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

#if !defined (ACE_WIN32)
extern char **environ;
#endif

#if OPENDDS_GCC
// Embed GDB Extension
asm(
  ".pushsection \".debug_gdb_scripts\", \"MS\",@progbits,1\n"
  ".byte 4\n" // 4 means this is an embedded Python script
  ".ascii \"gdb.inlined-script\\n\"\n"
  ".incbin \"../tools/scripts/gdbext.py\"\n"
  ".byte 0\n"
  ".popsection\n"
);
#endif

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

static const String REPO_DISCOVERY_TYPE("repository");
static const String RTPS_DISCOVERY_TYPE("rtps_discovery");

namespace {

String toupper(const String& x)
{
  String retval;
  for (String::const_iterator pos = x.begin(), limit = x.end(); pos != limit; ++pos) {
    retval.push_back(static_cast<char>(ACE_OS::ace_toupper(*pos)));
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
  , pending_timeout_(0,0) // Can't use COMMON_DCPS_PENDING_TIMEOUT_default due to initialization order.
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

ReactorTask_rch
Service_Participant::reactor_task()
{
  return rchandle_from(&reactor_task_);
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
        ACE_GUARD_RETURN(ACE_Thread_Mutex, ncm_guard, network_config_monitor_lock_,
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
#if OPENDDS_CONFIG_SECURITY
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

      parse_env();

      if (parse_args(argc, argv) != 0) {
        return DDS::DomainParticipantFactory::_nil();
      }

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

      reactor_task_.open_reactor_task(&thread_status_manager_, "Service_Participant");

      job_queue_ = make_rch<JobQueue>(reactor_task_.get_reactor());

      const bool monitor_enabled = config_store_->get_boolean(COMMON_DCPS_MONITOR,
                                                              COMMON_DCPS_MONITOR_default);

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
    network_config_monitor_ = make_rch<LinuxNetworkConfigMonitor>(rchandle_from(&reactor_task_));
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



void Service_Participant::parse_env()
{
#if defined (ACE_WIN32)
  LPTCH env_strings = GetEnvironmentStrings();

  // If the returned pointer is NULL, exit.
  if (!env_strings) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_env: Could not get environment strings\n"));
    }
    return;
  }

  LPTSTR env_string = (LPTSTR) env_strings;

  while (*env_string) {
    parse_env(ACE_TEXT_ALWAYS_CHAR(env_string));
    env_string += lstrlen(env_string) + 1;
  }
  FreeEnvironmentStrings(env_strings);

#else

  for (char** e = environ; *e; ++e) {
    parse_env(*e);
  }

#endif
}

void Service_Participant::parse_env(const String& p)
{
  // Only parse environment variables starting with OPENDDS_.
  if (p.substr(0, 8) == "OPENDDS_") {
    // Extract everything after OPENDDS_.
    const String q = p.substr(8);
    // q should have the form key=value
    String::size_type pos = q.find('=');
    if (pos != String::npos) {
      // Split into key and value.
      const String key = q.substr(0, pos);
      const String value = q.substr(pos + 1);
      config_store_->set(key.c_str(), value);
      config_reader_listener_->on_data_available(config_reader_);
    }
  }
}

int Service_Participant::parse_args(int& argc, ACE_TCHAR* argv[])
{
  int retval = 0;
  bool config_file_loaded = false;

  // Process logging options first, so they are in effect if we need to log
  // while processing other options.
  ACE_Arg_Shifter log_arg_shifter(argc, argv);
  while (log_arg_shifter.is_anything_left()) {
    const ACE_TCHAR* currentArg = 0;

    if ((currentArg = log_arg_shifter.get_the_parameter(ACE_TEXT("-ORBLogFile"))) != 0) {
      config_store_->set_string(COMMON_ORB_LOG_FILE, ACE_TEXT_ALWAYS_CHAR(currentArg));
      config_reader_listener_->on_data_available(config_reader_);
      log_arg_shifter.consume_arg();

    } else if ((currentArg = log_arg_shifter.get_the_parameter(ACE_TEXT("-ORBVerboseLogging"))) != 0) {
      config_store_->set_string(COMMON_ORB_VERBOSE_LOGGING, ACE_TEXT_ALWAYS_CHAR(currentArg));
      config_reader_listener_->on_data_available(config_reader_);
      log_arg_shifter.consume_arg();

    } else if ((currentArg = log_arg_shifter.get_the_parameter(ACE_TEXT("-DCPSSingleConfigFile"))) != 0) {
      config_store_->set_string("CommonDCPSSingleConfigFile", ACE_TEXT_ALWAYS_CHAR(currentArg));
      config_reader_listener_->on_data_available(config_reader_);
      log_arg_shifter.consume_arg();

    } else {
      log_arg_shifter.ignore_arg();
    }
  }

  // Change the default to false in OpenDDS 4.
  const bool single_config_file = config_store_->get_boolean("CommonDCPSSingleConfigFile", true);
  String single_config_file_name;

  ACE_Arg_Shifter arg_shifter(argc, argv);
  while (arg_shifter.is_anything_left()) {

    const String current = ACE_TEXT_ALWAYS_CHAR(arg_shifter.get_current());
    if (current == "-DCPSConfigFile") {
      arg_shifter.consume_arg();
      if (!arg_shifter.is_anything_left()) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Service_Participant::parse_args: %C requires a parameter\n",
                     current.c_str()));
        }
        retval = -1;
        break;
      }
      if (arg_shifter.is_parameter_next()) {
        const String filename = ACE_TEXT_ALWAYS_CHAR(arg_shifter.get_current());
        config_store_->set("CommonDCPSConfigFile", filename);
        config_reader_listener_->on_data_available(config_reader_);
        arg_shifter.consume_arg();

        if (single_config_file) {
          single_config_file_name = filename;
        } else {
          if (process_config_file(filename, true)) {
            config_file_loaded = true;
          } else {
            retval = -1;
          }
        }
      } else {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Service_Participant::parse_args: %C requires a parameter\n",
                     current.c_str()));
        }
        retval = -1;
        arg_shifter.ignore_arg();
      }
    } else if (toupper(current.substr(0, 5)) == "-DCPS" || toupper(current.substr(0, 11)) == "-FEDERATION") {
      arg_shifter.consume_arg();
      if (!arg_shifter.is_anything_left()) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Service_Participant::parse_args: %C requires a parameter\n",
                     current.c_str()));
        }
        retval = -1;
        break;
      }
      const String key = "COMMON" + current;
      if (arg_shifter.is_parameter_next()) {
        config_store_->set_string(key.c_str(), ACE_TEXT_ALWAYS_CHAR(arg_shifter.get_current()));
        config_reader_listener_->on_data_available(config_reader_);
        arg_shifter.consume_arg();
      } else {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Service_Participant::parse_args: %C requires a parameter\n",
                     current.c_str()));
        }
        retval = -1;
        arg_shifter.ignore_arg();
      }
    } else if (current.substr(0, 8) == "-OpenDDS") {
      arg_shifter.consume_arg();
      if (!arg_shifter.is_anything_left()) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Service_Participant::parse_args: %C requires a parameter\n",
                     current.c_str()));
        }
        retval = -1;
        break;
      }
      const String key = current.substr(8);
      if (arg_shifter.is_parameter_next()) {
        config_store_->set_string(key.c_str(), ACE_TEXT_ALWAYS_CHAR(arg_shifter.get_current()));
        config_reader_listener_->on_data_available(config_reader_);
        arg_shifter.consume_arg();
      } else {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Service_Participant::parse_args: %C requires a parameter\n",
                     current.c_str()));
        }
        retval = -1;
        arg_shifter.ignore_arg();
      }
    } else {
      arg_shifter.ignore_arg();
    }
  }

  if (single_config_file && !single_config_file_name.empty()) {
    if (process_config_file(single_config_file_name, false)) {
      config_file_loaded = true;
    } else {
      retval = -1;
    }
  }

  if (!config_file_loaded) {
    const String default_configuration_file = config_store_->get(DEFAULT_CONFIGURATION_FILE,
                                                                 DEFAULT_CONFIGURATION_FILE_default);
    if (!default_configuration_file.empty()) {
      if (!process_config_file(default_configuration_file, !single_config_file)) {
        retval = -1;
      }
    }
  }

  // Register static discovery.
  add_discovery(static_rchandle_cast<Discovery>(StaticDiscovery::instance()));

  // load any discovery configuration templates before rtps discovery
  // this will populate the domain_range_templates_
  int status = load_domain_ranges();

  if (status != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_args: "
                 "load_domain_ranges() returned %d\n",
                 status));
    }
    return -1;
  }


  // Domain config is loaded after Discovery (see below). Since the domain
  // could be a domain_range that specifies the DiscoveryTemplate, check
  // for config templates before loading any config information.

  status = this->load_discovery_configuration(RTPS_DISCOVERY_TYPE, false);

  if (status != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_args: "
                 "load_discovery_configuration() returned %d\n",
                 status));
    }
    return -1;
  }

  status = this->load_discovery_configuration(REPO_DISCOVERY_TYPE, false);

  if (status != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_args: "
                 "load_discovery_configuration() returned %d\n",
                 status));
    }
    return -1;
  }

  status = TransportRegistry::instance()->load_transport_configuration();

  if (status != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_args: "
                 "load_transport_configuration () returned %d\n",
                 status));
    }
    return -1;
  }

  const String global_transport_config = config_store_->get(COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG,
                                                            COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG_default);
  if (!global_transport_config.empty()) {
    TransportConfig_rch config = TransportRegistry::instance()->get_config(global_transport_config);
    if (config) {
      TransportRegistry::instance()->global_config(config);
    } else {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Service_Participant::parse_args: "
                   "Unable to locate specified global transport config: %C\n",
                   global_transport_config.c_str()));
      }
      return -1;
    }
  }

  // Needs to be loaded after the [rtps_discovery/*] and [repository/*]
  // sections to allow error reporting on bad discovery config names.
  // Also loaded after the transport configuration so that
  // DefaultTransportConfig within [domain/*] can use TransportConfig objects.
  status = load_domain_configuration();

  if (status != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::parse_args: "
                 "load_domain_configuration () returned %d\n",
                 status));
    }
    return -1;
  }

  // Needs to be loaded after transport configs and instances and domains.
  try {
    status = StaticDiscovery::instance()->load_configuration();

    if (status != 0) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Service_Participant::parse_args: "
                   "load_discovery_configuration() returned %d\n",
                   status));
      }
      return -1;
    }
  } catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in Service_Participant::parse_args: "
      "trying to load_discovery_configuration()");
    return -1;
  }

  // Indicates successful parsing of the command line
  return retval;
}

bool
Service_Participant::process_config_file(const String& config_name,
                                         bool allow_overwrite)
{
  if (config_name.empty()) {
    if (log_level >= LogLevel::Error) {
      ACE_DEBUG((LM_INFO,
                 "(%P|%t) ERROR: Service_Participant::process_config_file: "
                 "configuration file name is empty.\n"));
    }
    return false;
  }

  String config_fname = config_name;

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
                 "(%P|%t) ERROR: Service_Participant::process_config_file: "
                 "could not find config file \"%C\": %p\n",
                 config_fname.c_str(), ACE_TEXT("fopen")));
    }
    return false;
  }

  ACE_OS::fclose(in);

  if (log_level >= LogLevel::Info) {
    ACE_DEBUG((LM_INFO,
               "(%P|%t) INFO: Service_Participant::process_config_file: "
               "Going to load configuration from \"%C\"\n",
               config_fname.c_str()));
  }

  if (load_configuration(config_fname, allow_overwrite) != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::process_config_file: "
                 "load_configuration() failed.\n"));
    }
    return false;
  }

  config_reader_listener_->on_data_available(config_reader_);

  return true;
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
  const String scheduler_str = config_store_->get(COMMON_SCHEDULER,
                                                  COMMON_SCHEDULER_default);

  suseconds_t usec = config_store_->get_int32(COMMON_SCHEDULER_SLICE,
                                              COMMON_SCHEDULER_SLICE_default);
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
  config_store_->set((String("REPOSITORY_") + key).c_str(), String("@") + key);
  const String k = String("REPOSITORY_") + key + "_RepositoryIor";
  if (overwrite) {
    config_store_->set(k.c_str(), ior);
  }
  config_reader_listener_->on_data_available(config_reader_);

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Service_Participant::set_repo_ior: Repo[%C] == %C\n"),
               key.c_str(), ior));
  }

  if (!discovery_types_.count(REPO_DISCOVERY_TYPE)) {
    // Re-use a transport registry function to attempt a dynamic load of the
    // library that implements the 'repo_type' (InfoRepoDiscovery)
    TheTransportRegistry->load_transport_lib(REPO_DISCOVERY_TYPE);
  }

  if (discovery_types_.count(REPO_DISCOVERY_TYPE)) {
    discovery_types_[REPO_DISCOVERY_TYPE]->discovery_config();
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
  return config_store_->get_boolean(COMMON_DCPS_BIDIR_GIOP, COMMON_DCPS_BIDIR_GIOP_default);
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
  config_store_->set_string(COMMON_DCPS_DEFAULT_DISCOVERY, key.c_str());
}

Discovery::RepoKey
Service_Participant::get_default_discovery()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, cached_config_mutex_, COMMON_DCPS_DEFAULT_DISCOVERY_default);
  return default_discovery_;
}

Discovery_rch
Service_Participant::get_discovery(const DDS::DomainId_t domain)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, maps_lock_, Discovery_rch());

  // Start with the default discovery.
  Discovery::RepoKey repo = get_default_discovery();

  // Override with the discovery for the domain range.
  DomainRanges::const_iterator dr_pos = domain_ranges_.begin();
  for (DomainRanges::const_iterator limit = domain_ranges_.end(); dr_pos != limit; ++dr_pos) {
    if (dr_pos->belongs_to_domain_range(domain)) {
      repo = dr_pos->discovery_template(config_store_, repo);
      break;
    }
  }

  // Override with the discovery for the domain.
  DomainRepoMap::const_iterator pos = domainRepoMap_.find(domain);
  if (pos != domainRepoMap_.end()) {
    repo = pos->second;
  }

  RepoKeyDiscoveryMap::const_iterator location = discoveryMap_.find(repo);

  if (location == discoveryMap_.end()) {
    if (dr_pos != domain_ranges_.end()) {
      const int ret = configure_domain_range_instance(dr_pos, domain, repo);

      // return the newly configured domain and return it
      if (!ret) {
        return discoveryMap_[repo];
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
      bool ok = set_repo_ior(DEFAULT_REPO_IOR, Discovery::DEFAULT_REPO, true, false);

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
      return discoveryMap_[Discovery::DEFAULT_REPO];

    } else if (repo == Discovery::DEFAULT_RTPS) {

      int status = load_discovery_configuration(RTPS_DISCOVERY_TYPE, true);

      if (status != 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::get_Discovery ")
                   ACE_TEXT("failed attempt to load default RTPS discovery for domain %d.\n"),
                   domain));

        return Discovery_rch();
      }

      // Try to find it again
      location = discoveryMap_.find(Discovery::DEFAULT_RTPS);

      if (location == discoveryMap_.end()) {
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
  config_store_->set_int32(COMMON_FEDERATION_RECOVERY_DURATION, duration);
}

int
Service_Participant::federation_recovery_duration() const
{
  return config_store_->get_int32(COMMON_FEDERATION_RECOVERY_DURATION,
                                  COMMON_FEDERATION_RECOVERY_DURATION_default);
}

void
Service_Participant::federation_initial_backoff_seconds(int value)
{
  config_store_->set_int32(COMMON_FEDERATION_INITIAL_BACKOFF_SECONDS, value);
}

int
Service_Participant::federation_initial_backoff_seconds() const
{
  return config_store_->get_int32(COMMON_FEDERATION_INITIAL_BACKOFF_SECONDS,
                                  COMMON_FEDERATION_INITIAL_BACKOFF_SECONDS_default);
}

void
Service_Participant::federation_backoff_multiplier(int value)
{
  config_store_->set_int32(COMMON_FEDERATION_BACKOFF_MULTIPLIER, value);
}

int
Service_Participant::federation_backoff_multiplier() const
{
  return config_store_->get_int32(COMMON_FEDERATION_BACKOFF_MULTIPLIER,
                                  COMMON_FEDERATION_BACKOFF_MULTIPLIER_default);
}

void
Service_Participant::federation_liveliness(int value)
{
  config_store_->set_int32(COMMON_FEDERATION_LIVELINESS_DURATION, value);
}

int
Service_Participant::federation_liveliness() const
{
  return config_store_->get_int32(COMMON_FEDERATION_LIVELINESS_DURATION, COMMON_FEDERATION_LIVELINESS_DURATION_default);
}

void
Service_Participant::scheduler(long value)
{
  // Using a switch results in a compilation error since THR_SCHED_DEFAULT could be THR_SCHED_RR or THR_SCHED_FIFO.
  if (value == THR_SCHED_DEFAULT) {
    config_store_->set(COMMON_SCHEDULER, "SCHED_OTHER");
  } else if (value == THR_SCHED_RR) {
    config_store_->set(COMMON_SCHEDULER, "SCHED_RR");
  } else if (value == THR_SCHED_FIFO) {
    config_store_->set(COMMON_SCHEDULER, "SCHED_FIFO");
  } else {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING,
                 "(%P|%t) WARNING: Service_Participant::scheduler: cannot translate scheduler value %d\n",
                 value));
    }
    config_store_->set(COMMON_SCHEDULER, "");
  }
}

long
Service_Participant::scheduler() const
{
  const String str = config_store_->get(COMMON_SCHEDULER, "");
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
  config_store_->set_boolean(COMMON_DCPS_PUBLISHER_CONTENT_FILTER, flag);
}

bool
Service_Participant::publisher_content_filter() const
{
  return config_store_->get_boolean(COMMON_DCPS_PUBLISHER_CONTENT_FILTER,
                                    COMMON_DCPS_PUBLISHER_CONTENT_FILTER_default);
}

TimeDuration
Service_Participant::pending_timeout() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, cached_config_mutex_, COMMON_DCPS_PENDING_TIMEOUT_default);
  return pending_timeout_;
}

void Service_Participant::pending_timeout(const TimeDuration& value)
{
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, cached_config_mutex_);
    pending_timeout_ = value;
  }
  config_store_->set(COMMON_DCPS_PENDING_TIMEOUT, value, ConfigStoreImpl::Format_IntegerSeconds);
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
  return config_store_->get(COMMON_DCPS_BIT_TRANSPORT_IP_ADDRESS,
                            COMMON_DCPS_BIT_TRANSPORT_IP_ADDRESS_default);
}

int
Service_Participant::bit_transport_port() const
{
  return config_store_->get_int32(COMMON_DCPS_BIT_TRANSPORT_PORT,
                                  COMMON_DCPS_BIT_TRANSPORT_PORT_default);
}

void
Service_Participant::bit_transport_port(int port)
{
  config_store_->set_int32(COMMON_DCPS_BIT_TRANSPORT_PORT, port);
}

int
Service_Participant::bit_lookup_duration_msec() const
{
  return config_store_->get_int32(COMMON_DCPS_BIT_LOOKUP_DURATION_MSEC, COMMON_DCPS_BIT_LOOKUP_DURATION_MSEC_default);
}

void
Service_Participant::bit_lookup_duration_msec(int msec)
{
  config_store_->set_int32(COMMON_DCPS_BIT_LOOKUP_DURATION_MSEC, msec);
}

#if OPENDDS_CONFIG_SECURITY
bool
Service_Participant::get_security() const
{
  return config_store_->get_boolean(COMMON_DCPS_SECURITY, COMMON_DCPS_SECURITY_default);
}

void
Service_Participant::set_security(bool b)
{
  config_store_->set_boolean(COMMON_DCPS_SECURITY, b);
}
#endif

bool
Service_Participant::get_BIT() const
{
  return config_store_->get_boolean(COMMON_DCPS_BIT, COMMON_DCPS_BIT_default);
}

void
Service_Participant::set_BIT(bool b)
{
  config_store_->set_boolean(COMMON_DCPS_BIT, b);
}

NetworkAddress
Service_Participant::default_address() const
{
  return config_store_->get(COMMON_DCPS_DEFAULT_ADDRESS,
                            COMMON_DCPS_DEFAULT_ADDRESS_default,
                            ConfigStoreImpl::Format_No_Port,
                            ConfigStoreImpl::Kind_IPV4);
}

size_t
Service_Participant::n_chunks() const
{
  return config_store_->get_uint32(COMMON_DCPS_CHUNKS, COMMON_DCPS_CHUNKS_default);
}

void
Service_Participant::n_chunks(size_t chunks)
{
  config_store_->set_uint32(COMMON_DCPS_CHUNKS, static_cast<DDS::UInt32>(chunks));
}

size_t
Service_Participant::association_chunk_multiplier() const
{
  return config_store_->get_uint32(COMMON_DCPS_CHUNK_ASSOCIATION_MULTIPLIER,
                                  config_store_->get_uint32(COMMON_DCPS_CHUNK_ASSOCIATION_MUTLTIPLIER,
                                                            COMMON_DCPS_CHUNK_ASSOCIATION_MULTIPLIER_default));
}

void
Service_Participant::association_chunk_multiplier(size_t multiplier)
{
  config_store_->set_uint32(COMMON_DCPS_CHUNK_ASSOCIATION_MULTIPLIER, static_cast<DDS::UInt32>(multiplier));
}

void
Service_Participant::liveliness_factor(int factor)
{
  config_store_->set_int32(COMMON_DCPS_LIVELINESS_FACTOR, factor);
}

int
Service_Participant::liveliness_factor() const
{
  return config_store_->get_int32(COMMON_DCPS_LIVELINESS_FACTOR,
                                  COMMON_DCPS_LIVELINESS_FACTOR_default);
}

void
Service_Participant::register_discovery_type(const char* section_name,
                                             Discovery::Config* cfg)
{
  discovery_types_[section_name].reset(cfg);
}

int
Service_Participant::load_configuration(const String& config_fname,
                                        bool allow_overwrite)
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
    status = this->load_configuration(cf, ACE_TEXT_CHAR_TO_TCHAR(config_fname.c_str()), allow_overwrite);
  }

  return status;
}

int
Service_Participant::load_configuration(ACE_Configuration_Heap& config,
                                        const ACE_TCHAR* filename,
                                        bool allow_overwrite)
{
  process_section(*config_store_, config_reader_, config_reader_listener_, "", config, config.root_section(), allow_overwrite);
  TransportRegistry::instance()->add_config_alias(ACE_TEXT_ALWAYS_CHAR(filename), "$file");

  return 0;
}

DDS::DomainId_t
Service_Participant::DomainConfig::domain_id(RcHandle<ConfigStoreImpl> config_store) const
{
  DDS::DomainId_t di = -1;

  // Try using the domain name as an ID
  if (!convertToInteger(name_, di)) {
    di = -1;
  }

  return config_store->get_int32(config_key("DOMAIN_ID").c_str(), di);
}

String
Service_Participant::DomainConfig::discovery_config(RcHandle<ConfigStoreImpl> config_store) const
{
  const String r = config_store->get(COMMON_DCPS_DEFAULT_DISCOVERY,
                                     COMMON_DCPS_DEFAULT_DISCOVERY_default);
  String s = config_store->get(config_key("DOMAIN_REPO_KEY").c_str(), r);
  if (s == "-1") {
    s = r;
  }

  return config_store->get(config_key("DISCOVERY_CONFIG").c_str(), s);
}

String
Service_Participant::DomainConfig::default_transport_config(RcHandle<ConfigStoreImpl> config_store) const
{
  return config_store->get(config_key("DEFAULT_TRANSPORT_CONFIG").c_str(), "");
}

int
Service_Participant::load_domain_configuration()
{
  const DCPS::ConfigStoreImpl::StringList sections = config_store_->get_section_names("DOMAIN");
  for (DCPS::ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    const DomainConfig dc(*pos);
    if (!process_domain(dc.to_domain(config_store_))) {
      return -1;
    }
  }

  return 0;
}

bool
Service_Participant::process_domain(const Domain& domain)
{
  if (domain.domain_id() == -1) {
    // DomainId parameter is not set.
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Service_Participant::process_domain: "
                 "Missing DomainId value in [domain/%C] section.\n",
                 domain.name().c_str()));
    }
    return false;
  }

  const String& default_transport_config = domain.default_transport_config();
  if (!default_transport_config.empty()) {
    TransportRegistry* const reg = TransportRegistry::instance();
    TransportConfig_rch tc = reg->get_config(default_transport_config);
    if (tc.is_nil()) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Service_Participant::process_domain: "
                   "Unknown transport config %C in [domain/%C] section.\n",
                   default_transport_config.c_str(),
                   domain.name().c_str()));
      }
      return false;
    } else {
      reg->domain_default_config(domain.domain_id(), tc);
    }
  }

  // Check to see if the specified discovery configuration has been defined
  const Discovery::RepoKey& discovery_config = domain.discovery_config();
  if (!discovery_config.empty()) {
    if ((discovery_config != Discovery::DEFAULT_REPO) &&
        (discovery_config != Discovery::DEFAULT_RTPS) &&
        (discovery_config != Discovery::DEFAULT_STATIC) &&
        (discoveryMap_.find(discovery_config) == discoveryMap_.end())) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Service_Participant::process_domain: "
                   "Specified configuration (%C) not found.  Referenced in [domain/%C] section.\n",
                   discovery_config.c_str(),
                   domain.name().c_str()));
      }
      return false;
    }

    set_repo_domain(domain.domain_id(), discovery_config);
  }

  return true;
}


int
Service_Participant::load_domain_ranges()
{
  const DCPS::ConfigStoreImpl::StringList sections = config_store_->get_section_names("DOMAIN_RANGE");

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

int Service_Participant::configure_domain_range_instance(DomainRanges::const_iterator dr_pos,
                                                         DDS::DomainId_t domainId,
                                                         const Discovery::RepoKey& name)
{
  if (discoveryMap_.find(name) != discoveryMap_.end()) {
    // > 9 to limit number of messages.
    if (DCPS_debug_level > 9) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Service_Participant::configure_domain_range_instance(): ")
                 ACE_TEXT("domain %d already configured.\n"),
                 domainId));
    }
    return 0;
  }

  Domain domain(to_dds_string(domainId),
                domainId,
                name,
                dr_pos->default_transport_config(config_store_));
  if (!process_domain(domain)) {
    return -1;
  }

  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Service_Participant::configure_domain_range_instance(): ")
               ACE_TEXT("configure domain %d.\n"),
               domainId));
  }

  return 0;
}

int
Service_Participant::load_discovery_configuration(const String& discovery_type,
                                                  bool force)
{
  if (!force && !config_store_->has(discovery_type.c_str())) {
    return 0;
  }

  DiscoveryTypes::iterator iter = discovery_types_.find(discovery_type);

  if (iter == discovery_types_.end()) {
    // See if we can dynamically load the required libraries
    TheTransportRegistry->load_transport_lib(discovery_type);
    iter = discovery_types_.find(discovery_type);
  }

  if (iter != discovery_types_.end()) {
    // discovery code is loaded, process options
    return iter->second->discovery_config();
  } else {
    // No discovery code can be loaded, report an error
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                "(%P|%t) ERROR: Service_Participant::load_discovery_configuration: "
                 "Unable to load libraries for %C\n",
                 discovery_type.c_str()));
    }
    return -1;
  }
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
Service_Participant::DomainRange::discovery_template(RcHandle<ConfigStoreImpl> config_store,
                                                     const String& default_name) const
{
  return config_store->get(config_key("DiscoveryTemplate").c_str(), default_name);
}

String
Service_Participant::DomainRange::default_transport_config(RcHandle<ConfigStoreImpl> config_store) const
{
  const String global_transport_config = config_store->get(COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG,
                                                           COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG_default);
  return config_store->get(config_key("DefaultTransportConfig").c_str(), global_transport_config);
}

#if OPENDDS_POOL_ALLOCATOR
void
Service_Participant::configure_pool()
{
  const size_t pool_size = config_store_->get_uint32(COMMON_POOL_SIZE,
                                                     COMMON_POOL_SIZE_default);
  const size_t pool_granularity = config_store_->get_uint32(COMMON_POOL_GRANULARITY,
                                                            COMMON_POOL_GRANULARITY_default);
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
            config_store_->get(COMMON_DCPS_PERSISTENT_DATA_DIR,
                               COMMON_DCPS_PERSISTENT_DATA_DIR_default);
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
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, maps_lock_);
    if (discoveryMap_.count(discovery->key()) == 0) {
      discoveryMap_[discovery->key()] = discovery;
    }
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
  config_store_->set_string(DEFAULT_CONFIGURATION_FILE, ACE_TEXT_ALWAYS_CHAR(path));
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
  return config_store_->get_duration(COMMON_BIT_AUTOPURGE_NOWRITER_SAMPLES_DELAY,
                                     COMMON_BIT_AUTOPURGE_NOWRITER_SAMPLES_DELAY_default);
}

void
Service_Participant::bit_autopurge_nowriter_samples_delay(const DDS::Duration_t& delay)
{
  config_store_->set_duration(COMMON_BIT_AUTOPURGE_NOWRITER_SAMPLES_DELAY, delay);
}

DDS::Duration_t
Service_Participant::bit_autopurge_disposed_samples_delay() const
{
  return config_store_->get_duration(COMMON_BIT_AUTOPURGE_DISPOSED_SAMPLES_DELAY,
                                     COMMON_BIT_AUTOPURGE_DISPOSED_SAMPLES_DELAY_default);
}

void
Service_Participant::bit_autopurge_disposed_samples_delay(const DDS::Duration_t& delay)
{
  config_store_->set_duration(COMMON_BIT_AUTOPURGE_DISPOSED_SAMPLES_DELAY, delay);
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

namespace {
  const EnumList<Service_Participant::TypeObjectEncoding> type_object_encoding_kinds[] =
    {
      { Service_Participant::Encoding_Normal, "Normal" },
      { Service_Participant::Encoding_WriteOldFormat, "WriteOldFormat" },
      { Service_Participant::Encoding_ReadOldFormat, "ReadOldFormat" }
    };
}

Service_Participant::TypeObjectEncoding
Service_Participant::type_object_encoding() const
{
  return config_store_->get(COMMON_DCPS_TYPE_OBJECT_ENCODING, Encoding_Normal, type_object_encoding_kinds);
}

void Service_Participant::type_object_encoding(TypeObjectEncoding encoding)
{
  config_store_->set(COMMON_DCPS_TYPE_OBJECT_ENCODING, encoding, type_object_encoding_kinds);
}

void
Service_Participant::type_object_encoding(const char* encoding)
{
  config_store_->set(COMMON_DCPS_TYPE_OBJECT_ENCODING, encoding, type_object_encoding_kinds);
}

unsigned int
Service_Participant::printer_value_writer_indent() const
{
  return config_store_->get_uint32(COMMON_PRINTER_VALUE_WRITER_INDENT,
                                   COMMON_PRINTER_VALUE_WRITER_INDENT_default);
}

void
Service_Participant::printer_value_writer_indent(unsigned int value)
{
  config_store_->set_uint32(COMMON_PRINTER_VALUE_WRITER_INDENT, value);
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
      if (p.key() == COMMON_ORB_LOG_FILE) {
        set_log_file_name(p.value().c_str());
      } else if (p.key() == COMMON_ORB_VERBOSE_LOGGING) {
        set_log_verbose(ACE_OS::atoi(p.value().c_str()));
      } else if (p.key() == COMMON_DCPS_DEBUG_LEVEL) {
        set_DCPS_debug_level(ACE_OS::atoi(p.value().c_str()));
      } else if (p.key() == COMMON_DCPSRTI_SERIALIZATION) {
        if (ACE_OS::atoi(p.value().c_str()) == 0 && log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ConfigReaderListener::on_data_available: ")
                     ACE_TEXT("Argument ignored: DCPSRTISerialization is required to be enabled\n")));
        }
      } else if (p.key() == COMMON_DCPS_TRANSPORT_DEBUG_LEVEL) {
        OpenDDS::DCPS::Transport_debug_level = ACE_OS::atoi(p.value().c_str());
      } else if (p.key() == COMMON_DCPS_THREAD_STATUS_INTERVAL) {
        service_participant_.thread_status_manager_.thread_status_interval(TimeDuration(ACE_OS::atoi(p.value().c_str())));
#if OPENDDS_CONFIG_SECURITY
      } else if (p.key() == COMMON_DCPS_SECURITY_DEBUG_LEVEL) {
        security_debug.set_debug_level(ACE_OS::atoi(p.value().c_str()));
      } else if (p.key() == COMMON_DCPS_SECURITY_DEBUG) {
        security_debug.parse_flags(p.value().c_str());
      } else if (p.key() == COMMON_DCPS_SECURITY_FAKE_ENCRYPTION) {
        security_debug.fake_encryption = ACE_OS::atoi(p.value().c_str());
#endif
      } else if (p.key() == COMMON_DCPS_LOG_LEVEL) {
        log_level.set_from_string(p.value().c_str());
      } else if (p.key() == COMMON_DCPS_PENDING_TIMEOUT) {
        ACE_GUARD(ACE_Thread_Mutex, guard, service_participant_.cached_config_mutex_);
        service_participant_.pending_timeout_ =
          service_participant_.config_store_->get(COMMON_DCPS_PENDING_TIMEOUT,
                                                  COMMON_DCPS_PENDING_TIMEOUT_default,
                                                  ConfigStoreImpl::Format_IntegerSeconds);
      } else if (p.key() == COMMON_DCPS_DEFAULT_DISCOVERY) {
        ACE_GUARD(ACE_Thread_Mutex, guard, service_participant_.cached_config_mutex_);
        service_participant_.default_discovery_ =
          service_participant_.config_store_->get(COMMON_DCPS_DEFAULT_DISCOVERY,
                                                  COMMON_DCPS_DEFAULT_DISCOVERY_default);
      } else if (p.key() == CONFIG_DEBUG_LOGGING) {
        const bool flag = service_participant_.config_store_->get_boolean(CONFIG_DEBUG_LOGGING,
                                                                          CONFIG_DEBUG_LOGGING_default);
        service_participant_.config_store_->debug_logging = flag;
      }
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
