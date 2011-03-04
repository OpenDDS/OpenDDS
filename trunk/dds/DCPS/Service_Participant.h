/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DDS_DCPS_SERVICE_PARTICIPANT_H
#define OPENDDS_DDS_DCPS_SERVICE_PARTICIPANT_H

#include "DomainParticipantFactoryImpl.h"
#include "dds/DdsDcpsInfrastructureS.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsInfoC.h"
#include "DomainParticipantFactoryImpl.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/MonitorFactory.h"

#include "tao/PortableServer/PortableServer.h"

#include "ace/Task.h"
#include "ace/Configuration.h"
#include "ace/Time_Value.h"

#include <map>
#include <memory>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if defined(_MSC_VER) && _MSC_VER < 1300 && _MSC_VER >= 1200
# pragma warning( disable : 4231 )
#endif

namespace OpenDDS {
namespace DCPS {

class DataDurabilityCache;
class Monitor;

const char DEFAULT_ORB_NAME[] = "OpenDDS_DCPS";

/**
 * @class Service_Participant
 *
 * @brief Service entrypoint.
 *
 * This class is a singleton that allows DDS client applications to
 * configure OpenDDS.  This includes running the ORB to support
 * OpenDDS.
 *
 * @note The client will either create an ORB and call @c set_ORB()
 *       before calling @c get_domain_particpant_factory() and will
 *       run the ORB *or* it will not call @c set_ORB() first and
 *       @c get_domain_particpant_factory() will automatically
 *       create an ORB to be used by OpenDDS and will run that ORB
 *       in a thread it creates.
 *
 * @note This class may read a configuration file that will
 *       configure Transports as well as DCPS (e.g. number of ORB
 *       threads).
 */
class OpenDDS_Dcps_Export Service_Participant : public ACE_Task_Base {
  static int zero_argc;

public:

  /// Domain value for the default repository IOR.
  enum { ANY_DOMAIN = -1 };

  /// Key value for the default repository IOR.
  enum { DEFAULT_REPO = -1 };

  /// Key type for storing repository objects.
  typedef int RepoKey;

  /// Map type to access IOR strings from repository key values.
  typedef std::map<RepoKey, std::string> KeyIorMap;

  /// Constructor.
  Service_Participant();

  /// Destructor.
  ~Service_Participant();

  /// Return a singleton instance of this class.
  static Service_Participant * instance();

  /// Launch a thread to run the orb.
  virtual int svc();

  /**
   * Client provides an ORB for the OpenDDS client to use.
   *
   * @note The user is responsible for running the ORB.
   */
  int set_ORB(CORBA::ORB_ptr orb);

  /**
   * Get the ORB used by OpenDDS.
   *
   * Only valid after @c set_ORB() or
   * @c get_domain_participant_factory() called.
   */
  CORBA::ORB_ptr get_ORB();

  /**
   * Initialize the DDS client environment and get the
   * @c DomainParticipantFactory.
   *
   * This method consumes @c -DCPS* options and their arguments.
   * Unless the client/application code calls other methods to
   * define how the ORB is run, calling this method will
   * initiallize the ORB and then run it in a separate thread.
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
   * Will shutdown the ORB unless it was given via @c set_ORB().
   *
   * @note Required Precondition: all DomainParticipants have been
   *       deleted.
   */
  void shutdown();

  /**
   * Accessor of the poa that application used.
   *
   * @todo Currently this method return the rootpoa. We might
   *       create our own poa.
   */
  PortableServer::POA_ptr the_poa();

  /// Accessor of the DCPSInfo object reference.
  DCPSInfo_ptr get_repository(const DDS::DomainId_t domain);

  /// Access the key/IOR mappings currently in effect.
  const KeyIorMap& keyIorMap() const;

