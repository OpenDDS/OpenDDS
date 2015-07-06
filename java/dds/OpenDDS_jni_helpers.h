/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OpenDDS_jni_helpers_H
#define OpenDDS_jni_helpers_H

#include "idl2jni_jni.h"

#include "ace/Basic_Types.h"
#include "ace/INET_Addr.h"
#include "ace/SString.h"
#include <string>

namespace jvmSig {

enum sig_t {BYTE, CHAR, DOUBLE, FLOAT, INT, LONG, SHORT, BOOLEAN,
            STRING
           };
const char *sig[] = {"B", "C", "D", "F", "I", "J", "S", "Z",
                     "Ljava/lang/String;"
                    };

} // namespace jvmSig

template <typename C>
struct BaseField {
  virtual void toCxx(JNIEnv *jni, jclass clazz, jobject j, C &cxx) = 0;
  virtual void toJava(JNIEnv *jni, jclass clazz, const C &cxx, jobject j) = 0;
  virtual ~BaseField() {}

  const char *java_field_;
  explicit BaseField(const char *j)
    : java_field_(j)
  {}
};

template <typename C, size_t N>
void toCxx(JNIEnv *jni, jclass clazz, jobject j, C &cxx,
           BaseField<C> *(&fields)[N])
{
  for (size_t i(0); i < N; ++i) {
    fields[i]->toCxx(jni, clazz, j, cxx);
  }
}

template <typename C, size_t N>
void toJava(JNIEnv *jni, jclass clazz, const C &cxx, jobject j,
            BaseField<C> *(&fields)[N])
{
  for (size_t i(0); i < N; ++i) {
    fields[i]->toJava(jni, clazz, cxx, j);
  }
}

template <typename C, typename T, jvmSig::sig_t whichSig>
struct Field : BaseField<C> {
  typedef T C:: *memptr_t;
  memptr_t member_ptr_;

  Field(const char *j, memptr_t c)
    : BaseField<C> (j), member_ptr_(c)
  {}

  jfieldID getFid(JNIEnv *jni, jclass clazz) {
    return jni->GetFieldID(clazz, this->java_field_, jvmSig::sig[whichSig]);
  }
};

///SimpleFields are assignable from the JNI type to the C++ type (jint <-> int)
template <typename C, typename T, jvmSig::sig_t whichSig, typename JNI_T,
JNI_T(JNIEnv::*jniGetFn)(jobject, jfieldID),
void (JNIEnv::*jniSetFn)(jobject, jfieldID, JNI_T)>
struct SimpleField : Field<C, T, whichSig> {
  SimpleField(const char *j, typename Field<C, T, whichSig>::memptr_t c)
    : Field<C, T, whichSig> (j, c)
  {}

  void toCxx(JNIEnv *jni, jclass clazz, jobject obj, C &cxx) {
    jfieldID fid = this->getFid(jni, clazz);
    cxx.*(this->member_ptr_) = (jni->*jniGetFn)(obj, fid);
  }

  void toJava(JNIEnv *jni, jclass clazz, const C &cxx, jobject obj) {
    jfieldID fid = this->getFid(jni, clazz);
    (jni->*jniSetFn)(obj, fid, static_cast<JNI_T>(cxx.*(this->member_ptr_)));
  }
};

template <typename C>
struct BoolField : SimpleField<C, bool, jvmSig::BOOLEAN, jboolean,
      &JNIEnv::GetBooleanField, &JNIEnv::SetBooleanField> {
  typedef typename SimpleField<C, bool, jvmSig::BOOLEAN, jboolean,
  &JNIEnv::GetBooleanField, &JNIEnv::SetBooleanField>::memptr_t memptr_t;

  BoolField(const char *j, memptr_t c)
    : SimpleField<C, bool, jvmSig::BOOLEAN, jboolean,
        &JNIEnv::GetBooleanField, &JNIEnv::SetBooleanField> (j, c)
  {}
};

template <typename C>
struct IntField : SimpleField<C, int, jvmSig::INT, jint,
      &JNIEnv::GetIntField, &JNIEnv::SetIntField> {
  typedef typename SimpleField<C, int, jvmSig::INT, jint,
  &JNIEnv::GetIntField, &JNIEnv::SetIntField>::memptr_t memptr_t;

  IntField(const char *j, memptr_t c)
    : SimpleField<C, int, jvmSig::INT, jint,
        &JNIEnv::GetIntField, &JNIEnv::SetIntField> (j, c)
    {}
};

template <typename C>
struct SizetField : SimpleField<C, size_t, jvmSig::INT, jint,
      &JNIEnv::GetIntField, &JNIEnv::SetIntField> {
  typedef typename SimpleField<C, size_t, jvmSig::INT, jint,
  &JNIEnv::GetIntField, &JNIEnv::SetIntField>::memptr_t memptr_t;

  SizetField(const char *j, memptr_t c)
    : SimpleField<C, size_t, jvmSig::INT, jint,
        &JNIEnv::GetIntField, &JNIEnv::SetIntField> (j, c)
  {}
};

