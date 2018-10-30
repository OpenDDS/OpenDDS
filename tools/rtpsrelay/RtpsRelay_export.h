
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl RtpsRelay
// ------------------------------
#ifndef RTPSRELAY_EXPORT_H
#define RTPSRELAY_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (RTPSRELAY_HAS_DLL)
#  define RTPSRELAY_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && RTPSRELAY_HAS_DLL */

#if !defined (RTPSRELAY_HAS_DLL)
#  define RTPSRELAY_HAS_DLL 1
#endif /* ! RTPSRELAY_HAS_DLL */

#if defined (RTPSRELAY_HAS_DLL) && (RTPSRELAY_HAS_DLL == 1)
#  if defined (RTPSRELAY_BUILD_DLL)
#    define RtpsRelay_Export ACE_Proper_Export_Flag
#    define RTPSRELAY_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define RTPSRELAY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* RTPSRELAY_BUILD_DLL */
#    define RtpsRelay_Export ACE_Proper_Import_Flag
#    define RTPSRELAY_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define RTPSRELAY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* RTPSRELAY_BUILD_DLL */
#else /* RTPSRELAY_HAS_DLL == 1 */
#  define RtpsRelay_Export
#  define RTPSRELAY_SINGLETON_DECLARATION(T)
#  define RTPSRELAY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* RTPSRELAY_HAS_DLL == 1 */

// Set RTPSRELAY_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (RTPSRELAY_NTRACE)
#  if (ACE_NTRACE == 1)
#    define RTPSRELAY_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define RTPSRELAY_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !RTPSRELAY_NTRACE */

#if (RTPSRELAY_NTRACE == 1)
#  define RTPSRELAY_TRACE(X)
#else /* (RTPSRELAY_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define RTPSRELAY_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (RTPSRELAY_NTRACE == 1) */

#endif /* RTPSRELAY_EXPORT_H */

// End of auto generated file.
