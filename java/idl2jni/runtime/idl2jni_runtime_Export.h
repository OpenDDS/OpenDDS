// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl idl2jni_runtime
// ------------------------------
#ifndef IDL2JNI_RUNTIME_EXPORT_H
#define IDL2JNI_RUNTIME_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (IDL2JNI_RUNTIME_HAS_DLL)
#  define IDL2JNI_RUNTIME_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && IDL2JNI_RUNTIME_HAS_DLL */

#if !defined (IDL2JNI_RUNTIME_HAS_DLL)
#  define IDL2JNI_RUNTIME_HAS_DLL 1
#endif /* !IDL2JNI_RUNTIME_HAS_DLL */

#if defined (IDL2JNI_RUNTIME_HAS_DLL) && (IDL2JNI_RUNTIME_HAS_DLL == 1)
#  if defined (IDL2JNI_RUNTIME_BUILD_DLL)
#    define idl2jni_runtime_Export ACE_Proper_Export_Flag
#    define IDL2JNI_RUNTIME_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define IDL2JNI_RUNTIME_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* IDL2JNI_RUNTIME_BUILD_DLL */
#    define idl2jni_runtime_Export ACE_Proper_Import_Flag
#    define IDL2JNI_RUNTIME_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define IDL2JNI_RUNTIME_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* IDL2JNI_RUNTIME_BUILD_DLL */
#else /* IDL2JNI_RUNTIME_HAS_DLL == 1 */
#  define idl2jni_runtime_Export
#  define IDL2JNI_RUNTIME_SINGLETON_DECLARATION(T)
#  define IDL2JNI_RUNTIME_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* IDL2JNI_RUNTIME_HAS_DLL == 1 */

// Set IDL2JNI_RUNTIME_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (IDL2JNI_RUNTIME_NTRACE)
#  if (ACE_NTRACE == 1)
#    define IDL2JNI_RUNTIME_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define IDL2JNI_RUNTIME_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !IDL2JNI_RUNTIME_NTRACE */

#if (IDL2JNI_RUNTIME_NTRACE == 1)
#  define IDL2JNI_RUNTIME_TRACE(X)
#else /* (IDL2JNI_RUNTIME_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define IDL2JNI_RUNTIME_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (IDL2JNI_RUNTIME_NTRACE == 1) */

#endif /* IDL2JNI_RUNTIME_EXPORT_H */

// End of auto generated file.