  /** Accessors of the qos policy initial values. **/
  DDS::UserDataQosPolicy            initial_UserDataQosPolicy() const;
  DDS::TopicDataQosPolicy           initial_TopicDataQosPolicy() const;
  DDS::GroupDataQosPolicy           initial_GroupDataQosPolicy() const;
  DDS::TransportPriorityQosPolicy   initial_TransportPriorityQosPolicy() const;
  DDS::LifespanQosPolicy            initial_LifespanQosPolicy() const;
  DDS::DurabilityQosPolicy          initial_DurabilityQosPolicy() const;
  DDS::DurabilityServiceQosPolicy   initial_DurabilityServiceQosPolicy() const;
  DDS::PresentationQosPolicy        initial_PresentationQosPolicy() const;
  DDS::DeadlineQosPolicy            initial_DeadlineQosPolicy() const;
  DDS::LatencyBudgetQosPolicy       initial_LatencyBudgetQosPolicy() const;
  DDS::OwnershipQosPolicy           initial_OwnershipQosPolicy() const;
  DDS::OwnershipStrengthQosPolicy   initial_OwnershipStrengthQosPolicy() const;
  DDS::LivelinessQosPolicy          initial_LivelinessQosPolicy() const;
  DDS::TimeBasedFilterQosPolicy     initial_TimeBasedFilterQosPolicy() const;
  DDS::PartitionQosPolicy           initial_PartitionQosPolicy() const;
  DDS::ReliabilityQosPolicy         initial_ReliabilityQosPolicy() const;
  DDS::DestinationOrderQosPolicy    initial_DestinationOrderQosPolicy() const;
  DDS::HistoryQosPolicy             initial_HistoryQosPolicy() const;
  DDS::ResourceLimitsQosPolicy      initial_ResourceLimitsQosPolicy() const;
  DDS::EntityFactoryQosPolicy       initial_EntityFactoryQosPolicy() const;
  DDS::WriterDataLifecycleQosPolicy initial_WriterDataLifecycleQosPolicy() const;
  DDS::ReaderDataLifecycleQosPolicy initial_ReaderDataLifecycleQosPolicy() const;

  DDS::DomainParticipantFactoryQos  initial_DomainParticipantFactoryQos() const;
  DDS::DomainParticipantQos         initial_DomainParticipantQos() const;
  DDS::TopicQos                     initial_TopicQos() const;
  DDS::DataWriterQos                initial_DataWriterQos() const;
  DDS::PublisherQos                 initial_PublisherQos() const;
  DDS::DataReaderQos                initial_DataReaderQos() const;
  DDS::SubscriberQos                initial_SubscriberQos() const;

  /**
   * This accessor is to provide the configurable number of chunks
   * that a @c DataWriter's cached allocator need to allocate when
   * the resource limits are infinite.  Has a default, can be set
   * by the @c -DCPSChunks option, or by @c n_chunks() setter.
   */
  size_t   n_chunks() const;

  /// Set the value returned by @c n_chunks() accessor.
  /**
   * @see Accessor description.
   */
  void     n_chunks(size_t chunks);

  /// This accessor is to provide the multiplier for allocators
  /// that have resources used on a per association basis.
  /// Has a default, can be set by the
  /// @c -DCPSChunkAssociationMutltiplier
  /// option, or by @c n_association_chunk_multiplier() setter.
  size_t   association_chunk_multiplier() const;

  /// Set the value returned by
  /// @c n_association_chunk_multiplier() accessor.
  /**
   * See accessor description.
   */
  void     association_chunk_multiplier(size_t multiplier);

  /// Set the Liveliness propagation delay factor.
  /// @param factor % of lease period before sending a liveliness
  ///               message.
  void liveliness_factor(int factor);

  /// Accessor of the Liveliness propagation delay factor.
  /// @return % of lease period before sending a liveliness
  ///         message.
  int liveliness_factor() const;

  /// Load DCPSInfoRepo IORs.
  // CORBA strings are narrow so ior is const char* not ACE_TCHAR
  void set_repo_ior(const char* ior,
                    const RepoKey key = DEFAULT_REPO,
                    bool  attach_participant = true);

#ifdef DDS_HAS_WCHAR
  /// Convenience overload for wchar_t
  void set_repo_ior(const wchar_t* ior,
                    const RepoKey key = DEFAULT_REPO,
                    bool  attach_participant = true);
#endif

  /// Load DCPSInfoRepo reference directly.
  void set_repo(DCPSInfo_ptr repo,
                const RepoKey key = DEFAULT_REPO,
                bool  attach_participant = true);

