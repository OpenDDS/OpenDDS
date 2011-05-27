
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl TransportLatency
// ------------------------------
#ifndef TRANSPORTLATENCY_EXPORT_H
#define TRANSPORTLATENCY_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (TRANSPORTLATENCY_HAS_DLL)
#  define TRANSPORTLATENCY_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && TRANSPORTLATENCY_HAS_DLL */

#if !defined (TRANSPORTLATENCY_HAS_DLL)
#  define TRANSPORTLATENCY_HAS_DLL 1
#endif /* ! TRANSPORTLATENCY_HAS_DLL */

#if defined (TRANSPORTLATENCY_HAS_DLL) && (TRANSPORTLATENCY_HAS_DLL == 1)
#  if defined (TRANSPORTLATENCY_BUILD_DLL)
#    define TransportLatency_Export ACE_Proper_Export_Flag
#    define TRANSPORTLATENCY_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define TRANSPORTLATENCY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* TRANSPORTLATENCY_BUILD_DLL */
#    define TransportLatency_Export ACE_Proper_Import_Flag
#    define TRANSPORTLATENCY_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define TRANSPORTLATENCY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* TRANSPORTLATENCY_BUILD_DLL */
#else /* TRANSPORTLATENCY_HAS_DLL == 1 */
#  define TransportLatency_Export
#  define TRANSPORTLATENCY_SINGLETON_DECLARATION(T)
#  define TRANSPORTLATENCY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* TRANSPORTLATENCY_HAS_DLL == 1 */

// Set TRANSPORTLATENCY_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (TRANSPORTLATENCY_NTRACE)
#  if (ACE_NTRACE == 1)
#    define TRANSPORTLATENCY_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define TRANSPORTLATENCY_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !TRANSPORTLATENCY_NTRACE */

#if (TRANSPORTLATENCY_NTRACE == 1)
#  define TRANSPORTLATENCY_TRACE(X)
#else /* (TRANSPORTLATENCY_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define TRANSPORTLATENCY_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (TRANSPORTLATENCY_NTRACE == 1) */

#endif /* TRANSPORTLATENCY_EXPORT_H */

// End of auto generated file.
