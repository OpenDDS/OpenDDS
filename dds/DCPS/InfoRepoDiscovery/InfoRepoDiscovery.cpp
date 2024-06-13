/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "InfoRepoDiscovery.h"

#include "DataReaderRemoteImpl.h"
#include "DataWriterRemoteC.h"
#include "DataWriterRemoteImpl.h"
#include "FailoverListener.h"

#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DCPS_Utils.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportType.h"
#include "dds/DCPS/transport/framework/TransportType_rch.h"

#include "tao/ORB_Core.h"
#include "tao/BiDir_GIOP/BiDirGIOP.h"
#include "ace/Reactor.h"

#include <dds/OpenDDSConfigWrapper.h>

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Marked_Default_Qos.h"

#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/TransportInst.h"
#include "dds/DCPS/transport/tcp/TcpInst.h"
#include "dds/DCPS/transport/tcp/TcpInst_rch.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#endif

namespace {
const char ROOT_POA[] = "RootPOA";
const char BIDIR_POA[] = "BiDirPOA";

struct DestroyPolicy {
  explicit DestroyPolicy(const CORBA::Policy_ptr& p)
    : p_(CORBA::Policy::_duplicate(p)) {}

  ~DestroyPolicy() { p_->destroy(); }

  CORBA::Policy_var p_;
};

PortableServer::POA_ptr get_POA(CORBA::ORB_ptr orb, bool use_bidir_giop)
{
  CORBA::Object_var obj =
    orb->resolve_initial_references(ROOT_POA);
  PortableServer::POA_var root_poa = PortableServer::POA::_narrow(obj.in());

  if (use_bidir_giop) {
    while (true) {
      try {
        return root_poa->find_POA(BIDIR_POA, false /*activate*/);
      } catch (const PortableServer::POA::AdapterNonExistent&) {
        // go ahead and create it...
      }
      CORBA::PolicyList policies(1);
      policies.length(1);
      CORBA::Any policy;
      policy <<= BiDirPolicy::BOTH;
      policies[0] =
        orb->create_policy(BiDirPolicy::BIDIRECTIONAL_POLICY_TYPE, policy);
      DestroyPolicy destroy(policies[0]);
      PortableServer::POAManager_var manager = root_poa->the_POAManager();
      try {
        return root_poa->create_POA(BIDIR_POA, manager, policies);
      } catch (const PortableServer::POA::AdapterAlreadyExists&) {
        // another thread created it, try to find it again
      }
    }
  }

  return root_poa._retn();
}

/// Get a servant pointer given an object reference.
/// @throws PortableServer::POA::ObjectNotActive
///         PortableServer::POA::WrongAdapter
///         PortableServer::POA::WrongPolicy
template <class T_impl, class T_ptr>
T_impl* remote_reference_to_servant(T_ptr p, CORBA::ORB_ptr orb, bool use_bidir_giop)
{
  if (CORBA::is_nil(p)) {
    return 0;
  }

  PortableServer::POA_var poa = get_POA(orb, use_bidir_giop);

  T_impl* the_servant =
    dynamic_cast<T_impl*>(poa->reference_to_servant(p));

  // Use the ServantBase_var so that the servant's reference
  // count will not be changed by this operation.
  PortableServer::ServantBase_var servant = the_servant;

  return the_servant;
}

/// Given a servant, return the remote object reference from the local POA.
/// @throws PortableServer::POA::ServantNotActive,
///         PortableServer::POA::WrongPolicy
template <class T>
typename T::_stub_ptr_type servant_to_remote_reference(T* servant, CORBA::ORB_ptr orb, bool use_bidir_giop)
{
  PortableServer::POA_var poa = get_POA(orb, use_bidir_giop);
  PortableServer::ObjectId_var oid = poa->activate_object(servant);
  CORBA::Object_var obj = poa->id_to_reference(oid.in());

  typename T::_stub_ptr_type the_obj = T::_stub_type::_narrow(obj.in());
  return the_obj;
}

template <class T>
void deactivate_remote_object(T obj, CORBA::ORB_ptr orb, bool use_bidir_giop)
{
  PortableServer::POA_var poa = get_POA(orb, use_bidir_giop);
  PortableServer::ObjectId_var oid =
    poa->reference_to_id(obj);
  poa->deactivate_object(oid.in());
}

}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

