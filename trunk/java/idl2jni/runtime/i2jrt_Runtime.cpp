/*
 * $Id$
 */

#include "i2jrt_Runtime.h"
#include "idl2jni_runtime.h"

void JNICALL
Java_i2jrt_Runtime_setClassLoader (JNIEnv *env, jclass clazz, jobject cl)
{
  setClassLoader(env, cl);
}
