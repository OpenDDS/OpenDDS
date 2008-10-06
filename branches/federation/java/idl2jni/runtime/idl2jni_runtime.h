/* -*- C++ -*- */
#ifndef idl2jni_runtime_H
#define idl2jni_runtime_H

#include "idl2jni_jni.h"
#include "tao/Basic_Types.h"
#include "idl2jni_runtime_Export.h"
#include "15a_compat.h"

#include <vector>
#include <string>

#if TAO_MAJOR_VERSION == 1 && TAO_MINOR_VERSION < 5
   class TAO_String_Manager;
   class TAO_SeqElem_String_Manager;
   template<typename T, typename T_var> class TAO_Object_Manager;
#  define IDL2JNI_STRINGVAR class String_var;
#  define IDL2JNI_STRMAN TAO_String_Manager
#  define IDL2JNI_STRELM TAO_SeqElem_String_Manager
#  define IDL2JNI_STRELM_CONST TAO_SeqElem_String_Manager
#else
#  define IDL2JNI_STRINGVAR typedef TAO::String_var<CORBA::Char> String_var;
#  define IDL2JNI_STRMAN TAO::String_Manager
#  define IDL2JNI_STRELM TAO::details::charstr_sequence_element
#  define IDL2JNI_STRELM_CONST TAO::details::charstr_const_sequence_element
#endif

namespace TAO
{
  template<typename charT> class String_Manager_T;

  typedef String_Manager_T<CORBA::Char> String_Manager;

  template <typename charT> class String_var;

  namespace details
  {
    template<typename details> class string_sequence_element;
    template<typename details> class string_const_sequence_element;

    template<typename charT, bool dummy> struct string_traits;

    typedef string_traits<CORBA::Char, true> char_string_traits;
    typedef string_sequence_element<char_string_traits>
      charstr_sequence_element;
    typedef IDL2JNI_CONST_SEQ_ELEM charstr_const_sequence_element;

    template<typename obj_ref_traits>
    class object_reference_sequence_element;
    template<typename obj_ref_traits>
    class object_reference_const_sequence_element;
  }
}

namespace CORBA
{
  class SystemException;
  class Object;
  typedef Object *Object_ptr;
  IDL2JNI_STRINGVAR
}


idl2jni_runtime_Export
void copyToCxx (JNIEnv *jni, IDL2JNI_STRMAN &target, jobject source);

idl2jni_runtime_Export
void copyToJava (JNIEnv *jni, jobject &target, const IDL2JNI_STRMAN &source,
  bool createNewObject = false);


idl2jni_runtime_Export
void copyToCxx (JNIEnv *jni, CORBA::String_var &target, jobject source);

idl2jni_runtime_Export
void copyToJava (JNIEnv *jni, jobject &target, const char *source,
  bool createNewObject = false);


idl2jni_runtime_Export
void copyToCxx (JNIEnv *jni, IDL2JNI_STRELM target, jobject source);

idl2jni_runtime_Export
void copyToJava (JNIEnv *jni, jobject &target,
  const IDL2JNI_STRELM_CONST &source, bool createNewObject = false);


template <typename Traits>
void copyToCxx (JNIEnv *jni,
  TAO::details::object_reference_sequence_element<Traits> target,
  jobject source)
{
  typename Traits::object_type_var var;
  copyToCxx (jni, var, source);
  target = var;
}

template <typename Traits>
void copyToJava (JNIEnv *jni, jobject &target,
  const TAO::details::object_reference_const_sequence_element<Traits> &source,
  bool createNewObject = false)
{
  typename Traits::object_type_var var = Traits::duplicate (source);
  copyToJava (jni, target, var);
}


#if TAO_MAJOR_VERSION == 1 && TAO_MINOR_VERSION < 5
template <typename T>
void copyToCxx (JNIEnv *jni,
  TAO_Object_Manager<T, typename T::_var_type> target, jobject source)
{
  typename T::_var_type var;
  copyToCxx (jni, var, source);
  target = var;
}
#endif


idl2jni_runtime_Export
CORBA::Object_ptr recoverTaoObject (JNIEnv *jni, jobject source);


idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jboolean value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jchar value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jbyte value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jshort value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jint value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jlong value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jfloat value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jdouble value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jbooleanArray value,
  const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jcharArray value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jbyteArray value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jshortArray value,
  const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jintArray value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jlongArray value, const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jfloatArray value,
  const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jdoubleArray value,
  const char *sig);

idl2jni_runtime_Export
void holderize (JNIEnv *jni, jobject holder, jobject value, const char *sig);

//jobjectArray doesn't need an overload, it will just convert to jobject


template <typename T>
T deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jboolean deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jchar deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jbyte deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jshort deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jint deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jlong deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jfloat deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jdouble deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jbooleanArray deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jcharArray deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jbyteArray deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jshortArray deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jintArray deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jlongArray deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jfloatArray deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jdoubleArray deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jobject deholderize (JNIEnv *jni, jobject holder, const char *sig);

template<> idl2jni_runtime_Export
jobjectArray deholderize (JNIEnv *jni, jobject holder, const char *sig);


idl2jni_runtime_Export
void throw_java_exception (JNIEnv *jni, const CORBA::SystemException &se);

idl2jni_runtime_Export
void throw_cxx_exception (JNIEnv *jni, jthrowable excep);


class idl2jni_runtime_Export JStringMgr
{
public:
  JStringMgr (JNIEnv* jni, jstring input);
  ~JStringMgr ();

  const char* c_str() const
  {
    return c_str_;
  }

private:
  JNIEnv* jni_;
  jstring jstring_;
  const char* c_str_;
};


struct idl2jni_runtime_Export JniArgv
{
  JniArgv (JNIEnv *jni, jobject string_seq_holder);
  ~JniArgv ();

  JNIEnv *jni_;
  jobject ssholder_;
  std::vector<std::string> argv_;
  std::vector<const char *> orb_argv_;
  int argc_;

  char **orb_argv ()
  {
    return argc_ ? const_cast<char **> (&orb_argv_[0]) : 0;
  }
};


///Local guard object for attaching a C++ thread to the JVM
class idl2jni_runtime_Export JNIThreadAttacher
{
public:

  explicit JNIThreadAttacher (JavaVM *jvm)
    : jvm_ (jvm)
    , jni_ (0)
  {
    JavaVMAttachArgs args = {JNI_VERSION_1_4, 0, 0};
    void *jni;
    if (0 == jvm_->AttachCurrentThread (&jni, &args))
      {
        jni_ = reinterpret_cast<JNIEnv *> (jni);
      }
  }

  ~JNIThreadAttacher ()
  {
    jvm_->DetachCurrentThread ();
  }

  JNIEnv *getJNI () { return jni_; }

private:
  JavaVM *jvm_;
  JNIEnv *jni_;
};

#endif
