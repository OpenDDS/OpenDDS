/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "InfoRepoDiscovery.h"
#include "FailoverListener.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/ConfigUtils.h"

#include "tao/ORB_Core.h"
#include "ace/Reactor.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Marked_Default_Qos.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"

#include "dds/DCPS/transport/tcp/TcpInst.h"
#include "dds/DCPS/transport/tcp/TcpInst_rch.h"
#endif

namespace OpenDDS {
namespace DCPS {

InfoRepoDiscovery::InfoRepoDiscovery(const RepoKey& key,
                                     const std::string& ior)
  : Discovery(key),
    ior_(ior),
    bit_transport_port_(0),
    use_local_bit_config_(false),
    failoverListener_(0),
    orb_from_user_(false)
{
}

InfoRepoDiscovery::InfoRepoDiscovery(const RepoKey& key,
                                     const DCPSInfo_var& info)
  : Discovery(key),
    info_(info),
    bit_transport_port_(0),
    use_local_bit_config_(false),
    failoverListener_(0),
    orb_from_user_(false)
{
}

InfoRepoDiscovery::~InfoRepoDiscovery()
{
  delete this->failoverListener_;

  if (!orb_from_user_ && orb_runner_) {
    if (0 == orb_runner_->use_count_--) {
      orb_runner_->shutdown();
      delete orb_runner_;
      orb_runner_ = 0;
    }
  }
}

bool
InfoRepoDiscovery::set_ORB(CORBA::ORB_ptr orb)
{
  if (orb_.in() || !orb) {
    return false;
  }

  orb_ = CORBA::ORB::_duplicate(orb);
  orb_from_user_ = true;
  return true;
}

namespace
{
  DCPSInfo_ptr get_repo(const char* ior, CORBA::ORB_ptr orb)
  {
    CORBA::Object_var o;
    try {
      o = orb->string_to_object(ior);
    } catch (CORBA::INV_OBJREF&) {
      // host:port format causes an exception; try again
      // with corbaloc format
      std::string second_try("corbaloc:iiop:");
      second_try += ior;
      second_try += "/DCPSInfoRepo";

      o = orb->string_to_object(second_try.c_str());
    }

    return DCPSInfo::_narrow(o.in());
  }
}

DCPSInfo_var
InfoRepoDiscovery::get_dcps_info()
{
  if (CORBA::is_nil(this->info_.in())) {

    if (!orb_) {
      ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mtx_orb_runner_, 0);
      if (!orb_runner_) {
        orb_runner_ = new OrbRunner;
        ACE_ARGV* argv = TheServiceParticipant->ORB_argv();
        int argc = argv->argc();
        orb_runner_->orb_ =
          CORBA::ORB_init(argc, argv->argv(), DEFAULT_ORB_NAME);
        orb_runner_->use_count_ = 1;
        orb_runner_->activate();

        CORBA::Object_var rp =
          orb_runner_->orb_->resolve_initial_references("RootPOA");
        PortableServer::POA_var poa = PortableServer::POA::_narrow(rp);
        PortableServer::POAManager_var poa_manager = poa->the_POAManager();
        poa_manager->activate();
      } else {
        ++orb_runner_->use_count_;
      }
      orb_ = orb_runner_->orb_;
    }

    try {
      this->info_ = get_repo(this->ior_.c_str(), orb_);

      if (CORBA::is_nil(this->info_.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::get_dcps_info: ")
                   ACE_TEXT("unable to narrow DCPSInfo (%C) for key %C.\n"),
                   this->ior_.c_str(),
                   this->key().c_str()));
        return DCPSInfo::_nil();
      }

    } catch (const CORBA::Exception& ex) {
      ex._tao_print_exception("ERROR: InfoRepoDiscovery::get_dcps_info: failed to resolve ior - ");
      return DCPSInfo::_nil();
    }
  }

  return this->info_;
}

std::string
InfoRepoDiscovery::get_stringified_dcps_info_ior()
{
  return this->ior_;
}

