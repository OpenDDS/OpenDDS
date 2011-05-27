#include "idl2jni_runtime.h"
#include "idl2jni_BaseJavaPeer.h"

#include "tao/Version.h"
#if TAO_MAJOR_VERSION == 1 && TAO_MINOR_VERSION < 5
#  include "tao/Managed_Types.h"
#else
#  include "tao/String_Manager_T.h"
#  include "tao/String_Traits_T.h"
#  include "tao/String_Sequence_Element_T.h"
#endif

#include "tao/CORBA_String.h"
#include "tao/SystemException.h"

#ifndef IDL2JNI_NO_INC_SCSET_H
#  include "tao/String_Const_Sequence_Element_T.h"
#endif


JStringMgr::JStringMgr (JNIEnv* jni, jstring input)
  : jni_ (jni)
  , jstring_ (input)
  , c_str_ (jstring_ ? jni_->GetStringUTFChars (jstring_, 0) : "")
{
}

JStringMgr::~JStringMgr ()
{
  if (jstring_) jni_->ReleaseStringUTFChars (jstring_, c_str_);
}


JniArgv::JniArgv (JNIEnv *jni, jobject string_seq_holder)
  : jni_ (jni)
  , ssholder_ (string_seq_holder)
{
  jobjectArray ss_j = deholderize<jobjectArray> (jni, string_seq_holder,
    "[Ljava/lang/String;");
  jsize len = jni->GetArrayLength (ss_j);
  argv_.resize (len);
  orb_argv_.resize (len);
  for (jsize i = 0; i < len; ++i)
    {
      jstring jstr =
        static_cast<jstring> (jni->GetObjectArrayElement (ss_j, i));
      {
        JStringMgr jsm (jni, jstr);
        argv_[i] = jsm.c_str ();
        orb_argv_[i] = argv_[i].c_str ();
      }
      jni->DeleteLocalRef (jstr);
    }
  jni->DeleteLocalRef (ss_j);
  argc_ = len;
}

JniArgv::~JniArgv ()
{
  jobjectArray outArray =
    jni_->NewObjectArray (argc_, jni_->FindClass ("java/lang/String"), 0);
  for (jsize i = 0; i < argc_; ++i)
    {
      jstring str = jni_->NewStringUTF (orb_argv_[i]);
      jni_->SetObjectArrayElement (outArray, i, str);
      jni_->DeleteLocalRef (str);
    }
  holderize (jni_, ssholder_, outArray, "[Ljava/lang/String;");
  jni_->DeleteLocalRef (outArray);
}


void copyToCxx (JNIEnv *jni, IDL2JNI_STRMAN &target, jobject source)
{
  if (!source)
    {
      target = "";
      return;
    }
  jstring str = static_cast<jstring> (source);
  JStringMgr jsm (jni, str);
  const char *c_str = jsm.c_str ();
  target = c_str; //deep copy
}


void copyToJava (JNIEnv *jni, jobject &target, const IDL2JNI_STRMAN &source,
  bool)
{
  target = jni->NewStringUTF (source);
}


void copyToCxx (JNIEnv *jni, CORBA::String_var &target, jobject source)
{
  if (!source)
    {
      target = "";
      return;
    }
  jstring str = static_cast<jstring> (source);
  JStringMgr jsm (jni, str);
  const char *c_str = jsm.c_str ();
  target = c_str;
}


void copyToJava (JNIEnv *jni, jobject &target, const char *source, bool)
{
  target = jni->NewStringUTF (source);
}


void copyToCxx (JNIEnv *jni, IDL2JNI_STRELM target, jobject source)
{
  if (!source)
    {
      target = "";
      return;
    }
  jstring str = static_cast<jstring> (source);
  JStringMgr jsm (jni, str);
  const char *c_str = jsm.c_str ();
  target = c_str; //deep copy
}


void copyToJava (JNIEnv *jni, jobject &target,
  const IDL2JNI_STRELM_CONST &source, bool)
{
  target = jni->NewStringUTF (source);
}


