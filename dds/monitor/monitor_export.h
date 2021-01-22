
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl -s OpenDDS_monitor
// ------------------------------
#ifndef OPENDDS_MONITOR_EXPORT_H
#define OPENDDS_MONITOR_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (OPENDDS_MONITOR_HAS_DLL)
#  define OPENDDS_MONITOR_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && OPENDDS_MONITOR_HAS_DLL */

#if !defined (OPENDDS_MONITOR_HAS_DLL)
#  define OPENDDS_MONITOR_HAS_DLL 1
#endif /* ! OPENDDS_MONITOR_HAS_DLL */

#if defined (OPENDDS_MONITOR_HAS_DLL) && (OPENDDS_MONITOR_HAS_DLL == 1)
#  if defined (OPENDDS_MONITOR_BUILD_DLL)
#    define OpenDDS_monitor_Export ACE_Proper_Export_Flag
#    define OPENDDS_MONITOR_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_MONITOR_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* OPENDDS_MONITOR_BUILD_DLL */
#    define OpenDDS_monitor_Export ACE_Proper_Import_Flag
#    define OPENDDS_MONITOR_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_MONITOR_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* OPENDDS_MONITOR_BUILD_DLL */
#else /* OPENDDS_MONITOR_HAS_DLL == 1 */
#  define OpenDDS_monitor_Export
#  define OPENDDS_MONITOR_SINGLETON_DECLARATION(T)
#  define OPENDDS_MONITOR_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* OPENDDS_MONITOR_HAS_DLL == 1 */

// Set OPENDDS_MONITOR_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (OPENDDS_MONITOR_NTRACE)
#  if (ACE_NTRACE == 1)
#    define OPENDDS_MONITOR_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define OPENDDS_MONITOR_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !OPENDDS_MONITOR_NTRACE */

#if (OPENDDS_MONITOR_NTRACE == 1)
#  define OPENDDS_MONITOR_TRACE(X)
#else /* (OPENDDS_MONITOR_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define OPENDDS_MONITOR_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (OPENDDS_MONITOR_NTRACE == 1) */

#endif /* OPENDDS_MONITOR_EXPORT_H */

// End of auto generated file.
