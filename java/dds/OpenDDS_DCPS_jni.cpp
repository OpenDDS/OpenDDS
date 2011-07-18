/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "OpenDDS_DCPS_TheParticipantFactory.h"
#include "OpenDDS_DCPS_TheServiceParticipant.h"
#include "OpenDDS_DCPS_transport_TheTransportRegistry.h"
#include "OpenDDS_DCPS_transport_TransportConfig.h"
#include "OpenDDS_DCPS_transport_TcpInst.h"
#include "OpenDDS_DCPS_transport_UdpInst.h"
#include "OpenDDS_DCPS_transport_MulticastInst.h"
#include "OpenDDS_DCPS_transport_TransportInst.h"
#include "DDS_WaitSet.h"
#include "DDS_GuardCondition.h"

#include "idl2jni_runtime.h"
#include "OpenDDS_jni_helpers.h"

#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/PerConnectionSynchStrategy.h"
#include "dds/DCPS/transport/framework/PoolSynchStrategy.h"
#include "dds/DCPS/transport/framework/NullSynchStrategy.h"
#include "dds/DCPS/transport/tcp/TcpInst.h"
#include "dds/DCPS/transport/udp/UdpInst.h"
#include "dds/DCPS/transport/multicast/MulticastInst.h"
#include "dds/DCPS/transport/tcp/TcpInst_rch.h"
#include "dds/DCPS/transport/udp/UdpInst_rch.h"
#include "dds/DCPS/transport/multicast/MulticastInst_rch.h"
#include "dds/DCPS/transport/framework/TransportInst_rch.h"

#include "dds/DCPS/EntityImpl.h"
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

// Helper functions for transport configuration

template <typename CppClass>
CppClass* recoverCppObj(JNIEnv *jni, jobject jThis)
{
  jclass thisClass = jni->GetObjectClass(jThis);
  jfieldID jptr = jni->GetFieldID(thisClass, "_jni_pointer", "J");
  jlong ptr = jni->GetLongField(jThis, jptr);
  return reinterpret_cast<CppClass*>(ptr);
}

jobject constructTransportInst(JNIEnv *jni,
                               OpenDDS::DCPS::TransportInst_rch inst)
{
    jclass instClazz;
    if (inst->transport_type_ == "tcp") {
      instClazz = findClass(jni, "OpenDDS/DCPS/transport/TcpInst");
    } else if (inst->transport_type_ == "udp") {
      instClazz = findClass(jni, "OpenDDS/DCPS/transport/UdpInst");
    } else if (inst->transport_type_ == "multicast") {
      instClazz = findClass(jni, "OpenDDS/DCPS/transport/MulticastInst");
    }
    jmethodID ctor = jni->GetMethodID(instClazz, "<init>", "(J)V");
    return jni->NewObject(instClazz, ctor,
                          reinterpret_cast<jlong>(inst._retn()));
}

// TheTransportRegistry

// TheTransportRegistry::create_inst
jobject JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_create_1inst
(JNIEnv * jni, jclass, jstring name, jstring transport_type)
{
  JStringMgr jsm_name(jni, name);
  JStringMgr jsm_tt(jni, transport_type);

  try {
    OpenDDS::DCPS::TransportInst_rch inst =
      TheTransportRegistry->create_inst(jsm_name.c_str(), jsm_tt.c_str());
    return constructTransportInst(jni, inst);
  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
    return 0;
  }
}

// TheTransportRegistry::get_inst
jobject JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_get_1inst
(JNIEnv * jni, jclass, jstring name)
{
  JStringMgr jsm_name(jni, name);

  try {
    OpenDDS::DCPS::TransportInst_rch inst =
      TheTransportRegistry->get_inst(jsm_name.c_str());
    return constructTransportInst(jni, inst);
  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
    return 0;
  }
}

// TheTransportRegistry::remove_inst
void JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_remove_1inst
(JNIEnv * jni, jclass, jobject jobj)
{
  try {
    OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jobj),
                                          false);  // Don't take ownership
    if (inst != 0) {
      TheTransportRegistry->remove_inst(inst);
    }
  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
  }
}

// TheTransportRegistry::create_config
jobject JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_create_1config
(JNIEnv * jni, jclass, jstring name)
{
  JStringMgr jsm_name(jni, name);

  try {
    OpenDDS::DCPS::TransportConfig_rch config =
      TheTransportRegistry->create_config(jsm_name.c_str());
    jclass configClazz =
      findClass(jni, "OpenDDS/DCPS/transport/TransportConfig");
    jmethodID ctor = jni->GetMethodID(configClazz, "<init>", "(J)V");
    return jni->NewObject(configClazz, ctor,
                          reinterpret_cast<jlong>(config._retn()));

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
    return 0;
  }
}

