
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl StringKey
// ------------------------------
#ifndef STRINGKEY_EXPORT_H
#define STRINGKEY_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (STRINGKEY_HAS_DLL)
#  define STRINGKEY_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && STRINGKEY_HAS_DLL */

#if !defined (STRINGKEY_HAS_DLL)
#  define STRINGKEY_HAS_DLL 1
#endif /* ! STRINGKEY_HAS_DLL */

#if defined (STRINGKEY_HAS_DLL) && (STRINGKEY_HAS_DLL == 1)
#  if defined (STRINGKEY_BUILD_DLL)
#    define StringKey_Export ACE_Proper_Export_Flag
#    define STRINGKEY_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define STRINGKEY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* STRINGKEY_BUILD_DLL */
#    define StringKey_Export ACE_Proper_Import_Flag
#    define STRINGKEY_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define STRINGKEY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* STRINGKEY_BUILD_DLL */
#else /* STRINGKEY_HAS_DLL == 1 */
#  define StringKey_Export
#  define STRINGKEY_SINGLETON_DECLARATION(T)
#  define STRINGKEY_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* STRINGKEY_HAS_DLL == 1 */

// Set STRINGKEY_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (STRINGKEY_NTRACE)
#  if (ACE_NTRACE == 1)
#    define STRINGKEY_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define STRINGKEY_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !STRINGKEY_NTRACE */

#if (STRINGKEY_NTRACE == 1)
#  define STRINGKEY_TRACE(X)
#else /* (STRINGKEY_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define STRINGKEY_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (STRINGKEY_NTRACE == 1) */

#endif /* STRINGKEY_EXPORT_H */

// End of auto generated file.
