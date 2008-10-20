/*
 * $Id$
 */

#include <jni.h>

#include "ace/ace_wchar.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_string.h"

#include "tao/Exception.h"

#include "dds/InfoRepo/DCPSInfoRepoServ.h"

#include "org_opendds_jms_ir_DCPSInfoRepoService.h"

#define DCPSInfoRepoService_init \
        Java_org_opendds_jms_ir_DCPSInfoRepoService_init

#define DCPSInfoRepoService_fini \
        Java_org_opendds_jms_ir_DCPSInfoRepoService_fini

#define DCPSInfoRepoService_run \
        Java_org_opendds_jms_ir_DCPSInfoRepoService_run

#define DCPSInfoRepoService_shutdown \
        Java_org_opendds_jms_ir_DCPSInfoRepoService_shutdown

namespace
{
    jfieldID
    get_peer_fieldID(JNIEnv *env, jobject self)
    {
        return env->GetFieldID(env->GetObjectClass(self), "peer", "J");
    }

    InfoRepo *
    get_InfoRepo_peer(JNIEnv *env, jobject self)
    {
        return (InfoRepo *)
            env->GetLongField(self, get_peer_fieldID(env, self));
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
        set_InfoRepo_peer(env, self, NULL);
    }

    void
    throw_exception(JNIEnv *env,
        const char *className, const char *message = NULL)
    {
        jclass throwable = env->FindClass(className);
        if (throwable != NULL) {
            env->ThrowNew(throwable, message);
        }
    }

    void
    delete_argv(JNIEnv *env, ACE_TCHAR **argv, jsize len)
    {
        // Skip first element; argv[0] is a string literal.
        for (int i = 1; i < len; ++i) {
            ACE_OS::free(argv[i]);
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

        for (int i = 1; i < len; ++i) {
            jstring str = (jstring)env->GetObjectArrayElement(args, i - 1);

            if (str == NULL) {
                throw_exception(env, "java/lang/NullPointerException");
                delete_argv(env, argv, i - 1);
                return NULL;
            }

            const char *cs = env->GetStringUTFChars(str, NULL);

            argv[i] = ACE_TEXT_CHAR_TO_TCHAR(ACE_OS::strdup(cs));

            env->ReleaseStringUTFChars(str, cs);
        }
        return argv;
    }
}

void JNICALL
DCPSInfoRepoService_init(JNIEnv *env, jobject self, jobjectArray args)
{
    if (args == NULL) {
        throw_exception(env, "java/lang/NullPointerException");
        return;
    }

    if (!env->IsInstanceOf(args, env->FindClass("[Ljava/lang/String;"))) {
        throw_exception(env, "java/lang/IllegalArgumentException");
        return;
    }

    jsize len = env->GetArrayLength(args) + 1;

    ACE_TCHAR **argv = to_argv(env, args, len);
    if (argv != NULL) {
        try {
            InfoRepo *peer = new InfoRepo(len, argv);
            set_InfoRepo_peer(env, self, peer);
        }
        catch (InfoRepo::InitError &err) {
            throw_exception(env, "java/lang/IllegalArgumentException",
                err.msg_.c_str());
        }
        catch (CORBA::Exception &err) {
            throw_exception(env, "org/omg/CORBA/UNKNOWN",
                err._info().c_str());
        }
        catch (...) {
            throw_exception(env, "java/lang/UnknownError");
        }
        delete_argv(env, argv, len);
    }
}

void JNICALL
DCPSInfoRepoService_fini(JNIEnv *env, jobject self)
{
    InfoRepo *peer = get_InfoRepo_peer(env, self);
    if (peer != NULL) {
        delete_InfoRepo_peer(env, self, peer);
    }
}

void JNICALL
DCPSInfoRepoService_run(JNIEnv *env, jobject self)
{
    InfoRepo *peer = get_InfoRepo_peer(env, self);
    if (peer == NULL) {
        throw_exception(env, "java/lang/IllegalStateException");
        return;
    }
    peer->run();
}

void JNICALL
DCPSInfoRepoService_shutdown(JNIEnv *env, jobject self, jboolean finalize)
{
    InfoRepo *peer = get_InfoRepo_peer(env, self);
    if (peer == NULL) {
        throw_exception(env, "java/lang/IllegalStateException");
        return;
    }
    peer->shutdown();

    if (finalize) {
        DCPSInfoRepoService_fini(env, self);
    }
}