InfoRepoDiscovery::InfoRepoDiscovery(const String& name)
  : name_(name)
  , config_prefix_(ConfigPair::canonicalize("REPOSITORY_" + name))
  , use_bidir_giop_(TheServiceParticipant->use_bidir_giop())
  , orb_from_user_(false)
  , config_store_(make_rch<ConfigStoreImpl>(TheServiceParticipant->config_topic()))
{
  init_bidir_giop();
}

InfoRepoDiscovery::InfoRepoDiscovery(const String& name,
                                     const DCPSInfo_var& info)
  : name_(name)
  , config_prefix_(ConfigPair::canonicalize("REPOSITORY_" + name))
  , info_(info)
  , use_bidir_giop_(TheServiceParticipant->use_bidir_giop())
  , orb_from_user_(false)
  , config_store_(make_rch<ConfigStoreImpl>(TheServiceParticipant->config_topic()))
{
  init_bidir_giop();
}

void
InfoRepoDiscovery::init_bidir_giop()
{
  if (use_bidir_giop_) {
    ACE_Service_Object* const bidir_loader =
      ACE_Dynamic_Service<ACE_Service_Object>::instance(ACE_Service_Config::current(),
                                                        "BiDirGIOP_Loader");

    if (bidir_loader != 0) {
      bidir_loader->init(0, 0);
    }
  }
}

InfoRepoDiscovery::~InfoRepoDiscovery()
{
  if (!orb_from_user_ && orb_runner_) {
    if (0 == --orb_runner_->use_count_) {
      try {
        orb_runner_->shutdown();
      }
      catch (const CORBA::Exception& ex) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("ERROR: InfoRepoDiscovery::~InfoRepoDiscovery - ")
          ACE_TEXT("Exception caught during ORB shutdown: %C.\n"),
          ex._info().c_str()));
      }

      delete orb_runner_;
      orb_runner_ = 0;
    }
  }
}

bool
InfoRepoDiscovery::set_ORB(CORBA::ORB_ptr orb)
{
  if (!CORBA::is_nil (orb_.in()) || CORBA::is_nil (orb)) {
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
  ACE_Guard<ACE_Thread_Mutex> guard(lock_);
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
          orb_runner_->orb_->resolve_initial_references(ROOT_POA);
        PortableServer::POA_var poa = PortableServer::POA::_narrow(rp);
        PortableServer::POAManager_var poa_manager = poa->the_POAManager();
        poa_manager->activate();
      } else {
        ++orb_runner_->use_count_;
      }
      orb_ = orb_runner_->orb_;
    }

    try {
      const String ior_str = ior();
      if (ior_str.empty()) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::get_dcps_info: ")
                   ACE_TEXT("ior is empty for key %C.\n"),
                   this->key().c_str()));
        return DCPSInfo::_nil();
      }

      this->info_ = get_repo(ior_str.c_str(), orb_);
      if (CORBA::is_nil(this->info_.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::get_dcps_info: ")
                   ACE_TEXT("unable to narrow DCPSInfo (%C) for key %C.\n"),
                   ior_str.c_str(),
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
  return ior();
}

String
InfoRepoDiscovery::ior() const
{
  return TheServiceParticipant->config_store()->get(config_key("RepositoryIor").c_str(),
                                                    (key() == Discovery::DEFAULT_REPO) ? TheServiceParticipant->config_store()->get(COMMON_DCPS_INFO_REPO, "file://repo.ior") : "");
}

TransportConfig_rch
InfoRepoDiscovery::bit_config()
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  ACE_Guard<ACE_Thread_Mutex> guard(lock_);
  if (bit_config_.is_nil()) {
    const std::string cfg_name = TransportRegistry::DEFAULT_INST_PREFIX +
                                 std::string("_BITTransportConfig_") + key();
    bit_config_ = TransportRegistry::instance()->create_config(cfg_name);

    const std::string inst_name = TransportRegistry::DEFAULT_INST_PREFIX +
                                  std::string("_BITTCPTransportInst_") + key();
    TransportInst_rch inst =
      TransportRegistry::instance()->create_inst(inst_name, "tcp");
    bit_config_->instances_.push_back(inst);

    // Use a static cast to avoid dependency on the Tcp library
    TcpInst_rch tcp_inst = static_rchandle_cast<TcpInst>(inst);
    config_store_->set_int32(tcp_inst->config_key("DATALINK_RELEASE_DELAY").c_str(), 0);
    const int port = bit_transport_port();
    const String ip = bit_transport_ip();
    if (!ip.empty()) {
      config_store_->set(tcp_inst->config_key("LOCAL_ADDRESS").c_str(),
                         ip + ":" + to_dds_string(port));
    } else {
      String addr = tcp_inst->local_address();
      tcp_inst->set_port_in_addr_string(addr, port);
      config_store_->set(tcp_inst->config_key("LOCAL_ADDRESS").c_str(), addr);
    }

    if (DCPS_debug_level) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) InfoRepoDiscovery::bit_config")
                 ACE_TEXT(" - BIT tcp transport %C\n"), tcp_inst->local_address().c_str()));
    }
  }
  return bit_config_;