// TheTransportRegistry::get_config
jobject JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_get_1config
(JNIEnv * jni, jclass, jstring name)
{
  JStringMgr jsm_name(jni, name);

  try {
    OpenDDS::DCPS::TransportConfig_rch config =
      TheTransportRegistry->get_config(jsm_name.c_str());
    jclass configClazz =
      findClass(jni, "OpenDDS/DCPS/transport/TransportConfig");
    jmethodID ctor = jni->GetMethodID(configClazz, "<init>", "(J)V");
    return jni->NewObject(configClazz, ctor,
                          reinterpret_cast<jlong>(config._retn()));

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
    return 0;
  }
}

// TheTransportRegistry::remove_config
void JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_remove_1config
(JNIEnv * jni, jclass, jobject jobj)
{
  try {
    OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, jobj),
                                              false);  // Don't take ownership
    if (config != 0) {
      TheTransportRegistry->remove_config(config);
    }
  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
  }
}

// TheTransportRegistry::global_config
jobject JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_global_1config__
(JNIEnv * jni, jclass)
{
  try {
    OpenDDS::DCPS::TransportConfig_rch config =
      TheTransportRegistry->global_config();
    jclass configClazz =
      findClass(jni, "OpenDDS/DCPS/transport/TransportConfig");
    jmethodID ctor = jni->GetMethodID(configClazz, "<init>", "(J)V");
    return jni->NewObject(configClazz, ctor,
                          reinterpret_cast<jlong>(config._retn()));

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
    return 0;

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
    return 0;
  }
}

// TheTransportRegistry::global_config
void JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_global_1config__LOpenDDS_DCPS_transport_TransportConfig_2
(JNIEnv * jni, jclass, jobject jobj)
{
  try {
    OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, jobj),
                                              false); // Don't take ownership
    if (config != 0) {
      TheTransportRegistry->global_config(config);
    }
  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
  }
}

// TheTransportRegistry::bind_config
void JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_bind_1config__Ljava_lang_String_2LDDS_Entity_2
(JNIEnv * jni, jclass, jstring name, jobject entity_jobj)
{
  JStringMgr jsm_name(jni, name);

  try {
    DDS::Entity_var entity;
    copyToCxx(jni, entity, entity_jobj);

    TheTransportRegistry->bind_config(jsm_name.c_str(), entity.in());

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
  }
}

// TheTransportRegistry::bind_config
void JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_bind_1config__LOpenDDS_DCPS_transport_TransportConfig_2LDDS_Entity_2
(JNIEnv * jni, jclass, jobject config_jobj, jobject entity_jobj)
{
  try {
    OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, config_jobj),
                                              false); // Don't take ownership

    DDS::Entity_var entity;
    copyToCxx(jni, entity, entity_jobj);

    TheTransportRegistry->bind_config(config, entity.in());

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);

  } catch (const OpenDDS::DCPS::Transport::Exception &te) {
    throw_java_exception(jni, te);
  }
}

// TheTransportRegistry::release
void JNICALL Java_OpenDDS_DCPS_transport_TheTransportRegistry_release
(JNIEnv *, jclass)
{
  TheTransportRegistry->release();
}

// TransportConfig

// TransportConfig::_jni_fini
void JNICALL Java_OpenDDS_DCPS_transport_TransportConfig__1jni_1fini
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, jthis),
                                            false); // Don't take ownership
  config->_remove_ref();
}

// TransportConfig::getName
jstring JNICALL Java_OpenDDS_DCPS_transport_TransportConfig_getName
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, jthis),
                                            false); // Don't take ownership
  std::string name = config->name();
  jstring retStr = jni->NewStringUTF(name.c_str());
  return retStr;
}

// TransportConfig::addLast
void JNICALL Java_OpenDDS_DCPS_transport_TransportConfig_addLast
(JNIEnv * jni, jobject jthis, jobject inst_jobj)
{
  OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, jthis),
                                            false); // Don't take ownership
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, inst_jobj),
                                        false); // Don't take ownership
  config->instances_.push_back(inst);
}

// TransportConfig::countInstances
jlong JNICALL Java_OpenDDS_DCPS_transport_TransportConfig_countInstances
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, jthis),
                                            false); // Don't take ownership
  return config->instances_.size();
}

// TransportConfig::getInstance
jobject JNICALL Java_OpenDDS_DCPS_transport_TransportConfig_getInstance
(JNIEnv * jni, jobject jthis, jlong index)
{
  OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, jthis),
                                            false); // Don't take ownership
  if (index < config->instances_.size()) {
    OpenDDS::DCPS::TransportInst_rch inst = config->instances_[index];
    return constructTransportInst(jni, inst);
  }
  return 0;
}

