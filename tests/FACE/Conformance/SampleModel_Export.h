
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl SampleModel
// ------------------------------
#ifndef SAMPLEMODEL_EXPORT_H
#define SAMPLEMODEL_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (SAMPLEMODEL_HAS_DLL)
#  define SAMPLEMODEL_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && SAMPLEMODEL_HAS_DLL */

#if !defined (SAMPLEMODEL_HAS_DLL)
#  define SAMPLEMODEL_HAS_DLL 1
#endif /* ! SAMPLEMODEL_HAS_DLL */

#if defined (SAMPLEMODEL_HAS_DLL) && (SAMPLEMODEL_HAS_DLL == 1)
#  if defined (SAMPLEMODEL_BUILD_DLL)
#    define SampleModel_Export ACE_Proper_Export_Flag
#    define SAMPLEMODEL_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define SAMPLEMODEL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* SAMPLEMODEL_BUILD_DLL */
#    define SampleModel_Export ACE_Proper_Import_Flag
#    define SAMPLEMODEL_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define SAMPLEMODEL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* SAMPLEMODEL_BUILD_DLL */
#else /* SAMPLEMODEL_HAS_DLL == 1 */
#  define SampleModel_Export
#  define SAMPLEMODEL_SINGLETON_DECLARATION(T)
#  define SAMPLEMODEL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* SAMPLEMODEL_HAS_DLL == 1 */

// Set SAMPLEMODEL_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (SAMPLEMODEL_NTRACE)
#  if (ACE_NTRACE == 1)
#    define SAMPLEMODEL_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define SAMPLEMODEL_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !SAMPLEMODEL_NTRACE */

#if (SAMPLEMODEL_NTRACE == 1)
#  define SAMPLEMODEL_TRACE(X)
#else /* (SAMPLEMODEL_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define SAMPLEMODEL_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (SAMPLEMODEL_NTRACE == 1) */

#endif /* SAMPLEMODEL_EXPORT_H */

// End of auto generated file.
