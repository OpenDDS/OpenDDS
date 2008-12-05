/*
 * $Id$
 */

#include <jni.h>

#include "ace/config-lite.h"
#include "ace/ace_wchar.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_string.h"
#include "ace/Init_ACE.h"

#include "tao/Exception.h"

#include "dds/InfoRepo/DCPSInfoRepoServ.h"

#include "org_opendds_jms_DCPSInfoRepo.h"

#if ACE_MAJOR_VERSION == 5 && ACE_MINOR_VERSION < 5
# define ACE_PRE_5_5
#endif

#define DCPSInfoRepo_init \
  Java_org_opendds_jms_DCPSInfoRepo_init

#define DCPSInfoRepo_fini \
  Java_org_opendds_jms_DCPSInfoRepo_fini

#define DCPSInfoRepo_run \
  Java_org_opendds_jms_DCPSInfoRepo_run

#define DCPSInfoRepo_shutdown \
  Java_org_opendds_jms_DCPSInfoRepo_shutdown

namespace
{
  class InitGuard
  {
  public:
    InitGuard()
    {
      ACE::init();
    }

    ~InitGuard()
    {
      ACE::fini();
    }
  } init_guard;

  jfieldID
  get_peer_fieldID(JNIEnv *env, jobject self)
  {
    return env->GetFieldID(env->GetObjectClass(self), "peer", "J");
  }

  InfoRepo *
  get_InfoRepo_peer(JNIEnv *env, jobject self)
  {
    return (InfoRepo *)env->GetLongField(self, get_peer_fieldID(env, self));
  }

  void
  set_InfoRepo_peer(JNIEnv *env, jobject self, InfoRepo *peer)
  {
    env->SetLongField(self, get_peer_fieldID(env, self), (jlong)peer);
  }

  void
  delete_InfoRepo_peer(JNIEnv *env, jobject self, InfoRepo *peer)
  {
    delete peer;
    set_InfoRepo_peer(env, self, 0);
  }

  void
  throw_exception(JNIEnv *env, const char *className, const char *message = 0)
  {
    jclass throwable = env->FindClass(className);
    if (throwable != 0)
    {
      env->ThrowNew(throwable, message);
    }
  }

  void
  delete_argv(JNIEnv *, ACE_TCHAR **argv, jsize len)
  {
    // Skip first element; argv[0] is a string literal.
    for (int i = 1; i < len; ++i)
    {
#ifdef ACE_PRE_5_5
      ACE::String_Conversion::free(argv[i]);
#else  // ACE 5.5+
      ACE_OS::free(argv[i]);
#endif
    }
    delete[] argv;
  }

  ACE_TCHAR **
  to_argv(JNIEnv *env, jobjectArray args, jsize len)
  {
    ACE_TCHAR **argv = new ACE_TCHAR*[len];

    // InfoRepo::init and friends expect a traditional argv
    // (first element indicates process name). For compatibility,
    // insert a dummy value prior to filling the vector:
    argv[0] = (ACE_TCHAR *)ACE_TEXT("JAVA");

    for (int i = 1; i < len; ++i)
    {
      jstring str = (jstring)env->GetObjectArrayElement(args, i - 1);
      if (str == 0)
      {
        throw_exception(env, "java/lang/NullPointerException");
        delete_argv(env, argv, i - 1);
        return 0;
      }

      const char *cs = env->GetStringUTFChars(str, 0);

#ifdef ACE_PRE_5_5
      argv[i] = ACE_TEXT_TO_TCHAR_OUT(cs);
#else  // ACE 5.5+
      argv[i] = ACE_OS::strdup(ACE_TEXT_CHAR_TO_TCHAR(cs));
#endif

      env->ReleaseStringUTFChars(str, cs);
    }
    return argv;
  }
}

void JNICALL
DCPSInfoRepo_init(JNIEnv *env, jobject self, jobjectArray args)
{
  if (args == 0)
  {
    throw_exception(env, "java/lang/NullPointerException");
    return;
  }

  if (!env->IsInstanceOf(args, env->FindClass("[Ljava/lang/String;")))
  {
    throw_exception(env, "java/lang/IllegalArgumentException");
    return;
  }

  jsize len = env->GetArrayLength(args) + 1;

  ACE_TCHAR **argv = to_argv(env, args, len);
  if (argv != 0)
  {
    try
    {
      InfoRepo *peer = new InfoRepo(len, argv);
      set_InfoRepo_peer(env, self, peer);
    }
    catch (InfoRepo::InitError &e)
    {
      throw_exception(env, "java/lang/IllegalArgumentException",
        e.msg_.c_str());
    }
    catch (CORBA::Exception &e)
    {
      throw_exception(env, "org/omg/CORBA/UNKNOWN", e._info().c_str());
    }
    catch (...)
    {
      throw_exception(env, "java/lang/UnknownError");
    }
    delete_argv(env, argv, len);
  }
}

void JNICALL
DCPSInfoRepo_fini(JNIEnv *env, jobject self)
{
  InfoRepo *peer = get_InfoRepo_peer(env, self);
  if (peer != 0)
  {
    delete_InfoRepo_peer(env, self, peer);
  }
}

void JNICALL
DCPSInfoRepo_run(JNIEnv *env, jobject self)
{
  InfoRepo *peer = get_InfoRepo_peer(env, self);
  if (peer == 0)
  {
    throw_exception(env, "java/lang/IllegalStateException");
    return;
  }
  peer->run();
}

void JNICALL
DCPSInfoRepo_shutdown(JNIEnv *env, jobject self)
{
  InfoRepo *peer = get_InfoRepo_peer(env, self);
  if (peer == 0)
  {
    throw_exception(env, "java/lang/IllegalStateException");
    return;
  }
  peer->sync_shutdown();
}
