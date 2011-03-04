/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "debug.h"
#include "Service_Participant.h"
#include "InfoRepoUtils.h"
#include "BuiltInTopicUtils.h"
#include "DataDurabilityCache.h"
#include "RepoIdConverter.h"
#include "MonitorFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "tao/ORB_Core.h"
#include "tao/TAO_Singleton.h"

#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"
#include "ace/Configuration_Import_Export.h"
#include "ace/Service_Config.h"
#include "ace/Argv_Type_Converter.h"
#include "ace/Auto_Ptr.h"
#include "ace/Sched_Params.h"

#include <vector>
#include <sstream>

#if !defined (__ACE_INLINE__)
#include "Service_Participant.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

int Service_Participant::zero_argc = 0;

const int DEFAULT_BIT_TRANSPORT_PORT = 0; // let the OS pick the port

const size_t DEFAULT_NUM_CHUNKS = 20;

const size_t DEFAULT_CHUNK_MULTIPLIER = 10;

const int DEFAULT_FEDERATION_RECOVERY_DURATION       = 900; // 15 minutes in seconds.
const int DEFAULT_FEDERATION_INITIAL_BACKOFF_SECONDS = 1;   // Wait only 1 second.
const int DEFAULT_FEDERATION_BACKOFF_MULTIPLIER      = 2;   // Exponential backoff.
const int DEFAULT_FEDERATION_LIVELINESS              = 60;  // 1 minute hearbeat.

const int BIT_LOOKUP_DURATION_MSEC = 2000;

static ACE_TString config_fname(ACE_TEXT(""));

static const ACE_TCHAR DEFAULT_REPO_IOR[] = ACE_TEXT("file://repo.ior");

static const ACE_CString DEFAULT_PERSISTENT_DATA_DIR = "OpenDDS-durable-data-dir";

static const ACE_TCHAR COMMON_SECTION_NAME[] = ACE_TEXT("common");
static const ACE_TCHAR DOMAIN_SECTION_NAME[] = ACE_TEXT("domain");
static const ACE_TCHAR REPO_SECTION_NAME[]   = ACE_TEXT("repository");

static bool got_debug_level = false;
static bool got_info = false;
static bool got_chunks = false;
static bool got_chunk_association_multiplier = false;
static bool got_liveliness_factor = false;
static bool got_bit_transport_port = false;
static bool got_bit_transport_ip = false;
static bool got_bit_lookup_duration_msec = false;
static bool got_bit_flag = false;
static bool got_publisher_content_filter = false;

Service_Participant::Service_Participant()
  : orb_(CORBA::ORB::_nil()),
    orb_from_user_(0),
    dp_factory_servant_(0),
    n_chunks_(DEFAULT_NUM_CHUNKS),
    association_chunk_multiplier_(DEFAULT_CHUNK_MULTIPLIER),
    liveliness_factor_(80),
    bit_enabled_(
#ifdef DDS_HAS_MINIMUM_BIT
      false
#else
      true
#endif
    ),
    bit_lookup_duration_msec_(BIT_LOOKUP_DURATION_MSEC),
    monitor_factory_(0),
    monitor_(0),
    federation_recovery_duration_(DEFAULT_FEDERATION_RECOVERY_DURATION),
    federation_initial_backoff_seconds_(DEFAULT_FEDERATION_INITIAL_BACKOFF_SECONDS),
    federation_backoff_multiplier_(DEFAULT_FEDERATION_BACKOFF_MULTIPLIER),
    federation_liveliness_(DEFAULT_FEDERATION_LIVELINESS),
    schedulerQuantum_(ACE_Time_Value::zero),
    scheduler_(-1),
    priority_min_(0),
    priority_max_(0),
    publisher_content_filter_(true),
    transient_data_cache_(),
    persistent_data_cache_(),
    persistent_data_dir_(DEFAULT_PERSISTENT_DATA_DIR),
    pending_timeout_(ACE_Time_Value::zero)
{
  initialize();
}

Service_Participant::~Service_Participant()
{
  delete monitor_;
}

Service_Participant *
Service_Participant::instance()
{
  // Hide the template instantiation to prevent multiple instances
  // from being created.

  return
    TAO_Singleton<Service_Participant, TAO_SYNCH_MUTEX>::instance();
}