#else
  return TransportConfig_rch();
#endif
}

RcHandle<BitSubscriber>
InfoRepoDiscovery::init_bit(DomainParticipantImpl* participant)
{
#if defined (DDS_HAS_MINIMUM_BIT)
  ACE_UNUSED_ARG(participant);
  return RcHandle<BitSubscriber>();
#else
  if (!TheServiceParticipant->get_BIT()) {
    return RcHandle<BitSubscriber>();
  }

  if (create_bit_topics(participant) != DDS::RETCODE_OK) {
    return RcHandle<BitSubscriber>();
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
    return RcHandle<BitSubscriber>();
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
      DDS::ParticipantBuiltinTopicDataDataReader_var pbit_dr =
        DDS::ParticipantBuiltinTopicDataDataReader::_narrow(dr.in());

      DataReaderListener_var failover = new FailoverListener(key());
      pbit_dr->set_listener(failover, DEFAULT_STATUS_MASK);
      // No need to invoke the listener.
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

    const DDS::ReturnCode_t ret = bit_subscriber->enable();
    if (ret != DDS::RETCODE_OK) {
      if (DCPS_debug_level) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) InfoRepoDiscovery::init_bit")
                   ACE_TEXT(" - Error <%C> enabling subscriber\n"), retcode_to_string(ret)));
      }
      return RcHandle<BitSubscriber>();
    }

  } catch (const CORBA::Exception&) {
    ACE_ERROR((LM_ERROR, "(%P|%t) InfoRepoDiscovery::init_bit, "
                         "exception during DataReader initialization\n"));
    return RcHandle<BitSubscriber>();
  }
  return make_rch<BitSubscriber>(bit_subscriber);
#endif
}

void
InfoRepoDiscovery::fini_bit(DCPS::DomainParticipantImpl* /* participant */)
{
  // nothing to do for DCPSInfoRepo
}

Discovery::RepoKey
InfoRepoDiscovery::key() const
{
  return TheServiceParticipant->config_store()->get(config_key("RepositoryKey").c_str(), name_);
}

bool
InfoRepoDiscovery::active()
{
  try {
    // invoke a CORBA call, if we are active then there will be no exception
    get_dcps_info()->_is_a("Not_An_IDL_Type");
    return true;
  } catch (const CORBA::Exception&) {
    return false;
  }
}

int
InfoRepoDiscovery::bit_transport_port() const
{
  return TheServiceParticipant->config_store()->get_int32(config_key("DCPSBitTransportPort").c_str(),
                                                          TheServiceParticipant->bit_transport_port());
}

void
InfoRepoDiscovery::bit_transport_port(int port)
{
  TheServiceParticipant->config_store()->set_int32(config_key("DCPSBitTransportPort").c_str(), port);
}

