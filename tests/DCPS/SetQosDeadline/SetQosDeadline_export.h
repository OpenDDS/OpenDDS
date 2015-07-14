
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl SetQosDeadline
// ------------------------------
#ifndef SETQOSDEADLINE_EXPORT_H
#define SETQOSDEADLINE_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SETQOSDEADLINE_HAS_DLL)
#  define SETQOSDEADLINE_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SETQOSDEADLINE_HAS_DLL */

#if !defined (SETQOSDEADLINE_HAS_DLL)
#  define SETQOSDEADLINE_HAS_DLL 1
#endif /* ! SETQOSDEADLINE_HAS_DLL */

#if defined (SETQOSDEADLINE_HAS_DLL) && (SETQOSDEADLINE_HAS_DLL == 1)
#  if defined (SETQOSDEADLINE_BUILD_DLL)
#    define SetQosDeadline_Export ACE_Proper_Export_Flag
#    define SETQOSDEADLINE_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SETQOSDEADLINE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SETQOSDEADLINE_BUILD_DLL */
#    define SetQosDeadline_Export ACE_Proper_Import_Flag
#    define SETQOSDEADLINE_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SETQOSDEADLINE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SETQOSDEADLINE_BUILD_DLL */
#else /* SETQOSDEADLINE_HAS_DLL == 1 */
#  define SetQosDeadline_Export
#  define SETQOSDEADLINE_SINGLETON_DECLARATION(T)
#  define SETQOSDEADLINE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SETQOSDEADLINE_HAS_DLL == 1 */

// Set SETQOSDEADLINE_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SETQOSDEADLINE_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SETQOSDEADLINE_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SETQOSDEADLINE_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SETQOSDEADLINE_NTRACE */

#if (SETQOSDEADLINE_NTRACE == 1)
#  define SETQOSDEADLINE_TRACE(X)
#else /* (SETQOSDEADLINE_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SETQOSDEADLINE_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SETQOSDEADLINE_NTRACE == 1) */

#endif /* SETQOSDEADLINE_EXPORT_H */

// End of auto generated file.
