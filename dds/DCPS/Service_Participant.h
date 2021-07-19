/*
 *
 *
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
#include "NetworkConfigMonitor.h"
#include "NetworkConfigModifier.h"
#include "Recorder.h"
#include "Replayer.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#include <ace/Task.h>
#include <ace/Configuration.h>
#include <ace/Time_Value.h>
#include <ace/ARGV.h>
#include <ace/Barrier.h>

#include <memory>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
class DataDurabilityCache;
#endif

const char DEFAULT_ORB_NAME[] = "OpenDDS_DCPS";

class ShutdownListener {
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

  /// Get the common timer interface.
  /// Intended for use by OpenDDS internals only.
  ACE_Reactor_Timer_Interface* timer();

  ACE_Reactor* reactor();

  ACE_thread_t reactor_owner() const;

  ReactorInterceptor_rch interceptor() const;

  void set_shutdown_listener(ShutdownListener* listener);

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
   * @note Required Precondition: all DomainParticipants have been
   *       deleted.
   */
  void shutdown();

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
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  const DDS::OwnershipStrengthQosPolicy& initial_OwnershipStrengthQosPolicy() const;
#endif
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
  /// @c -DCPSChunkAssociationMutltiplier
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
                    bool attach_participant = true);

#ifdef DDS_HAS_WCHAR
  /// Convenience overload for wchar_t
  bool set_repo_ior(const wchar_t* ior,
                    Discovery::RepoKey key = Discovery::DEFAULT_REPO,
                    bool attach_participant = true);
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
  int& federation_recovery_duration();
  int  federation_recovery_duration() const;
  //@}

  /// Accessors for FederationInitialBackoffSeconds.
  //@{
  int& federation_initial_backoff_seconds();
  int  federation_initial_backoff_seconds() const;
  //@}

  /// Accessors for FederationBackoffMultiplier.
  //@{
  int& federation_backoff_multiplier();
  int  federation_backoff_multiplier() const;
  //@}

  /// Accessors for FederationLivelinessDuration.
  //@{
  int& federation_liveliness();
  int  federation_liveliness() const;
  //@}

  /// Accessors for scheduling policy value.
  //@{
  long& scheduler();
  long  scheduler() const;
  //@}

  /// Accessors for PublisherContentFilter.
  //@{
  bool& publisher_content_filter();
  bool  publisher_content_filter() const;
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
  bool get_security() {
    return security_enabled_;
  }

  void set_security(bool b) {
    security_enabled_ = b;
  }
#endif

  bool get_BIT() {
    return bit_enabled_;
  }

  void set_BIT(bool b) {
    bit_enabled_ = b;
  }

  const ACE_INET_Addr& default_address() const { return default_address_; }

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

  bool get_transport_base_config_name(DDS::DomainId_t domainId, ACE_TString& name) const;

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
  NetworkConfigMonitor_rch network_config_monitor();


  DDS::Duration_t bit_autopurge_nowriter_samples_delay() const;
  void bit_autopurge_nowriter_samples_delay(const DDS::Duration_t& duration);

  DDS::Duration_t bit_autopurge_disposed_samples_delay() const;
  void bit_autopurge_disposed_samples_delay(const DDS::Duration_t& duration);

  /**
   * Get TypeInformation of a remote entity given the corresponding BuiltinTopicKey_t.
   */
  XTypes::TypeInformation get_type_information(DDS::DomainParticipant_ptr participant,
                                               const DDS::BuiltinTopicKey_t& key) const;

  /**
   * Get TypeObject of a remote entity given the corresponding TypeIdentifier.
   */
  XTypes::TypeObject get_type_object(DDS::DomainParticipant_ptr participant,
                                     const XTypes::TypeIdentifier& ti) const;

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
  int load_configuration();

  /**
   * Load the common configuration to the Service_Participant
   * singleton.
   *
   * @note The values from command line can overwrite the values
   *       in configuration file.
   */
  int load_common_configuration(ACE_Configuration_Heap& cf,
                                const ACE_TCHAR* filename);

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
  int load_domain_ranges(ACE_Configuration_Heap& cf);

  /**
   * Load the discovery template information
   */
  int load_discovery_templates(ACE_Configuration_Heap& cf);

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

  ReactorTask reactor_task_;

  RcHandle<DomainParticipantFactoryImpl> dp_factory_servant_;

  /// The RepoKey to Discovery object mapping
  RepoKeyDiscoveryMap discoveryMap_;

  /// The DomainId to RepoKey mapping.
  DomainRepoMap domainRepoMap_;

  Discovery::RepoKey defaultDiscovery_;

  /// The lock to serialize DomainParticipantFactory singleton
  /// creation and shutdown.
  TAO_SYNCH_MUTEX      factory_lock_;

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
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  DDS::OwnershipStrengthQosPolicy     initial_OwnershipStrengthQosPolicy_;
#endif
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
  DDS::DataRepresentationQosPolicy initial_DataRepresentationQosPolicy_;

  DDS::DomainParticipantQos           initial_DomainParticipantQos_;
  DDS::TopicQos                       initial_TopicQos_;
  DDS::DataWriterQos                  initial_DataWriterQos_;
  DDS::PublisherQos                   initial_PublisherQos_;
  DDS::DataReaderQos                  initial_DataReaderQos_;
  DDS::SubscriberQos                  initial_SubscriberQos_;
  DDS::DomainParticipantFactoryQos    initial_DomainParticipantFactoryQos_;
  DDS::TypeConsistencyEnforcementQosPolicy initial_TypeConsistencyEnforcementQosPolicy_;

  /// The configurable value of the number chunks that the
  /// @c DataWriter's cached allocator can allocate.
  size_t                                 n_chunks_;

  /// The configurable value of maximum number of expected
  /// associations for publishers and subscribers.  This is used
  /// to pre allocate enough memory and reduce heap allocations.
  size_t                                 association_chunk_multiplier_;

  /// The propagation delay factor.
  int                                    liveliness_factor_;

  /// The builtin topic transport address.
  ACE_TString bit_transport_ip_;

  /// The builtin topic transport port number.
  int bit_transport_port_;

  bool bit_enabled_;