  /// Rebind a domain from one repository to another.
  void remap_domains(const RepoKey oldKey,
                     const RepoKey newKey,
                     bool attach_participant = true);

  /// Bind DCPSInfoRepo IORs to domains.
  void set_repo_domain(const DDS::DomainId_t domain,
                       const RepoKey repo,
                       bool attach_participant = true);

  /// Convert domainId to repository key.
  RepoKey domain_to_repo(const DDS::DomainId_t domain) const;

  /// Failover to a new repository.
  void repository_lost(const RepoKey key);

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

  /// Accessor for pending data timeout.
  ACE_Time_Value pending_timeout() const;

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
  int bit_transport_port(RepoKey repo = DEFAULT_REPO) const;
  void bit_transport_port(int port, RepoKey repo = DEFAULT_REPO);
  //@}

  /// Accessor of the TransportImpl used by the builtin topics.
  TransportImpl_rch bit_transport_impl(DDS::DomainId_t domain = ANY_DOMAIN);

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

  bool get_BIT() {
    return bit_enabled_;
  }

  void set_BIT(bool b) {
    bit_enabled_ = b;
  }

  ///Create the TransportImpl for all builtin topics.
  int init_bit_transport_impl(DDS::DomainId_t domain = ANY_DOMAIN);

  /// Get the data durability cache corresponding to the given
  /// DurabilityQosPolicy and sample list depth.
  DataDurabilityCache * get_data_durability_cache(
    DDS::DurabilityQosPolicy const & durability);

private:

  /// Initalize default qos.
  void initialize();

  /// Initialize the thread scheduling and initial priority.
  void initializeScheduling();

  /**
   * Parse the command line for user options. e.g. "-DCPSInfo <iorfile>".
   * It consumes -DCPS* options and their arguments
   */
  int parse_args(int &argc, ACE_TCHAR *argv[]);

  /**
   * Import the configuration file to the ACE_Configuration_Heap
   * object and load common section configuration to the
   * Service_Participant singleton and load the factory and
   * transport section configuration to the TransportFactory
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
  int load_common_configuration();

  /**
   * Load the domain configuration to the Service_Participant
   * singleton.
   */
  int load_domain_configuration();

  /**
   * Load the repository configuration to the Service_Participant
   * singleton.
   */
  int load_repo_configuration();

// public:

  /// The orb object reference which can be provided by client or
  /// initialized by this sigleton.
  CORBA::ORB_var orb_;

  /// @c true if set_ORB() was called.
  int orb_from_user_;

  /// The root poa object reference.
  PortableServer::POA_var root_poa_;

  /// The domain participant factory servant.
  /**
   * Allocate the factory on the heap to avoid the circular
   * dependency since the
   * OpenDDS::DCPS::DomainParticipantFactoryImpl constructor calls
   * the OpenDDS::DCPS::Service_Participant singleton.
   */
  DomainParticipantFactoryImpl*        dp_factory_servant_;

  /// The domain participant factory object reference.
  DDS::DomainParticipantFactory_var  dp_factory_;

  /// The DomainId to RepoKey mapping.
  typedef std::map<DDS::DomainId_t, RepoKey> DomainRepoMap;
  DomainRepoMap domainRepoMap_;

  /// Repository key to IOR string values.
  KeyIorMap keyIorMap_;

  /// The DomainId to DCPSInfo/repository object references
  /// container.
  typedef std::map<RepoKey, DCPSInfo_var> RepoMap;
  RepoMap repoMap_;

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

  DDS::DomainParticipantQos           initial_DomainParticipantQos_;
  DDS::TopicQos                       initial_TopicQos_;
  DDS::DataWriterQos                  initial_DataWriterQos_;
  DDS::PublisherQos                   initial_PublisherQos_;
  DDS::DataReaderQos                  initial_DataReaderQos_;
  DDS::SubscriberQos                  initial_SubscriberQos_;
  DDS::DomainParticipantFactoryQos    initial_DomainParticipantFactoryQos_;