String
InfoRepoDiscovery::bit_transport_ip() const
{
  return TheServiceParticipant->config_store()->get(config_key("DCPSBitTransportIPAddress").c_str(),
                                                    TheServiceParticipant->bit_transport_ip());
}

void
InfoRepoDiscovery::bit_transport_ip(const String& ip)
{
  return TheServiceParticipant->config_store()->set(config_key("DCPSBitTransportIPAddress").c_str(), ip);
}

// Participant operations:

bool
InfoRepoDiscovery::attach_participant(DDS::DomainId_t domainId,
                                      const GUID_t& participantId)
{
  try {
    return get_dcps_info()->attach_participant(domainId, participantId);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::attach_participant: ");
    return false;
  }
}

OpenDDS::DCPS::GUID_t
InfoRepoDiscovery::generate_participant_guid()
{
  return GUID_UNKNOWN;
}

DCPS::AddDomainStatus
InfoRepoDiscovery::add_domain_participant(DDS::DomainId_t domainId,
                                          const DDS::DomainParticipantQos& qos,
                                          XTypes::TypeLookupService_rch /*tls*/)
{
  try {
    const DCPSInfo_var info = get_dcps_info();
    if (!CORBA::is_nil(info)) {
      return info->add_domain_participant(domainId, qos);
    }
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::add_domain_participant: ");
  }
  const DCPS::AddDomainStatus ads = {OpenDDS::DCPS::GUID_UNKNOWN, false /*federated*/};
  return ads;
}

#if OPENDDS_CONFIG_SECURITY
DCPS::AddDomainStatus
InfoRepoDiscovery::add_domain_participant_secure(
  DDS::DomainId_t /*domain*/,
  const DDS::DomainParticipantQos& /*qos*/,
  XTypes::TypeLookupService_rch /*tls*/,
  const OpenDDS::DCPS::GUID_t& /*guid*/,
  DDS::Security::IdentityHandle /*id*/,
  DDS::Security::PermissionsHandle /*perm*/,
  DDS::Security::ParticipantCryptoHandle /*part_crypto*/)
{
  const DCPS::AddDomainStatus ads = {OpenDDS::DCPS::GUID_UNKNOWN, false /*federated*/};
  return ads;
}
#endif

bool
InfoRepoDiscovery::remove_domain_participant(DDS::DomainId_t domainId,
                                             const GUID_t& participantId)
{
  try {
    get_dcps_info()->remove_domain_participant(domainId, participantId);
    return true;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::remove_domain_participant: ");
    return false;
  }
}

bool
InfoRepoDiscovery::ignore_domain_participant(DDS::DomainId_t domainId,
                                             const GUID_t& myParticipantId,
                                             const GUID_t& ignoreId)
{
  try {
    get_dcps_info()->ignore_domain_participant(domainId, myParticipantId, ignoreId);
    return true;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::ignore_domain_participant: ");
    return false;
  }
}

bool
InfoRepoDiscovery::update_domain_participant_qos(DDS::DomainId_t domainId,
                                                 const GUID_t& participant,
                                                 const DDS::DomainParticipantQos& qos)
{
  try {
    return get_dcps_info()->update_domain_participant_qos(domainId, participant, qos);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::update_domain_participant_qos: ");
    return false;
  }
}

// Topic operations:

DCPS::TopicStatus
InfoRepoDiscovery::assert_topic(DCPS::GUID_t_out topicId, DDS::DomainId_t domainId,
                                const GUID_t& participantId, const char* topicName,
                                const char* dataTypeName, const DDS::TopicQos& qos,
                                bool hasDcpsKey, TopicCallbacks* /*topic_callbacks*/)
{
  try {
    return get_dcps_info()->assert_topic(topicId, domainId, participantId, topicName,
      dataTypeName, qos, hasDcpsKey);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::assert_topic: ");
    return DCPS::INTERNAL_ERROR;
  }
}

