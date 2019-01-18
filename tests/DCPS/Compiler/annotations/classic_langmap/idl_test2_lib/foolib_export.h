
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl FooLib
// ------------------------------
#ifndef FOOLIB_EXPORT_H
#define FOOLIB_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS)
# if !defined (FOOLIB_HAS_DLL)
#   define FOOLIB_HAS_DLL 0
# endif /* ! FOOLIB_HAS_DLL */
#else
#if !defined (FOOLIB_HAS_DLL)
#  define FOOLIB_HAS_DLL 1
#endif /* ! FOOLIB_HAS_DLL */
#endif /* ACE_AS_STATIC_LIBS */

#if defined (FOOLIB_HAS_DLL) && (FOOLIB_HAS_DLL == 1)
#  if defined (FOOLIB_BUILD_DLL)
#    define FooLib_Export ACE_Proper_Export_Flag
#    define FOOLIB_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define FOOLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* FOOLIB_BUILD_DLL */
#    define FooLib_Export ACE_Proper_Import_Flag
#    define FOOLIB_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define FOOLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* FOOLIB_BUILD_DLL */
#else /* FOOLIB_HAS_DLL == 1 */
#  define FooLib_Export
#  define FOOLIB_SINGLETON_DECLARATION(T)
#  define FOOLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* FOOLIB_HAS_DLL == 1 */

// Set FOOLIB_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (FOOLIB_NTRACE)
#  if (ACE_NTRACE == 1)
#    define FOOLIB_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define FOOLIB_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !FOOLIB_NTRACE */

#if (FOOLIB_NTRACE == 1)
#  define FOOLIB_TRACE(X)
#else /* (FOOLIB_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define FOOLIB_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (FOOLIB_NTRACE == 1) */

#endif /* FOOLIB_EXPORT_H */

// End of auto generated file.