  DDS::LivelinessLostStatus           initial_liveliness_lost_status_ ;
  DDS::OfferedDeadlineMissedStatus    initial_offered_deadline_missed_status_ ;
  DDS::OfferedIncompatibleQosStatus   initial_offered_incompatible_qos_status_ ;
  DDS::PublicationMatchedStatus       initial_publication_match_status_ ;

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
  typedef std::map<RepoKey, ACE_TString> RepoTransportIpMap;
  RepoTransportIpMap bitTransportIpMap_;

  /// The builtin topic transport port number.
  typedef std::map<RepoKey, int> RepoTransportPortMap;
  RepoTransportPortMap bitTransportPortMap_;

  /// The mapping from DomainId to transport implementations.
  typedef std::map<DDS::DomainId_t, TransportImpl_rch> RepoTransportMap;
  RepoTransportMap bitTransportMap_;

  bool bit_enabled_;

  /// The timeout for lookup data from the builtin topic
  /// @c DataReader.
  int bit_lookup_duration_msec_;

  /// The configuration object that imports the configuration
  /// file.
  ACE_Configuration_Heap cf_;

public:
  /// Pointer to the monitor factory that is used to create
  /// monitor objects.
  MonitorFactory* monitor_factory_;

  /// Pointer to the monitor object for this object
  Monitor* monitor_;

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
  ACE_Time_Value schedulerQuantum_;

  /// Scheduling policy value used for setting thread priorities.
  long scheduler_;

  /// Minimum priority value for the current scheduling policy.
  int priority_min_;

  /// Maximum priority value for the current scheduling policy.
  int priority_max_;

  /// Allow the publishing side to do content filtering?
  bool publisher_content_filter_;

  /// The @c TRANSIENT data durability cache.
  std::auto_ptr<DataDurabilityCache> transient_data_cache_;

  /// The @c PERSISTENT data durability cache.
  std::auto_ptr<DataDurabilityCache> persistent_data_cache_;

  /// The @c PERSISTENT data durability directory.
  ACE_CString persistent_data_dir_;

  /// Number of seconds to wait on pending samples to be sent
  /// or dropped.
  ACE_Time_Value pending_timeout_;

  /// Guard access to the internal maps.
  ACE_Recursive_Thread_Mutex maps_lock_;
};

#   define TheServiceParticipant OpenDDS::DCPS::Service_Participant::instance()

#   define TheParticipantFactory TheServiceParticipant->get_domain_participant_factory()

#   define TheParticipantFactoryWithArgs(argc, argv) TheServiceParticipant->get_domain_participant_factory(argc, argv)

/// Get a servant pointer given an object reference.
/// @throws PortableServer::POA::OjbectNotActive,
///         PortableServer::POA::WrongAdapter
///         PortableServer::POA::WongPolicy
template <class T_impl, class T_ptr>
T_impl* remote_reference_to_servant(T_ptr p)
{
  if (CORBA::is_nil(p)) {
    return 0;
  }

  PortableServer::POA_var poa = TheServiceParticipant->the_poa();

  T_impl* the_servant =
    dynamic_cast<T_impl*>(poa->reference_to_servant(p));

  // Use the ServantBase_var so that the servant's reference
  // count will not be changed by this operation.
  PortableServer::ServantBase_var servant = the_servant;

  return the_servant;
}

/// Given a servant, return the emote object reference from the local POA.
/// @throws PortableServer::POA::ServantNotActive,
///         PortableServer::POA::WrongPolicy
template <class T>
typename T::_stub_ptr_type servant_to_remote_reference(
  T *servant)
{
  PortableServer::POA_var poa = TheServiceParticipant->the_poa();

  PortableServer::ObjectId_var oid = poa->activate_object(servant);

  CORBA::Object_var obj = poa->id_to_reference(oid.in());

  typename T::_stub_ptr_type the_obj = T::_stub_type::_narrow(obj.in());
  return the_obj;
}

template <class T>
void deactivate_remote_object(T obj)
{
  PortableServer::POA_var poa = TheServiceParticipant->the_poa();
  PortableServer::ObjectId_var oid =
    poa->reference_to_id(obj);
  poa->deactivate_object(oid.in());
}

} // namespace DCPS
} // namespace OpenDDS

#if defined(__ACE_INLINE__)
#include "Service_Participant.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_SERVICE_PARTICIPANT_H  */
