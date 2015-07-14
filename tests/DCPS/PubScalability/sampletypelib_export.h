
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl SampleTypeLib
// ------------------------------
#ifndef SAMPLETYPELIB_EXPORT_H
#define SAMPLETYPELIB_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS)
# if !defined (SAMPLETYPELIB_HAS_DLL)
#   define SAMPLETYPELIB_HAS_DLL 0
# endif /* ! SAMPLETYPELIB_HAS_DLL */
#else
#if !defined (SAMPLETYPELIB_HAS_DLL)
#  define SAMPLETYPELIB_HAS_DLL 1
#endif /* ! SAMPLETYPELIB_HAS_DLL */
#endif /* ACE_AS_STATIC_LIBS */

#if defined (SAMPLETYPELIB_HAS_DLL) && (SAMPLETYPELIB_HAS_DLL == 1)
#  if defined (SAMPLETYPELIB_BUILD_DLL)
#    define SampleTypeLib_Export ACE_Proper_Export_Flag
#    define SAMPLETYPELIB_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SAMPLETYPELIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SAMPLETYPELIB_BUILD_DLL */
#    define SampleTypeLib_Export ACE_Proper_Import_Flag
#    define SAMPLETYPELIB_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SAMPLETYPELIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SAMPLETYPELIB_BUILD_DLL */
#else /* SAMPLETYPELIB_HAS_DLL == 1 */
#  define SampleTypeLib_Export
#  define SAMPLETYPELIB_SINGLETON_DECLARATION(T)
#  define SAMPLETYPELIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SAMPLETYPELIB_HAS_DLL == 1 */

// Set SAMPLETYPELIB_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SAMPLETYPELIB_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SAMPLETYPELIB_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SAMPLETYPELIB_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SAMPLETYPELIB_NTRACE */

#if (SAMPLETYPELIB_NTRACE == 1)
#  define SAMPLETYPELIB_TRACE(X)
#else /* (SAMPLETYPELIB_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SAMPLETYPELIB_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SAMPLETYPELIB_NTRACE == 1) */

#endif /* SAMPLETYPELIB_EXPORT_H */

// End of auto generated file.
