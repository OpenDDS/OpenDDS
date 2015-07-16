
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl dissector
// ------------------------------
#ifndef DISSECTOR_EXPORT_H
#define DISSECTOR_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (DISSECTOR_HAS_DLL)
#  define DISSECTOR_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && DISSECTOR_HAS_DLL */

#if !defined (DISSECTOR_HAS_DLL)
#  define DISSECTOR_HAS_DLL 1
#endif /* ! DISSECTOR_HAS_DLL */

#if defined (DISSECTOR_HAS_DLL) && (DISSECTOR_HAS_DLL == 1)
#  if defined (DISSECTOR_BUILD_DLL)
#    define dissector_Export ACE_Proper_Export_Flag
#    define DISSECTOR_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define DISSECTOR_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* DISSECTOR_BUILD_DLL */
#    define dissector_Export ACE_Proper_Import_Flag
#    define DISSECTOR_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define DISSECTOR_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* DISSECTOR_BUILD_DLL */
#else /* DISSECTOR_HAS_DLL == 1 */
#  define dissector_Export
#  define DISSECTOR_SINGLETON_DECLARATION(T)
#  define DISSECTOR_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* DISSECTOR_HAS_DLL == 1 */

// Set DISSECTOR_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (DISSECTOR_NTRACE)
#  if (ACE_NTRACE == 1)
#    define DISSECTOR_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define DISSECTOR_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !DISSECTOR_NTRACE */

#if (DISSECTOR_NTRACE == 1)
#  define DISSECTOR_TRACE(X)
#else /* (DISSECTOR_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define DISSECTOR_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (DISSECTOR_NTRACE == 1) */

#endif /* DISSECTOR_EXPORT_H */

// End of auto generated file.