int
Service_Participant::svc()
{
  {
    bool done = false;

    // Ignore all signals to avoid
    //     ERROR: <something descriptive> Interrupted system call
    // The main thread will handle signals.
    sigset_t set;
    ACE_OS::sigfillset(&set);
    ACE_OS::thr_sigsetmask(SIG_SETMASK, &set, NULL);

    while (!done) {
      try {
        if (orb_->orb_core()->has_shutdown() == false) {
          orb_->run();
        }

        done = true;

      } catch (const CORBA::SystemException& sysex) {
        sysex._tao_print_exception(
          "ERROR: Service_Participant::svc");

      } catch (const CORBA::UserException& userex) {
        userex._tao_print_exception(
          "ERROR: Service_Participant::svc");

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "ERROR: Service_Participant::svc");
      }

      if (orb_->orb_core()->has_shutdown()) {
        done = true;

      } else {
        orb_->orb_core()->reactor()->reset_reactor_event_loop();
      }
    }
  }

  return 0;
}

int
Service_Participant::set_ORB(CORBA::ORB_ptr orb)
{
  // The orb is already created by the
  // get_domain_participant_factory() call.
  ACE_ASSERT(CORBA::is_nil(orb_.in()));
  // The provided orb should not be nil.
  ACE_ASSERT(!CORBA::is_nil(orb));

  orb_ = CORBA::ORB::_duplicate(orb);
  orb_from_user_ = 1;
  return 0;
}

CORBA::ORB_ptr
Service_Participant::get_ORB()
{
  // This method should be called after either set_ORB is called
  // or get_domain_participant_factory is called.
  ACE_ASSERT(!CORBA::is_nil(orb_.in()));

  return CORBA::ORB::_duplicate(orb_.in());
}

PortableServer::POA_ptr
Service_Participant::the_poa()
{
  if (CORBA::is_nil(root_poa_.in())) {
    CORBA::Object_var obj =
      orb_->resolve_initial_references("RootPOA");
    root_poa_ = PortableServer::POA::_narrow(obj.in());
  }

  return PortableServer::POA::_duplicate(root_poa_.in());
}

