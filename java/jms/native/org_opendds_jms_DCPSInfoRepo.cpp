/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <jni.h>

#include "ace/config-lite.h"
#include "ace/ace_wchar.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_string.h"
#include "ace/Init_ACE.h"

#include "tao/Exception.h"

#include "dds/InfoRepo/DCPSInfoRepoServ.h"
#include "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery.h"

#include "org_opendds_jms_DCPSInfoRepo.h"

namespace {

struct InitGuard {
  InitGuard() {
    ACE::init();
  }

  ~InitGuard() {
    ACE::fini();
  }

} init_guard;

jfieldID
get_peer_fieldID(JNIEnv* env, jobject self)
{
  return env->GetFieldID(env->GetObjectClass(self), "peer", "J");
}

InfoRepo*
get_InfoRepo_peer(JNIEnv* env, jobject self)
{
  return reinterpret_cast<InfoRepo*>
         (env->GetLongField(self, get_peer_fieldID(env, self)));
}

void
set_InfoRepo_peer(JNIEnv* env, jobject self, InfoRepo* peer)
{
  env->SetLongField(self, get_peer_fieldID(env, self),
                    reinterpret_cast<jlong>(peer));
}

void
delete_InfoRepo_peer(JNIEnv* env, jobject self, InfoRepo* peer)
{
  delete peer;
  set_InfoRepo_peer(env, self, 0);
}

void
throw_exception(JNIEnv* env, const char* name, const char* message = 0)
{
  jclass throwable = env->FindClass(name);

  if (throwable != 0)
    env->ThrowNew(throwable, message);
}

void
delete_argv(JNIEnv*, ACE_TCHAR** argv, jsize len)
{
  // Skip first element; argv[0] is a string literal.
  for (int i = 1; i < len; ++i) {
    ACE_OS::free(argv[i]);
  }

  delete[] argv;
}

ACE_TCHAR**
to_argv(JNIEnv* env, jobjectArray args, jsize len)
{
  ACE_TCHAR** argv = new ACE_TCHAR*[len];

  // InfoRepo::init and friends expect a traditional argv
  // (first element is process name). For compatibility,
  // insert a dummy value prior to filling the vector:
  argv[0] = const_cast<ACE_TCHAR*>(ACE_TEXT("JAVA"));

  for (int i = 1; i < len; ++i) {
    jstring arg = reinterpret_cast<jstring>
                  (env->GetObjectArrayElement(args, i - 1));

    if (arg == 0) {
      throw_exception(env, "java/lang/NullPointerException");
      delete_argv(env, argv, i - 1);
      return 0;
    }

    const char* cs = env->GetStringUTFChars(arg, 0);

    argv[i] = ACE_OS::strdup(ACE_TEXT_CHAR_TO_TCHAR(cs));

    env->ReleaseStringUTFChars(arg, cs);
  }

  return argv;
}

} // namespace

void JNICALL
Java_org_opendds_jms_DCPSInfoRepo_init(JNIEnv* env, jobject self, jobjectArray args)
{
  if (args == 0) {
    throw_exception(env, "java/lang/NullPointerException");
    return;
  }

  if (!env->IsInstanceOf(args, env->FindClass("[Ljava/lang/String;"))) {
    throw_exception(env, "java/lang/IllegalArgumentException");
    return;
  }

  jsize len = env->GetArrayLength(args) + 1;

  ACE_TCHAR** argv = to_argv(env, args, len);

  if (argv != 0) {
    try {
      OpenDDS::DCPS::InfoRepoDiscovery::StaticInitializer init;

      InfoRepo *peer = new InfoRepo(len, argv);
      set_InfoRepo_peer(env, self, peer);

    } catch (const InfoRepo::InitError& e) {
      throw_exception(env, "java/lang/IllegalArgumentException", e.msg_.c_str());

    } catch (const CORBA::Exception& e) {
      throw_exception(env, "org/omg/CORBA/UNKNOWN", e._info().c_str());

    } catch (...) {
      throw_exception(env, "java/lang/UnknownError");
    }

    delete_argv(env, argv, len);
  }
}

void JNICALL
Java_org_opendds_jms_DCPSInfoRepo_fini(JNIEnv* env, jobject self)
{
  InfoRepo* peer = get_InfoRepo_peer(env, self);

  if (peer != 0) {
    delete_InfoRepo_peer(env, self, peer);
  }
}

void JNICALL
Java_org_opendds_jms_DCPSInfoRepo_run(JNIEnv* env, jobject self)
{
  InfoRepo* peer = get_InfoRepo_peer(env, self);

  if (peer == 0) {
    throw_exception(env, "java/lang/IllegalStateException");
    return;
  }

  peer->run();
}

void JNICALL
Java_org_opendds_jms_DCPSInfoRepo_shutdown(JNIEnv* env, jobject self)
{
  InfoRepo* peer = get_InfoRepo_peer(env, self);

  if (peer == 0) {
    throw_exception(env, "java/lang/IllegalStateException");
    return;
  }

  peer->sync_shutdown();
}