// TransportConfig::getSwapBytes
jboolean JNICALL Java_OpenDDS_DCPS_transport_TransportConfig_getSwapBytes
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, jthis),
                                            false); // Don't take ownership
  return config->swap_bytes_;
}

// TransportConfig::setSwapBytes
void JNICALL Java_OpenDDS_DCPS_transport_TransportConfig_setSwapBytes
(JNIEnv * jni, jobject jthis, jboolean val)
{
  OpenDDS::DCPS::TransportConfig_rch config(recoverCppObj<OpenDDS::DCPS::TransportConfig>(jni, jthis),
                                            false); // Don't take ownership
  config->swap_bytes_ = val;
}

// TransportInst

//_TransportInst::jni_fini
void JNICALL Java_OpenDDS_DCPS_transport_TransportInst__1jni_1fini
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->_remove_ref();

}

// TransportInst::getName
jstring JNICALL Java_OpenDDS_DCPS_transport_TransportInst_getName
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  jstring retStr = jni->NewStringUTF(inst->name().c_str());
  return retStr;
}

// TransportInst::getQueueMessagesPerPool
jint JNICALL Java_OpenDDS_DCPS_transport_TransportInst_getQueueMessagesPerPool
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->queue_messages_per_pool_;
}

// TransportInst::setQueueMessagesPerPool
void JNICALL Java_OpenDDS_DCPS_transport_TransportInst_setQueueMessagesPerPool
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->queue_messages_per_pool_ = val;
}

// TransportInst::getQueueInitialPools
jint JNICALL Java_OpenDDS_DCPS_transport_TransportInst_getQueueInitialPools
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->queue_initial_pools_;
}

// TransportInst::setQueueInitialPools
void JNICALL Java_OpenDDS_DCPS_transport_TransportInst_setQueueInitialPools
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->queue_initial_pools_ = val;
}

// TransportInst::getMaxPacketSize
jint JNICALL Java_OpenDDS_DCPS_transport_TransportInst_getMaxPacketSize
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->max_packet_size_;
}

// TransportInst::setMaxPacketSize
void JNICALL Java_OpenDDS_DCPS_transport_TransportInst_setMaxPacketSize
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->max_packet_size_ = val;
}

// TransportInst::getMaxSamplesPerPacket
jint JNICALL Java_OpenDDS_DCPS_transport_TransportInst_getMaxSamplesPerPacket
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->max_samples_per_packet_;
}

// TransportInst::setMaxSamplesPerPacket
void JNICALL Java_OpenDDS_DCPS_transport_TransportInst_setMaxSamplesPerPacket
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->max_samples_per_packet_ = val;
}

// TransportInst::getOptimumPacketSize
jint JNICALL Java_OpenDDS_DCPS_transport_TransportInst_getOptimumPacketSize
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->optimum_packet_size_;
}

// TransportInst::setOptimumPacketSize
void JNICALL Java_OpenDDS_DCPS_transport_TransportInst_setOptimumPacketSize
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->optimum_packet_size_ = val;
}

// TransportInst::isThreadPerConnection
jboolean JNICALL Java_OpenDDS_DCPS_transport_TransportInst_isThreadPerConnection
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->thread_per_connection_;
}

// TransportInst::setThreadPerConnection
void JNICALL Java_OpenDDS_DCPS_transport_TransportInst_setThreadPerConnection
(JNIEnv * jni, jobject jthis, jboolean val)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->thread_per_connection_ = val;
}

// TransportInst::getDatalinkReleaseDelay
jint JNICALL Java_OpenDDS_DCPS_transport_TransportInst_getDatalinkReleaseDelay
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->datalink_release_delay_;
}

// TransportInst::setDatalinkReleaseDelay
void JNICALL Java_OpenDDS_DCPS_transport_TransportInst_setDatalinkReleaseDelay
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->datalink_release_delay_ = val;
}

// TransportInst::getDatalinkControlChunks
jint JNICALL Java_OpenDDS_DCPS_transport_TransportInst_getDatalinkControlChunks
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->datalink_control_chunks_;
}

// TransportInst::setDatalinkControlChunks
void JNICALL Java_OpenDDS_DCPS_transport_TransportInst_setDatalinkControlChunks
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TransportInst_rch inst(recoverCppObj<OpenDDS::DCPS::TransportInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->datalink_control_chunks_ = val;
}

// TcpInst