void
Service_Participant::shutdown()
{
  try {
    ACE_GUARD(TAO_SYNCH_MUTEX, guard, this->factory_lock_);

    if (!CORBA::is_nil(orb_.in())) {
      if (!orb_from_user_) {
        orb_->shutdown(0);
        this->wait();
      }

      // Don't delete the participants - require the client code
      // to delete participants
#if 0

      //TBD return error code from this call
      // -- non-empty entity will make this call return failure
      if (dp_factory_impl_->delete_contained_participants() != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::shutdown, ")
                   ACE_TEXT("delete_contained_participants failed.\n")));
      }

#endif

      if (!orb_from_user_) {
        root_poa_->destroy(1, 1);
        orb_->destroy();
      }

      orb_ = CORBA::ORB::_nil();
    }

    dp_factory_ = DDS::DomainParticipantFactory::_nil();

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: Service_Participant::shutdown");
    return;
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
  if (CORBA::is_nil(dp_factory_.in())) {
    ACE_GUARD_RETURN(TAO_SYNCH_MUTEX,
                     guard,
                     this->factory_lock_,
                     DDS::DomainParticipantFactory::_nil());

    if (CORBA::is_nil(dp_factory_.in())) {
      try {
        if (CORBA::is_nil(orb_.in())) {
          //TBD: allow user to specify the ORB id

          // Use a unique ORB for the DDS Service
          // to avoid conflicts with other CORBA code
          orb_ = CORBA::ORB_init(argc, argv, DEFAULT_ORB_NAME);
        }

        if (parse_args(argc, argv) != 0) {
          return DDS::DomainParticipantFactory::_nil();
        }

        ACE_ASSERT(!CORBA::is_nil(orb_.in()));

        if (config_fname == ACE_TEXT("")) {
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

        // Establish the default scheduling mechanism and
        // priority here.  Sadly, the ORB is already
        // initialized so we have no influence over its
        // scheduling or thread priority(ies).

        /// @TODO: Move ORB intitialization to after the
        ///        configuration file is processed and the
        ///        initial scheduling policy and priority are
        ///        established.
        this->initializeScheduling();

        CORBA::Object_var poa_object =
          orb_->resolve_initial_references("RootPOA");

        root_poa_ = PortableServer::POA::_narrow(poa_object.in());

        if (CORBA::is_nil(root_poa_.in())) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: ")
                     ACE_TEXT("Service_Participant::get_domain_participant_factory, ")
                     ACE_TEXT("nil RootPOA\n")));
          return DDS::DomainParticipantFactory::_nil();
        }

        ACE_NEW_RETURN(dp_factory_servant_,
                       DomainParticipantFactoryImpl(),
                       DDS::DomainParticipantFactory::_nil());

        dp_factory_ = dp_factory_servant_;

        // Give ownership to poa.
        //REMOVE SHH ???? dp_factory_servant_->_remove_ref ();

        if (CORBA::is_nil(dp_factory_.in())) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: ")
                     ACE_TEXT("Service_Participant::get_domain_participant_factory, ")
                     ACE_TEXT("nil DomainParticipantFactory. \n")));
          return DDS::DomainParticipantFactory::_nil();
        }

        if (!this->orb_from_user_) {
          PortableServer::POAManager_var poa_manager =
            root_poa_->the_POAManager();

          poa_manager->activate();

          if (activate(THR_NEW_LWP | THR_JOINABLE, 1) == -1) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("ERROR: Service_Participant::get_domain_participant_factory, ")
                       ACE_TEXT("Failed to activate the orb task.")));
            return DDS::DomainParticipantFactory::_nil();
          }
        }

        this->monitor_factory_ =
          ACE_Dynamic_Service<MonitorFactory>::instance ("OpenDDS_Monitor");
        if (this->monitor_factory_ == 0) {
          // Use the stubbed factory
          this->monitor_factory_ = new MonitorFactory;
        }
        this->monitor_ = this->monitor_factory_->create_sp_monitor(this);

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "ERROR: Service_Participant::get_domain_participant_factory");
        return DDS::DomainParticipantFactory::_nil();
      }
    }
  }

  return DDS::DomainParticipantFactory::_duplicate(dp_factory_.in());
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
      this->set_repo_ior(currentArg, DEFAULT_REPO);
      arg_shifter.consume_arg();
      got_info = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSInfo"))) != 0) {
      // Deprecated, use -DCPSInfoRepo
      this->set_repo_ior(currentArg, DEFAULT_REPO);
      arg_shifter.consume_arg();
      got_info = true;

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
      this->bitTransportPortMap_[ DEFAULT_REPO] = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_bit_transport_port = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBitTransportIPAddress"))) != 0) {
      /// No need to guard this insertion as we are still single
      /// threaded here.
      this->bitTransportIpMap_[ DEFAULT_REPO] = currentArg;
      arg_shifter.consume_arg();
      got_bit_transport_ip = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBitLookupDurationMsec"))) != 0) {
      bit_lookup_duration_msec_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_bit_lookup_duration_msec = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBit"))) != 0) {
      bit_enabled_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_bit_flag = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSTransportDebugLevel"))) != 0) {
      OpenDDS::DCPS::Transport_debug_level = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSPersistentDataDir"))) != 0) {
      this->persistent_data_dir_ = ACE_TEXT_ALWAYS_CHAR(currentArg);
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSPendingTimeout"))) != 0) {
      this->pending_timeout_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSPublisherContentFilter"))) != 0) {
      this->publisher_content_filter_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
      got_publisher_content_filter = true;

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

    } else {
      arg_shifter.ignore_arg();
    }
  }

  // Indicates sucessful parsing of the command line
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
  initial_OwnershipStrengthQosPolicy_.value = 0;

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
  initial_DataWriterQos_.ownership_strength = initial_OwnershipStrengthQosPolicy_;
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
#else
    ACE_Sched_Params params(
      ace_scheduler,
      ACE_Sched_Params::priority_min(ace_scheduler),
      ACE_SCOPE_THREAD,
      this->schedulerQuantum_);

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
void
Service_Participant::set_repo_ior(const wchar_t* ior,
                                  const RepoKey key,
                                  bool  attach_participant)
{
  set_repo_ior(ACE_Wide_To_Ascii(ior).char_rep(), key, attach_participant);
}
#endif

void
Service_Participant::set_repo_ior(const char* ior, const RepoKey key, bool attach_participant)
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Service_Participant::set_repo_ior: Repo[ %d] == %C\n"),
               key, ior));
  }

  // This is a global used for the bizzare commandline/configfile
  // processing done for this class.
  got_info = true;

  // Delare this outside the try/catch scope since we use it later.
  DCPSInfo_var repo;

  try {
    repo = InfoRepoUtils::get_repo(ior,orb_.in());

    if (CORBA::is_nil(repo.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Service_Participant::set_repo_ior: ")
                 ACE_TEXT("unable to narrow DCPSInfo (%C) for key %d. \n"),
                 ior,
                 key));
      return;
    }

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      "ERROR: Service_Participant::set_repo_ior: failed to resolve ior - ");
    return;
  }

  // If we made it this far, the IOR is valid, so store the key/IOR
  // mapping for informational purposes.
  this->keyIorMap_[ key] = ior;

  // Actually install the repository to the mappings.
  this->set_repo(repo.in(), key, attach_participant);
}