TransportConfig_rch
InfoRepoDiscovery::bit_config()
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (bit_config_.is_nil()) {
    const std::string cfg_name = TransportRegistry::DEFAULT_INST_PREFIX +
                                 "_BITTransportConfig_" + key();
    bit_config_ = TransportRegistry::instance()->create_config(cfg_name);

    const std::string inst_name = TransportRegistry::DEFAULT_INST_PREFIX +
                                  "_BITTCPTransportInst_" + key();
    TransportInst_rch inst =
      TransportRegistry::instance()->create_inst(inst_name, "tcp");
    bit_config_->instances_.push_back(inst);

    if (!use_local_bit_config_) {
      bit_transport_ip_ = TheServiceParticipant->bit_transport_ip();
      bit_transport_port_ = TheServiceParticipant->bit_transport_port();
    }

    // Use a static cast to avoid dependency on the Tcp library
    TcpInst_rch tcp_inst = static_rchandle_cast<TcpInst>(inst);

    tcp_inst->datalink_release_delay_ = 0;
    if (bit_transport_ip_ == "") {
      tcp_inst->local_address_.set_port_number(bit_transport_port_);
    } else {
      tcp_inst->local_address_ = ACE_INET_Addr(bit_transport_port_,
                                               bit_transport_ip_.c_str());
    }

    std::stringstream out;
    out << bit_transport_ip_ << ':' << bit_transport_port_;
    tcp_inst->local_address_str_ = out.str();
  }
  return bit_config_;
#else
  return 0;
#endif
}

