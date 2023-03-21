// ws_common.h
//
// Common typedefs, macros, and functions for portability across
// wireshark versions.

#ifndef WS_COMMON_H
#define WS_COMMON_H

#include "ws-wrapper-headers/config.h"
#include "ws-wrapper-headers/tvbuff.h"

#include <epan/packet.h>
#include <epan/conversation.h>

/*
 * Version 2.5 (Released as 2.6)
 * This is out of order because it is required by
 * ws_find_or_create_conversation().
 */
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(2, 5, 0)

// find_conversation after 2.5/2.6 takes endpoint_type instead of port_type
// but also provides a shortcut function which we will use instead.
inline conversation_t* ws_find_conversation(packet_info* pinfo)
{
  return find_conversation_pinfo(pinfo, 0);
}

#else // if 2.4.x or before

// else emulate it
inline conversation_t* ws_find_conversation(packet_info* pinfo)
{
  return find_conversation(
    pinfo->fd->num, &pinfo->src, &pinfo->dst,
    pinfo->ptype, pinfo->srcport, pinfo->destport, 0
  );
}

#endif

/*
 * Version 1.4
 */
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(1, 4, 0)

// 1.4 introduced find_or_create_conversation
inline conversation_t *ws_find_or_create_conversation(packet_info *pinfo) {
  return find_or_create_conversation(pinfo);
}

#else // if 1.3.x or before

// else emulate it
inline conversation_t *ws_find_or_create_conversation(packet_info *pinfo) {
  conversation_t *conv = NULL;
  if (!(conv = ws_find_conversation(pinfo))) {
    conv = conversation_new(
      pinfo->fd->num, &pinfo->src, &pinfo->dst,
      pinfo->ptype, pinfo->srcport, pinfo->destport, 0
    );
  }
  return conv;
}

#endif

/*
 * Version 1.8
 */
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(1, 8, 0)

#define WS_CONST const
#define WS_HEUR_DISSECTOR_T_EXTRA_PARAM ,void*

#else // Before 1.8

#define WS_CONST
#define WS_HEUR_DISSECTOR_T_EXTRA_PARAM

#endif

/*
 * Version 1.12
 *
 * This is the earliest version to support conventional sample dissection and
 * expert info, and probably was the biggest change to the Wireshark EPAN API
 * so far.
 */
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(1, 12, 0)

// Emulate tvb_get_string for ws_tvb_get_ephemeral_string below
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(2, 2, 0)
inline guint8* ws_tvb_get_string(wmem_allocator_t* alloc, tvbuff_t *tvb,
                                 gint offset, gint length)
{
  return tvb_get_string_enc(alloc, tvb, offset, length, ENC_ASCII);
}
#else
#define ws_tvb_get_string ::tvb_get_string
#endif

inline guint8 *ws_tvb_get_ephemeral_string(tvbuff_t *tvb, const gint offset, const gint length) {
  return ws_tvb_get_string(wmem_packet_scope(), tvb, offset, length);
}
inline void *ws_ep_tvb_memdup(tvbuff_t *tvb, const gint offset, size_t remainder) {
  return ::tvb_memdup(wmem_packet_scope(), tvb, offset, remainder);
}

#define WS_TCP_DISSECT_PDUS_EXTRA_ARG ,0
#define WS_DISSECTOR_T_RETURN_TYPE int
#define WS_DISSECTOR_T_RETURN_TYPE_INT
#define WS_DISSECTOR_T_EXTRA_PARAM ,void*

#else // Before 1.12

inline guint8 *ws_tvb_get_ephemeral_string(tvbuff_t *tvb, const gint offset, const gint length) {
  return ::tvb_get_ephemeral_string(tvb, offset, length);
}
inline void *ws_ep_tvb_memdup(tvbuff_t *tvb, const gint offset, size_t remainder) {
  return ::ep_tvb_memdup(tvb, offset, remainder);
}

#define WS_TCP_DISSECT_PDUS_EXTRA_ARG
#define WS_DISSECTOR_T_RETURN_TYPE void
#define WS_DISSECTOR_T_EXTRA_PARAM

// Field Display Enum was created for 1.12 and is used many places in the
// sample dissector, so we're disabling sample dissection.
#undef NO_ITL
#define NO_ITL
typedef int field_display_e; // Dummy type for code outside NO_ITL blocks

// Older expert info was completely different system before 1.12
#define NO_EXPERT

#endif

/*
 * Version 2.0
 */
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(2, 0, 0)

#define WS_DISSECTOR_EXTRA_ARG ,0
#define ws_tvb_length tvb_reported_length
#define WS_GET_PDU_LEN_EXTRA_PARAM ,void*
#define WS_HEUR_DISSECTOR_EXTRA_ARGS1(ucase, lcase) \
  "OpenDDS over " #ucase, "opendds_" #lcase,
#define WS_HEUR_DISSECTOR_EXTRA_ARGS2 , HEURISTIC_ENABLE

#else // Before 2.0

#define WS_DISSECTOR_EXTRA_ARG
#define ws_tvb_length tvb_length
#define WS_GET_PDU_LEN_EXTRA_PARAM
#define WS_HEUR_DISSECTOR_EXTRA_ARGS1(A, B)
#define WS_HEUR_DISSECTOR_EXTRA_ARGS2

#endif

/*
 * Version 2.2
 */
#if WIRESHARK_VERSION >= WIRESHARK_VERSION_NUMBER(2, 2, 0)

#define WS_DISSECTOR_RETURN_TYPE int
#define WS_DISSECTOR_RETURN_INT
#define ws_dissector_add_handle dissector_add_for_decode_as
#define WS_CONV_IDX conv_index
// create_dissector_handle wants the new dissector_t here
#define WS_DISSECTOR_EXTRA_PARAM ,void*

#else // Before 2.2

#define WS_DISSECTOR_RETURN_TYPE void
#define ws_dissector_add_handle dissector_add_handle
#define WS_CONV_IDX index
#define WS_DISSECTOR_EXTRA_PARAM

#endif

#endif // WS_COMMON_H
