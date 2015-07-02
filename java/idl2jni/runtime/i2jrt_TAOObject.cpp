/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "i2jrt_TAOObject.h"
#include "idl2jni_runtime.h"

#include "tao/Object.h"
#include "tao/SystemException.h"

CORBA::Object_ptr recoverTaoObject(JNIEnv *jni, jobject source)
{
  if (!source) return CORBA::Object::_nil();

  jclass clazz = findClass(jni, "i2jrt/TAOObject");
  jfieldID fid = jni->GetFieldID(clazz, "_jni_ptr", "J");
  jlong _jni_ptr = jni->GetLongField(source, fid);
  return reinterpret_cast<CORBA::Object_ptr>(_jni_ptr);
}

void JNICALL Java_i2jrt_TAOObject__1jni_1fini(JNIEnv *jni, jobject jThis)
{
  CORBA::release(recoverTaoObject(jni, jThis));
}

// TODO: throw NullPointerException if jThis is null?  Why is Java even calling
// us in that case?

jboolean JNICALL Java_i2jrt_TAOObject__1is_1a(JNIEnv *jni, jobject jThis,
                                              jstring repoID)
{
  JStringMgr jsm(jni, repoID);
  CORBA::Object_ptr ptr = recoverTaoObject(jni, jThis);

  try {
    return ptr->_is_a(jsm.c_str());

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
  }

  return 0;
}

jboolean JNICALL Java_i2jrt_TAOObject__1is_1equivalent(JNIEnv *jni, jobject jThis,
                                                       jobject jThat)
{
  CORBA::Object_ptr ptr = recoverTaoObject(jni, jThis);
  CORBA::Object_ptr rhs = recoverTaoObject(jni, jThat);

  try {
    return ptr->_is_equivalent(rhs);

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
  }

  return 0;
}

jboolean JNICALL Java_i2jrt_TAOObject__1non_1existent(JNIEnv *jni,
                                                      jobject jThis)
{
  CORBA::Object_ptr ptr = recoverTaoObject(jni, jThis);

  try {
    return ptr->_non_existent();

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
  }

  return 0;

}

jint JNICALL Java_i2jrt_TAOObject__1hash(JNIEnv *jni, jobject jThis,
                                         jint i)
{
  CORBA::Object_ptr ptr = recoverTaoObject(jni, jThis);

  try {
    return ptr->_hash(i);

  } catch (const CORBA::SystemException &se) {
    throw_java_exception(jni, se);
  }

  return 0;
}

jobject JNICALL Java_i2jrt_TAOObject__1duplicate(JNIEnv *jni, jobject jThis)
{
  CORBA::Object_ptr ptr = recoverTaoObject(jni, jThis);

  if (CORBA::is_nil(ptr)) return 0;

  CORBA::Object_ptr dupl = CORBA::Object::_duplicate(ptr);
  jclass clazz = jni->GetObjectClass(jThis);
  jmethodID ctor = jni->GetMethodID(clazz, "<init>", "(J)V");
  return jni->NewObject(clazz, ctor, reinterpret_cast<jlong>(dupl));
}

void JNICALL Java_i2jrt_TAOObject__1release(JNIEnv *jni, jobject jThis)
{
  jclass clazz = findClass(jni, "i2jrt/TAOObject");
  jfieldID fid = jni->GetFieldID(clazz, "_jni_ptr", "J");
  jlong _jni_ptr = jni->GetLongField(jThis, fid);
  CORBA::Object_ptr o = reinterpret_cast<CORBA::Object_ptr>(_jni_ptr);
  CORBA::release(o);
  jni->SetLongField(jThis, fid,
                    reinterpret_cast<jlong>(CORBA::Object::_nil()));
}
