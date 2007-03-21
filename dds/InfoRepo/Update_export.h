
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Update
// ------------------------------
#ifndef UPDATE_EXPORT_H
#define UPDATE_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (UPDATE_HAS_DLL)
#  define UPDATE_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && UPDATE_HAS_DLL */

#if !defined (UPDATE_HAS_DLL)
#  define UPDATE_HAS_DLL 1
#endif /* ! UPDATE_HAS_DLL */

#if defined (UPDATE_HAS_DLL) && (UPDATE_HAS_DLL == 1)
#  if defined (UPDATE_BUILD_DLL)
#    define Update_Export ACE_Proper_Export_Flag
#    define UPDATE_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define UPDATE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* UPDATE_BUILD_DLL */
#    define Update_Export ACE_Proper_Import_Flag
#    define UPDATE_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define UPDATE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* UPDATE_BUILD_DLL */
#else /* UPDATE_HAS_DLL == 1 */
#  define Update_Export
#  define UPDATE_SINGLETON_DECLARATION(T)
#  define UPDATE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* UPDATE_HAS_DLL == 1 */

// Set UPDATE_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (UPDATE_NTRACE)
#  if (ACE_NTRACE == 1)
#    define UPDATE_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define UPDATE_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !UPDATE_NTRACE */

#if (UPDATE_NTRACE == 1)
#  define UPDATE_TRACE(X)
#else /* (UPDATE_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define UPDATE_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (UPDATE_NTRACE == 1) */

#endif /* UPDATE_EXPORT_H */

// End of auto generated file.