void
Service_Participant::set_repo(DCPSInfo_ptr repo, const RepoKey key, bool attach_participant)
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Service_Participant::set_repo: setting Repo[ %d]\n"),
               key));
  }

  // Any previously held reference at this key will be released, right?
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->maps_lock_);
    this->repoMap_[ key] = DCPSInfo::_duplicate(repo);
  }

  // Force a call to attach_participant() for all domains bound to
  // this repository.
  this->remap_domains(key, key, attach_participant);
}

void
Service_Participant::remap_domains(const RepoKey oldKey,
                                   const RepoKey newKey,
                                   bool attach_participant)
{
  // Search the mappings for any domains mapped to this repository.
  std::vector<DDS::DomainId_t> domainList;
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
                                     const RepoKey key,
                                     bool attach_participant)
{
  std::vector<std::pair<DCPSInfo_var, RepoId> > repoList;
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->maps_lock_);
    DomainRepoMap::const_iterator where = this->domainRepoMap_.find(domain);

    if ((where == this->domainRepoMap_.end()) || (where->second != key)) {
      // Only assign entries into the map when they change the
      // contents.
      this->domainRepoMap_[ domain] = key;

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Service_Participant::set_repo_domain: ")
                   ACE_TEXT("Domain[ %d] = Repo[ %d].\n"),
                   domain, key));
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
    if (0 != this->dp_factory_servant_) {
      // Map of domains to sets of participants.
      const DomainParticipantFactoryImpl::DPMap& participants
      = this->dp_factory_servant_->participants();

      // Extract the set of participants for the current domain.
      DomainParticipantFactoryImpl::DPMap::const_iterator
      which  = participants.find(domain);

      if (which != participants.end()) {
        // Extract the repository to attach this domain to.
        RepoMap::const_iterator location = this->repoMap_.find(key);

        if (location != this->repoMap_.end()) {
          for (DomainParticipantFactoryImpl::DPSet::const_iterator
               current  = which->second.begin();
               current != which->second.end();
               ++current) {
            try {
              // Attach each DomainParticipant in this domain to this
              // repository.
              RepoId id = current->svt_->get_id();
              repoList.push_back(std::make_pair(location->second, id));

              if (DCPS_debug_level > 0) {
                RepoIdConverter converter(id);
                ACE_DEBUG((LM_DEBUG,
                           ACE_TEXT("(%P|%t) Service_Participant::set_repo_domain: ")
                           ACE_TEXT("participant %C attached to Repo[ %d].\n"),
                           std::string(converter).c_str(),
                           key));
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
      RepoIdConverter converter(repoList[ index].second);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Service_Participant::set_repo_domain: ")
                 ACE_TEXT("(%d of %d) attaching domain %d participant %C to Repo[ %d].\n"),
                 (1+index), repoList.size(), domain,
                 std::string(converter).c_str(),
                 key));
    }

    if (attach_participant)
    {
      repoList[ index].first->attach_participant(domain, repoList[ index].second);
    }
  }
}

void
Service_Participant::repository_lost(const RepoKey key)
{
  // Find the lost repository.
  RepoMap::iterator initialLocation = this->repoMap_.find(key);
  RepoMap::iterator current         = initialLocation;

  if (current == this->repoMap_.end()) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: Service_Participant::repository_lost: ")
               ACE_TEXT("lost repository %d was not present, ")
               ACE_TEXT("finding another anyway.\n"),
               key));

  } else {
    // Start with the repository *after* the lost one.
    ++current;
  }

  // Calculate the bounding end time for attempts.
  ACE_Time_Value recoveryFailedTime
  = ACE_OS::gettimeofday()
    + ACE_Time_Value(this->federation_recovery_duration(), 0);

  // Backoff delay.
  int backoff = this->federation_initial_backoff_seconds();

  // Keep trying until the total recovery time specified is exceeded.
  while (recoveryFailedTime > ACE_OS::gettimeofday()) {

    // Wrap to the beginning at the end of the list.
    if (current == this->repoMap_.end()) {
      // Continue to traverse the list.
      current = this->repoMap_.begin();
    }

    // Handle reaching the lost repository by waiting before trying
    // again.
    if (current == initialLocation) {
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Service_Participant::repository_lost: ")
                   ACE_TEXT("waiting %d seconds to traverse the ")
                   ACE_TEXT("repository list another time ")
                   ACE_TEXT("for lost key %d.\n"),
                   backoff,
                   key));
      }

      // Wait to traverse the list and try again.
      ACE_OS::sleep(backoff);

      // Exponentially backoff delay.
      backoff *= this->federation_backoff_multiplier();

      // Don't increment current to allow us to reattach to the
      // original repository if it is restarted.
    }

    try {
      // Check the availability of the current repository.
      if (false == current->second->_is_a("Not_An_IDL_Type")) {

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::repository_lost: ")
                     ACE_TEXT("replacing repository %d with %d.\n"),
                     key,
                     current->first));
        }

        // If we reach here, the validate_connection() call succeeded
        // and the repository is reachable.
        this->remap_domains(key, current->first);

        // Now we are done.  This is the only non-failure exit from
        // this method.
        return;

      } else if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Service_Participant::repository_lost: ")
                   ACE_TEXT("repository %d reference to %d unexpected _is_a return.\n"),
                   key,
                   current->first));
      }

    } catch (const CORBA::Exception&) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Service_Participant::repository_lost: ")
                 ACE_TEXT("repository %d was not available to replace %d, ")
                 ACE_TEXT("looking for another.\n"),
                 current->first,
                 key));
    }

    // Move to the next candidate repository.
    ++current;
  }

  // If we reach here, we have exceeded the total recovery time
  // specified.
  ACE_ASSERT(recoveryFailedTime == ACE_Time_Value::zero);
}