template <typename C>
struct UshortField : SimpleField<C, unsigned short, jvmSig::SHORT, jshort,
      &JNIEnv::GetShortField, &JNIEnv::SetShortField> {
  typedef typename SimpleField<C, unsigned short, jvmSig::SHORT, jshort,
  &JNIEnv::GetShortField, &JNIEnv::SetShortField>::memptr_t memptr_t;

  UshortField(const char *j, memptr_t c)
    : SimpleField<C, unsigned short, jvmSig::SHORT, jshort,
        &JNIEnv::GetShortField, &JNIEnv::SetShortField> (j, c)
  {}
};

template <typename C>
struct Uint32Field : SimpleField<C, ACE_UINT32, jvmSig::INT, jint,
      &JNIEnv::GetIntField, &JNIEnv::SetIntField> {
  typedef typename SimpleField<C, ACE_UINT32, jvmSig::INT, jint,
  &JNIEnv::GetIntField, &JNIEnv::SetIntField>::memptr_t memptr_t;

  Uint32Field(const char *j, memptr_t c)
    : SimpleField<C, ACE_UINT32, jvmSig::INT, jint,
        &JNIEnv::GetIntField, &JNIEnv::SetIntField> (j, c)
  {}
};

template <typename C>
struct UlongField : SimpleField<C, unsigned long, jvmSig::INT, jint,
      &JNIEnv::GetIntField, &JNIEnv::SetIntField> {
  typedef typename SimpleField<C, unsigned long, jvmSig::INT, jint,
  &JNIEnv::GetIntField, &JNIEnv::SetIntField>::memptr_t memptr_t;

  UlongField(const char *j, memptr_t c)
    : SimpleField<C, unsigned long, jvmSig::INT, jint,
        &JNIEnv::GetIntField, &JNIEnv::SetIntField> (j, c)
  {}
};

template <typename C>
struct DoubleField : SimpleField<C, double, jvmSig::DOUBLE, jdouble,
      &JNIEnv::GetDoubleField, &JNIEnv::SetDoubleField> {
  typedef typename SimpleField<C, double, jvmSig::DOUBLE, jdouble,
  &JNIEnv::GetDoubleField, &JNIEnv::SetDoubleField>::memptr_t memptr_t;

  DoubleField(const char *j, memptr_t c)
    : SimpleField<C, double, jvmSig::DOUBLE, jdouble,
        &JNIEnv::GetDoubleField, &JNIEnv::SetDoubleField> (j, c)
  {}
};

template <typename C>
struct InetAddrField : Field<C, std::string, jvmSig::STRING> {
  typedef typename Field<C, std::string, jvmSig::STRING>::memptr_t memptr_t;
  typedef ACE_INET_Addr C::*inetaddr_t;
  inetaddr_t addr_;
  InetAddrField(const char *j, memptr_t string_field, inetaddr_t addr_field)
    : Field<C, std::string, jvmSig::STRING> (j, string_field)
    , addr_(addr_field)
  {}

  void toCxx(JNIEnv *jni, jclass clazz, jobject obj, C &cxx) {
    jfieldID fid = this->getFid(jni, clazz);
    jobject src = jni->GetObjectField(obj, fid);
    JStringMgr jsm(jni, static_cast<jstring>(src));
    (cxx.*addr_).set(jsm.c_str());
    if (this->member_ptr_ != 0) {
      cxx.*(this->member_ptr_) = jsm.c_str();
    }
  }

  void toJava(JNIEnv *jni, jclass clazz, const C &cxx, jobject obj) {
    jfieldID fid = this->getFid(jni, clazz);
    jstring str;
    if (this->member_ptr_ != 0) {
      str = jni->NewStringUTF((cxx.*(this->member_ptr_)).c_str());
    } else {
      char buf[64];
      (cxx.*addr_).addr_to_string(buf, sizeof(buf));
      str = jni->NewStringUTF(buf);
    }
    jni->SetObjectField(obj, fid, str);
    jni->DeleteLocalRef(str);
  }
};

template <typename C>
struct TimeField : Field<C, long, jvmSig::LONG> {
  typedef typename Field<C, long, jvmSig::LONG>::memptr_t memptr_t;
  typedef ACE_Time_Value C::*timeval_t;
  timeval_t time_;
  TimeField(const char *j, timeval_t time_field)
    : Field<C, long, jvmSig::LONG> (j, 0)
    , time_(time_field)
  {}

  void toCxx(JNIEnv *jni, jclass clazz, jobject obj, C &cxx) {
    jfieldID fid = this->getFid(jni, clazz);
    jlong value = jni->GetLongField(obj, fid);
    (cxx.*time_).msec(static_cast<long>(value));
  }

  void toJava(JNIEnv *jni, jclass clazz, const C &cxx, jobject obj) {
    jfieldID fid = this->getFid(jni, clazz);
    jlong value = static_cast<jlong>((cxx.*time_).msec());
    jni->SetLongField(obj, fid, value);
  }
};
#endif
