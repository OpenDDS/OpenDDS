#ifndef OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_TVBUFF_H
#define OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_TVBUFF_H

#include "config.h"

/*
 * Older versions of tvbuff.h don't have an extern "C", but it also includes
 * glib.h, so we can't just wrap the include in a extern "C".
 *
 * The workaround appears to play dirty. We have to know the functions we need
 * to use, change the name of them in the header and declare them extern "C"
 * ourselves. For this to work we can't have previously included tvbuff.h.
 */
#if WIRESHARK_VERSION < WIRESHARK_VERSION_NUMBER(1, 6, 6) // When tvbuff got extern "C"
#  define TVBUFF_WORKAROUND
#  ifdef __TVBUFF_H__
#    error "tvbuff.h already was included"
#  endif
#  define tvb_length opendds_workaround_tvb_length
#  define tvb_reported_length opendds_workaround_tvb_reported_length
#  define tvb_memdup opendds_workaround_tvb_memdup
#  define ep_tvb_memdup opendds_workaround_ep_tvb_memdup
#  define tvb_get_ptr opendds_workaround_tvb_get_ptr
#  define tvb_get_string opendds_workaround_tvb_get_string
#  define tvb_get_ephemeral_string opendds_workaround_tvb_get_ephemeral_string
#  define tvb_memeql opendds_workaround_tvb_memeql
#endif // WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(1, 6, 0)

#include <epan/tvbuff.h>

#ifdef TVBUFF_WORKAROUND
#  undef tvb_length
#  undef tvb_reported_length
#  undef tvb_memdup
#  undef ep_tvb_memdup
#  undef tvb_get_ptr
#  undef tvb_get_string
#  undef tvb_get_ephemeral_string
#  undef tvb_memeql

extern "C" {
  guint tvb_length(tvbuff_t*);
  guint tvb_reported_length(tvbuff_t*);
  void* tvb_memdup(tvbuff_t*, gint, size_t);
  void* ep_tvb_memdup(tvbuff_t*, gint, size_t);
  const guint8* tvb_get_ptr(tvbuff_t*, gint, gint);
  guint8* tvb_get_string(tvbuff_t*, gint, gint);
  guint8* tvb_get_ephemeral_string(tvbuff_t*, gint, gint);
  gint tvb_memeql(tvbuff_t*, gint, const guint8*, size_t);
}
#endif // TVBUFF_WORKAROUND

#endif