const Service_Participant::KeyIorMap&
Service_Participant::keyIorMap() const
{
  return this->keyIorMap_;
}

DCPSInfo_ptr
Service_Participant::get_repository(const DDS::DomainId_t domain)
{
  RepoKey repo = DEFAULT_REPO;
  DomainRepoMap::const_iterator where = this->domainRepoMap_.find(domain);

  if (where != this->domainRepoMap_.end()) {
    repo = where->second;
  }

  RepoMap::const_iterator location = this->repoMap_.find(repo);

  if (location == this->repoMap_.end()) {
    if (repo == DEFAULT_REPO) {
      // Set the default repository IOR if it hasn't already happened
      // by this point.  This is why this can't be const.
      this->set_repo_ior(DEFAULT_REPO_IOR, DEFAULT_REPO);
      location = this->repoMap_.find(repo);

      if (location == this->repoMap_.end()) {
        // The default IOR was invalid.
        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::get_repository: ")
                     ACE_TEXT("failed attempt to set default IOR for domain %d.\n"),
                     domain));
        }

        return OpenDDS::DCPS::DCPSInfo::_nil();

      } else {
        // Found the default!
        if (DCPS_debug_level > 4) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) Service_Participant::get_repository: ")
                     ACE_TEXT("returning default repository for domain %d.\n"),
                     domain));
        }

        return OpenDDS::DCPS::DCPSInfo::_duplicate(location->second);
      }

    } else {
      // Non-default repositories _must_ be loaded by application.
      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Service_Participant::get_repository: ")
                   ACE_TEXT("repository for domain %d was not set.\n"),
                   domain));
      }

      return OpenDDS::DCPS::DCPSInfo::_nil();
    }
  }

  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Service_Participant::get_repository: ")
               ACE_TEXT("returning repository for domain %d.\n"),
               domain));
  }

  return OpenDDS::DCPS::DCPSInfo::_duplicate(location->second);
}

int
Service_Participant::bit_transport_port(RepoKey repo) const
{
  RepoTransportPortMap::const_iterator where = this->bitTransportPortMap_.find(repo);

  if (where == this->bitTransportPortMap_.end()) {
    return -1;
  }

  return where->second;
}

void
Service_Participant::bit_transport_port(int port, RepoKey repo)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->maps_lock_);
  this->bitTransportPortMap_[ repo] = port;
  got_bit_transport_port = true;
}

