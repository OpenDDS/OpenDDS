#include "OpenDDS_DCPS_TheParticipantFactory.h"
#include "OpenDDS_DCPS_TheServiceParticipant.h"
#include "OpenDDS_DCPS_transport_TheTransportFactory.h"
#include "OpenDDS_DCPS_transport_TransportImpl.h"
#include "DDS_WaitSet.h"
#include "DDS_GuardCondition.h"

#include "idl2jni_runtime.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/GuardCondition.h"

#include "DdsDcpsDomainJC.h"
#include "DdsDcpsPublicationJC.h"
#include "DdsDcpsSubscriptionJC.h"


jobject JNICALL Java_OpenDDS_DCPS_TheParticipantFactory_WithArgs (JNIEnv *jni,
  jclass, jobject ssholder)
{
  JniArgv jargv (jni, ssholder);
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs (jargv.argc_, jargv.orb_argv ());
      jobject j_dpf;
      copyToJava (jni, j_dpf, dpf, true);
      return j_dpf;
    }
  catch (const CORBA::SystemException &se)
    {
      throw_java_exception (jni, se);
      return 0;
    }
}


void JNICALL Java_OpenDDS_DCPS_TheServiceParticipant_shutdown (JNIEnv *, jclass)
{
  TheServiceParticipant->shutdown ();
}

jint JNICALL Java_OpenDDS_DCPS_TheServiceParticipant_domain_1to_1repo
  (JNIEnv *, jclass, jint domain)
{
  OpenDDS::DCPS::Service_Participant::RepoKey key =
    TheServiceParticipant->domain_to_repo (domain); 
  return static_cast<jint> (key);
}

void JNICALL Java_OpenDDS_DCPS_TheServiceParticipant_set_1repo_1domain
  (JNIEnv *, jclass, jint domain, jint repo)
{
  TheServiceParticipant->set_repo_domain (domain, repo);
}

void JNICALL Java_OpenDDS_DCPS_TheServiceParticipant_set_1repo_1ior
  (JNIEnv *envp, jclass, jstring ior, jint repo)
{
  JStringMgr jsm (envp, ior);
  TheServiceParticipant->set_repo_ior (jsm.c_str (), repo);
}

jobject JNICALL
Java_OpenDDS_DCPS_transport_TheTransportFactory_create_1transport_1impl
  (JNIEnv *jni, jclass, jint id, jboolean auto_configure)
{
  OpenDDS::DCPS::TransportImpl_rch transport =
    TheTransportFactory->create_transport_impl (id, auto_configure);
  jclass implClazz = jni->FindClass ("OpenDDS/DCPS/transport/TransportImpl");
  jmethodID ctor = jni->GetMethodID (implClazz, "<init>", "(J)V");
  return jni->NewObject (implClazz, ctor,
    reinterpret_cast<jlong> (transport._retn ()));
}


void JNICALL Java_OpenDDS_DCPS_transport_TheTransportFactory_release (JNIEnv *,
  jclass)
{
  TheTransportFactory->release ();
}


namespace
{
  OpenDDS::DCPS::TransportImpl *recoverTransportImpl (JNIEnv *jni,
    jobject jThis)
  {
    jclass thisClass = jni->GetObjectClass (jThis);
    jfieldID jptr = jni->GetFieldID (thisClass, "_jni_pointer", "J");
    jlong ptr = jni->GetLongField (jThis, jptr);
    return reinterpret_cast<OpenDDS::DCPS::TransportImpl *> (ptr);
  }
}


void JNICALL Java_OpenDDS_DCPS_transport_TransportImpl__1jni_1fini
  (JNIEnv *jni, jobject jThis)
{
  OpenDDS::DCPS::TransportImpl *ti = recoverTransportImpl (jni, jThis);
  ti->_remove_ref();
}


jobject JNICALL Java_OpenDDS_DCPS_transport_TransportImpl_attach_1to_1publisher
  (JNIEnv *jni, jobject jThis, jobject jPub)
{
  OpenDDS::DCPS::TransportImpl *ti = recoverTransportImpl (jni, jThis);
  DDS::Publisher_var pub;
  copyToCxx (jni, pub, jPub);
  OpenDDS::DCPS::PublisherImpl *pub_impl =
    dynamic_cast<OpenDDS::DCPS::PublisherImpl *> (pub.in ());
  OpenDDS::DCPS::AttachStatus stat = pub_impl->attach_transport (ti);
  jclass clazz = jni->FindClass ("OpenDDS/DCPS/transport/AttachStatus");
  jmethodID mid = jni->GetMethodID (clazz, "<init>", "(I)V");
  return jni->NewObject (clazz, mid, static_cast<jint> (stat));
}


jobject JNICALL Java_OpenDDS_DCPS_transport_TransportImpl_attach_1to_1subscriber
  (JNIEnv *jni, jobject jThis, jobject jSub)
{
  OpenDDS::DCPS::TransportImpl *ti = recoverTransportImpl (jni, jThis);
  DDS::Subscriber_var sub;
  copyToCxx (jni, sub, jSub);
  OpenDDS::DCPS::SubscriberImpl *sub_impl =
    dynamic_cast<OpenDDS::DCPS::SubscriberImpl *> (sub.in ());
  OpenDDS::DCPS::AttachStatus stat = sub_impl->attach_transport (ti);
  jclass clazz = jni->FindClass ("OpenDDS/DCPS/transport/AttachStatus");
  jmethodID mid = jni->GetMethodID (clazz, "<init>", "(I)V");
  return jni->NewObject (clazz, mid, static_cast<jint> (stat));
}


// WaitSet and GuardCondition

jlong JNICALL Java_DDS_WaitSet__1jni_1init (JNIEnv *, jclass)
{
  return reinterpret_cast<jlong> (static_cast<CORBA::Object_ptr>(
    new DDS::WaitSet));
}


jlong JNICALL Java_DDS_GuardCondition__1jni_1init(JNIEnv *, jclass)
{
  return reinterpret_cast<jlong> (static_cast<CORBA::Object_ptr>(
    new DDS::GuardCondition));
}