DDS::Subscriber_ptr
InfoRepoDiscovery::init_bit(DomainParticipantImpl* participant)
{
#if defined (DDS_HAS_MINIMUM_BIT)
  ACE_UNUSED_ARG(participant);
  return 0;
#else
  if (!TheServiceParticipant->get_BIT()) {
    return 0;
  }

  if (create_bit_topics(participant) != DDS::RETCODE_OK) {
    return 0;
  }

  DDS::Subscriber_var bit_subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   DDS::SubscriberListener::_nil(),
                                   DEFAULT_STATUS_MASK);
  try {
    TransportConfig_rch config = bit_config();
    TransportRegistry::instance()->bind_config(config, bit_subscriber);

  } catch (const Transport::Exception&) {
    ACE_ERROR((LM_ERROR, "(%P|%t) InfoRepoDiscovery::init_bit, "
                         "exception during transport initialization\n"));
    return 0;
  }

  // DataReaders
  try {
    DDS::DataReaderQos participantReaderQos;
    bit_subscriber->get_default_datareader_qos(participantReaderQos);
    participantReaderQos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    if (participant->federated()) {
      participantReaderQos.liveliness.lease_duration.nanosec = 0;
      participantReaderQos.liveliness.lease_duration.sec =
        TheServiceParticipant->federation_liveliness();
    }

    DDS::TopicDescription_var bit_part_topic =
      participant->lookup_topicdescription(BUILT_IN_PARTICIPANT_TOPIC);

    DDS::DataReader_var dr =
      bit_subscriber->create_datareader(bit_part_topic,
                                        participantReaderQos,
                                        DDS::DataReaderListener::_nil(),
                                        DEFAULT_STATUS_MASK);

    if (participant->federated()) {
      DDS::ParticipantBuiltinTopicDataDataReader* pbit_dr =
        DDS::ParticipantBuiltinTopicDataDataReader::_narrow(dr.in());

      // Create and attach the listener.
      failoverListener_ = new FailoverListener(key());
      pbit_dr->set_listener(failoverListener_, DEFAULT_STATUS_MASK);
    }

    DDS::DataReaderQos dr_qos;
    bit_subscriber->get_default_datareader_qos(dr_qos);
    dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    DDS::TopicDescription_var bit_topic_topic =
      participant->lookup_topicdescription(BUILT_IN_TOPIC_TOPIC);

    dr = bit_subscriber->create_datareader(bit_topic_topic,
                                           dr_qos,
                                           DDS::DataReaderListener::_nil(),
                                           DEFAULT_STATUS_MASK);

    DDS::TopicDescription_var bit_pub_topic =
      participant->lookup_topicdescription(BUILT_IN_PUBLICATION_TOPIC);

    dr = bit_subscriber->create_datareader(bit_pub_topic,
                                           dr_qos,
                                           DDS::DataReaderListener::_nil(),
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    DDS::TopicDescription_var bit_sub_topic =
      participant->lookup_topicdescription(BUILT_IN_SUBSCRIPTION_TOPIC);

    dr = bit_subscriber->create_datareader(bit_sub_topic,
                                           dr_qos,
                                           DDS::DataReaderListener::_nil(),
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  } catch (const CORBA::Exception&) {
    ACE_ERROR((LM_ERROR, "(%P|%t) InfoRepoDiscovery::init_bit, "
                         "exception during DataReader initialization\n"));
    return 0;
  }
  return bit_subscriber._retn();
#endif
}

RepoId
InfoRepoDiscovery::bit_key_to_repo_id(DomainParticipantImpl* /*participant*/,
                                      const char* /*bit_topic_name*/,
                                      const DDS::BuiltinTopicKey_t& key) const
{
  RepoId id = RepoIdBuilder::create();
  RepoIdBuilder builder(id);
  builder.federationId(key.value[0]);
  builder.participantId(key.value[1]);
  builder.entityId(key.value[2]);
  return id;
}

bool
InfoRepoDiscovery::active()
{
  return (!get_dcps_info()->_is_a("Not_An_IDL_Type"));
}

// Participant operations:

bool
InfoRepoDiscovery::attach_participant(DDS::DomainId_t domainId,
                                      const RepoId& participantId)
{
  return get_dcps_info()->attach_participant(domainId, participantId);
}

DCPS::AddDomainStatus
InfoRepoDiscovery::add_domain_participant(DDS::DomainId_t domainId,
                                          const DDS::DomainParticipantQos& qos)
{
  return get_dcps_info()->add_domain_participant(domainId, qos);
}

void
InfoRepoDiscovery::remove_domain_participant(DDS::DomainId_t domainId,
                                             const RepoId& participantId)
{
  get_dcps_info()->remove_domain_participant(domainId, participantId);
}

void
InfoRepoDiscovery::ignore_domain_participant(DDS::DomainId_t domainId,
                                             const RepoId& myParticipantId,
                                             const RepoId& ignoreId)
{
  get_dcps_info()->ignore_domain_participant(domainId, myParticipantId, ignoreId);
}

bool
InfoRepoDiscovery::update_domain_participant_qos(DDS::DomainId_t domainId,
                                                 const RepoId& participant,
                                                 const DDS::DomainParticipantQos& qos)
{
  return get_dcps_info()->update_domain_participant_qos(domainId, participant, qos);
}

// Topic operations:

DCPS::TopicStatus
InfoRepoDiscovery::assert_topic(DCPS::RepoId_out topicId, DDS::DomainId_t domainId,
                                const RepoId& participantId, const char* topicName,
                                const char* dataTypeName, const DDS::TopicQos& qos,
                                bool hasDcpsKey)
{
  return get_dcps_info()->assert_topic(topicId, domainId, participantId, topicName,
    dataTypeName, qos, hasDcpsKey);
}

DCPS::TopicStatus
InfoRepoDiscovery::find_topic(DDS::DomainId_t domainId, const char* topicName,
                              CORBA::String_out dataTypeName, DDS::TopicQos_out qos,
                              DCPS::RepoId_out topicId)
{
  return get_dcps_info()->find_topic(domainId, topicName, dataTypeName, qos, topicId);
}

DCPS::TopicStatus
InfoRepoDiscovery::remove_topic(DDS::DomainId_t domainId, const RepoId& participantId,
                                const RepoId& topicId)
{
  return get_dcps_info()->remove_topic(domainId, participantId, topicId);
}

void
InfoRepoDiscovery::ignore_topic(DDS::DomainId_t domainId, const RepoId& myParticipantId,
                                const RepoId& ignoreId)
{
  get_dcps_info()->ignore_topic(domainId, myParticipantId, ignoreId);
}

bool
InfoRepoDiscovery::update_topic_qos(const RepoId& topicId, DDS::DomainId_t domainId,
                                    const RepoId& participantId, const DDS::TopicQos& qos)
{
  return get_dcps_info()->update_topic_qos(topicId, domainId, participantId, qos);
}


// Publication operations:

RepoId
InfoRepoDiscovery::add_publication(DDS::DomainId_t domainId,
                                   const RepoId& participantId,
                                   const RepoId& topicId,
                                   DCPS::DataWriterRemote_ptr publication,
                                   const DDS::DataWriterQos& qos,
                                   const DCPS::TransportLocatorSeq& transInfo,
                                   const DDS::PublisherQos& publisherQos)
{
  return get_dcps_info()->add_publication(domainId, participantId, topicId,
    publication, qos, transInfo, publisherQos);
}

void
InfoRepoDiscovery::remove_publication(DDS::DomainId_t domainId,
                                      const RepoId& participantId,
                                      const RepoId& publicationId)
{
  get_dcps_info()->remove_publication(domainId, participantId, publicationId);
}

void
InfoRepoDiscovery::ignore_publication(DDS::DomainId_t domainId,
                                      const RepoId& participantId,
                                      const RepoId& ignoreId)
{
  get_dcps_info()->ignore_publication(domainId, participantId, ignoreId);
}

bool
InfoRepoDiscovery::update_publication_qos(DDS::DomainId_t domainId,
                                          const RepoId& participantId,
                                          const RepoId& dwId,
                                          const DDS::DataWriterQos& qos,
                                          const DDS::PublisherQos& publisherQos)
{
  return get_dcps_info()->update_publication_qos(domainId, participantId, dwId, qos,
    publisherQos);
}


// Subscription operations:

RepoId
InfoRepoDiscovery::add_subscription(DDS::DomainId_t domainId,
                                    const RepoId& participantId,
                                    const RepoId& topicId,
                                    DCPS::DataReaderRemote_ptr subscription,
                                    const DDS::DataReaderQos& qos,
                                    const DCPS::TransportLocatorSeq& transInfo,
                                    const DDS::SubscriberQos& subscriberQos,
                                    const char* filterExpr,
                                    const DDS::StringSeq& params)
{
  return get_dcps_info()->add_subscription(domainId, participantId, topicId, subscription,
    qos, transInfo, subscriberQos, filterExpr, params);
}

void
InfoRepoDiscovery::remove_subscription(DDS::DomainId_t domainId,
                                       const RepoId& participantId,
                                       const RepoId& subscriptionId)
{
  get_dcps_info()->remove_subscription(domainId, participantId, subscriptionId);
}

void
InfoRepoDiscovery::ignore_subscription(DDS::DomainId_t domainId,
                                       const RepoId& participantId,
                                       const RepoId& ignoreId)
{
  get_dcps_info()->ignore_subscription(domainId, participantId, ignoreId);
}

bool
InfoRepoDiscovery::update_subscription_qos(DDS::DomainId_t domainId,
                                           const RepoId& participantId,
                                           const RepoId& drId,
                                           const DDS::DataReaderQos& qos,
                                           const DDS::SubscriberQos& subQos)
{
  return get_dcps_info()->update_subscription_qos(domainId, participantId, drId,
    qos, subQos);
}

bool
InfoRepoDiscovery::update_subscription_params(DDS::DomainId_t domainId,
                                              const RepoId& participantId,
                                              const RepoId& subId,
                                              const DDS::StringSeq& params)

{
  return get_dcps_info()->update_subscription_params(domainId, participantId, subId,
    params);
}


// Managing reader/writer associations:

void
InfoRepoDiscovery::association_complete(DDS::DomainId_t domainId,
                                        const RepoId& participantId,
                                        const RepoId& localId, const RepoId& remoteId)
{
  get_dcps_info()->association_complete(domainId, participantId, localId, remoteId);
}

void
InfoRepoDiscovery::disassociate_participant(DDS::DomainId_t domainId,
                                            const RepoId& localId,
                                            const RepoId& remoteId)
{
  get_dcps_info()->disassociate_participant(domainId, localId, remoteId);
}

void
InfoRepoDiscovery::disassociate_subscription(DDS::DomainId_t domainId,
                                             const RepoId& participantId,
                                             const RepoId& localId, const RepoId& remoteId)
{
  get_dcps_info()->disassociate_subscription(domainId, participantId, localId, remoteId);
}

void
InfoRepoDiscovery::disassociate_publication(DDS::DomainId_t domainId,
                                            const RepoId& participantId,
                                            const RepoId& localId, const RepoId& remoteId)
{
  get_dcps_info()->disassociate_publication(domainId, participantId, localId, remoteId);
}

void
InfoRepoDiscovery::shutdown()
{
  get_dcps_info()->shutdown();
}

namespace {
  const ACE_TCHAR REPO_SECTION_NAME[] = ACE_TEXT("repository");
}

int
InfoRepoDiscovery::Config::discovery_config(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key repo_sect;

  if (cf.open_section(root, REPO_SECTION_NAME, 0, repo_sect) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any repository (sub)section. The code default configuration will be used.
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: InfoRepoDiscovery::Config::discovery_config ")
                 ACE_TEXT("failed to open [%s] section.\n"),
                 REPO_SECTION_NAME));
    }

    return 0;

  } else {
    // Ensure there are no properties in this section
    ValueMap vm;
    if (pullValues(cf, repo_sect, vm) > 0) {
      // There are values inside [repo]
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) InfoRepoDiscovery::Config::discovery_config ")
                        ACE_TEXT("repo sections must have a subsection name\n")),
                       -1);
    }
    // Process the subsections of this section (the individual repos)
    KeyList keys;
    if (processSections( cf, repo_sect, keys ) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) InfoRepoDiscovery::Config::discovery_config ")
                        ACE_TEXT("too many nesting layers in the [repo] section.\n")),
                       -1);
    }

    // Loop through the [repo/*] sections
    for (KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
      std::string repo_name = (*it).first;

      ValueMap values;
      pullValues( cf, (*it).second, values );
      Discovery::RepoKey repoKey = Discovery::DEFAULT_REPO;
      bool repoKeySpecified = false, bitIpSpecified = false,
        bitPortSpecified = false;
      std::string repoIor;
      int bitPort = 0;
      std::string bitIp;
      for (ValueMap::const_iterator it=values.begin(); it != values.end(); ++it) {
        std::string name = (*it).first;
        if (name == "RepositoryKey") {
          repoKey = (*it).second;
          repoKeySpecified = true;
          if (DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) [repository/%C]: RepositoryKey == %C\n"),
                       repo_name.c_str(), repoKey.c_str()));
          }

        } else if (name == "RepositoryIor") {
          repoIor = (*it).second;

          if (DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) [repository/%C]: RepositoryIor == %C\n"),
                       repo_name.c_str(), repoIor.c_str()));
          }
        } else if (name == "DCPSBitTransportIPAddress") {
          bitIp = (*it).second;
          bitIpSpecified = true;
          if (DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) [repository/%C]: DCPSBitTransportIPAddress == %C\n"),
                       repo_name.c_str(), bitIp.c_str()));
          }
        } else if (name == "DCPSBitTransportPort") {
          std::string value = (*it).second;
          bitPort = ACE_OS::atoi(value.c_str());
          bitPortSpecified = true;
          if (convertToInteger(value, bitPort)) {
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) InfoRepoDiscovery::Config::discovery_config ")
                              ACE_TEXT("Illegal integer value for DCPSBitTransportPort (%C) in [repository/%C] section.\n"),
                              value.c_str(), repo_name.c_str()),
                             -1);
          }
          if (DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) [repository/%C]: DCPSBitTransportPort == %d\n"),
                       repo_name.c_str(), bitPort));
          }
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) InfoRepoDiscovery::Config::discovery_config ")
                            ACE_TEXT("Unexpected entry (%C) in [repository/%C] section.\n"),
                            name.c_str(), repo_name.c_str()),
                           -1);
        }
      }

      if (values.find("RepositoryIor") == values.end()) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) InfoRepoDiscovery::Config::discovery_config ")
                          ACE_TEXT("Repository section [repository/%C] section is missing RepositoryIor value.\n"),
                          repo_name.c_str()),
                         -1);
      }

      if (!repoKeySpecified) {
        // If the RepositoryKey option was not specified, use the section
        // name as the repo key
        repoKey = repo_name;
      }
      InfoRepoDiscovery_rch discovery =
        new InfoRepoDiscovery(repoKey, repoIor.c_str());
      if (bitPortSpecified) discovery->bit_transport_port(bitPort);
      if (bitIpSpecified) discovery->bit_transport_ip(bitIp);
      TheServiceParticipant->add_discovery(
        DCPS::static_rchandle_cast<Discovery>(discovery));
    }
  }

  return 0;
}

