/*
 * $Id$
 */

#include "i2jrt_Runtime.h"
#include "idl2jni_runtime.h"

void JNICALL
Java_i2jrt_Runtime_setClassLoader (JNIEnv *env, jclass clazz, jobject cl)
{
  jobject old;

  // Delete GlobalRef if exists
  old = JNIThreadAttacher::getClassLoader ();
  if (old != 0)
    {
      env->DeleteGlobalRef (old); 
    }
  JNIThreadAttacher::setClassLoader (env->NewGlobalRef (cl));
}