int
Service_Participant::init_bit_transport_impl(DDS::DomainId_t domain)
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  RepoKey repo = DEFAULT_REPO;
  DomainRepoMap::const_iterator where = this->domainRepoMap_.find(domain);

  if (where != this->domainRepoMap_.end()) {
    repo = where->second;
  }

  // Assign BIT transport key values starting from BIT_ALL_TRAFFIC as a base.
  OpenDDS::DCPS::TransportIdType transportKey = BIT_ALL_TRAFFIC + domain;

  if (false == TheTransportFactory->obtain(transportKey).is_nil()) {
    // The transport for this repo has already been created/configured.
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Domain[ %d].transport already loaded.\n"),
                 domain));
    }

    return 0;
  }

  this->bitTransportMap_[ domain]
  = TheTransportFactory->create_transport_impl(transportKey,
                                               ACE_TEXT("SimpleTcp"),
                                               DONT_AUTO_CONFIG);

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Domain[ %d].transport == %x local_address=%s:%d \n"),
               domain, this->bitTransportMap_[ domain].in(), bitTransportIpMap_[ repo].c_str(),
               bitTransportPortMap_[ repo]));
  }

  TransportConfiguration_rch config
  = TheTransportFactory->get_or_create_configuration(transportKey, ACE_TEXT("SimpleTcp"));

  SimpleTcpConfiguration* tcp_config
  = static_cast <SimpleTcpConfiguration*>(config.in());

  tcp_config->datalink_release_delay_ = 0;

  if (0 == this->bitTransportIpMap_[ repo].length()) {
    tcp_config->local_address_.set_port_number(this->bitTransportPortMap_[ repo]);

  } else {
    tcp_config->local_address_
    = ACE_INET_Addr(
        this->bitTransportPortMap_[ repo],
        this->bitTransportIpMap_[ repo].c_str());
  }

  std::stringstream out;
  out << this->bitTransportPortMap_[ repo];

  tcp_config->local_address_str_ = this->bitTransportIpMap_[ repo];
  tcp_config->local_address_str_ += ACE_TEXT(":");
  tcp_config->local_address_str_ += ACE_TEXT_CHAR_TO_TCHAR(out.str().c_str());

  if (this->bitTransportMap_[ domain]->configure(config.in()) != 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Service_Participant::init_bit_transport_impl: ")
               ACE_TEXT("Failed to configure transport for domain %d.\n"),
               domain));
    return -1;

  } else {
    return 0;
  }

#else
  ACE_UNUSED_ARG(domain);
  return -1;
#endif // DDS_HAS_MINIMUM_BIT
}

TransportImpl_rch
Service_Participant::bit_transport_impl(DDS::DomainId_t domain)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->maps_lock_, TransportImpl_rch());

  if (this->bitTransportMap_[ domain].is_nil()) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Initializing BIT transport for domain %d\n"),
                 domain));
    }

    init_bit_transport_impl(domain);
  }

  return this->bitTransportMap_[ domain];
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
  }

  status = this->load_common_configuration();

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_common_configuration () returned %d\n"),
                      status),
                     -1);
  }

  status = this->load_domain_configuration();

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_domain_configuration () returned %d\n"),
                      status),
                     -1);
  }

  status = this->load_repo_configuration();

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_repo_configuration () returned %d\n"),
                      status),
                     -1);
  }

  status = TheTransportFactory->load_transport_configuration(this->cf_);

  if (status != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_configuration ")
                      ACE_TEXT("load_transport_configuration () returned %d\n"),
                      status),
                     -1);
  }

  return 0;
}

