
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Lifespan
// ------------------------------
#ifndef LIFESPAN_EXPORT_H
#define LIFESPAN_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (LIFESPAN_HAS_DLL)
#  define LIFESPAN_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && LIFESPAN_HAS_DLL */

#if !defined (LIFESPAN_HAS_DLL)
#  define LIFESPAN_HAS_DLL 1
#endif /* ! LIFESPAN_HAS_DLL */

#if defined (LIFESPAN_HAS_DLL) && (LIFESPAN_HAS_DLL == 1)
#  if defined (LIFESPAN_BUILD_DLL)
#    define Lifespan_Export ACE_Proper_Export_Flag
#    define LIFESPAN_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define LIFESPAN_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* LIFESPAN_BUILD_DLL */
#    define Lifespan_Export ACE_Proper_Import_Flag
#    define LIFESPAN_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define LIFESPAN_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* LIFESPAN_BUILD_DLL */
#else /* LIFESPAN_HAS_DLL == 1 */
#  define Lifespan_Export
#  define LIFESPAN_SINGLETON_DECLARATION(T)
#  define LIFESPAN_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* LIFESPAN_HAS_DLL == 1 */

// Set LIFESPAN_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (LIFESPAN_NTRACE)
#  if (ACE_NTRACE == 1)
#    define LIFESPAN_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define LIFESPAN_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !LIFESPAN_NTRACE */

#if (LIFESPAN_NTRACE == 1)
#  define LIFESPAN_TRACE(X)
#else /* (LIFESPAN_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define LIFESPAN_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (LIFESPAN_NTRACE == 1) */

#endif /* LIFESPAN_EXPORT_H */

// End of auto generated file.
