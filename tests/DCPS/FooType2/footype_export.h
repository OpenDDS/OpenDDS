
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl FooType
// ------------------------------
#ifndef FOOTYPE_EXPORT_H
#define FOOTYPE_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS)
# if !defined (FOOTYPE_HAS_DLL)
#   define FOOTYPE_HAS_DLL 0
# endif /* ! FOOTYPE_HAS_DLL */
#else
#if !defined (FOOTYPE_HAS_DLL)
#  define FOOTYPE_HAS_DLL 1
#endif /* ! FOOTYPE_HAS_DLL */
#endif /* ACE_AS_STATIC_LIBS */

#if defined (FOOTYPE_HAS_DLL) && (FOOTYPE_HAS_DLL == 1)
#  if defined (FOOTYPE_BUILD_DLL)
#    define FooType_Export ACE_Proper_Export_Flag
#    define FOOTYPE_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define FOOTYPE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* FOOTYPE_BUILD_DLL */
#    define FooType_Export ACE_Proper_Import_Flag
#    define FOOTYPE_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define FOOTYPE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* FOOTYPE_BUILD_DLL */
#else /* FOOTYPE_HAS_DLL == 1 */
#  define FooType_Export
#  define FOOTYPE_SINGLETON_DECLARATION(T)
#  define FOOTYPE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* FOOTYPE_HAS_DLL == 1 */

// Set FOOTYPE_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (FOOTYPE_NTRACE)
#  if (ACE_NTRACE == 1)
#    define FOOTYPE_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define FOOTYPE_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !FOOTYPE_NTRACE */

#if (FOOTYPE_NTRACE == 1)
#  define FOOTYPE_TRACE(X)
#else /* (FOOTYPE_NTRACE == 1) */
#  define FOOTYPE_TRACE(X) ACE_TRACE_IMPL(X)
#endif /* (FOOTYPE_NTRACE == 1) */

#endif /* FOOTYPE_EXPORT_H */

// End of auto generated file.
