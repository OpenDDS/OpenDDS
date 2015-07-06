
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl SetQosPartition
// ------------------------------
#ifndef SETQOSPARTITION_EXPORT_H
#define SETQOSPARTITION_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SETQOSPARTITION_HAS_DLL)
#  define SETQOSPARTITION_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SETQOSPARTITION_HAS_DLL */

#if !defined (SETQOSPARTITION_HAS_DLL)
#  define SETQOSPARTITION_HAS_DLL 1
#endif /* ! SETQOSPARTITION_HAS_DLL */

#if defined (SETQOSPARTITION_HAS_DLL) && (SETQOSPARTITION_HAS_DLL == 1)
#  if defined (SETQOSPARTITION_BUILD_DLL)
#    define SetQosPartition_Export ACE_Proper_Export_Flag
#    define SETQOSPARTITION_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SETQOSPARTITION_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SETQOSPARTITION_BUILD_DLL */
#    define SetQosPartition_Export ACE_Proper_Import_Flag
#    define SETQOSPARTITION_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SETQOSPARTITION_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SETQOSPARTITION_BUILD_DLL */
#else /* SETQOSPARTITION_HAS_DLL == 1 */
#  define SetQosPartition_Export
#  define SETQOSPARTITION_SINGLETON_DECLARATION(T)
#  define SETQOSPARTITION_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SETQOSPARTITION_HAS_DLL == 1 */

// Set SETQOSPARTITION_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SETQOSPARTITION_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SETQOSPARTITION_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SETQOSPARTITION_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SETQOSPARTITION_NTRACE */

#if (SETQOSPARTITION_NTRACE == 1)
#  define SETQOSPARTITION_TRACE(X)
#else /* (SETQOSPARTITION_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SETQOSPARTITION_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SETQOSPARTITION_NTRACE == 1) */

#endif /* SETQOSPARTITION_EXPORT_H */

// End of auto generated file.
