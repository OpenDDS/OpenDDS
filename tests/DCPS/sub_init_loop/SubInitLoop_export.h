
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl SubInitLoop
// ------------------------------
#ifndef SUBINITLOOP_EXPORT_H
#define SUBINITLOOP_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SUBINITLOOP_HAS_DLL)
#  define SUBINITLOOP_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SUBINITLOOP_HAS_DLL */

#if !defined (SUBINITLOOP_HAS_DLL)
#  define SUBINITLOOP_HAS_DLL 1
#endif /* ! SUBINITLOOP_HAS_DLL */

#if defined (SUBINITLOOP_HAS_DLL) && (SUBINITLOOP_HAS_DLL == 1)
#  if defined (SUBINITLOOP_BUILD_DLL)
#    define SubInitLoop_Export ACE_Proper_Export_Flag
#    define SUBINITLOOP_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SUBINITLOOP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SUBINITLOOP_BUILD_DLL */
#    define SubInitLoop_Export ACE_Proper_Import_Flag
#    define SUBINITLOOP_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SUBINITLOOP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SUBINITLOOP_BUILD_DLL */
#else /* SUBINITLOOP_HAS_DLL == 1 */
#  define SubInitLoop_Export
#  define SUBINITLOOP_SINGLETON_DECLARATION(T)
#  define SUBINITLOOP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SUBINITLOOP_HAS_DLL == 1 */

// Set SUBINITLOOP_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SUBINITLOOP_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SUBINITLOOP_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SUBINITLOOP_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SUBINITLOOP_NTRACE */

#if (SUBINITLOOP_NTRACE == 1)
#  define SUBINITLOOP_TRACE(X)
#else /* (SUBINITLOOP_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SUBINITLOOP_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SUBINITLOOP_NTRACE == 1) */

#endif /* SUBINITLOOP_EXPORT_H */

// End of auto generated file.
