// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl dcps_java
// ------------------------------
#ifndef DCPS_JAVA_EXPORT_H
#define DCPS_JAVA_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (DCPS_JAVA_HAS_DLL)
#  define DCPS_JAVA_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && DCPS_JAVA_HAS_DLL */

#if !defined (DCPS_JAVA_HAS_DLL)
#  define DCPS_JAVA_HAS_DLL 1
#endif /* !DCPS_JAVA_HAS_DLL */

#if defined (DCPS_JAVA_HAS_DLL) && (DCPS_JAVA_HAS_DLL == 1)
#  if defined (DCPS_JAVA_BUILD_DLL)
#    define dcps_java_Export ACE_Proper_Export_Flag
#    define DCPS_JAVA_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define DCPS_JAVA_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* DCPS_JAVA_BUILD_DLL */
#    define dcps_java_Export ACE_Proper_Import_Flag
#    define DCPS_JAVA_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define DCPS_JAVA_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* DCPS_JAVA_BUILD_DLL */
#else /* DCPS_JAVA_HAS_DLL == 1 */
#  define dcps_java_Export
#  define DCPS_JAVA_SINGLETON_DECLARATION(T)
#  define DCPS_JAVA_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* DCPS_JAVA_HAS_DLL == 1 */

// Set DCPS_JAVA_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (DCPS_JAVA_NTRACE)
#  if (ACE_NTRACE == 1)
#    define DCPS_JAVA_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define DCPS_JAVA_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !DCPS_JAVA_NTRACE */

#if (DCPS_JAVA_NTRACE == 1)
#  define DCPS_JAVA_TRACE(X)
#else /* (DCPS_JAVA_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define DCPS_JAVA_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (DCPS_JAVA_NTRACE == 1) */

#endif /* DCPS_JAVA_EXPORT_H */

// End of auto generated file.