#define HOLDER_PRIMITIVE(JNI_T, JNIFN, SIG)                                   \
void holderize (JNIEnv *jni, jobject holder, JNI_T value, const char *)       \
{                                                                             \
  jclass holderClazz = jni->GetObjectClass (holder);                          \
  jfieldID fid = jni->GetFieldID (holderClazz, "value", #SIG);                \
  jni->Set##JNIFN##Field (holder, fid, value);                                \
}                                                                             \
void holderize (JNIEnv *jni, jobject holder, JNI_T##Array value, const char *)\
{                                                                             \
  jclass holderClazz = jni->GetObjectClass (holder);                          \
  jfieldID fid = jni->GetFieldID (holderClazz, "value", "[" #SIG);            \
  jni->SetObjectField (holder, fid, value);                                   \
}
HOLDER_PRIMITIVE (jboolean, Boolean, Z)
HOLDER_PRIMITIVE (jchar, Char, C)
HOLDER_PRIMITIVE (jbyte, Byte, B)
HOLDER_PRIMITIVE (jshort, Short, S)
HOLDER_PRIMITIVE (jint, Int, I)
HOLDER_PRIMITIVE (jlong, Long, J)
HOLDER_PRIMITIVE (jfloat, Float, F)
HOLDER_PRIMITIVE (jdouble, Double, D)


void holderize (JNIEnv *jni, jobject holder, jobject value, const char *sig)
{
  jclass holderClazz = jni->GetObjectClass (holder);
  jfieldID fid = jni->GetFieldID (holderClazz, "value", sig);
  jni->SetObjectField (holder, fid, value);
}



#define DEHOLDER_PRIMITIVE(JNI_T, JNIFN, SIG)                               \
template<>                                                                  \
JNI_T deholderize (JNIEnv *jni, jobject holder, const char *)               \
{                                                                           \
  jclass holderClazz = jni->GetObjectClass (holder);                        \
  jfieldID fid = jni->GetFieldID (holderClazz, "value", #SIG);              \
  return jni->Get##JNIFN##Field (holder, fid);                              \
}                                                                           \
template <>                                                                 \
JNI_T##Array deholderize (JNIEnv *jni, jobject holder, const char *)        \
{                                                                           \
  jclass holderClazz = jni->GetObjectClass (holder);                        \
  jfieldID fid = jni->GetFieldID (holderClazz, "value", "[" #SIG);          \
  return static_cast<JNI_T##Array> (jni->GetObjectField (holder, fid));     \
}
DEHOLDER_PRIMITIVE (jboolean, Boolean, Z)
DEHOLDER_PRIMITIVE (jchar, Char, C)
DEHOLDER_PRIMITIVE (jbyte, Byte, B)
DEHOLDER_PRIMITIVE (jshort, Short, S)
DEHOLDER_PRIMITIVE (jint, Int, I)
DEHOLDER_PRIMITIVE (jlong, Long, J)
DEHOLDER_PRIMITIVE (jfloat, Float, F)
DEHOLDER_PRIMITIVE (jdouble, Double, D)


template<>
jobject deholderize (JNIEnv *jni, jobject holder, const char *sig)
{
  jclass holderClazz = jni->GetObjectClass (holder);
  jfieldID fid = jni->GetFieldID (holderClazz, "value", sig);
  return jni->GetObjectField (holder, fid);
}

template<>
jobjectArray deholderize (JNIEnv *jni, jobject holder, const char *sig)
{
  jclass holderClazz = jni->GetObjectClass (holder);
  jfieldID fid = jni->GetFieldID (holderClazz, "value", sig);
  return static_cast<jobjectArray> (jni->GetObjectField (holder, fid));
}


#define CORBA_SYSTEM_EXCEPTION(NAME)                                        \
  if (dynamic_cast<const CORBA::NAME *> (&se))                              \
    {                                                                       \
      jclass clazz = jni->FindClass ("org/omg/CORBA/" #NAME);               \
      jmethodID mid = jni->GetMethodID (clazz, "<init>",                    \
        "(ILorg/omg/CORBA/CompletionStatus;)V");                            \
      jthrowable jex = static_cast<jthrowable> (jni->NewObject (clazz, mid, \
        minor, j_completed));                                               \
      jni->Throw (jex);                                                     \
    }                                                                       \
  else

void throw_java_exception (JNIEnv *jni, const CORBA::SystemException &se)
{
  CORBA::ULong minor = se.minor ();
  jclass cs_clazz = jni->FindClass ("org/omg/CORBA/CompletionStatus");
  jmethodID cs_mid = jni->GetStaticMethodID (cs_clazz, "from_int",
    "(I)Lorg/omg/CORBA/CompletionStatus;");
  jobject j_completed = jni->CallStaticObjectMethod (cs_clazz, cs_mid,
    static_cast<jint> (se.completed ()));
  CORBA_SYSTEM_EXCEPTION (UNKNOWN)
  CORBA_SYSTEM_EXCEPTION (BAD_PARAM)
  CORBA_SYSTEM_EXCEPTION (NO_MEMORY)
  CORBA_SYSTEM_EXCEPTION (IMP_LIMIT)
  CORBA_SYSTEM_EXCEPTION (COMM_FAILURE)
  CORBA_SYSTEM_EXCEPTION (INV_OBJREF)
  CORBA_SYSTEM_EXCEPTION (OBJECT_NOT_EXIST)
  CORBA_SYSTEM_EXCEPTION (NO_PERMISSION)
  CORBA_SYSTEM_EXCEPTION (INTERNAL)
  CORBA_SYSTEM_EXCEPTION (MARSHAL)
  CORBA_SYSTEM_EXCEPTION (INITIALIZE)
  CORBA_SYSTEM_EXCEPTION (NO_IMPLEMENT)
  CORBA_SYSTEM_EXCEPTION (BAD_TYPECODE)
  CORBA_SYSTEM_EXCEPTION (BAD_OPERATION)
  CORBA_SYSTEM_EXCEPTION (NO_RESOURCES)
  CORBA_SYSTEM_EXCEPTION (NO_RESPONSE)
  CORBA_SYSTEM_EXCEPTION (PERSIST_STORE)
  CORBA_SYSTEM_EXCEPTION (BAD_INV_ORDER)
  CORBA_SYSTEM_EXCEPTION (TRANSIENT)
  CORBA_SYSTEM_EXCEPTION (FREE_MEM)
  CORBA_SYSTEM_EXCEPTION (INV_IDENT)
  CORBA_SYSTEM_EXCEPTION (INV_FLAG)
  CORBA_SYSTEM_EXCEPTION (INTF_REPOS)
  CORBA_SYSTEM_EXCEPTION (BAD_CONTEXT)
  CORBA_SYSTEM_EXCEPTION (OBJ_ADAPTER)
  CORBA_SYSTEM_EXCEPTION (DATA_CONVERSION)
  CORBA_SYSTEM_EXCEPTION (INV_POLICY)
  CORBA_SYSTEM_EXCEPTION (REBIND)
  CORBA_SYSTEM_EXCEPTION (TIMEOUT)
  CORBA_SYSTEM_EXCEPTION (TRANSACTION_UNAVAILABLE)
  CORBA_SYSTEM_EXCEPTION (TRANSACTION_MODE)
  CORBA_SYSTEM_EXCEPTION (TRANSACTION_REQUIRED)
  CORBA_SYSTEM_EXCEPTION (TRANSACTION_ROLLEDBACK)
  CORBA_SYSTEM_EXCEPTION (INVALID_TRANSACTION)
  CORBA_SYSTEM_EXCEPTION (CODESET_INCOMPATIBLE)
  CORBA_SYSTEM_EXCEPTION (BAD_QOS)
  CORBA_SYSTEM_EXCEPTION (INVALID_ACTIVITY)
  CORBA_SYSTEM_EXCEPTION (ACTIVITY_COMPLETED)
  CORBA_SYSTEM_EXCEPTION (ACTIVITY_REQUIRED)
  //the macro ends with "else" so this is the fallback case
    {
      jclass clazz = jni->FindClass ("org/omg/CORBA/UNKNOWN");
      jmethodID mid = jni->GetMethodID (clazz, "<init>",
        "(ILorg/omg/CORBA/CompletionStatus;)V");
      jthrowable jex = static_cast<jthrowable> (jni->NewObject (clazz, mid,
        minor, j_completed));
      jni->Throw (jex);
    }
}


#define TAO_SYSTEM_EXCEPTION(NAME)                                            \
  if (clazz == jni->FindClass ("org/omg/CORBA/" #NAME))                       \
    {                                                                         \
      jfieldID fid_min = jni->GetFieldID (clazz, "minor", "I");               \
      jint minor = jni->GetIntField (excep, fid_min);                         \
      jfieldID fid_comp = jni->GetFieldID (clazz, "completed",                \
        "Lorg/omg/CORBA/CompletionStatus;");                                  \
      jobject comp_obj = jni->GetObjectField (excep, fid_comp);               \
      jint comp = jni->CallIntMethod (comp_obj, mid_comp_val);                \
      jni->DeleteLocalRef (comp_obj);                                         \
      throw CORBA::NAME (minor, static_cast<CORBA::CompletionStatus> (comp)); \
    }                                                                         \
  else

void throw_cxx_exception (JNIEnv *jni, jthrowable excep)
{
  // call jni->ExceptionClear() before this method exits (via throw)
  struct ClearTheException
  {
    explicit ClearTheException (JNIEnv *j)
      : j_ (j)
    {}
    ~ClearTheException ()
    {
      j_->ExceptionClear ();
    }
    JNIEnv *j_;
  } cte (jni);

  jclass clazz = jni->GetObjectClass (excep);
  jclass clazz_comp = jni->FindClass ("org/omg/CORBA/CompletionStatus");
  jmethodID mid_comp_val = jni->GetMethodID (clazz_comp, "value", "()I");
  TAO_SYSTEM_EXCEPTION (UNKNOWN)
  TAO_SYSTEM_EXCEPTION (BAD_PARAM)
  TAO_SYSTEM_EXCEPTION (NO_MEMORY)
  TAO_SYSTEM_EXCEPTION (IMP_LIMIT)
  TAO_SYSTEM_EXCEPTION (COMM_FAILURE)
  TAO_SYSTEM_EXCEPTION (INV_OBJREF)
  TAO_SYSTEM_EXCEPTION (OBJECT_NOT_EXIST)
  TAO_SYSTEM_EXCEPTION (NO_PERMISSION)
  TAO_SYSTEM_EXCEPTION (INTERNAL)
  TAO_SYSTEM_EXCEPTION (MARSHAL)
  TAO_SYSTEM_EXCEPTION (INITIALIZE)
  TAO_SYSTEM_EXCEPTION (NO_IMPLEMENT)
  TAO_SYSTEM_EXCEPTION (BAD_TYPECODE)
  TAO_SYSTEM_EXCEPTION (BAD_OPERATION)
  TAO_SYSTEM_EXCEPTION (NO_RESOURCES)
  TAO_SYSTEM_EXCEPTION (NO_RESPONSE)
  TAO_SYSTEM_EXCEPTION (PERSIST_STORE)
  TAO_SYSTEM_EXCEPTION (BAD_INV_ORDER)
  TAO_SYSTEM_EXCEPTION (TRANSIENT)
  TAO_SYSTEM_EXCEPTION (FREE_MEM)
  TAO_SYSTEM_EXCEPTION (INV_IDENT)
  TAO_SYSTEM_EXCEPTION (INV_FLAG)
  TAO_SYSTEM_EXCEPTION (INTF_REPOS)
  TAO_SYSTEM_EXCEPTION (BAD_CONTEXT)
  TAO_SYSTEM_EXCEPTION (OBJ_ADAPTER)
  TAO_SYSTEM_EXCEPTION (DATA_CONVERSION)
  TAO_SYSTEM_EXCEPTION (INV_POLICY)
  TAO_SYSTEM_EXCEPTION (REBIND)
  TAO_SYSTEM_EXCEPTION (TIMEOUT)
  TAO_SYSTEM_EXCEPTION (TRANSACTION_UNAVAILABLE)
  TAO_SYSTEM_EXCEPTION (TRANSACTION_MODE)
  TAO_SYSTEM_EXCEPTION (TRANSACTION_REQUIRED)
  TAO_SYSTEM_EXCEPTION (TRANSACTION_ROLLEDBACK)
  TAO_SYSTEM_EXCEPTION (INVALID_TRANSACTION)
  TAO_SYSTEM_EXCEPTION (CODESET_INCOMPATIBLE)
  TAO_SYSTEM_EXCEPTION (BAD_QOS)
  TAO_SYSTEM_EXCEPTION (INVALID_ACTIVITY)
  TAO_SYSTEM_EXCEPTION (ACTIVITY_COMPLETED)
  TAO_SYSTEM_EXCEPTION (ACTIVITY_REQUIRED)
  // the macro ends with "else" so this is the fallback case
    {
      jni->ExceptionDescribe ();
      throw CORBA::UNKNOWN ();
    }
}


IDL2JNI_BaseJavaPeer::~IDL2JNI_BaseJavaPeer ()
{
  JNIThreadAttacher jta (jvm_);
  jta.getJNI ()->DeleteGlobalRef (globalCallback_);
}
