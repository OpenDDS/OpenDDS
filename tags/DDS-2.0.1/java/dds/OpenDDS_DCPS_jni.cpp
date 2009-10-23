/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "OpenDDS_DCPS_TheParticipantFactory.h"
#include "OpenDDS_DCPS_TheServiceParticipant.h"
#include "OpenDDS_DCPS_transport_TheTransportFactory.h"
#include "OpenDDS_DCPS_transport_TransportImpl.h"
#include "OpenDDS_DCPS_transport_ReliableMulticastConfiguration.h"
#include "OpenDDS_DCPS_transport_SimpleMcastConfiguration.h"
#include "OpenDDS_DCPS_transport_SimpleTcpConfiguration.h"
#include "OpenDDS_DCPS_transport_SimpleUnreliableDgramConfiguration.h"
#include "DDS_WaitSet.h"
#include "DDS_GuardCondition.h"

#include "idl2jni_runtime.h"
#include "OpenDDS_jni_helpers.h"

#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/PerConnectionSynchStrategy.h"
#include "dds/DCPS/transport/framework/PoolSynchStrategy.h"
#include "dds/DCPS/transport/framework/NullSynchStrategy.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticastTransportConfiguration.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUnreliableDgramConfiguration.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleMcastConfiguration.h"

#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/GuardCondition.h"

#include "DdsDcpsDomainJC.h"
#include "DdsDcpsPublicationJC.h"
#include "DdsDcpsSubscriptionJC.h"

#include "ace/Service_Config.h"
#include "ace/Service_Repository.h"

// TheParticipantFactory

jobject JNICALL Java_OpenDDS_DCPS_TheParticipantFactory_WithArgs(JNIEnv *jni,
                                                                 jclass, jobject ssholder)
{
  JniArgv jargv(jni, ssholder);

  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(jargv.argc_, jargv.orb_argv());
    jobject j_dpf;
    copyToJava(jni, j_dpf, dpf, true);
    return j_dpf;

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;
  }
}

jobject JNICALL Java_OpenDDS_DCPS_TheParticipantFactory_getInstance(JNIEnv *jni, jclass)
{
  try {
    jobject j_dpf;
    copyToJava(jni, j_dpf, TheParticipantFactory, true);
    return j_dpf;

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;
  }
}

// TheServiceParticipant

void JNICALL Java_OpenDDS_DCPS_TheServiceParticipant_shutdown(JNIEnv *, jclass)
{
  TheServiceParticipant->shutdown();
}

jint JNICALL Java_OpenDDS_DCPS_TheServiceParticipant_domain_1to_1repo
(JNIEnv *, jclass, jint domain)
{
  OpenDDS::DCPS::Service_Participant::RepoKey key =
    TheServiceParticipant->domain_to_repo(domain);
  return static_cast<jint>(key);
}

void JNICALL Java_OpenDDS_DCPS_TheServiceParticipant_set_1repo_1domain
(JNIEnv *, jclass, jint domain, jint repo)
{
  TheServiceParticipant->set_repo_domain(domain, repo);
}

void JNICALL Java_OpenDDS_DCPS_TheServiceParticipant_set_1repo_1ior
(JNIEnv *envp, jclass, jstring ior, jint repo)
{
  JStringMgr jsm(envp, ior);

  try {
    TheServiceParticipant->set_repo_ior(jsm.c_str(), repo);

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(envp, se);
  }
}

// Exception translation

