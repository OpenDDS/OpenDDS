
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl Stool_Builder
// ------------------------------
#ifndef STOOL_BUILDER_EXPORT_H
#define STOOL_BUILDER_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (STOOL_BUILDER_HAS_DLL)
#  define STOOL_BUILDER_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && STOOL_BUILDER_HAS_DLL */

#if !defined (STOOL_BUILDER_HAS_DLL)
#  define STOOL_BUILDER_HAS_DLL 1
#endif /* ! STOOL_BUILDER_HAS_DLL */

#if defined (STOOL_BUILDER_HAS_DLL) && (STOOL_BUILDER_HAS_DLL == 1)
#  if defined (STOOL_BUILDER_BUILD_DLL)
#    define Stool_Builder_Export ACE_Proper_Export_Flag
#    define STOOL_BUILDER_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define STOOL_BUILDER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* STOOL_BUILDER_BUILD_DLL */
#    define Stool_Builder_Export ACE_Proper_Import_Flag
#    define STOOL_BUILDER_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define STOOL_BUILDER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* STOOL_BUILDER_BUILD_DLL */
#else /* STOOL_BUILDER_HAS_DLL == 1 */
#  define Stool_Builder_Export
#  define STOOL_BUILDER_SINGLETON_DECLARATION(T)
#  define STOOL_BUILDER_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* STOOL_BUILDER_HAS_DLL == 1 */

// Set STOOL_BUILDER_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (STOOL_BUILDER_NTRACE)
#  if (ACE_NTRACE == 1)
#    define STOOL_BUILDER_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define STOOL_BUILDER_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !STOOL_BUILDER_NTRACE */

#if (STOOL_BUILDER_NTRACE == 1)
#  define STOOL_BUILDER_TRACE(X)
#else /* (STOOL_BUILDER_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define STOOL_BUILDER_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (STOOL_BUILDER_NTRACE == 1) */

#endif /* STOOL_BUILDER_EXPORT_H */

// End of auto generated file.
