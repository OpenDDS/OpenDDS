
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl RtpsRelayLib
// ------------------------------
#ifndef RTPSRELAYLIB_EXPORT_H
#define RTPSRELAYLIB_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (RTPSRELAYLIB_HAS_DLL)
#  define RTPSRELAYLIB_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && RTPSRELAYLIB_HAS_DLL */

#if !defined (RTPSRELAYLIB_HAS_DLL)
#  define RTPSRELAYLIB_HAS_DLL 1
#endif /* ! RTPSRELAYLIB_HAS_DLL */

#if defined (RTPSRELAYLIB_HAS_DLL) && (RTPSRELAYLIB_HAS_DLL == 1)
#  if defined (RTPSRELAYLIB_BUILD_DLL)
#    define RtpsRelayLib_Export ACE_Proper_Export_Flag
#    define RTPSRELAYLIB_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define RTPSRELAYLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* RTPSRELAYLIB_BUILD_DLL */
#    define RtpsRelayLib_Export ACE_Proper_Import_Flag
#    define RTPSRELAYLIB_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define RTPSRELAYLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* RTPSRELAYLIB_BUILD_DLL */
#else /* RTPSRELAYLIB_HAS_DLL == 1 */
#  define RtpsRelayLib_Export
#  define RTPSRELAYLIB_SINGLETON_DECLARATION(T)
#  define RTPSRELAYLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* RTPSRELAYLIB_HAS_DLL == 1 */

// Set RTPSRELAYLIB_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (RTPSRELAYLIB_NTRACE)
#  if (ACE_NTRACE == 1)
#    define RTPSRELAYLIB_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define RTPSRELAYLIB_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !RTPSRELAYLIB_NTRACE */

#if (RTPSRELAYLIB_NTRACE == 1)
#  define RTPSRELAYLIB_TRACE(X)
#else /* (RTPSRELAYLIB_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define RTPSRELAYLIB_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (RTPSRELAYLIB_NTRACE == 1) */

#endif /* RTPSRELAYLIB_EXPORT_H */

// End of auto generated file.
