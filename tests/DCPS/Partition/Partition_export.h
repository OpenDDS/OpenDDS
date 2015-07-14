
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Partition
// ------------------------------
#ifndef PARTITION_EXPORT_H
#define PARTITION_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (PARTITION_HAS_DLL)
#  define PARTITION_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && PARTITION_HAS_DLL */

#if !defined (PARTITION_HAS_DLL)
#  define PARTITION_HAS_DLL 1
#endif /* ! PARTITION_HAS_DLL */

#if defined (PARTITION_HAS_DLL) && (PARTITION_HAS_DLL == 1)
#  if defined (PARTITION_BUILD_DLL)
#    define Partition_Export ACE_Proper_Export_Flag
#    define PARTITION_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define PARTITION_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* PARTITION_BUILD_DLL */
#    define Partition_Export ACE_Proper_Import_Flag
#    define PARTITION_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define PARTITION_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* PARTITION_BUILD_DLL */
#else /* PARTITION_HAS_DLL == 1 */
#  define Partition_Export
#  define PARTITION_SINGLETON_DECLARATION(T)
#  define PARTITION_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* PARTITION_HAS_DLL == 1 */

// Set PARTITION_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (PARTITION_NTRACE)
#  if (ACE_NTRACE == 1)
#    define PARTITION_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define PARTITION_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !PARTITION_NTRACE */

#if (PARTITION_NTRACE == 1)
#  define PARTITION_TRACE(X)
#else /* (PARTITION_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define PARTITION_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (PARTITION_NTRACE == 1) */

#endif /* PARTITION_EXPORT_H */

// End of auto generated file.