void
InfoRepoDiscovery::OrbRunner::shutdown()
{
  orb_->shutdown();
  wait();
}

InfoRepoDiscovery::OrbRunner* InfoRepoDiscovery::orb_runner_;
ACE_Thread_Mutex InfoRepoDiscovery::mtx_orb_runner_;

int
InfoRepoDiscovery::OrbRunner::svc()
{
  // this method was originally Service_Participant::svc()
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
        "ERROR: InfoRepoDiscovery::OrbRunner");

    } catch (const CORBA::UserException& userex) {
      userex._tao_print_exception(
        "ERROR: InfoRepoDiscovery::OrbRunner");

    } catch (const CORBA::Exception& ex) {
      ex._tao_print_exception(
        "ERROR: InfoRepoDiscovery::OrbRunner");
    }

    if (orb_->orb_core()->has_shutdown()) {
      done = true;

    } else {
      orb_->orb_core()->reactor()->reset_reactor_event_loop();
    }
  }

  return 0;
}


InfoRepoDiscovery::StaticInitializer::StaticInitializer()
{
  TheServiceParticipant->register_discovery_type("repository", new Config);
}

int
IRDiscoveryLoader::init(int, ACE_TCHAR*[])
{
  // no-op: since the library is loaded, InfoRepoDiscovery::StaticInitializer
  // has already been constructed.
  return 0;
}

ACE_FACTORY_DEFINE(OpenDDS_InfoRepoDiscovery, IRDiscoveryLoader);
ACE_STATIC_SVC_DEFINE(
  IRDiscoveryLoader,
  ACE_TEXT("OpenDDS_InfoRepoDiscovery"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME(IRDiscoveryLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0)

} // namespace DCPS
} // namespace OpenDDS
