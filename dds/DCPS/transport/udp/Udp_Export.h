// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl OpenDDS_Udp
// ------------------------------
#ifndef OPENDDS_UDP_EXPORT_H
#define OPENDDS_UDP_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (OPENDDS_UDP_HAS_DLL)
#  define OPENDDS_UDP_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && OPENDDS_UDP_HAS_DLL */

#if !defined (OPENDDS_UDP_HAS_DLL)
#  define OPENDDS_UDP_HAS_DLL 1
#endif /* ! OPENDDS_UDP_HAS_DLL */

#if defined (OPENDDS_UDP_HAS_DLL) && (OPENDDS_UDP_HAS_DLL == 1)
#  if defined (OPENDDS_UDP_BUILD_DLL)
#    define OpenDDS_Udp_Export ACE_Proper_Export_Flag
#    define OPENDDS_UDP_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_UDP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* OPENDDS_UDP_BUILD_DLL */
#    define OpenDDS_Udp_Export ACE_Proper_Import_Flag
#    define OPENDDS_UDP_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_UDP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* OPENDDS_UDP_BUILD_DLL */
#else /* OPENDDS_UDP_HAS_DLL == 1 */
#  define OpenDDS_Udp_Export
#  define OPENDDS_UDP_SINGLETON_DECLARATION(T)
#  define OPENDDS_UDP_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* OPENDDS_UDP_HAS_DLL == 1 */

// Set OPENDDS_UDP_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (OPENDDS_UDP_NTRACE)
#  if (ACE_NTRACE == 1)
#    define OPENDDS_UDP_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define OPENDDS_UDP_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !OPENDDS_UDP_NTRACE */

#if (OPENDDS_UDP_NTRACE == 1)
#  define OPENDDS_UDP_TRACE(X)
#else /* (OPENDDS_UDP_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define OPENDDS_UDP_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (OPENDDS_UDP_NTRACE == 1) */

#endif /* OPENDDS_UDP_EXPORT_H */

// End of auto generated file.
