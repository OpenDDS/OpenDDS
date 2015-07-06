
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl OpenDDS_FACE
// ------------------------------
#ifndef OPENDDS_FACE_EXPORT_H
#define OPENDDS_FACE_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (OPENDDS_FACE_HAS_DLL)
#  define OPENDDS_FACE_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && OPENDDS_FACE_HAS_DLL */

#if !defined (OPENDDS_FACE_HAS_DLL)
#  define OPENDDS_FACE_HAS_DLL 1
#endif /* ! OPENDDS_FACE_HAS_DLL */

#if defined (OPENDDS_FACE_HAS_DLL) && (OPENDDS_FACE_HAS_DLL == 1)
#  if defined (OPENDDS_FACE_BUILD_DLL)
#    define OpenDDS_FACE_Export ACE_Proper_Export_Flag
#    define OPENDDS_FACE_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_FACE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* OPENDDS_FACE_BUILD_DLL */
#    define OpenDDS_FACE_Export ACE_Proper_Import_Flag
#    define OPENDDS_FACE_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define OPENDDS_FACE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* OPENDDS_FACE_BUILD_DLL */
#else /* OPENDDS_FACE_HAS_DLL == 1 */
#  define OpenDDS_FACE_Export
#  define OPENDDS_FACE_SINGLETON_DECLARATION(T)
#  define OPENDDS_FACE_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* OPENDDS_FACE_HAS_DLL == 1 */

// Set OPENDDS_FACE_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (OPENDDS_FACE_NTRACE)
#  if (ACE_NTRACE == 1)
#    define OPENDDS_FACE_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define OPENDDS_FACE_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !OPENDDS_FACE_NTRACE */

#if (OPENDDS_FACE_NTRACE == 1)
#  define OPENDDS_FACE_TRACE(X)
#else /* (OPENDDS_FACE_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define OPENDDS_FACE_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (OPENDDS_FACE_NTRACE == 1) */

#endif /* OPENDDS_FACE_EXPORT_H */

// End of auto generated file.