// TcpInst::getLocalAddress
jstring JNICALL Java_OpenDDS_DCPS_transport_TcpInst_getLocalAddress
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  jstring retStr = jni->NewStringUTF(inst->local_address_str_.c_str());
  return retStr;
}

// TcpInst::setLocalAddress
void JNICALL Java_OpenDDS_DCPS_transport_TcpInst_setLocalAddress
(JNIEnv * jni, jobject jthis, jstring val)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  JStringMgr jsm_val(jni, val);
  inst->local_address_str_ = jsm_val.c_str();
  inst->local_address_.set(inst->local_address_str_.c_str());
}

// TcpInst::isEnableNagleAlgorithm
jboolean JNICALL Java_OpenDDS_DCPS_transport_TcpInst_isEnableNagleAlgorithm
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  return inst->enable_nagle_algorithm_;
}

// TcpInst::setEnableNagleAlgorithm
void JNICALL Java_OpenDDS_DCPS_transport_TcpInst_setEnableNagleAlgorithm
(JNIEnv * jni, jobject jthis, jboolean val)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  inst->enable_nagle_algorithm_ = val;
}

// TcpInst::getConnRetryInitialDelay
jint JNICALL Java_OpenDDS_DCPS_transport_TcpInst_getConnRetryInitialDelay
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  return inst->conn_retry_initial_delay_;
}

// TcpInst::setConnRetryInitialDelay
void JNICALL Java_OpenDDS_DCPS_transport_TcpInst_setConnRetryInitialDelay
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  inst->conn_retry_initial_delay_ = val;
}

// TcpInst::getConnRetryBackoffMultiplier
jdouble JNICALL Java_OpenDDS_DCPS_transport_TcpInst_getConnRetryBackoffMultiplier
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  return inst->conn_retry_backoff_multiplier_;
}

// TcpInst::setConnRetryBackoffMultiplier
void JNICALL Java_OpenDDS_DCPS_transport_TcpInst_setConnRetryBackoffMultiplier
(JNIEnv * jni, jobject jthis, jdouble val)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  inst->conn_retry_backoff_multiplier_ = val;
}

// TcpInst::getConnRetryAttempts
jint JNICALL Java_OpenDDS_DCPS_transport_TcpInst_getConnRetryAttempts
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  return inst->conn_retry_attempts_;
}

// TcpInst::setConnRetryAttempts
void JNICALL Java_OpenDDS_DCPS_transport_TcpInst_setConnRetryAttempts
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  inst->conn_retry_attempts_ = val;
}

// TcpInst::getMaxOutputPausePeriod
jint JNICALL Java_OpenDDS_DCPS_transport_TcpInst_getMaxOutputPausePeriod
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  return inst->max_output_pause_period_;
}

// TcpInst::setMaxOutputPausePeriod
void JNICALL Java_OpenDDS_DCPS_transport_TcpInst_setMaxOutputPausePeriod
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  inst->max_output_pause_period_ = val;
}

// TcpInst::getPassiveReconnectDuration
jint JNICALL Java_OpenDDS_DCPS_transport_TcpInst_getPassiveReconnectDuration
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  return inst->passive_reconnect_duration_;
}

// TcpInst::setPassiveReconnectDuration
void JNICALL Java_OpenDDS_DCPS_transport_TcpInst_setPassiveReconnectDuration
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  inst->passive_reconnect_duration_ = val;
}

// TcpInst::getPassiveConnectDuration
jint JNICALL Java_OpenDDS_DCPS_transport_TcpInst_getPassiveConnectDuration
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  return inst->passive_connect_duration_;
}

// TcpInst::setPassiveConnectDuration
void JNICALL Java_OpenDDS_DCPS_transport_TcpInst_setPassiveConnectDuration
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::TcpInst_rch inst(recoverCppObj<OpenDDS::DCPS::TcpInst>(jni, jthis),
                                  false); // Don't take ownership
  inst->passive_connect_duration_ = val;
}

// UdpInst

// UdpInst::getLocalAddress
jstring JNICALL Java_OpenDDS_DCPS_transport_UdpInst_getLocalAddress
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::UdpInst_rch inst(recoverCppObj<OpenDDS::DCPS::UdpInst>(jni, jthis),
                                  false); // Don't take ownership
  ACE_TCHAR buffer[1024];
  inst->local_address_.addr_to_string(buffer, 1024, 1);
  std::string addr_str = ACE_TEXT_ALWAYS_CHAR(buffer);
  jstring retStr = jni->NewStringUTF(addr_str.c_str());
  return retStr;
}

