// ws_common.h
//
// Common typedefs, macros, and functions for portability across
// wireshark versions.

#ifndef WS_COMMON_H
#define WS_COMMON_H

// Make a simple integer value for version numbers
#define WIRESHARK_VERSION_NUMBER(MAJOR,MINOR,MICRO) \
MAJOR * 1000000 + MINOR * 1000 + MICRO
#define WIRESHARK_VERSION WIRESHARK_VERSION_NUMBER(VERSION_MAJOR,VERSION_MINOR,VERSION_MICRO)

#if (WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(1,12,0) )
inline guint8 *ws_tvb_get_ephemeral_string(tvbuff_t *tvb, const gint offset, const gint length) {
  return ::tvb_get_string(wmem_packet_scope(), tvb, offset, length);
}
inline void *ws_ep_tvb_memdup(tvbuff_t *tvb, const gint offset, size_t remainder) {
  return ::tvb_memdup(wmem_packet_scope(), tvb, offset, remainder);
}
#define WS_TCP_DISSECT_PDUS_EXTRA_ARG ,0
#define WS_DISSECTOR_T_EXTRA_PARAM ,void*
#define WS_DISSECTOR_T_RETURN_TYPE int
#define WS_DISSECTOR_T_RETURN_TYPE_INT
#else
inline guint8 *ws_tvb_get_ephemeral_string(tvbuff_t *tvb, const gint offset, const gint length) {
  return ::tvb_get_ephemeral_string(tvb, offset, length);
}
inline void *ws_ep_tvb_memdup(tvbuff_t *tvb, const gint offset, size_t remainder) {
  return ::ep_tvb_memdup(tvb, offset, remainder);
}
#define WS_TCP_DISSECT_PDUS_EXTRA_ARG
#define WS_DISSECTOR_T_EXTRA_PARAM
#define WS_DISSECTOR_T_RETURN_TYPE void
#endif

#if (WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(1,8,0) )
#define WS_CONST const
#define WS_HEUR_DISSECTOR_T_EXTRA_PARAM ,void*
#else
#define WS_CONST
#define WS_HEUR_DISSECTOR_T_EXTRA_PARAM
#endif

#endif
