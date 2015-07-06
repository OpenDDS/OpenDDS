
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl InstancesLib
// ------------------------------
#ifndef INSTANCESLIB_EXPORT_H
#define INSTANCESLIB_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (INSTANCESLIB_HAS_DLL)
#  define INSTANCESLIB_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && INSTANCESLIB_HAS_DLL */

#if !defined (INSTANCESLIB_HAS_DLL)
#  define INSTANCESLIB_HAS_DLL 1
#endif /* ! INSTANCESLIB_HAS_DLL */

#if defined (INSTANCESLIB_HAS_DLL) && (INSTANCESLIB_HAS_DLL == 1)
#  if defined (INSTANCESLIB_BUILD_DLL)
#    define InstancesLib_Export ACE_Proper_Export_Flag
#    define INSTANCESLIB_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define INSTANCESLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* INSTANCESLIB_BUILD_DLL */
#    define InstancesLib_Export ACE_Proper_Import_Flag
#    define INSTANCESLIB_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define INSTANCESLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* INSTANCESLIB_BUILD_DLL */
#else /* INSTANCESLIB_HAS_DLL == 1 */
#  define InstancesLib_Export
#  define INSTANCESLIB_SINGLETON_DECLARATION(T)
#  define INSTANCESLIB_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* INSTANCESLIB_HAS_DLL == 1 */

// Set INSTANCESLIB_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (INSTANCESLIB_NTRACE)
#  if (ACE_NTRACE == 1)
#    define INSTANCESLIB_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define INSTANCESLIB_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !INSTANCESLIB_NTRACE */

#if (INSTANCESLIB_NTRACE == 1)
#  define INSTANCESLIB_TRACE(X)
#else /* (INSTANCESLIB_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define INSTANCESLIB_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (INSTANCESLIB_NTRACE == 1) */

#endif /* INSTANCESLIB_EXPORT_H */

// End of auto generated file.