#if defined(OPENDDS_SECURITY)
  bool security_enabled_;
#endif

  /// The timeout for lookup data from the builtin topic
  /// @c DataReader.
  int bit_lookup_duration_msec_;

  /// The default network address to use.
  ACE_INET_Addr default_address_;

  /// The configuration object that imports the configuration
  /// file.
  ACE_Configuration_Heap cf_;

  /// Specifies the name of the transport configuration that
  /// is used when the entity tree does not specify one.  If
  /// not set, the default transport configuration is used.
  ACE_TString global_transport_config_;

  // domain range template support
  struct DomainRange
  {
    DDS::DomainId_t range_start;
    DDS::DomainId_t range_end;
    OPENDDS_STRING discovery_template_name;
    OPENDDS_STRING transport_config_name;
    ValueMap domain_info;

    DomainRange() : range_start(-1), range_end(-1) {}
  };

  struct DiscoveryInfo
  {
    OPENDDS_STRING discovery_name;
    ValueMap customizations;
    ValueMap disc_info;
  };

  OPENDDS_MAP(DDS::DomainId_t, OPENDDS_STRING) domain_to_transport_name_map_;

  OPENDDS_VECTOR(DomainRange) domain_ranges_;

  OPENDDS_VECTOR(DiscoveryInfo) discovery_infos_;

  int parse_domain_range(const OPENDDS_STRING& range, int& start, int& end);

  bool has_domain_range() const;

  bool get_domain_range_info(DDS::DomainId_t id, DomainRange& inst);

  bool process_customizations(DDS::DomainId_t id, const OPENDDS_STRING& discovery_name, ValueMap& customs);

  OpenDDS::DCPS::Discovery::RepoKey get_discovery_template_instance_name(DDS::DomainId_t id);

  bool is_discovery_template(const OPENDDS_STRING& name);

public:
  // thread status reporting
  TimeDuration get_thread_status_interval();
  void set_thread_status_interval(TimeDuration interval);

  /// getter for lock that protects the static initialization of XTypes related data structures
  ACE_Thread_Mutex& get_static_xtypes_lock();

  /// Get the service participant's thread status manager.
  ThreadStatusManager* get_thread_status_manager();

  /// Pointer to the monitor factory that is used to create
  /// monitor objects.
  MonitorFactory* monitor_factory_;

  /// Pointer to the monitor object for this object
  unique_ptr<Monitor> monitor_;

private:
  /// The FederationRecoveryDuration value in seconds.
  int federation_recovery_duration_;

  /// The FederationInitialBackoffSeconds value.
  int federation_initial_backoff_seconds_;

  /// This FederationBackoffMultiplier.
  int federation_backoff_multiplier_;

  /// This FederationLivelinessDuration.
  int federation_liveliness_;

  /// Scheduling policy value from configuration file.
  ACE_TString schedulerString_;

  /// Scheduler time slice from configuration file.
  TimeDuration schedulerQuantum_;

#if OPENDDS_POOL_ALLOCATOR
  /// Pool size from configuration file.
  size_t pool_size_;

  /// Pool granularity from configuration file.
  size_t pool_granularity_;
#endif

  /// Scheduling policy value used for setting thread priorities.
  long scheduler_;

  /// Minimum priority value for the current scheduling policy.
  int priority_min_;

  /// Maximum priority value for the current scheduling policy.
  int priority_max_;

  /// Allow the publishing side to do content filtering?
  bool publisher_content_filter_;

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE

  /// The @c TRANSIENT data durability cache.
  unique_ptr<DataDurabilityCache> transient_data_cache_;

  /// The @c PERSISTENT data durability cache.
  unique_ptr<DataDurabilityCache> persistent_data_cache_;

  /// The @c PERSISTENT data durability directory.
  ACE_CString persistent_data_dir_;

#endif

  /// Number of seconds to wait on pending samples to be sent
  /// or dropped.
  TimeDuration pending_timeout_;

  /// Enable TAO's Bidirectional GIOP?
  bool bidir_giop_;

  /// Enable Internal Thread Status Monitoring
  TimeDuration thread_status_interval_;

  ThreadStatusManager thread_status_manager_;

  /// Thread mutex used to protect the static initialization of XTypes data structures
  ACE_Thread_Mutex xtypes_lock_;

  /// Enable Monitor functionality
  bool monitor_enabled_;

  /// Used to track state of service participant
  bool shut_down_;

  ShutdownListener* shutdown_listener_;

  /// Guard access to the internal maps.
  ACE_Recursive_Thread_Mutex maps_lock_;

  static int zero_argc;

  /**
   * If set before TheParticipantFactoryWithArgs and -DCPSConfigFile is not
   * passed, use this as the configuration file.
   */
  ACE_TString default_configuration_file_;

  NetworkConfigMonitor_rch network_config_monitor_;
  mutable ACE_Thread_Mutex network_config_monitor_lock_;

  DDS::Duration_t bit_autopurge_nowriter_samples_delay_;
  DDS::Duration_t bit_autopurge_disposed_samples_delay_;
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
