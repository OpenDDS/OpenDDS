/* -*- C++ -*- */
#ifndef OpenDDS_jni_helpers_H
#define OpenDDS_jni_helpers_H

#include "idl2jni_jni.h"

#include "ace/INET_Addr.h"
#include <string>

namespace jvmSig
{
  enum sig_t {BYTE, CHAR, DOUBLE, FLOAT, INT, LONG, SHORT, BOOLEAN,
    STRING};
  const char *sig[] = {"B", "C", "D", "F", "I", "J", "S", "Z",
   "Ljava/lang/String;"};
}

template <typename C>
struct BaseField
{
  virtual void toCxx (JNIEnv *jni, jclass clazz, jobject j, C &cxx) = 0;
  virtual void toJava (JNIEnv *jni, jclass clazz, const C &cxx, jobject j) = 0;

  const char *java_field_;
  explicit BaseField (const char *j)
    : java_field_ (j) {}
};

template <typename C, size_t N>
void toCxx (JNIEnv *jni, jclass clazz, jobject j, C &cxx,
  BaseField<C> *(&fields)[N])
{
  for (size_t i (0); i < N; ++i)
    {
      fields[i]->toCxx (jni, clazz, j, cxx);
    }
}

template <typename C, size_t N>
void toJava (JNIEnv *jni, jclass clazz, const C &cxx, jobject j,
  BaseField<C> *(&fields)[N])
{
  for (size_t i (0); i < N; ++i)
    {
      fields[i]->toJava (jni, clazz, cxx, j);
    }
}

template <typename C, typename T, jvmSig::sig_t whichSig>
struct Field : BaseField<C>
{
  typedef T C:: *memptr_t;
  memptr_t member_ptr_;
  
  Field (const char *j, memptr_t c)
    : BaseField (j), member_ptr_ (c) {}

  jfieldID getFid (JNIEnv *jni, jclass clazz)
  {
    return jni->GetFieldID (clazz, java_field_, jvmSig::sig[whichSig]);
  }
};

///SimpleFields are assignable from the JNI type to the C++ type (jint <-> int)
template <typename C, typename T, jvmSig::sig_t whichSig, typename JNI_T,
  JNI_T (JNIEnv::*jniGetFn) (jobject, jfieldID),
  void (JNIEnv::*jniSetFn) (jobject, jfieldID, JNI_T)>
struct SimpleField : Field<C, T, whichSig>
{
  SimpleField (const char *j, memptr_t c)
    : Field (j, c) {}

  void toCxx (JNIEnv *jni, jclass clazz, jobject obj, C &cxx)
  {
    jfieldID fid = getFid (jni, clazz);
    cxx.*member_ptr_ = (jni->*jniGetFn) (obj, fid);
  }

  void toJava (JNIEnv *jni, jclass clazz, const C &cxx, jobject obj)
  {
    jfieldID fid = getFid (jni, clazz);
    (jni->*jniSetFn) (obj, fid, cxx.*member_ptr_);
  }
};

template <typename C>
struct BoolField : SimpleField<C, bool, jvmSig::BOOLEAN, jboolean,
  &JNIEnv::GetBooleanField, &JNIEnv::SetBooleanField>
{
  BoolField (const char *j, memptr_t c)
    : SimpleField (j, c) {}
};

template <typename C>
struct IntField : SimpleField<C, int, jvmSig::INT, jint,
  &JNIEnv::GetIntField, &JNIEnv::SetIntField>
{
  IntField (const char *j, memptr_t c)
    : SimpleField (j, c) {}
};

template <typename C>
struct SizetField : SimpleField<C, size_t, jvmSig::INT, jint,
  &JNIEnv::GetIntField, &JNIEnv::SetIntField>
{
  SizetField (const char *j, memptr_t c)
    : SimpleField (j, c) {}
};

template <typename C>
struct UlongField : SimpleField<C, unsigned long, jvmSig::INT, jint,
  &JNIEnv::GetIntField, &JNIEnv::SetIntField>
{
  UlongField (const char *j, memptr_t c)
    : SimpleField (j, c) {}
};

template <typename C>
struct DoubleField : SimpleField<C, double, jvmSig::DOUBLE, jdouble,
  &JNIEnv::GetDoubleField, &JNIEnv::SetDoubleField>
{
  DoubleField (const char *j, memptr_t c)
    : SimpleField (j, c) {}
};

template <typename C>
struct InetAddrField : Field<C, std::string, jvmSig::STRING>
{
  typedef ACE_INET_Addr C::*inetaddr_t;
  inetaddr_t addr_;
  InetAddrField (const char *j, memptr_t string_field, inetaddr_t addr_field)
    : Field (j, string_field), addr_ (addr_field) {}

  void toCxx (JNIEnv *jni, jclass clazz, jobject obj, C &cxx)
  {
    jfieldID fid = getFid (jni, clazz);
    jobject src = jni->GetObjectField (obj, fid);
    JStringMgr jsm (jni, static_cast<jstring> (src));
    cxx.*member_ptr_ = jsm.c_str ();
    (cxx.*addr_).set (jsm.c_str ());
  }

  void toJava (JNIEnv *jni, jclass clazz, const C &cxx, jobject obj)
  {
    jfieldID fid = getFid (jni, clazz);
    jstring str = jni->NewStringUTF ((cxx.*member_ptr_).c_str ());
    jni->SetObjectField (obj, fid, str);
    jni->DeleteLocalRef (str);
  }
};

#endif