DCPS::TopicStatus
InfoRepoDiscovery::find_topic(DDS::DomainId_t domainId,
                              const DCPS::GUID_t& /*participantId*/,
                              const char* topicName,
                              CORBA::String_out dataTypeName,
                              DDS::TopicQos_out qos,
                              DCPS::GUID_t_out topicId)
{
  try {
    return get_dcps_info()->find_topic(domainId, topicName, dataTypeName, qos, topicId);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::find_topic: ");
    return DCPS::INTERNAL_ERROR;
  }
}

DCPS::TopicStatus
InfoRepoDiscovery::remove_topic(DDS::DomainId_t domainId, const GUID_t& participantId,
                                const GUID_t& topicId)
{
  try {
    return get_dcps_info()->remove_topic(domainId, participantId, topicId);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::remove_topic: ");
    return DCPS::INTERNAL_ERROR;
  }
}

bool
InfoRepoDiscovery::ignore_topic(DDS::DomainId_t domainId, const GUID_t& myParticipantId,
                                const GUID_t& ignoreId)
{
  try {
    get_dcps_info()->ignore_topic(domainId, myParticipantId, ignoreId);
    return true;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::ignore_topic: ");
    return false;
  }
}

bool
InfoRepoDiscovery::update_topic_qos(const GUID_t& topicId, DDS::DomainId_t domainId,
                                    const GUID_t& participantId, const DDS::TopicQos& qos)
{
  try {
    return get_dcps_info()->update_topic_qos(topicId, domainId, participantId, qos);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::update_topic_qos: ");
    return false;
  }
}


// Publication operations:

bool
InfoRepoDiscovery::add_publication(DDS::DomainId_t domainId,
                                   const GUID_t& participantId,
                                   const GUID_t& topicId,
                                   DCPS::DataWriterCallbacks_rch publication,
                                   const DDS::DataWriterQos& qos,
                                   const DCPS::TransportLocatorSeq& transInfo,
                                   const DDS::PublisherQos& publisherQos,
                                   const XTypes::TypeInformation& type_info)
{


  try {
    DCPS::DataWriterRemoteImpl* writer_remote_impl = 0;
    ACE_NEW_RETURN(writer_remote_impl,
                   DataWriterRemoteImpl(*publication),
                   false);

    //this is taking ownership of the DataWriterRemoteImpl (server side) allocated above
    PortableServer::ServantBase_var writer_remote(writer_remote_impl);

    //this is the client reference to the DataWriterRemoteImpl
    OpenDDS::DCPS::DataWriterRemote_var dr_remote_obj =
      servant_to_remote_reference(writer_remote_impl, orb_, use_bidir_giop_);
    //turn into a octet seq to pass through generated files
    DDS::OctetSeq serializedTypeInfo;
    XTypes::serialize_type_info(type_info, serializedTypeInfo);

    const GUID_t pubId = get_dcps_info()->reserve_publication_id(domainId, participantId, topicId);
    publication->set_publication_id(pubId);

    if (!get_dcps_info()->add_publication(domainId,
                                          participantId,
                                          topicId,
                                          pubId,
                                          dr_remote_obj,
                                          qos,
                                          transInfo,
                                          publisherQos,
                                          serializedTypeInfo)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::add_publication: ")
                 ACE_TEXT("failed to add publication\n")));
      return false;
    }

    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, this->lock_, false);
    // take ownership of the client allocated above
    dataWriterMap_[pubId] = dr_remote_obj;
    return true;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::add_publication: ");
    return false;
  }
}

bool
InfoRepoDiscovery::remove_publication(DDS::DomainId_t domainId,
                                      const GUID_t& participantId,
                                      const GUID_t& publicationId)
{
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, this->lock_, false);
    removeDataWriterRemote(publicationId);
  }
  bool removed = false;
  try {
    get_dcps_info()->remove_publication(domainId, participantId, publicationId);
    removed = true;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::remove_publication: ");
  }

  return removed;
}

bool
InfoRepoDiscovery::ignore_publication(DDS::DomainId_t domainId,
                                      const GUID_t& participantId,
                                      const GUID_t& ignoreId)
{
  try {
    get_dcps_info()->ignore_publication(domainId, participantId, ignoreId);
    return true;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::ignore_publication: ");
    return false;
  }
}