int
Service_Participant::load_common_configuration()
{
  const ACE_Configuration_Section_Key &root = this->cf_.root_section();
  ACE_Configuration_Section_Key sect;

  if (this->cf_.open_section(root, COMMON_SECTION_NAME, 0, sect) != 0) {
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
    if (got_debug_level) {
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSDebugLevel value from command option (overrides value if it's in config file).\n")));
    } else {
      GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("DCPSDebugLevel"), DCPS_debug_level, int)
    }

    if (got_info) {
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSInfoRepo value from command option (overrides value if it's in config file).\n")));
    } else {
      ACE_TString value;
      GET_CONFIG_STRING_VALUE(this->cf_, sect, ACE_TEXT("DCPSInfoRepo"), value)
      this->set_repo_ior(value.c_str(), DEFAULT_REPO);
    }

    if (got_chunks) {
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSChunks value from command option (overrides value if it's in config file).\n")));
    } else {
      GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("DCPSChunks"), this->n_chunks_, size_t)
    }

    if (got_chunk_association_multiplier) {
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSChunkAssociationMutltiplier value from command option (overrides value if it's in config file).\n")));
    } else {
      GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("DCPSChunkAssociationMutltiplier"), this->association_chunk_multiplier_, size_t)
    }

    if (got_bit_transport_port) {
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSBitTransportPort value from command option (overrides value if it's in config file).\n")));
    } else {
      GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("DCPSBitTransportPort"), this->bitTransportPortMap_[ DEFAULT_REPO], int)
    }

    if (got_bit_transport_ip) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSBitTransportIPAddress value from command option (overrides value if it's in config file).\n")));
    } else {
      GET_CONFIG_STRING_VALUE(this->cf_, sect, ACE_TEXT("DCPSBitTransportIPAddress"), this->bitTransportIpMap_[ DEFAULT_REPO])
    }

    if (got_liveliness_factor) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSLivelinessFactor value from command option (overrides value if it's in config file).\n")));
    } else {
      GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("DCPSLivelinessFactor"), this->liveliness_factor_, int)
    }

    if (got_bit_lookup_duration_msec) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSBitLookupDurationMsec value from command option (overrides value if it's in config file).\n")));
    } else {
      GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("DCPSBitLookupDurationMsec"), this->bit_lookup_duration_msec_, int)
    }

    if (got_bit_flag) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSBit value from command option (overrides value if it's in config file).\n")));
    } else {
      GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("DCPSBit"), this->bit_enabled_, int)
    }

    if (got_publisher_content_filter) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) NOTICE: using DCPSPublisherContentFilter ")
                 ACE_TEXT("value from command option (overrides value if it's ")
                 ACE_TEXT("in config file).\n")));
    } else {
      GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("DCPSPublisherContentFilter"),
        this->publisher_content_filter_, bool)
    }

    // These are not handled on the command line.
    GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("FederationRecoveryDuration"), this->federation_recovery_duration_, int)
    GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("FederationInitialBackoffSeconds"), this->federation_initial_backoff_seconds_, int)
    GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("FederationBackoffMultiplier"), this->federation_backoff_multiplier_, int)
    GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("FederationLivelinessDuration"), this->federation_liveliness_, int)

    //
    // Establish the scheduler if specified.
    //
    GET_CONFIG_STRING_VALUE(this->cf_, sect, ACE_TEXT("scheduler"), this->schedulerString_)

    suseconds_t usec(0);

    GET_CONFIG_VALUE(this->cf_, sect, ACE_TEXT("scheduler_slice"), usec, suseconds_t)

    if (usec > 0)
      this->schedulerQuantum_.usec(usec);
  }

  return 0;
}

int
Service_Participant::load_domain_configuration()
{
  const ACE_Configuration_Section_Key &root = this->cf_.root_section();
  ACE_Configuration_Section_Key domainKey;

  if (this->cf_.open_section(root, DOMAIN_SECTION_NAME, 0, domainKey) != 0) {
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
    ACE_TString sectionName;

    for (int index = 0;
         (0 == this->cf_.enumerate_sections(domainKey, index, sectionName));
         ++index) {
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Examining section: %s\n"), sectionName.c_str()));
      }

      ACE_Configuration_Section_Key sectionKey;

      if (0 != this->cf_.open_section(domainKey, sectionName.c_str(), 0, sectionKey)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_domain_configuration ")
                   ACE_TEXT("Unable to open [%s] section.\n"),
                   sectionName.c_str()));
        continue;
      }

      ACE_TString domainIdString;

      if (0 != this->cf_.get_string_value(sectionKey, ACE_TEXT("DomainId"), domainIdString)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_domain_configuration ")
                   ACE_TEXT("Unable to obtain value for DomainId in [%s] section\n"),
                   sectionName.c_str()));
        continue;
      }

      /// @TODO: Check this conversion.
      DDS::DomainId_t domainId = ACE_OS::atoi(domainIdString.c_str());

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %s: DomainId == %d\n"),
                   sectionName.c_str(), domainId));
      }

      ACE_TString keyString;

      if (0 != this->cf_.get_string_value(sectionKey, ACE_TEXT("DomainRepoKey"), keyString)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_domain_configuration ")
                   ACE_TEXT("Unable to obtain value for DomainRepoKey in [%s] section\n"),
                   sectionName.c_str()));
        continue;

      }

      RepoKey repoKey = DEFAULT_REPO;

      if (keyString != ACE_TEXT("DEFAULT_REPO")) {
        /// @TODO: Check this conversion.
        repoKey = ACE_OS::atoi(keyString.c_str());
      }

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %s: DomainRepoKey == %d\n"),
                   sectionName.c_str(), repoKey));
      }

      this->set_repo_domain(domainId, repoKey);
    }
  }

  return 0;
}

