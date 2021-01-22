// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl OpenDDS_Dcps
// ------------------------------
#ifndef OPENDDS_DCPS_EXPORT_H
#define OPENDDS_DCPS_EXPORT_H

#include "ace/config-all.h"
#include "dds/Versioned_Namespace.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (OPENDDS_DCPS_HAS_DLL)
#  define OPENDDS_DCPS_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && OPENDDS_DCPS_HAS_DLL */

#if !defined (OPENDDS_DCPS_HAS_DLL)
#  define OPENDDS_DCPS_HAS_DLL 1
#endif /* !OPENDDS_DCPS_HAS_DLL */

#if defined (OPENDDS_DCPS_HAS_DLL) && (OPENDDS_DCPS_HAS_DLL == 1)
#  if defined (OPENDDS_DCPS_BUILD_DLL)
#    define OpenDDS_Dcps_Export ACE_Proper_Export_Flag
#    define OPENDDS_DCPS_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_DCPS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* OPENDDS_DCPS_BUILD_DLL */
#    define OpenDDS_Dcps_Export ACE_Proper_Import_Flag
#    define OPENDDS_DCPS_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_DCPS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* OPENDDS_DCPS_BUILD_DLL */
#else /* OPENDDS_DCPS_HAS_DLL == 1 */
#  define OpenDDS_Dcps_Export
#  define OPENDDS_DCPS_SINGLETON_DECLARATION(T)
#  define OPENDDS_DCPS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* OPENDDS_DCPS_HAS_DLL == 1 */

// Set OPENDDS_DCPS_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (OPENDDS_DCPS_NTRACE)
#  if (ACE_NTRACE == 1)
#    define OPENDDS_DCPS_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define OPENDDS_DCPS_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !OPENDDS_DCPS_NTRACE */

#if (OPENDDS_DCPS_NTRACE == 1)
#  define OPENDDS_DCPS_TRACE(X)
#else /* (OPENDDS_DCPS_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define OPENDDS_DCPS_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (OPENDDS_DCPS_NTRACE == 1) */

#endif /* OPENDDS_DCPS_EXPORT_H */

// End of auto generated file.