bool
InfoRepoDiscovery::update_publication_qos(DDS::DomainId_t domainId,
                                          const GUID_t& participantId,
                                          const GUID_t& dwId,
                                          const DDS::DataWriterQos& qos,
                                          const DDS::PublisherQos& publisherQos)
{
  try {
    return get_dcps_info()->update_publication_qos(domainId, participantId, dwId,
      qos, publisherQos);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::update_publication_qos: ");
    return false;
  }
}


// Subscription operations:

bool
InfoRepoDiscovery::add_subscription(DDS::DomainId_t domainId,
                                    const GUID_t& participantId,
                                    const GUID_t& topicId,
                                    DCPS::DataReaderCallbacks_rch subscription,
                                    const DDS::DataReaderQos& qos,
                                    const DCPS::TransportLocatorSeq& transInfo,
                                    const DDS::SubscriberQos& subscriberQos,
                                    const char* filterClassName,
                                    const char* filterExpr,
                                    const DDS::StringSeq& params,
                                    const XTypes::TypeInformation& type_info)
{
  try {
    DCPS::DataReaderRemoteImpl* reader_remote_impl = 0;
    ACE_NEW_RETURN(reader_remote_impl,
                   DataReaderRemoteImpl(*subscription),
                   false);

    //this is taking ownership of the DataReaderRemoteImpl (server side) allocated above
    PortableServer::ServantBase_var reader_remote(reader_remote_impl);

    //this is the client reference to the DataReaderRemoteImpl
    OpenDDS::DCPS::DataReaderRemote_var dr_remote_obj =
      servant_to_remote_reference(reader_remote_impl, orb_, use_bidir_giop_);
    //turn into a octet seq to pass through generated files
    DDS::OctetSeq serializedTypeInfo;
    XTypes::serialize_type_info(type_info, serializedTypeInfo);

    const GUID_t subId = get_dcps_info()->reserve_subscription_id(domainId, participantId, topicId);
    subscription->set_subscription_id(subId);

    if (!get_dcps_info()->add_subscription(domainId,
                                           participantId,
                                           topicId,
                                           subId,
                                           dr_remote_obj,
                                           qos,
                                           transInfo,
                                           subscriberQos,
                                           filterClassName,
                                           filterExpr,
                                           params,
                                           serializedTypeInfo)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::add_subscription: ")
                 ACE_TEXT("failed to add subscription\n")));
      return false;
    }

    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, this->lock_, false);
    // take ownership of the client allocated above
    dataReaderMap_[subId] = dr_remote_obj;
    return true;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::add_subscription: ");
    return false;
  }
}

bool
InfoRepoDiscovery::remove_subscription(DDS::DomainId_t domainId,
                                       const GUID_t& participantId,
                                       const GUID_t& subscriptionId)
{
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, this->lock_, false);
    removeDataReaderRemote(subscriptionId);
  }
  bool removed = false;
  try {
    get_dcps_info()->remove_subscription(domainId, participantId, subscriptionId);
    removed = true;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::remove_subscription: ");
  }

  return removed;
}

bool
InfoRepoDiscovery::ignore_subscription(DDS::DomainId_t domainId,
                                       const GUID_t& participantId,
                                       const GUID_t& ignoreId)
{
  try {
    get_dcps_info()->ignore_subscription(domainId, participantId, ignoreId);
    return true;
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::ignore_subscription: ");
    return false;
  }
}

bool
InfoRepoDiscovery::update_subscription_qos(DDS::DomainId_t domainId,
                                           const GUID_t& participantId,
                                           const GUID_t& drId,
                                           const DDS::DataReaderQos& qos,
                                           const DDS::SubscriberQos& subQos)
{
  try {
    return get_dcps_info()->update_subscription_qos(domainId, participantId,
      drId, qos, subQos);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::update_subscription_qos: ");
    return false;
  }
}

bool
InfoRepoDiscovery::update_subscription_params(DDS::DomainId_t domainId,
                                              const GUID_t& participantId,
                                              const GUID_t& subId,
                                              const DDS::StringSeq& params)