int
Service_Participant::load_repo_configuration()
{
  const ACE_Configuration_Section_Key &root = this->cf_.root_section();
  ACE_Configuration_Section_Key domainKey;

  if (this->cf_.open_section(root, REPO_SECTION_NAME, 0, domainKey) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any domain (sub)section. The code default configuration will be used.
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: Service_Participant::load_repo_configuration ")
                 ACE_TEXT("failed to open [%s] section.\n"),
                 REPO_SECTION_NAME));
    }

    return 0;

  } else {
    ACE_TString sectionName;

    for (int index = 0;
         (0 == this->cf_.enumerate_sections(domainKey, index, sectionName));
         ++index) {
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Examining section: %s\n"), sectionName.c_str()));
      }

      ACE_Configuration_Section_Key sectionKey;

      if (0 != this->cf_.open_section(domainKey, sectionName.c_str(), 0, sectionKey)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_repo_configuration ")
                   ACE_TEXT("Unable to open [%s] section.\n"),
                   sectionName.c_str()));
        continue;
      }

      ACE_TString keyString;

      if (0 != this->cf_.get_string_value(sectionKey, ACE_TEXT("RepositoryKey"), keyString)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_repo_configuration ")
                   ACE_TEXT("Unable to obtain value for RepositoryKey in [%s] section\n"),
                   sectionName.c_str()));
        continue;
      }

      /// @TODO: Check this conversion.
      RepoKey repoKey = ACE_OS::atoi(keyString.c_str());

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %s: RepositoryKey == %d\n"),
                   sectionName.c_str(), repoKey));
      }

      ACE_TString repoIor;

      if (0 != this->cf_.get_string_value(sectionKey, ACE_TEXT("RepositoryIor"), repoIor)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Service_Participant::load_repo_configuration ")
                   ACE_TEXT("Unable to obtain value for RepositoryIor in [%s] section\n"),
                   sectionName.c_str()));
        continue;

      }

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %s: RepositoryIor == %s\n"),
                   sectionName.c_str(), repoIor.c_str()));
      }

      this->set_repo_ior(repoIor.c_str(), repoKey);

      ACE_TString bitIp;
      this->cf_.get_string_value(sectionKey, ACE_TEXT("DCPSBitTransportIPAddress"), bitIp);

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %s: DCPSBitTransportIPAddress == %s\n"),
                   sectionName.c_str(), bitIp.c_str()));
      }

      this->bitTransportIpMap_[ repoKey]   = bitIp;

      ACE_TString portString;
      this->cf_.get_string_value(sectionKey, ACE_TEXT("DCPSBitTransportPort"), portString);

      int bitPort = ACE_OS::atoi(portString.c_str());

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %s: DCPSBitTransportPort == %d\n"),
                   sectionName.c_str(), bitPort));
      }

      this->bitTransportPortMap_[ repoKey] = bitPort;
    }
  }

  return 0;
}

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

      if (this->transient_data_cache_.get() == 0) {
        ACE_auto_ptr_reset(this->transient_data_cache_,
                           new DataDurabilityCache(kind));
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
        if (this->persistent_data_cache_.get() == 0) {
          ACE_auto_ptr_reset(this->persistent_data_cache_,
                             new DataDurabilityCache(kind,
                                                     this->persistent_data_dir_));
        }

      } catch (const std::exception& ex) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: Service_Participant::get_data_durability_cache ")
                     ACE_TEXT("failed to create PERSISTENT cache, falling back on ")
                     ACE_TEXT("TRANSIENT behavior: %C\n"), ex.what()));
        }

        ACE_auto_ptr_reset(this->persistent_data_cache_,
                           new DataDurabilityCache(DDS::TRANSIENT_DURABILITY_QOS));
      }
    }

    cache = this->persistent_data_cache_.get();
  }

  return cache;
}

} // namespace DCPS
} // namespace OpenDDS

// gcc on AIX needs explicit instantiation of the singleton templates
#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION) || (defined (__GNUC__) && defined (_AIX))

template class TAO_Singleton<Service_Participant, TAO_SYNCH_MUTEX>;

#elif defined (ACE_HAS_TEMPLATENSTANTIATION_PRAGMA)

#pragma instantiate TAO_Singleton<Service_Participant, TAO_SYNCH_MUTEX>

#endif /*ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
