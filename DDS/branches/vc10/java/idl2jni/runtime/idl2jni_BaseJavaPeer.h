/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef idl2jni_BaseJavaPeer_H
#define idl2jni_BaseJavaPeer_H

#include "idl2jni_runtime_Export.h"
#include "idl2jni_runtime.h"
#include "idl2jni_jni.h"
#include "tao/LocalObject.h"
#include "tao/Version.h"

class idl2jni_runtime_Export IDL2JNI_BaseJavaPeer
#if TAO_MAJOR_VERSION > 1 || TAO_MINOR_VERSION > 6
  : public virtual CORBA::LocalObject
#else
  : public virtual TAO_Local_RefCounted_Object
#endif
{
public:
  IDL2JNI_BaseJavaPeer(JNIEnv *jni, jobject local)
    : globalCallback_(jni->NewGlobalRef(local))
    , cl_(jni->NewGlobalRef(getContextClassLoader(jni)))
  {
    jni->GetJavaVM(&jvm_);
  }

  virtual ~IDL2JNI_BaseJavaPeer();

protected:
  jobject globalCallback_;
  jobject cl_;
  JavaVM *jvm_;

private: //unimplemented
  IDL2JNI_BaseJavaPeer(const IDL2JNI_BaseJavaPeer&);
  IDL2JNI_BaseJavaPeer &operator= (const IDL2JNI_BaseJavaPeer&);
};

#endif