// UdpInst::setLocalAddress
void JNICALL Java_OpenDDS_DCPS_transport_UdpInst_setLocalAddress
(JNIEnv * jni, jobject jthis, jstring val)
{
  OpenDDS::DCPS::UdpInst_rch inst(recoverCppObj<OpenDDS::DCPS::UdpInst>(jni, jthis),
                                  false); // Don't take ownership
  JStringMgr jsm_val(jni, val);
  inst->local_address_.set(jsm_val.c_str());
}

// MulticastInst

// MulticastInst::getDefaultToIPv6
jboolean JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getDefaultToIPv6
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->default_to_ipv6_;
}

// MulticastInst::setDefaultToIPv6
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setDefaultToIPv6
(JNIEnv * jni, jobject jthis, jboolean val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->default_to_ipv6_ = val;
}

// MulticastInst::getPortOffset
jshort JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getPortOffset
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->port_offset_;
}

// MulticastInst::setPortOffset
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setPortOffset
(JNIEnv * jni, jobject jthis, jshort val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->port_offset_ = val;
}

// MulticastInst::getGroupAddress
jstring JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getGroupAddress
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  ACE_TCHAR buffer[1024];
  inst->group_address_.addr_to_string(buffer, 1024, 1);
  std::string addr_str = ACE_TEXT_ALWAYS_CHAR(buffer);
  jstring retStr = jni->NewStringUTF(addr_str.c_str());
  return retStr;
}

// MulticastInst::setGroupAddress
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setGroupAddress
(JNIEnv * jni, jobject jthis, jstring val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  JStringMgr jsm_val(jni, val);
  inst->group_address_.set(jsm_val.c_str());
}

// MulticastInst::getReliable
jboolean JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getReliable
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->reliable_;
}

// MulticastInst::setReliable
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setReliable
(JNIEnv * jni, jobject jthis, jboolean val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->reliable_ = val;
}

// MulticastInst::getSynBackoff
jdouble JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getSynBackoff
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->syn_backoff_;
}

// MulticastInst::setSynBackoff
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setSynBackoff
(JNIEnv * jni, jobject jthis, jdouble val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->syn_backoff_ = val;
}

// MulticastInst::getSynInterval
jlong JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getSynInterval
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->syn_interval_.msec();
}

// MulticastInst::setSynInterval
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setSynInterval
(JNIEnv * jni, jobject jthis, jlong val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->syn_interval_.msec(long(val));
}

// MulticastInst::getSynTimeout
jlong JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getSynTimeout
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->syn_timeout_.msec();
}

// MulticastInst::setSynTimeout
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setSynTimeout
(JNIEnv * jni, jobject jthis, jlong val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->syn_timeout_.msec(long(val));
}

// MulticastInst::getNakDepth
jint JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getNakDepth
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->nak_depth_;
}

// MulticastInst::setNakDepth
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setNakDepth
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->nak_depth_ = val;
}

// MulticastInst::getNakInterval
jlong JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getNakInterval
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->nak_interval_.msec();
}

// MulticastInst::setNakInterval
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setNakInterval
(JNIEnv * jni, jobject jthis, jlong val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->nak_interval_.msec(long(val));
}

// MulticastInst::getNakDelayInterval
jint JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getNakDelayInterval
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->nak_delay_intervals_;
}

// MulticastInst::setNakDelayInterval
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setNakDelayInterval
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->nak_delay_intervals_ = val;
}

// MulticastInst::getNakMax
jint JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getNakMax
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->nak_max_;
}

// MulticastInst::setNakMax
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setNakMax
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->nak_max_ = val;
}

// MulticastInst::getNakTimeout
jlong JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getNakTimeout
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->nak_timeout_.msec();
}

// MulticastInst::setNakTimeout
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setNakTimeout
(JNIEnv * jni, jobject jthis, jlong val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->nak_timeout_.msec(long(val));
}

// MulticastInst::getTimeToLive
jint JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getTimeToLive
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->ttl_;
}

// MulticastInst::setTimeToLive
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setTimeToLive
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->ttl_ = val;
}

// MulticastInst::getRcvBufferSize
jint JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_getRcvBufferSize
(JNIEnv * jni, jobject jthis)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  return inst->rcv_buffer_size_;
}

// MulticastInst::setRcvBufferSize
void JNICALL Java_OpenDDS_DCPS_transport_MulticastInst_setRcvBufferSize
(JNIEnv * jni, jobject jthis, jint val)
{
  OpenDDS::DCPS::MulticastInst_rch inst(recoverCppObj<OpenDDS::DCPS::MulticastInst>(jni, jthis),
                                        false); // Don't take ownership
  inst->rcv_buffer_size_ = val;
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
