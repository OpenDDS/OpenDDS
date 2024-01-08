/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SERVICE_PARTICIPANT_H
#define OPENDDS_DCPS_SERVICE_PARTICIPANT_H

#include "Definitions.h"
#include "MonitorFactory.h"
#include "Discovery.h"
#include "PoolAllocator.h"
#include "DomainParticipantFactoryImpl.h"
#include "ConfigUtils.h"
#include "unique_ptr.h"
#include "ReactorTask.h"
#include "JobQueue.h"
#include "NetworkConfigMonitor.h"
#include "NetworkConfigModifier.h"
#include "Recorder.h"
#include "Replayer.h"
#include "TimeSource.h"
#include "AtomicBool.h"
#include "ConfigStoreImpl.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#include <ace/Task.h>
#include <ace/Time_Value.h>
#include <ace/ARGV.h>
#include <ace/Barrier.h>

#include <memory>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const char OPENDDS_COMMON_BIT_AUTOPURGE_DISPOSED_SAMPLES_DELAY[] =
  "OPENDDS_COMMON_BIT_AUTOPURGE_DISPOSED_SAMPLES_DELAY";
const DDS::Duration_t OPENDDS_COMMON_BIT_AUTOPURGE_DISPOSED_SAMPLES_DELAY_default =
  { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

const char OPENDDS_COMMON_BIT_AUTOPURGE_NOWRITER_SAMPLES_DELAY[] =
  "OPENDDS_COMMON_BIT_AUTOPURGE_NOWRITER_SAMPLES_DELAY";
const DDS::Duration_t OPENDDS_COMMON_BIT_AUTOPURGE_NOWRITER_SAMPLES_DELAY_default =
  { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

const char OPENDDS_COMMON_DCPSRTI_SERIALIZATION[] = "OPENDDS_COMMON_DCPSRTI_SERIALIZATION";

const char OPENDDS_COMMON_DCPS_BIDIR_GIOP[] = "OPENDDS_COMMON_DCPS_BIDIR_GIOP";
const bool OPENDDS_COMMON_DCPS_BIDIR_GIOP_default = true;

const char OPENDDS_COMMON_DCPS_BIT[] = "OPENDDS_COMMON_DCPS_BIT";
#ifdef DDS_HAS_MINIMUM_BIT
const bool OPENDDS_COMMON_DCPS_BIT_default = false;
#else
const bool OPENDDS_COMMON_DCPS_BIT_default = true;
#endif

const char OPENDDS_COMMON_DCPS_BIT_LOOKUP_DURATION_MSEC[] = "OPENDDS_COMMON_DCPS_BIT_LOOKUP_DURATION_MSEC";
const int OPENDDS_COMMON_DCPS_BIT_LOOKUP_DURATION_MSEC_default = 2000;

const char OPENDDS_COMMON_DCPS_BIT_TRANSPORT_IP_ADDRESS[] = "OPENDDS_COMMON_DCPS_BIT_TRANSPORT_IP_ADDRESS";
const String OPENDDS_COMMON_DCPS_BIT_TRANSPORT_IP_ADDRESS_default = "";

const char OPENDDS_COMMON_DCPS_BIT_TRANSPORT_PORT[] = "OPENDDS_COMMON_DCPS_BIT_TRANSPORT_PORT";
const int OPENDDS_COMMON_DCPS_BIT_TRANSPORT_PORT_default = 0;

const char OPENDDS_COMMON_DCPS_CHUNKS[] = "OPENDDS_COMMON_DCPS_CHUNKS";
const size_t OPENDDS_COMMON_DCPS_CHUNKS_default = 20;

const char OPENDDS_COMMON_DCPS_CHUNK_ASSOCIATION_MULTIPLIER[] = "OPENDDS_COMMON_DCPS_CHUNK_ASSOCIATION_MULTIPLIER";
const char OPENDDS_COMMON_DCPS_CHUNK_ASSOCIATION_MUTLTIPLIER[] = "OPENDDS_COMMON_DCPS_CHUNK_ASSOCIATION_MUTLTIPLIER";
const size_t OPENDDS_COMMON_DCPS_CHUNK_ASSOCIATION_MULTIPLIER_default = 10;

const char OPENDDS_COMMON_DCPS_CONFIG_FILE[] = "OPENDDS_COMMON_DCPS_CONFIG_FILE";
const String OPENDDS_COMMON_DCPS_CONFIG_FILE_default = "";

const char OPENDDS_COMMON_DCPS_DEBUG_LEVEL[] = "OPENDDS_COMMON_DCPS_DEBUG_LEVEL";

const char OPENDDS_COMMON_DCPS_DEFAULT_ADDRESS[] = "OPENDDS_COMMON_DCPS_DEFAULT_ADDRESS";
const NetworkAddress OPENDDS_COMMON_DCPS_DEFAULT_ADDRESS_default = NetworkAddress("0.0.0.0:0");

const char OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY[] = "OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY";
#ifdef DDS_DEFAULT_DISCOVERY_METHOD
const Discovery::RepoKey OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY_default = DDS_DEFAULT_DISCOVERY_METHOD;
#else
# ifdef OPENDDS_SAFETY_PROFILE
const Discovery::RepoKey OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY_default = Discovery::DEFAULT_RTPS;
# else
const Discovery::RepoKey OPENDDS_COMMON_DCPS_DEFAULT_DISCOVERY_default = Discovery::DEFAULT_REPO;
# endif
#endif

const char OPENDDS_COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG[] = "OPENDDS_COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG";
const String OPENDDS_COMMON_DCPS_GLOBAL_TRANSPORT_CONFIG_default = "";

const char OPENDDS_COMMON_DCPS_INFO_REPO[] = "OPENDDS_COMMON_DCPS_INFO_REPO";

const char OPENDDS_COMMON_DCPS_LIVELINESS_FACTOR[] = "OPENDDS_COMMON_DCPS_LIVELINESS_FACTOR";
const int OPENDDS_COMMON_DCPS_LIVELINESS_FACTOR_default = 80;

const char OPENDDS_COMMON_DCPS_LOG_LEVEL[] = "OPENDDS_COMMON_DCPS_LOG_LEVEL";

const char OPENDDS_COMMON_DCPS_MONITOR[] = "OPENDDS_COMMON_DCPS_MONITOR";
const bool OPENDDS_COMMON_DCPS_MONITOR_default = false;

const char OPENDDS_COMMON_DCPS_PENDING_TIMEOUT[] = "OPENDDS_COMMON_DCPS_PENDING_TIMEOUT";
// Can't use TimeDuration::zero_value since initialization order is undefined.
const TimeDuration OPENDDS_COMMON_DCPS_PENDING_TIMEOUT_default(0, 0);

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
const char OPENDDS_COMMON_DCPS_PERSISTENT_DATA_DIR[] = "OPENDDS_COMMON_DCPS_PERSISTENT_DATA_DIR";
const String OPENDDS_COMMON_DCPS_PERSISTENT_DATA_DIR_default = "OpenDDS-durable-data-dir";
#endif

const char OPENDDS_COMMON_DCPS_PUBLISHER_CONTENT_FILTER[] = "OPENDDS_COMMON_DCPS_PUBLISHER_CONTENT_FILTER";
const bool OPENDDS_COMMON_DCPS_PUBLISHER_CONTENT_FILTER_default = true;

const char OPENDDS_COMMON_DCPS_THREAD_STATUS_INTERVAL[] = "OPENDDS_COMMON_DCPS_THREAD_STATUS_INTERVAL";

const char OPENDDS_COMMON_DCPS_TRANSPORT_DEBUG_LEVEL[] = "OPENDDS_COMMON_DCPS_TRANSPORT_DEBUG_LEVEL";

const char OPENDDS_COMMON_DCPS_TYPE_OBJECT_ENCODING[] = "OPENDDS_COMMON_DCPS_TYPE_OBJECT_ENCODING";
const String OPENDDS_COMMON_DCPS_TYPE_OBJECT_ENCODING_default = "Normal";

const char OPENDDS_COMMON_FEDERATION_BACKOFF_MULTIPLIER[] = "OPENDDS_COMMON_FEDERATION_BACKOFF_MULTIPLIER";
const int OPENDDS_COMMON_FEDERATION_BACKOFF_MULTIPLIER_default = 2;

const char OPENDDS_COMMON_FEDERATION_INITIAL_BACKOFF_SECONDS[] = "OPENDDS_COMMON_FEDERATION_INITIAL_BACKOFF_SECONDS";
const int OPENDDS_COMMON_FEDERATION_INITIAL_BACKOFF_SECONDS_default = 1;

const char OPENDDS_COMMON_FEDERATION_LIVELINESS_DURATION[] = "OPENDDS_COMMON_FEDERATION_LIVELINESS_DURATION";
const int OPENDDS_COMMON_FEDERATION_LIVELINESS_DURATION_default = 60;

const char OPENDDS_COMMON_FEDERATION_RECOVERY_DURATION[] = "OPENDDS_COMMON_FEDERATION_RECOVERY_DURATION";
const int OPENDDS_COMMON_FEDERATION_RECOVERY_DURATION_default = 900;

const char OPENDDS_COMMON_ORB_LOG_FILE[] = "OPENDDS_COMMON_ORB_LOG_FILE";

const char OPENDDS_COMMON_ORB_VERBOSE_LOGGING[] = "OPENDDS_COMMON_ORB_VERBOSE_LOGGING";

const char OPENDDS_COMMON_PRINTER_VALUE_WRITER_INDENT[] = "OPENDDS_COMMON_PRINTER_VALUE_WRITER_INDENT";
const unsigned int OPENDDS_COMMON_PRINTER_VALUE_WRITER_INDENT_default = 4;

const char OPENDDS_COMMON_SCHEDULER[] = "OPENDDS_COMMON_SCHEDULER";
const String OPENDDS_COMMON_SCHEDULER_default = "";

const char OPENDDS_COMMON_SCHEDULER_SLICE[] = "OPENDDS_COMMON_SCHEDULER_SLICE";
const int OPENDDS_COMMON_SCHEDULER_SLICE_default = 0;

const char OPENDDS_DEFAULT_CONFIGURATION_FILE[] = "OPENDDS_DEFAULT_CONFIGURATION_FILE";
const String OPENDDS_DEFAULT_CONFIGURATION_FILE_default = "";

#ifdef OPENDDS_SECURITY
const char OPENDDS_COMMON_DCPS_SECURITY[] = "OPENDDS_COMMON_DCPS_SECURITY";
const bool OPENDDS_COMMON_DCPS_SECURITY_default = false;

const char OPENDDS_COMMON_DCPS_SECURITY_DEBUG[] = "OPENDDS_COMMON_DCPS_SECURITY_DEBUG";

const char OPENDDS_COMMON_DCPS_SECURITY_DEBUG_LEVEL[] = "OPENDDS_COMMON_DCPS_SECURITY_DEBUG_LEVEL";

const char OPENDDS_COMMON_DCPS_SECURITY_FAKE_ENCRYPTION[] = "OPENDDS_COMMON_DCPS_SECURITY_FAKE_ENCRYPTION";
#endif

#if OPENDDS_POOL_ALLOCATOR
const char OPENDDS_COMMON_POOL_GRANULARITY[] = "OPENDDS_COMMON_POOL_GRANULARITY";
const size_t OPENDDS_COMMON_POOL_GRANULARITY_default = 8;

const char OPENDDS_COMMON_POOL_SIZE[] = "OPENDDS_COMMON_POOL_SIZE";
const size_t OPENDDS_COMMON_POOL_SIZE_default = 1024 * 1024 * 16;
#endif

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
class DataDurabilityCache;
#endif
class ThreadStatusManager;

const char DEFAULT_ORB_NAME[] = "OpenDDS_DCPS";

class ShutdownListener : public virtual RcObject {
public:
  virtual ~ShutdownListener() {}
  virtual void notify_shutdown() = 0;
};

/**
 * @class Service_Participant
 *
 * @brief Service entrypoint.
 *
 * This class is a singleton that allows DDS client applications to
 * configure OpenDDS.
 *
 * @note This class may read a configuration file that will
 *       configure Transports as well as DCPS (e.g. number of ORB
 *       threads).
 */
class OpenDDS_Dcps_Export Service_Participant {
public:
  /// Domain value for the default repository IOR.
  enum { ANY_DOMAIN = -1 };

  Service_Participant();

  ~Service_Participant();

  /// Return a singleton instance of this class.
  static Service_Participant* instance();

  const TimeSource& time_source() const;

  /// Get the common timer interface.
  /// Intended for use by OpenDDS internals only.
  ACE_Reactor_Timer_Interface* timer();

  ACE_Reactor* reactor();

  ACE_thread_t reactor_owner() const;

  ReactorInterceptor_rch interceptor() const;

  JobQueue_rch job_queue() const;

  void set_shutdown_listener(RcHandle<ShutdownListener> listener);

  /**
   * Initialize the DDS client environment and get the
   * @c DomainParticipantFactory.
   *
   * This method consumes @c -DCPS* and -ORB* options and their arguments.
   */
  DDS::DomainParticipantFactory_ptr get_domain_participant_factory(
    int &argc = zero_argc,
    ACE_TCHAR *argv[] = 0);

#ifdef ACE_USES_WCHAR
  DDS::DomainParticipantFactory_ptr
  get_domain_participant_factory(int &argc, char *argv[]);
#endif

  /**
   * Stop being a participant in the service.
   *
   * @note
   * All Domain Participants have to be deleted before calling or
   * DDS::RETCODE_PRECONDITION_NOT_MET is returned.
   *
   * If the Service Participant has already been shutdown then
   * DDS::RETCODE_ALREADY_DELETED will be returned.
   */
  DDS::ReturnCode_t shutdown();

  /// Accessor for if the participant has been shutdown
  bool is_shut_down() const;

  /// Accessor of the Discovery object for a given domain.
  Discovery_rch get_discovery(const DDS::DomainId_t domain);

  /** Accessors of the qos policy initial values. **/
  const DDS::UserDataQosPolicy& initial_UserDataQosPolicy() const;
  const DDS::TopicDataQosPolicy& initial_TopicDataQosPolicy() const;
  const DDS::GroupDataQosPolicy& initial_GroupDataQosPolicy() const;
  const DDS::TransportPriorityQosPolicy& initial_TransportPriorityQosPolicy() const;
  const DDS::LifespanQosPolicy& initial_LifespanQosPolicy() const;
  const DDS::DurabilityQosPolicy& initial_DurabilityQosPolicy() const;
  const DDS::DurabilityServiceQosPolicy& initial_DurabilityServiceQosPolicy() const;
  const DDS::PresentationQosPolicy& initial_PresentationQosPolicy() const;
  const DDS::DeadlineQosPolicy& initial_DeadlineQosPolicy() const;
  const DDS::LatencyBudgetQosPolicy& initial_LatencyBudgetQosPolicy() const;
  const DDS::OwnershipQosPolicy& initial_OwnershipQosPolicy() const;
  const DDS::OwnershipStrengthQosPolicy& initial_OwnershipStrengthQosPolicy() const;
  const DDS::LivelinessQosPolicy& initial_LivelinessQosPolicy() const;
  const DDS::TimeBasedFilterQosPolicy& initial_TimeBasedFilterQosPolicy() const;
  const DDS::PartitionQosPolicy& initial_PartitionQosPolicy() const;
  const DDS::ReliabilityQosPolicy& initial_ReliabilityQosPolicy() const;
  const DDS::DestinationOrderQosPolicy& initial_DestinationOrderQosPolicy() const;
  const DDS::HistoryQosPolicy& initial_HistoryQosPolicy() const;
  const DDS::ResourceLimitsQosPolicy& initial_ResourceLimitsQosPolicy() const;
  const DDS::EntityFactoryQosPolicy& initial_EntityFactoryQosPolicy() const;
  const DDS::WriterDataLifecycleQosPolicy& initial_WriterDataLifecycleQosPolicy() const;
  const DDS::ReaderDataLifecycleQosPolicy& initial_ReaderDataLifecycleQosPolicy() const;
  const DDS::PropertyQosPolicy& initial_PropertyQosPolicy() const;
  const DDS::DataRepresentationQosPolicy& initial_DataRepresentationQosPolicy() const;

  const DDS::DomainParticipantFactoryQos& initial_DomainParticipantFactoryQos() const;
  const DDS::DomainParticipantQos& initial_DomainParticipantQos() const;
  const DDS::TopicQos& initial_TopicQos() const;
  const DDS::DataWriterQos& initial_DataWriterQos() const;
  const DDS::PublisherQos& initial_PublisherQos() const;
  const DDS::DataReaderQos& initial_DataReaderQos() const;
  const DDS::SubscriberQos& initial_SubscriberQos() const;
  const DDS::TypeConsistencyEnforcementQosPolicy& initial_TypeConsistencyEnforcementQosPolicy() const;

  /**
   * This accessor is to provide the configurable number of chunks
   * that a @c DataWriter's cached allocator need to allocate when
   * the resource limits are infinite.  Has a default, can be set
   * by the @c -DCPSChunks option, or by @c n_chunks() setter.
   */
  size_t n_chunks() const;

  /// Set the value returned by @c n_chunks() accessor.
  /**
   * @see Accessor description.
   */
  void n_chunks(size_t chunks);

  /// This accessor is to provide the multiplier for allocators
  /// that have resources used on a per association basis.
  /// Has a default, can be set by the
  /// @c -DCPSChunkAssociationMultiplier
  /// option, or by @c n_association_chunk_multiplier() setter.
  size_t association_chunk_multiplier() const;

  /// Set the value returned by
  /// @c n_association_chunk_multiplier() accessor.
  /**
   * See accessor description.
   */
  void association_chunk_multiplier(size_t multiplier);

  /// Set the Liveliness propagation delay factor.
  /// @param factor % of lease period before sending a liveliness
  ///               message.
  void liveliness_factor(int factor);

  /// Accessor of the Liveliness propagation delay factor.
  /// @return % of lease period before sending a liveliness
  ///         message.
  int liveliness_factor() const;

  ///
  void add_discovery(Discovery_rch discovery);

  bool set_repo_ior(const char* ior,
                    Discovery::RepoKey key = Discovery::DEFAULT_REPO,
                    bool attach_participant = true,
                    bool overwrite = true);

#ifdef DDS_HAS_WCHAR
  /// Convenience overload for wchar_t
  bool set_repo_ior(const wchar_t* ior,
                    Discovery::RepoKey key = Discovery::DEFAULT_REPO,
                    bool attach_participant = true,
                    bool overwrite = true);
#endif

  bool use_bidir_giop() const;

  /// Rebind a domain from one repository to another.
  void remap_domains(Discovery::RepoKey oldKey,
                     Discovery::RepoKey newKey,
                     bool attach_participant = true);

  /// Bind DCPSInfoRepo IORs to domains.
  void set_repo_domain(const DDS::DomainId_t domain,
                       Discovery::RepoKey repo,
                       bool attach_participant = true);

  void set_default_discovery(const Discovery::RepoKey& defaultDiscovery);
  Discovery::RepoKey get_default_discovery();



  /// Convert domainId to repository key.
  Discovery::RepoKey domain_to_repo(const DDS::DomainId_t domain) const;

  /// Failover to a new repository.
  void repository_lost(Discovery::RepoKey key);

  /// Accessors for FederationRecoveryDuration in seconds.
  //@{
  void federation_recovery_duration(int);
  int federation_recovery_duration() const;
  //@}

  /// Accessors for FederationInitialBackoffSeconds.
  //@{
  void federation_initial_backoff_seconds(int);
  int federation_initial_backoff_seconds() const;
  //@}

  /// Accessors for FederationBackoffMultiplier.
  //@{
  void federation_backoff_multiplier(int);
  int federation_backoff_multiplier() const;
  //@}

  /// Accessors for FederationLivelinessDuration.
  //@{
  void federation_liveliness(int);
  int federation_liveliness() const;
  //@}

  /// Accessors for scheduling policy value.
  //@{
  void scheduler(long);
  long scheduler() const;
  //@}

  /// Accessors for PublisherContentFilter.
  //@{
  void publisher_content_filter(bool);
  bool publisher_content_filter() const;
  //@}

  /// Accessors for pending data timeout.
  //@{
  TimeDuration pending_timeout() const;
  void pending_timeout(const TimeDuration& value);
  //@}

  /// Get a new pending timeout deadline
  MonotonicTimePoint new_pending_timeout_deadline() const;

  /// Accessors for priority extremums for the current scheduler.
  //@{
  int priority_min() const;
  int priority_max() const;
  //@}

  /**
   * Accessors for @c bit_transport_port_.
   *
   * The accessor is used for client application to configure
   * the local transport listening port number.
   *
   * @note The default port is INVALID. The user needs call
   *       this function to setup the desired port number.
   */
  //@{
  int bit_transport_port() const;
  void bit_transport_port(int port);
  //@}

  OPENDDS_STRING bit_transport_ip() const;

  /**
   * Accessor for bit_lookup_duration_msec_.
   * The accessor is used for client application to configure
   * the timeout for lookup data from the builtin topic
   * datareader.  Value is in milliseconds.
   */
  //@{
  int bit_lookup_duration_msec() const;
  void bit_lookup_duration_msec(int msec);
  //@}

#if defined(OPENDDS_SECURITY)
  bool get_security() const;
  void set_security(bool b);
#endif

  bool get_BIT() const;
  void set_BIT(bool b);

  NetworkAddress default_address() const;

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  /// Get the data durability cache corresponding to the given
  /// DurabilityQosPolicy and sample list depth.
  DataDurabilityCache * get_data_durability_cache(
    DDS::DurabilityQosPolicy const & durability);
#endif

  /// For internal OpenDDS Use (needed for monitor code)
  typedef OPENDDS_MAP(Discovery::RepoKey, Discovery_rch) RepoKeyDiscoveryMap;
  const RepoKeyDiscoveryMap& discoveryMap() const;
  typedef OPENDDS_MAP(DDS::DomainId_t, Discovery::RepoKey) DomainRepoMap;
  const DomainRepoMap& domainRepoMap() const;

  void register_discovery_type(const char* section_name,
                               Discovery::Config* cfg);

#ifndef OPENDDS_SAFETY_PROFILE
  ACE_ARGV* ORB_argv() { return &ORB_argv_; }
#endif

  /**
   *  Create a Recorder object.
   */
  Recorder_ptr create_recorder(DDS::DomainParticipant_ptr participant,
                               DDS::Topic_ptr a_topic,
                               const DDS::SubscriberQos & subscriber_qos,
                               const DDS::DataReaderQos & datareader_qos,
                               const RecorderListener_rch & a_listener );



  /**
   *  Delete an existing Recorder from its DomainParticipant.
   */
  DDS::ReturnCode_t delete_recorder(Recorder_ptr recorder);

  /**
   *  Create a Replayer object
   */
  Replayer_ptr create_replayer(DDS::DomainParticipant_ptr participant,
                               DDS::Topic_ptr a_topic,
                               const DDS::PublisherQos & publisher_qos,
                               const DDS::DataWriterQos & datawriter_qos,
                               const ReplayerListener_rch & a_listener );

  /**
   *  Delete an existing Replayer from its DomainParticipant.
   */
  DDS::ReturnCode_t delete_replayer(Replayer_ptr replayer);

  /**
   *  Create a topic that does not have the data type registered.
   */
  DDS::Topic_ptr create_typeless_topic(DDS::DomainParticipant_ptr participant,
                                      const char * topic_name,
                                      const char * type_name,
                                      bool type_has_keys,
                                      const DDS::TopicQos & qos,
                                      DDS::TopicListener_ptr a_listener = 0,
                                      DDS::StatusMask mask = 0);

  /**
   * Import the configuration file to the ACE_Configuration_Heap
   * object and load common section configuration to the
   * Service_Participant singleton and load the factory and
   * transport section configuration to the TransportRegistry
   * singleton.
   */
  int load_configuration(ACE_Configuration_Heap& cf,
                         const ACE_TCHAR* filename);

  /**
   * Used by TransportRegistry to determine if a domain ID
   * is part of a [DomainRange]
   */
  bool belongs_to_domain_range(DDS::DomainId_t domainId) const;

  bool get_transport_base_config_name(DDS::DomainId_t domainId, String& name) const;

#ifdef OPENDDS_SAFETY_PROFILE
  /**
   * Configure the safety profile pool
   */
  void configure_pool();
#endif

  /**
   * Set a configuration file to use if -DCPSConfigFile wasn't passed to
   * TheParticipantFactoryWithArgs. Must be used before
   * TheParticipantFactory*() functions are called.
   */
  void default_configuration_file(const ACE_TCHAR* path);

#ifdef OPENDDS_NETWORK_CONFIG_MODIFIER
  NetworkConfigModifier* network_config_modifier();
#endif

  DDS::Duration_t bit_autopurge_nowriter_samples_delay() const;
  void bit_autopurge_nowriter_samples_delay(const DDS::Duration_t& duration);

  DDS::Duration_t bit_autopurge_disposed_samples_delay() const;
  void bit_autopurge_disposed_samples_delay(const DDS::Duration_t& duration);

  /**
   * Get TypeInformation of a remote entity given the corresponding BuiltinTopicKey_t.
   */
  XTypes::TypeInformation get_type_information(DDS::DomainParticipant_ptr participant,
                                               const DDS::BuiltinTopicKey_t& key) const;

#ifndef OPENDDS_SAFETY_PROFILE
  DDS::ReturnCode_t get_dynamic_type(DDS::DynamicType_var& type,
    DDS::DomainParticipant_ptr participant, const DDS::BuiltinTopicKey_t& key) const;
#endif

  /**
   * Get TypeObject for a given TypeIdentifier.
   */
  XTypes::TypeObject get_type_object(DDS::DomainParticipant_ptr participant,
                                     const XTypes::TypeIdentifier& ti) const;

  enum TypeObjectEncoding { Encoding_Normal, Encoding_WriteOldFormat, Encoding_ReadOldFormat };
  TypeObjectEncoding type_object_encoding() const;
  void type_object_encoding(TypeObjectEncoding encoding);
  void type_object_encoding(const char* encoding);

  RcHandle<InternalTopic<NetworkInterfaceAddress> > network_interface_address_topic() const
  {
    return network_interface_address_topic_;
  }

  unsigned int printer_value_writer_indent() const;
  void printer_value_writer_indent(unsigned int value);

  ConfigTopic_rch config_topic() const
  {
    return config_topic_;
  }

  RcHandle<ConfigStoreImpl> config_store() const
  {
    return config_store_;
  }

private:

  /// Initialize default qos.
  void initialize();

  /// Initialize the thread scheduling and initial priority.
  void initializeScheduling();

  /**
   * Parse the command line for user options. e.g. "-DCPSInfoRepo <iorfile>".
   * It consumes -DCPS* options and their arguments
   */
  int parse_args(int &argc, ACE_TCHAR *argv[]);

  /**
   * Import the configuration file to the ACE_Configuration_Heap
   * object and load common section configuration to the
   * Service_Participant singleton and load the factory and
   * transport section configuration to the TransportRegistry
   * singleton.
   */
  int load_configuration(const String& config_fname);

  /**
   * Load the domain configuration to the Service_Participant
   * singleton.
   */
  int load_domain_configuration(ACE_Configuration_Heap& cf,
                                const ACE_TCHAR* filename);

  /**
   * Load the domain range template configuration
   * prior to discovery and domain configuration
   */
  int load_domain_ranges();

  /**
   * Load the discovery template information
   */
  int load_discovery_templates();

  /**
   * Process the domain range template and activate the
   * domain for the given domain ID
   */
  int configure_domain_range_instance(DDS::DomainId_t domainId);

  /**
   * Load the discovery configuration to the Service_Participant
   * singleton.
   */
  int load_discovery_configuration(ACE_Configuration_Heap& cf,
                                   const ACE_TCHAR* section_name);

  /**
   * Create and load a discovery config from a discovery template
   */
  int configure_discovery_template(DDS::DomainId_t domainId,
                                   const OPENDDS_STRING& discovery_name);

  typedef OPENDDS_MAP(OPENDDS_STRING, container_supported_unique_ptr<Discovery::Config>) DiscoveryTypes;
  DiscoveryTypes discovery_types_;

#ifndef OPENDDS_SAFETY_PROFILE
  ACE_ARGV ORB_argv_;
#endif

  const TimeSource time_source_;
  ReactorTask reactor_task_;
  JobQueue_rch job_queue_;

  RcHandle<DomainParticipantFactoryImpl> dp_factory_servant_;

  /// The RepoKey to Discovery object mapping
  RepoKeyDiscoveryMap discoveryMap_;

  /// The DomainId to RepoKey mapping.
  DomainRepoMap domainRepoMap_;

  /// The lock to serialize DomainParticipantFactory singleton
  /// creation and shutdown.
  mutable ACE_Thread_Mutex factory_lock_;

  /// The initial values of qos policies.
  DDS::UserDataQosPolicy              initial_UserDataQosPolicy_;
  DDS::TopicDataQosPolicy             initial_TopicDataQosPolicy_;
  DDS::GroupDataQosPolicy             initial_GroupDataQosPolicy_;
  DDS::TransportPriorityQosPolicy     initial_TransportPriorityQosPolicy_;
  DDS::LifespanQosPolicy              initial_LifespanQosPolicy_;
  DDS::DurabilityQosPolicy            initial_DurabilityQosPolicy_;
  DDS::DurabilityServiceQosPolicy     initial_DurabilityServiceQosPolicy_;
  DDS::PresentationQosPolicy          initial_PresentationQosPolicy_;
  DDS::DeadlineQosPolicy              initial_DeadlineQosPolicy_;
  DDS::LatencyBudgetQosPolicy         initial_LatencyBudgetQosPolicy_;
  DDS::OwnershipQosPolicy             initial_OwnershipQosPolicy_;
  DDS::OwnershipStrengthQosPolicy     initial_OwnershipStrengthQosPolicy_;
  DDS::LivelinessQosPolicy            initial_LivelinessQosPolicy_;
  DDS::TimeBasedFilterQosPolicy       initial_TimeBasedFilterQosPolicy_;
  DDS::PartitionQosPolicy             initial_PartitionQosPolicy_;
  DDS::ReliabilityQosPolicy           initial_ReliabilityQosPolicy_;
  DDS::DestinationOrderQosPolicy      initial_DestinationOrderQosPolicy_;
  DDS::HistoryQosPolicy               initial_HistoryQosPolicy_;
  DDS::ResourceLimitsQosPolicy        initial_ResourceLimitsQosPolicy_;
  DDS::EntityFactoryQosPolicy         initial_EntityFactoryQosPolicy_;
  DDS::WriterDataLifecycleQosPolicy   initial_WriterDataLifecycleQosPolicy_;
  DDS::ReaderDataLifecycleQosPolicy   initial_ReaderDataLifecycleQosPolicy_;
  DDS::PropertyQosPolicy              initial_PropertyQosPolicy_;
  DDS::DataRepresentationQosPolicy    initial_DataRepresentationQosPolicy_;

  DDS::DomainParticipantQos           initial_DomainParticipantQos_;
  DDS::TopicQos                       initial_TopicQos_;
  DDS::DataWriterQos                  initial_DataWriterQos_;
  DDS::PublisherQos                   initial_PublisherQos_;
  DDS::DataReaderQos                  initial_DataReaderQos_;
  DDS::SubscriberQos                  initial_SubscriberQos_;
  DDS::DomainParticipantFactoryQos    initial_DomainParticipantFactoryQos_;
  DDS::TypeConsistencyEnforcementQosPolicy initial_TypeConsistencyEnforcementQosPolicy_;

  // domain range template support
  class DomainRange {
  public:
    DomainRange(const String& name)
      : name_(name)
      , config_prefix_(ConfigPair::canonicalize(String("OPENDDS_DOMAIN_RANGE_") + name))
      , range_start_(-1)
      , range_end_(-1)
    {}

    int parse_domain_range();

    const String& name() const { return name_; }
    const String& config_prefix() const { return config_prefix_; }
    String discovery_template_name(RcHandle<ConfigStoreImpl> config_store) const;
    String transport_config_name(RcHandle<ConfigStoreImpl> config_store) const;
    DCPS::ConfigStoreImpl::StringMap domain_info(RcHandle<ConfigStoreImpl> config_store) const;

    bool belongs_to_domain_range(DDS::DomainId_t domain_id) const
    {
      return domain_id >= range_start_ && domain_id <= range_end_;
    }

  private:
    String config_key(const String& key) const
    {
      return ConfigPair::canonicalize(config_prefix_ + "_" + key);
    }

    String name_;
    String config_prefix_;
    DDS::DomainId_t range_start_;
    DDS::DomainId_t range_end_;
  };

  class DiscoveryInfo {
  public:
    DiscoveryInfo(const String& name)
      : discovery_name_(name)
      , config_prefix_(ConfigPair::canonicalize(String("OPENDDS_RTPS_DISCOVERY_") + name))
    {}

    const String& discovery_name() const { return discovery_name_; }
    DCPS::ConfigStoreImpl::StringMap customizations(RcHandle<ConfigStoreImpl> config_store) const;
    DCPS::ConfigStoreImpl::StringMap disc_info(RcHandle<ConfigStoreImpl> config_store) const;

  private:
    String config_key(const String& key) const
    {
      return ConfigPair::canonicalize(config_prefix_ + "_" + key);
    }

    String discovery_name_;
    String config_prefix_;
  };

  OPENDDS_MAP(DDS::DomainId_t, OPENDDS_STRING) domain_to_transport_name_map_;

  OPENDDS_VECTOR(DomainRange) domain_ranges_;

  OPENDDS_VECTOR(DiscoveryInfo) discovery_infos_;

  bool has_domain_range() const;

  bool get_domain_range_info(DDS::DomainId_t id, DomainRange& inst);

  bool process_customizations(DDS::DomainId_t id, const OPENDDS_STRING& discovery_name, ValueMap& customs);

  OpenDDS::DCPS::Discovery::RepoKey get_discovery_template_instance_name(DDS::DomainId_t id);

  bool is_discovery_template(const OPENDDS_STRING& name);

public:
  /// getter for lock that protects the static initialization of XTypes related data structures
  ACE_Thread_Mutex& get_static_xtypes_lock();

  /// Get the service participant's thread status manager.
  ThreadStatusManager& get_thread_status_manager();

  /// Pointer to the monitor factory that is used to create
  /// monitor objects.
  MonitorFactory* monitor_factory_;

  /// Pointer to the monitor object for this object
  unique_ptr<Monitor> monitor_;

private:
  /// Minimum priority value for the current scheduling policy.
  int priority_min_;

  /// Maximum priority value for the current scheduling policy.
  int priority_max_;

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE

  /// The @c TRANSIENT data durability cache.
  unique_ptr<DataDurabilityCache> transient_data_cache_;

  /// The @c PERSISTENT data durability cache.
  unique_ptr<DataDurabilityCache> persistent_data_cache_;

#endif

  ThreadStatusManager thread_status_manager_;

  /// Thread mutex used to protect the static initialization of XTypes data structures
  ACE_Thread_Mutex xtypes_lock_;

  /// Used to track state of service participant
  AtomicBool shut_down_;

  RcHandle<ShutdownListener> shutdown_listener_;

  /// Guard access to the internal maps.
  ACE_Recursive_Thread_Mutex maps_lock_;

  static int zero_argc;

  NetworkConfigMonitor_rch network_config_monitor_;
  mutable ACE_Thread_Mutex network_config_monitor_lock_;

  RcHandle<InternalTopic<NetworkInterfaceAddress> > network_interface_address_topic_;

  ConfigTopic_rch config_topic_;
  RcHandle<ConfigStoreImpl> config_store_;
  ConfigReader_rch config_reader_;
  ConfigReaderListener_rch config_reader_listener_;

  class ConfigReaderListener : public InternalDataReaderListener<ConfigPair> {
  public:
    ConfigReaderListener(Service_Participant& service_participant)
      : service_participant_(service_participant)
    {}

    void on_data_available(InternalDataReader_rch reader);

  private:
    Service_Participant& service_participant_;
  };

  // This mutex protection configuration that is cached for efficient access.
  mutable ACE_Thread_Mutex cached_config_mutex_;
  TimeDuration pending_timeout_;
  Discovery::RepoKey default_discovery_;
};

#define TheServiceParticipant OpenDDS::DCPS::Service_Participant::instance()

#define TheParticipantFactory TheServiceParticipant->get_domain_participant_factory()

#define TheParticipantFactoryWithArgs(argc, argv) TheServiceParticipant->get_domain_participant_factory(argc, argv)

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "Service_Participant.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_SERVICE_PARTICIPANT_H  */