#define TRANSPORT_EXCEPTION(NAME)                                       \
  if (dynamic_cast<const NAME *>(&te))                                  \
  {                                                                     \
    jclass clazz =                                                      \
      findClass (jni, "OpenDDS/DCPS/transport/" #NAME "Exception");     \
    jni->ThrowNew (clazz, "OpenDDS transport exception: " #NAME);       \
  }                                                                     \
  else

namespace {

void throw_java_exception(JNIEnv *jni,
                          const OpenDDS::DCPS::Transport::Exception &te)
{
  using namespace OpenDDS::DCPS::Transport;
  TRANSPORT_EXCEPTION(NotFound)
  TRANSPORT_EXCEPTION(Duplicate)
  TRANSPORT_EXCEPTION(UnableToCreate)
  TRANSPORT_EXCEPTION(MiscProblem)
  TRANSPORT_EXCEPTION(NotConfigured)
  TRANSPORT_EXCEPTION(ConfigurationConflict)
  // fallback case from if/else chain
  {
    jclass clazz =
      findClass(jni, "OpenDDS/DCPS/transport/TransportException");
    jni->ThrowNew(clazz, "OpenDDS transport exception (unknown type)");
  }
}

} // namespace

// TheTransportFactory

jobject JNICALL
Java_OpenDDS_DCPS_transport_TheTransportFactory_create_1transport_1impl__IZ
(JNIEnv *jni, jclass, jint id, jboolean auto_configure)
{
  try {
    OpenDDS::DCPS::TransportImpl_rch transport =
      TheTransportFactory->create_transport_impl(id, auto_configure);
    jclass implClazz =
      findClass(jni, "OpenDDS/DCPS/transport/TransportImpl");
    jmethodID ctor = jni->GetMethodID(implClazz, "<init>", "(J)V");
    return jni->NewObject(implClazz, ctor,
                          reinterpret_cast<jlong>(transport._retn()));

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
    return 0;
  }
}

jobject JNICALL
Java_OpenDDS_DCPS_transport_TheTransportFactory_create_1transport_1impl__ILjava_lang_String_2Z
(JNIEnv *jni, jclass, jint id, jstring type, jboolean auto_configure)
{
  JStringMgr jsm(jni, type);
  ACE_TString typeCxx(ACE_TEXT_CHAR_TO_TCHAR(jsm.c_str()));

  try {
    OpenDDS::DCPS::TransportImpl_rch transport =
      TheTransportFactory->create_transport_impl(id, typeCxx,
                                                 auto_configure);
    jclass implClazz =
      findClass(jni, "OpenDDS/DCPS/transport/TransportImpl");
    jmethodID ctor = jni->GetMethodID(implClazz, "<init>", "(J)V");
    return jni->NewObject(implClazz, ctor,
                          reinterpret_cast<jlong>(transport._retn()));

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
    return 0;
  }
}

// Descriptions of Configuration object mapping
//   (TheTransportFactory continues below)

namespace BaseConfig {

typedef OpenDDS::DCPS::TransportConfiguration Cfg;
const char *jclassName = "OpenDDS/DCPS/transport/TransportConfiguration";

// sendThreadStrategy is written as a special case
const char *strategySig =
  "LOpenDDS/DCPS/transport/TransportConfiguration$ThreadSynchStrategy;";
const char *strategyClass =
  "OpenDDS/DCPS/transport/TransportConfiguration$ThreadSynchStrategy";

BoolField<Cfg> sb("swapBytes", &Cfg::swap_bytes_);
SizetField<Cfg> qmpp("queueMessagesPerPool", &Cfg::queue_messages_per_pool_);
SizetField<Cfg> qip("queueInitialPools", &Cfg::queue_initial_pools_);
Uint32Field<Cfg> mps("maxPacketSize", &Cfg::max_packet_size_);
SizetField<Cfg> mspp("maxSamplesPerPacket", &Cfg::max_samples_per_packet_);
Uint32Field<Cfg> ops("optimumPacketSize", &Cfg::optimum_packet_size_);
BoolField<Cfg> tpc("threadPerConnection", &Cfg::thread_per_connection_);

BaseField<Cfg> *fields[] = {&sb, &qmpp, &qip, &mps, &mspp, &ops, &tpc};

} // namespace BaseConfig

namespace TcpConfig {

typedef OpenDDS::DCPS::SimpleTcpConfiguration Tcp;
const char *jclassName = "OpenDDS/DCPS/transport/SimpleTcpConfiguration";
const ACE_TCHAR *configName = ACE_TEXT("SimpleTcp");

const ACE_TCHAR *svcName = ACE_TEXT("DCPS_SimpleTcpLoader");
const ACE_TCHAR *svcConfDir =
  ACE_TEXT("dynamic DCPS_SimpleTcpLoader Service_Object * ")
  ACE_TEXT("SimpleTcp:_make_DCPS_SimpleTcpLoader() \"-type SimpleTcp\"");

InetAddrField<Tcp> la("localAddress",
                      &Tcp::local_address_str_, &Tcp::local_address_);
BoolField<Tcp> ena("enableNagleAlgorithm",
                   &Tcp::enable_nagle_algorithm_);
IntField<Tcp> crid("connRetryInitialDelay",
                   &Tcp::conn_retry_initial_delay_);
DoubleField<Tcp> crbm("connRetryBackoffMultiplier",
                      &Tcp::conn_retry_backoff_multiplier_);
IntField<Tcp> cra("connRetryAttempts",
                  &Tcp::conn_retry_attempts_);
IntField<Tcp> mopp("maxOutputPausePeriod",
                   &Tcp::max_output_pause_period_);
IntField<Tcp> prd("passiveReconnectDuration",
                  &Tcp::passive_reconnect_duration_);
UlongField<Tcp> pcd("passiveConnectDuration",
                    &Tcp::passive_connect_duration_);

BaseField<Tcp> *fields[] = {&la, &ena, &crid, &crbm, &cra, &mopp, &prd, &pcd};

} // namespace TcpConfig

namespace RMcastConfig {

typedef OpenDDS::DCPS::ReliableMulticastTransportConfiguration RMcast;
const char *jclassName =
  "OpenDDS/DCPS/transport/ReliableMulticastConfiguration";
const ACE_TCHAR *configName = ACE_TEXT("ReliableMulticast");

const ACE_TCHAR *svcName = ACE_TEXT("OPENDDS_DCPS_ReliableMulticastLoader");
const ACE_TCHAR *svcConfDir =
  ACE_TEXT("dynamic OPENDDS_DCPS_ReliableMulticastLoader Service_Object * ")
  ACE_TEXT("ReliableMulticast:_make_OPENDDS_DCPS_ReliableMulticastLoader()")
  ACE_TEXT(" \"\"");

InetAddrField<RMcast> la("localAddress",
                         &RMcast::local_address_str_, &RMcast::local_address_);
InetAddrField<RMcast> mga("multicastGroupAddress",
                          &RMcast::multicast_group_address_str_, &RMcast::multicast_group_address_);
BoolField<RMcast> r("receiver", &RMcast::receiver_);
SizetField<RMcast> shs("senderHistorySize", &RMcast::sender_history_size_);
SizetField<RMcast> rbs("receiverBufferSize", &RMcast::receiver_buffer_size_);

BaseField<RMcast> *fields[] = {&la, &mga, &r, &shs, &rbs};

} // namespace RMcastConfig

namespace UdpConfig {

typedef OpenDDS::DCPS::SimpleUnreliableDgramConfiguration Udp;
const char *jclassName =
  "OpenDDS/DCPS/transport/SimpleUnreliableDgramConfiguration";

const char *jclassNameUdp = "OpenDDS/DCPS/transport/SimpleUdpConfiguration";
const ACE_TCHAR *configName = ACE_TEXT("SimpleUdp");

const ACE_TCHAR *svcName =
  ACE_TEXT("OPENDDS_DCPS_SimpleUnreliableDgramLoader");
const ACE_TCHAR *svcConfDir =
  ACE_TEXT("dynamic OPENDDS_DCPS_SimpleUnreliableDgramLoader Service_Object")
  ACE_TEXT(" * SimpleUnreliableDgram:")
  ACE_TEXT("_make_OPENDDS_DCPS_SimpleUnreliableDgramLoader() ")
  ACE_TEXT("\"-type SimpleUdp -type SimpleMcast\"");

InetAddrField<Udp> la("localAddress",
                      &Udp::local_address_str_, &Udp::local_address_);
IntField<Udp> mopp("maxOutputPausePeriod", &Udp::max_output_pause_period_);

BaseField<Udp> *fields[] = {&la, &mopp};

} // namespace UdpConfig

namespace McastConfig {

typedef OpenDDS::DCPS::SimpleMcastConfiguration Mcast;
const char *jclassName = "OpenDDS/DCPS/transport/SimpleMcastConfiguration";
const ACE_TCHAR *configName = ACE_TEXT("SimpleMcast");

const ACE_TCHAR *svcName = UdpConfig::svcName;
const ACE_TCHAR *svcConfDir = UdpConfig::svcConfDir;

InetAddrField<Mcast> mga("multicastGroupAddress",
                         &Mcast::multicast_group_address_str_, &Mcast::multicast_group_address_);
BoolField<Mcast> r("receiver", &Mcast::receiver_);

BaseField<Mcast> *fields[] = {&mga, &r};

} // namespace McastConfig

namespace {

void loadLibIfNeeded(const ACE_TCHAR *svcname, const ACE_TCHAR *svcconf)
{
  ACE_Service_Gestalt *asg = ACE_Service_Config::current();

  if (asg->find(svcname) == -1 /*not found*/) {
    ACE_Service_Config::process_directive(svcconf);
  }
}

} // namespace

jobject get_or_create_impl(JNIEnv *jni, jint id, const ACE_TString &typeCxx)
{
  using namespace OpenDDS::DCPS;

  jclass clazz_specific = 0;

  if (typeCxx == TcpConfig::configName) {
    clazz_specific = findClass(jni, TcpConfig::jclassName);
    loadLibIfNeeded(TcpConfig::svcName, TcpConfig::svcConfDir);

  } else if (typeCxx == RMcastConfig::configName) {
    clazz_specific = findClass(jni, RMcastConfig::jclassName);
    loadLibIfNeeded(RMcastConfig::svcName, RMcastConfig::svcConfDir);

  } else if (typeCxx == UdpConfig::configName) {
    clazz_specific = findClass(jni, UdpConfig::jclassNameUdp);
    loadLibIfNeeded(UdpConfig::svcName, UdpConfig::svcConfDir);

  } else if (typeCxx == McastConfig::configName) {
    clazz_specific = findClass(jni, McastConfig::jclassName);
    loadLibIfNeeded(McastConfig::svcName, McastConfig::svcConfDir);
  }

  //unknown type, let OpenDDS find out and throw its exception from get_or_...

  TransportConfiguration_rch tc =
    TheTransportFactory->get_or_create_configuration(id, typeCxx);

  jmethodID mid_ctor = jni->GetMethodID(clazz_specific, "<init>", "(I)V");
  jobject jConf = jni->NewObject(clazz_specific, mid_ctor, id);

  // Configure common properties (TransportConfiguration)
  jclass clazz_tc = findClass(jni, BaseConfig::jclassName);
  toJava(jni, clazz_tc, *tc.in(), jConf, BaseConfig::fields);
  //   private ThreadSynchStrategy sendThreadStrategy;
  {
    ThreadSynchStrategy *tss = tc->send_thread_strategy();
    jclass enumClazz = findClass(jni, BaseConfig::strategyClass);
    jfieldID enumFid;

    if (dynamic_cast<PerConnectionSynchStrategy *>(tss)) {
      enumFid = jni->GetStaticFieldID(enumClazz, "PER_CONNECTION_SYNCH",
                                      BaseConfig::strategySig);

    } else if (dynamic_cast<PoolSynchStrategy *>(tss)) {
      enumFid = jni->GetStaticFieldID(enumClazz, "POOL_SYNCH",
                                      BaseConfig::strategySig);

    } else {
      enumFid = jni->GetStaticFieldID(enumClazz, "NULL_SYNCH",
                                      BaseConfig::strategySig);
    }

    jfieldID fid = jni->GetFieldID(clazz_tc, "sendThreadStrategy",
                                   BaseConfig::strategySig);
    jobject enumVal = jni->GetStaticObjectField(enumClazz, enumFid);
    jni->SetObjectField(jConf, fid, enumVal);
    jni->DeleteLocalRef(enumVal);
  }

  // Configure transport-specific properties
  jmethodID mid_cfg = jni->GetMethodID(clazz_tc, "loadSpecificConfig", "(J)V");
  jni->CallVoidMethod(jConf, mid_cfg, reinterpret_cast<jlong>(tc.in()));

  return jConf;
}

jobject JNICALL
Java_OpenDDS_DCPS_transport_TheTransportFactory_get_1or_1create_1configuration
(JNIEnv *jni, jclass, jint id, jstring type)
{
  JStringMgr jsm(jni, type);
  ACE_TString typeCxx(ACE_TEXT_CHAR_TO_TCHAR(jsm.c_str()));

  try {
    return get_or_create_impl(jni, id, typeCxx);

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
    return 0;
  }
}

void JNICALL Java_OpenDDS_DCPS_transport_TheTransportFactory_release__
(JNIEnv *, jclass)
{
  TheTransportFactory->release();
}

void JNICALL Java_OpenDDS_DCPS_transport_TheTransportFactory_release__I
(JNIEnv *, jclass, jint id)
{
  TheTransportFactory->release(id);
}

// TransportImpl

namespace {

OpenDDS::DCPS::TransportImpl *recoverTransportImpl(JNIEnv *jni,
                                                   jobject jThis)
{
  jclass thisClass = jni->GetObjectClass(jThis);
  jfieldID jptr = jni->GetFieldID(thisClass, "_jni_pointer", "J");
  jlong ptr = jni->GetLongField(jThis, jptr);
  return reinterpret_cast<OpenDDS::DCPS::TransportImpl *>(ptr);
}

} // namespace

void JNICALL Java_OpenDDS_DCPS_transport_TransportImpl__1jni_1fini
(JNIEnv *jni, jobject jThis)
{
  OpenDDS::DCPS::TransportImpl *ti = recoverTransportImpl(jni, jThis);
  ti->_remove_ref();
}

jobject JNICALL Java_OpenDDS_DCPS_transport_TransportImpl_attach_1to_1publisher
(JNIEnv *jni, jobject jThis, jobject jPub)
{
  OpenDDS::DCPS::TransportImpl *ti = recoverTransportImpl(jni, jThis);
  DDS::Publisher_var pub;
  copyToCxx(jni, pub, jPub);
  OpenDDS::DCPS::PublisherImpl *pub_impl =
    dynamic_cast<OpenDDS::DCPS::PublisherImpl *>(pub.in());
  OpenDDS::DCPS::AttachStatus stat = pub_impl->attach_transport(ti);
  jclass clazz = findClass(jni, "OpenDDS/DCPS/transport/AttachStatus");
  jmethodID mid = jni->GetStaticMethodID(clazz, "from_int",
                                         "(I)LOpenDDS/DCPS/transport/AttachStatus;");
  return jni->CallStaticObjectMethod(clazz, mid, static_cast<jint>(stat));
}

jobject JNICALL Java_OpenDDS_DCPS_transport_TransportImpl_attach_1to_1subscriber
(JNIEnv *jni, jobject jThis, jobject jSub)
{
  OpenDDS::DCPS::TransportImpl *ti = recoverTransportImpl(jni, jThis);
  DDS::Subscriber_var sub;
  copyToCxx(jni, sub, jSub);
  OpenDDS::DCPS::SubscriberImpl *sub_impl =
    dynamic_cast<OpenDDS::DCPS::SubscriberImpl *>(sub.in());
  OpenDDS::DCPS::AttachStatus stat = sub_impl->attach_transport(ti);
  jclass clazz = findClass(jni, "OpenDDS/DCPS/transport/AttachStatus");
  jmethodID mid = jni->GetStaticMethodID(clazz, "from_int",
                                         "(I)LOpenDDS/DCPS/transport/AttachStatus;");
  return jni->CallStaticObjectMethod(clazz, mid, static_cast<jint>(stat));
}

jint config_impl(JNIEnv *jni, jobject jThis, jobject jConf)
{
  using namespace OpenDDS::DCPS;
  TransportImpl *ti = recoverTransportImpl(jni, jThis);

  jclass clazz_tc = findClass(jni, BaseConfig::jclassName);
  jfieldID fid_id = jni->GetFieldID(clazz_tc, "id", "I");
  jint id = jni->GetIntField(jConf, fid_id);
  TransportConfiguration_rch tc =
    TheTransportFactory->get_configuration(id);

  // Configure common properties (TransportConfiguration)
  toCxx(jni, clazz_tc, jConf, *tc.in(), BaseConfig::fields);
  //   private ThreadSynchStrategy sendThreadStrategy;
  {
    jfieldID fid = jni->GetFieldID(clazz_tc, "sendThreadStrategy",
                                   BaseConfig::strategySig);
    jobject enumVal = jni->GetObjectField(jConf, fid);
    jclass enumClazz = findClass(jni, BaseConfig::strategyClass);
    jfieldID fid_pc = jni->GetStaticFieldID(enumClazz, "PER_CONNECTION_SYNCH",
                                            BaseConfig::strategySig);
    jobject perConnection = jni->GetStaticObjectField(enumClazz, fid_pc);

    if (jni->IsSameObject(enumVal, perConnection)) {
      tc->send_thread_strategy(new PerConnectionSynchStrategy);

    } else {
      jfieldID fid_ps = jni->GetStaticFieldID(enumClazz, "POOL_SYNCH",
                                              BaseConfig::strategySig);
      jobject poolSynch = jni->GetStaticObjectField(enumClazz, fid_ps);
      tc->send_thread_strategy(
        jni->IsSameObject(enumVal, poolSynch)
        ? static_cast<ThreadSynchStrategy *>(new PoolSynchStrategy)
        : static_cast<ThreadSynchStrategy *>(new NullSynchStrategy));
      jni->DeleteLocalRef(poolSynch);
    }

    jni->DeleteLocalRef(perConnection);
    jni->DeleteLocalRef(enumVal);
  }

  // Configure transport-specific properties
  jmethodID mid_cfg = jni->GetMethodID(clazz_tc, "saveSpecificConfig", "(J)V");
  jni->CallVoidMethod(jConf, mid_cfg, reinterpret_cast<jlong>(tc.in()));

  return ti->configure(tc.in());
}

jint JNICALL Java_OpenDDS_DCPS_transport_TransportImpl_configure
(JNIEnv *jni, jobject jThis, jobject jConf)
{
  try {
    return config_impl(jni, jThis, jConf);

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
    return 0;
  }
}

namespace {

template <typename T>
void narrowTransportConfig(T *&transportConfig, jlong javaPtr)
{
  OpenDDS::DCPS::TransportConfiguration *tc =
    reinterpret_cast<OpenDDS::DCPS::TransportConfiguration *>(javaPtr);
  transportConfig = static_cast<T *>(tc);
}

} // namespace

// SimpleTcpConfiguration

void JNICALL
Java_OpenDDS_DCPS_transport_SimpleTcpConfiguration_saveSpecificConfig
(JNIEnv *jni, jobject jThis, jlong ptr)
{
  OpenDDS::DCPS::SimpleTcpConfiguration *tc;
  narrowTransportConfig(tc, ptr);
  jclass clazz = findClass(jni, TcpConfig::jclassName);
  toCxx(jni, clazz, jThis, *tc, TcpConfig::fields);
}

void JNICALL
Java_OpenDDS_DCPS_transport_SimpleTcpConfiguration_loadSpecificConfig
(JNIEnv *jni, jobject jThis, jlong ptr)
{
  OpenDDS::DCPS::SimpleTcpConfiguration *tc;
  narrowTransportConfig(tc, ptr);
  jclass clazz = findClass(jni, TcpConfig::jclassName);
  toJava(jni, clazz, *tc, jThis, TcpConfig::fields);
}

// ReliableMulticastConfiguration

void JNICALL
Java_OpenDDS_DCPS_transport_ReliableMulticastConfiguration_saveSpecificConfig
(JNIEnv *jni, jobject jThis, jlong ptr)
{
  OpenDDS::DCPS::ReliableMulticastTransportConfiguration *tc;
  narrowTransportConfig(tc, ptr);
  jclass clazz = findClass(jni, RMcastConfig::jclassName);
  toCxx(jni, clazz, jThis, *tc, RMcastConfig::fields);
}

void JNICALL
Java_OpenDDS_DCPS_transport_ReliableMulticastConfiguration_loadSpecificConfig
(JNIEnv *jni, jobject jThis, jlong ptr)
{
  OpenDDS::DCPS::ReliableMulticastTransportConfiguration *tc;
  narrowTransportConfig(tc, ptr);
  jclass clazz = findClass(jni, RMcastConfig::jclassName);
  toJava(jni, clazz, *tc, jThis, RMcastConfig::fields);
}

// SimpleUnreliableDgramConfiguration

void JNICALL
Java_OpenDDS_DCPS_transport_SimpleUnreliableDgramConfiguration_saveSpecificConfig
(JNIEnv *jni, jobject jThis, jlong ptr)
{
  OpenDDS::DCPS::SimpleUnreliableDgramConfiguration *tc;
  narrowTransportConfig(tc, ptr);
  jclass clazz = findClass(jni, UdpConfig::jclassName);
  toCxx(jni, clazz, jThis, *tc, UdpConfig::fields);
}

void JNICALL
Java_OpenDDS_DCPS_transport_SimpleUnreliableDgramConfiguration_loadSpecificConfig
(JNIEnv *jni, jobject jThis, jlong ptr)
{
  OpenDDS::DCPS::SimpleUnreliableDgramConfiguration *tc;
  narrowTransportConfig(tc, ptr);
  jclass clazz = findClass(jni, UdpConfig::jclassName);
  toJava(jni, clazz, *tc, jThis, UdpConfig::fields);
}

// SimpleMcastConfiguration

void JNICALL
Java_OpenDDS_DCPS_transport_SimpleMcastConfiguration_saveMcastConfig
(JNIEnv *jni, jobject jThis, jlong ptr)
{
  OpenDDS::DCPS::SimpleMcastConfiguration *tc;
  narrowTransportConfig(tc, ptr);
  jclass clazz = findClass(jni, McastConfig::jclassName);
  toCxx(jni, clazz, jThis, *tc, McastConfig::fields);
}

void JNICALL
Java_OpenDDS_DCPS_transport_SimpleMcastConfiguration_loadMcastConfig
(JNIEnv *jni, jobject jThis, jlong ptr)
{
  OpenDDS::DCPS::SimpleMcastConfiguration *tc;
  narrowTransportConfig(tc, ptr);
  jclass clazz = findClass(jni, McastConfig::jclassName);
  toJava(jni, clazz, *tc, jThis, McastConfig::fields);
}

// WaitSet and GuardCondition

jlong JNICALL Java_DDS_WaitSet__1jni_1init(JNIEnv *, jclass)
{
  return reinterpret_cast<jlong>(static_cast<CORBA::Object_ptr>(
                                 new DDS::WaitSet));
}

jlong JNICALL Java_DDS_GuardCondition__1jni_1init(JNIEnv *, jclass)
{
  return reinterpret_cast<jlong>(static_cast<CORBA::Object_ptr>(
                                 new DDS::GuardCondition));
}
