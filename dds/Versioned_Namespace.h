// -*- C++ -*-

//=============================================================================
/**
 * @file    Versioned_Namespace.h
 *
 * Versioned namespace support.
 *
 * Useful for preventing conflicts when using a third party library.
 *
 * @author Ossama Othman <ossama@dre.vanderbilt.edu>
 */
//=============================================================================

#ifndef OPENDDS_VERSIONED_NAMESPACE_H
#define OPENDDS_VERSIONED_NAMESPACE_H

#if !defined (OPENDDS_HAS_VERSIONED_NAMESPACE) \
  && defined (ACE_HAS_VERSIONED_NAMESPACE) \
  && ACE_HAS_VERSIONED_NAMESPACE == 1
# define OPENDDS_HAS_VERSIONED_NAMESPACE 1
#endif  /* !OPENDDS_HAS_VERSIONED_NAMESPACE
           && ACE_HAS_VERSIONED_NAMESPACE == 1*/

#if defined (OPENDDS_HAS_VERSIONED_NAMESPACE) && OPENDDS_HAS_VERSIONED_NAMESPACE == 1

# ifndef OPENDDS_VERSIONED_NAMESPACE_NAME
#  include "dds/Version.h"

// Preprocessor symbols will not be expanded if they are
// concatenated.  Force the preprocessor to expand them during the
// argument prescan by calling a macro that itself calls another that
// performs the actual concatenation.
#  define OPENDDS_MAKE_VERSIONED_NAMESPACE_NAME_IMPL(MAJOR,MINOR,MICRO) DDS_ ## MAJOR ## _ ## MINOR ## _ ## MICRO
#  define OPENDDS_MAKE_VERSIONED_NAMESPACE_NAME(MAJOR,MINOR,MICRO) OPENDDS_MAKE_VERSIONED_NAMESPACE_NAME_IMPL(MAJOR,MINOR,MICRO)
#  define OPENDDS_VERSIONED_NAMESPACE_NAME OPENDDS_MAKE_VERSIONED_NAMESPACE_NAME(DDS_MAJOR_VERSION,DDS_MINOR_VERSION,DDS_MICRO_VERSION)
# endif  /* !OPENDDS_VERSIONED_NAMESPACE_NAME */

# define OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL namespace OPENDDS_VERSIONED_NAMESPACE_NAME {
# define OPENDDS_END_VERSIONED_NAMESPACE_DECL } \
  using namespace OPENDDS_VERSIONED_NAMESPACE_NAME;
#else
# define OPENDDS_VERSIONED_NAMESPACE_NAME
# define OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
# define OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_HAS_VERSIONED_NAMESPACE */

#endif  /* !OPENDDS_VERSIONED_NAMESPACE_H */
