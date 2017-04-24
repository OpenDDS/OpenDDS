
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl SampleLost
// ------------------------------
#ifndef SAMPLELOST_EXPORT_H
#define SAMPLELOST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SAMPLELOST_HAS_DLL)
#  define SAMPLELOST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SAMPLELOST_HAS_DLL */

#if !defined (SAMPLELOST_HAS_DLL)
#  define SAMPLELOST_HAS_DLL 1
#endif /* ! SAMPLELOST_HAS_DLL */

#if defined (SAMPLELOST_HAS_DLL) && (SAMPLELOST_HAS_DLL == 1)
#  if defined (SAMPLELOST_BUILD_DLL)
#    define SampleLost_Export ACE_Proper_Export_Flag
#    define SAMPLELOST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SAMPLELOST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SAMPLELOST_BUILD_DLL */
#    define SampleLost_Export ACE_Proper_Import_Flag
#    define SAMPLELOST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SAMPLELOST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SAMPLELOST_BUILD_DLL */
#else /* SAMPLELOST_HAS_DLL == 1 */
#  define SampleLost_Export
#  define SAMPLELOST_SINGLETON_DECLARATION(T)
#  define SAMPLELOST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SAMPLELOST_HAS_DLL == 1 */

// Set SAMPLELOST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SAMPLELOST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SAMPLELOST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SAMPLELOST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SAMPLELOST_NTRACE */

#if (SAMPLELOST_NTRACE == 1)
#  define SAMPLELOST_TRACE(X)
#else /* (SAMPLELOST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SAMPLELOST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SAMPLELOST_NTRACE == 1) */

#endif /* SAMPLELOST_EXPORT_H */

// End of auto generated file.
