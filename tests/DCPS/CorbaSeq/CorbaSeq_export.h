
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl CorbaSeq
// ------------------------------
#ifndef CORBASEQ_EXPORT_H
#define CORBASEQ_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (CORBASEQ_HAS_DLL)
#  define CORBASEQ_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && CORBASEQ_HAS_DLL */

#if !defined (CORBASEQ_HAS_DLL)
#  define CORBASEQ_HAS_DLL 1
#endif /* ! CORBASEQ_HAS_DLL */

#if defined (CORBASEQ_HAS_DLL) && (CORBASEQ_HAS_DLL == 1)
#  if defined (CORBASEQ_BUILD_DLL)
#    define CorbaSeq_Export ACE_Proper_Export_Flag
#    define CORBASEQ_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define CORBASEQ_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* CORBASEQ_BUILD_DLL */
#    define CorbaSeq_Export ACE_Proper_Import_Flag
#    define CORBASEQ_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define CORBASEQ_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* CORBASEQ_BUILD_DLL */
#else /* CORBASEQ_HAS_DLL == 1 */
#  define CorbaSeq_Export
#  define CORBASEQ_SINGLETON_DECLARATION(T)
#  define CORBASEQ_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* CORBASEQ_HAS_DLL == 1 */

// Set CORBASEQ_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (CORBASEQ_NTRACE)
#  if (ACE_NTRACE == 1)
#    define CORBASEQ_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define CORBASEQ_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !CORBASEQ_NTRACE */

#if (CORBASEQ_NTRACE == 1)
#  define CORBASEQ_TRACE(X)
#else /* (CORBASEQ_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define CORBASEQ_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (CORBASEQ_NTRACE == 1) */

#endif /* CORBASEQ_EXPORT_H */

// End of auto generated file.
