/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef idl2jni_runtime_H
#define idl2jni_runtime_H

#include "idl2jni_jni.h"
#include "idl2jni_runtime_Export.h"

#include "tao/Basic_Types.h"

#include "ace/Global_Macros.h"

#include <vector>
#include <string>

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

namespace TAO {

template<typename charT> class String_Manager_T;

typedef String_Manager_T<CORBA::Char> String_Manager;

template <typename charT> class String_var;

namespace details {
  template<typename details> class string_sequence_element;
  template<typename details> class string_const_sequence_element;

  template<typename charT, bool dummy> struct string_traits;

  typedef string_traits<CORBA::Char, true> char_string_traits;
  typedef string_sequence_element<char_string_traits> charstr_sequence_element;
  typedef string_const_sequence_element<char_string_traits>
    charstr_const_sequence_element;

  template<typename obj_ref_traits> class object_reference_sequence_element;
  template<typename obj_ref_traits>
    class object_reference_const_sequence_element;
}

} // namespace TAO

namespace CORBA {

class SystemException;
class Object;
typedef Object *Object_ptr;
typedef TAO::String_var<CORBA::Char> String_var;

} // namespace CORBA

TAO_END_VERSIONED_NAMESPACE_DECL

idl2jni_runtime_Export
void copyToCxx(JNIEnv *jni, TAO::String_Manager &target, jobject source);

idl2jni_runtime_Export
void copyToJava(JNIEnv *jni, jobject &target, const TAO::String_Manager &source,
                bool createNewObject = false);

idl2jni_runtime_Export
void copyToCxx(JNIEnv *jni, CORBA::String_var &target, jobject source);

idl2jni_runtime_Export
void copyToJava(JNIEnv *jni, jobject &target, const char *source,
                bool createNewObject = false);

idl2jni_runtime_Export
void copyToCxx(JNIEnv *jni, TAO::details::charstr_sequence_element target,
               jobject source);

idl2jni_runtime_Export
void copyToJava(JNIEnv *jni, jobject &target,
                const TAO::details::charstr_const_sequence_element &source,
                bool createNewObject = false);

template <typename Traits>
void copyToCxx(JNIEnv *jni,
               TAO::details::object_reference_sequence_element<Traits> target,
               jobject source)
{
  typename Traits::object_type_var var;
  copyToCxx(jni, var, source);
  target = var;
}

template <typename Traits>
void copyToJava(JNIEnv *jni, jobject &target,
  const TAO::details::object_reference_const_sequence_element<Traits> &source,
  bool /* createNewObject = false */)
{
  typename Traits::object_type_var var = Traits::duplicate(source);
  copyToJava(jni, target, var);
}

idl2jni_runtime_Export
jobject currentThread(JNIEnv *);

idl2jni_runtime_Export
jobject getContextClassLoader(JNIEnv *);

idl2jni_runtime_Export
void setContextClassLoader(JNIEnv *, jobject);

idl2jni_runtime_Export
jclass findClass(JNIEnv *, const char *);

idl2jni_runtime_Export
CORBA::Object_ptr recoverTaoObject(JNIEnv *jni, jobject source);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jboolean value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jchar value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jbyte value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jshort value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jint value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jlong value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jfloat value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jdouble value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jbooleanArray value,
               const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jcharArray value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jbyteArray value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jshortArray value,
               const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jintArray value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jlongArray value, const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jfloatArray value,
               const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jdoubleArray value,
               const char *sig);

idl2jni_runtime_Export
void holderize(JNIEnv *jni, jobject holder, jobject value, const char *sig);

//jobjectArray doesn't need an overload, it will just convert to jobject

template <typename T>
T deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jboolean deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jchar deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jbyte deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jshort deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jint deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jlong deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jfloat deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jdouble deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jbooleanArray deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jcharArray deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jbyteArray deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jshortArray deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jintArray deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jlongArray deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jfloatArray deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jdoubleArray deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jobject deholderize(JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jobjectArray deholderize(JNIEnv *jni, jobject holder, const char *sig);

idl2jni_runtime_Export
void throw_java_exception(JNIEnv *jni, const CORBA::SystemException &se);

idl2jni_runtime_Export
void throw_cxx_exception(JNIEnv *jni, jthrowable excep);

class idl2jni_runtime_Export JStringMgr {
public:
  JStringMgr(JNIEnv* jni, jstring input);
  ~JStringMgr();

  const char* c_str() const {
    return c_str_;
  }

  const ACE_TCHAR* tc_str() const {
    return ACE_TEXT_CHAR_TO_TCHAR(c_str_);
  }

private:
  JNIEnv* jni_;
  jstring jstring_;
  const char* c_str_;
};

struct idl2jni_runtime_Export JniArgv {
  JniArgv(JNIEnv *jni, jobject string_seq_holder);
  ~JniArgv();

  JNIEnv *jni_;
  jobject ssholder_;
  std::vector<std::string> argv_;
  std::vector<const char *> orb_argv_;
  int argc_;

  char **orb_argv() {
    return argc_ ? const_cast<char **>(&orb_argv_[0]) : 0;
  }
};

///Local guard object for attaching a C++ thread to the JVM
class idl2jni_runtime_Export JNIThreadAttacher {
public:

  explicit JNIThreadAttacher(JavaVM *jvm, jobject cl = 0)
  : jvm_(jvm)
  , jni_(0) {
#ifdef ACE_ANDROID
    typedef JNIEnv* JNIEnv_t;
#else
    typedef void* JNIEnv_t;
#endif
    JNIEnv_t jni;
    if (jvm_->AttachCurrentThread(&jni, 0) != 0) {
      throw std::exception();
    }

    jni_ = reinterpret_cast<JNIEnv *>(jni);

    if (cl != 0) setContextClassLoader(jni_, cl);
  }

  ~JNIThreadAttacher() {
    jvm_->DetachCurrentThread();
  }

  JNIEnv *getJNI() {
    return jni_;
  }

private:
  JavaVM *jvm_;
  JNIEnv *jni_;
};

#endif