{
  try {
    return get_dcps_info()->update_subscription_params(domainId, participantId,
      subId, params);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: InfoRepoDiscovery::update_subscription_params: ");
    return false;
  }
}


// Managing reader/writer associations:

void
InfoRepoDiscovery::removeDataReaderRemote(const GUID_t& subscriptionId)
{
  DataReaderMap::iterator drr = dataReaderMap_.find(subscriptionId);
  if (drr == dataReaderMap_.end()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::removeDataReaderRemote: ")
               ACE_TEXT(" could not find DataReader for subscriptionId.\n")));
    return;
  }

  try {
    DataReaderRemoteImpl* impl =
      remote_reference_to_servant<DataReaderRemoteImpl>(drr->second.in(), orb_, use_bidir_giop_);
    impl->detach_parent();
    deactivate_remote_object(drr->second.in(), orb_, use_bidir_giop_);
  } catch (const CORBA::BAD_INV_ORDER&) {
    // The orb may throw ::CORBA::BAD_INV_ORDER when is has been shutdown.
    // Ignore it anyway.
  } catch (const CORBA::OBJECT_NOT_EXIST&) {
    // Same for CORBA::OBJECT_NOT_EXIST
  }

  dataReaderMap_.erase(drr);
}

void
InfoRepoDiscovery::removeDataWriterRemote(const GUID_t& publicationId)
{
  DataWriterMap::iterator dwr = dataWriterMap_.find(publicationId);
  if (dwr == dataWriterMap_.end()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::removeDataWriterRemote: ")
               ACE_TEXT(" could not find DataWriter for publicationId.\n")));
    return;
  }

  try {
    DataWriterRemoteImpl* impl =
      remote_reference_to_servant<DataWriterRemoteImpl>(dwr->second.in(), orb_, use_bidir_giop_);
    impl->detach_parent();
    deactivate_remote_object(dwr->second.in(), orb_, use_bidir_giop_);
  } catch (const CORBA::BAD_INV_ORDER&) {
    // The orb may throw ::CORBA::BAD_INV_ORDER when is has been shutdown.
    // Ignore it anyway.
  } catch (const CORBA::OBJECT_NOT_EXIST&) {
    // Same for CORBA::OBJECT_NOT_EXIST
  }

  dataWriterMap_.erase(dwr);
}

int
InfoRepoDiscovery::Config::discovery_config()
{
  const Service_Participant::RepoKeyDiscoveryMap& discoveryMap = TheServiceParticipant->discoveryMap();

  typedef OPENDDS_VECTOR(String) VecType;
  const VecType cseq = TheServiceParticipant->config_store()->get_section_names("REPOSITORY");
  for (VecType::const_iterator pos = cseq.begin(), limit = cseq.end(); pos != limit; ++pos) {
    if (discoveryMap.find(*pos) == discoveryMap.end()) {
      InfoRepoDiscovery_rch discovery(make_rch<InfoRepoDiscovery>(*pos));
      TheServiceParticipant->add_discovery(DCPS::static_rchandle_cast<Discovery>(discovery));
    }
  }

  return 0;
}

void
InfoRepoDiscovery::OrbRunner::shutdown()
{
  orb_->shutdown();
  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  {
    ThreadStatusManager::Sleeper s(thread_status_manager);
    wait();
  }
  orb_->destroy();
}

InfoRepoDiscovery::OrbRunner* InfoRepoDiscovery::orb_runner_;
ACE_Thread_Mutex InfoRepoDiscovery::mtx_orb_runner_;

int
InfoRepoDiscovery::OrbRunner::svc()
{
  ThreadStatusManager::Start s(TheServiceParticipant->get_thread_status_manager(), "OrbRunner");

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

class InfoRepoType : public TransportType {
public:
  const char* name() { return "repository"; }

  TransportInst_rch new_inst(const std::string&,
                             bool)
  {
    return TransportInst_rch();
  }
};

InfoRepoDiscovery::StaticInitializer::StaticInitializer()
{
  TransportRegistry* registry = TheTransportRegistry;
  if (!registry->register_type(make_rch<InfoRepoType>())) {
    return;
  }
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL
