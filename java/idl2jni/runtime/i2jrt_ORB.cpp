/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "i2jrt_ORB.h"
#include "idl2jni_runtime.h"

#include "tao/ORB.h"
#include "tao/CORBA_methods.h"
#include "tao/SystemException.h"

#include <vector>
#include <string>
#include <cassert>

jobject JNICALL Java_i2jrt_ORB_init(JNIEnv *jni, jclass clazz,
                                    jobject ssholder, jstring orbId)
{
  JniArgv jargv(jni, ssholder);
  JStringMgr jsm_orbId(jni, orbId);

  CORBA::ORB_var orb;

  try {
    orb = CORBA::ORB_init(jargv.argc_, jargv.orb_argv(),
                          jsm_orbId.c_str());

  } catch (const CORBA::SystemException &se) {
    se._tao_print_exception("I2JRT ORB_init:");
    throw_java_exception(jni, se);
    return 0;
  }

  jmethodID ctor = jni->GetMethodID(clazz, "<init>", "(J)V");
  assert(ctor);
  return jni->NewObject(clazz, ctor, reinterpret_cast<jlong>(orb._retn()));
}

CORBA::ORB_ptr recoverTaoORB(JNIEnv *jni, jobject source)
{
  jclass clazz = findClass(jni, "i2jrt/ORB");
  jfieldID fid = jni->GetFieldID(clazz, "_jni_ptr", "J");
  jlong _jni_ptr = jni->GetLongField(source, fid);
  return reinterpret_cast<CORBA::ORB_ptr>(_jni_ptr);
}

void JNICALL Java_i2jrt_ORB__1jni_1fini(JNIEnv *jni, jobject jThis)
{
  CORBA::release(recoverTaoORB(jni, jThis));
}

jstring JNICALL Java_i2jrt_ORB_object_1to_1string(JNIEnv *jni, jobject jThis,
                                                  jobject obj)
{
  CORBA::Object_ptr tao_obj = recoverTaoObject(jni, obj);

  try {
    const char *str = recoverTaoORB(jni, jThis)->object_to_string(tao_obj);
    return jni->NewStringUTF(str);

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
  }

  return 0;
}

jobject JNICALL Java_i2jrt_ORB_string_1to_1object(JNIEnv *jni, jobject jThis,
                                                  jstring str)
{
  JStringMgr jsm(jni, str);

  try {
    CORBA::Object_ptr tao_obj =
      recoverTaoORB(jni, jThis)->string_to_object(jsm.c_str());
    jclass clazz = findClass(jni, "i2jrt/TAOObject");
    jmethodID ctor = jni->GetMethodID(clazz, "<init>", "(J)V");
    return jni->NewObject(clazz, ctor, reinterpret_cast<jlong>(tao_obj));

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
  }

  return 0;
}

void JNICALL Java_i2jrt_ORB_shutdown(JNIEnv *jni, jobject jThis, jboolean wait)
{
  try {
    recoverTaoORB(jni, jThis)->shutdown(wait);

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
  }
}

void JNICALL Java_i2jrt_ORB_destroy(JNIEnv *jni, jobject jThis)
{
  try {
    recoverTaoORB(jni, jThis)->destroy();

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
  }
}
