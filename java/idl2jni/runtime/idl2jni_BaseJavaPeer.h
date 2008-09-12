/* -*- C++ -*- */
#ifndef idl2jni_BaseJavaPeer_H
#define idl2jni_BaseJavaPeer_H

#include "idl2jni_runtime_Export.h"
#include "idl2jni_jni.h"
#include "tao/LocalObject.h"

class idl2jni_runtime_Export IDL2JNI_BaseJavaPeer
  : public virtual TAO_Local_RefCounted_Object
{
public:
  IDL2JNI_BaseJavaPeer (JNIEnv *jni, jobject local)
    : globalCallback_ (jni->NewGlobalRef (local))
  {
    jni->GetJavaVM (&jvm_);
  }

  virtual ~IDL2JNI_BaseJavaPeer ();

protected:
  jobject globalCallback_;
  JavaVM *jvm_;

private: //unimplemented
  IDL2JNI_BaseJavaPeer (const IDL2JNI_BaseJavaPeer&);
  IDL2JNI_BaseJavaPeer &operator= (const IDL2JNI_BaseJavaPeer&);
};

#endif
