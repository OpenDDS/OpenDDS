
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl -s SimpleMcast
// ------------------------------
#ifndef SIMPLEMCAST_EXPORT_H
#define SIMPLEMCAST_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SIMPLEMCAST_HAS_DLL)
#  define SIMPLEMCAST_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SIMPLEMCAST_HAS_DLL */

#if !defined (SIMPLEMCAST_HAS_DLL)
#  define SIMPLEMCAST_HAS_DLL 1
#endif /* ! SIMPLEMCAST_HAS_DLL */

#if defined (SIMPLEMCAST_HAS_DLL) && (SIMPLEMCAST_HAS_DLL == 1)
#  if defined (SIMPLEMCAST_BUILD_DLL)
#    define SimpleMcast_Export ACE_Proper_Export_Flag
#    define SIMPLEMCAST_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SIMPLEMCAST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SIMPLEMCAST_BUILD_DLL */
#    define SimpleMcast_Export ACE_Proper_Import_Flag
#    define SIMPLEMCAST_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SIMPLEMCAST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SIMPLEMCAST_BUILD_DLL */
#else /* SIMPLEMCAST_HAS_DLL == 1 */
#  define SimpleMcast_Export
#  define SIMPLEMCAST_SINGLETON_DECLARATION(T)
#  define SIMPLEMCAST_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SIMPLEMCAST_HAS_DLL == 1 */

// Set SIMPLEMCAST_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SIMPLEMCAST_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SIMPLEMCAST_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SIMPLEMCAST_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SIMPLEMCAST_NTRACE */

#if (SIMPLEMCAST_NTRACE == 1)
#  define SIMPLEMCAST_TRACE(X)
#else /* (SIMPLEMCAST_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SIMPLEMCAST_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SIMPLEMCAST_NTRACE == 1) */

#endif /* SIMPLEMCAST_EXPORT_H */

// End of auto generated file.
