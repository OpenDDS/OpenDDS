
// -*- C++ -*-
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl key_annotation_idl
// ------------------------------
#ifndef KEY_ANNOTATION_IDL_EXPORT_H
#define KEY_ANNOTATION_IDL_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (KEY_ANNOTATION_IDL_HAS_DLL)
#  define KEY_ANNOTATION_IDL_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && KEY_ANNOTATION_IDL_HAS_DLL */

#if !defined (KEY_ANNOTATION_IDL_HAS_DLL)
#  define KEY_ANNOTATION_IDL_HAS_DLL 1
#endif /* ! KEY_ANNOTATION_IDL_HAS_DLL */

#if defined (KEY_ANNOTATION_IDL_HAS_DLL) && (KEY_ANNOTATION_IDL_HAS_DLL == 1)
#  if defined (KEY_ANNOTATION_IDL_BUILD_DLL)
#    define key_annotation_idl_Export ACE_Proper_Export_Flag
#    define KEY_ANNOTATION_IDL_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define KEY_ANNOTATION_IDL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* KEY_ANNOTATION_IDL_BUILD_DLL */
#    define key_annotation_idl_Export ACE_Proper_Import_Flag
#    define KEY_ANNOTATION_IDL_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define KEY_ANNOTATION_IDL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* KEY_ANNOTATION_IDL_BUILD_DLL */
#else /* KEY_ANNOTATION_IDL_HAS_DLL == 1 */
#  define key_annotation_idl_Export
#  define KEY_ANNOTATION_IDL_SINGLETON_DECLARATION(T)
#  define KEY_ANNOTATION_IDL_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* KEY_ANNOTATION_IDL_HAS_DLL == 1 */

// Set KEY_ANNOTATION_IDL_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (KEY_ANNOTATION_IDL_NTRACE)
#  if (ACE_NTRACE == 1)
#    define KEY_ANNOTATION_IDL_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define KEY_ANNOTATION_IDL_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !KEY_ANNOTATION_IDL_NTRACE */

#if (KEY_ANNOTATION_IDL_NTRACE == 1)
#  define KEY_ANNOTATION_IDL_TRACE(X)
#else /* (KEY_ANNOTATION_IDL_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define KEY_ANNOTATION_IDL_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (KEY_ANNOTATION_IDL_NTRACE == 1) */

#endif /* KEY_ANNOTATION_IDL_EXPORT_H */

// End of auto generated file.
