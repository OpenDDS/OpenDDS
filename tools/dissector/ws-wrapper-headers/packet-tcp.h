#ifndef OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_PACKET_TCP_H
#define OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_PACKET_TCP_H

#ifdef __PACKET_TCP_H__
#  error "packet-tcp.h already was included without this wrapper!"
#endif

#if WIRESHARK_VERSION < WIRESHARK_VERSION_NUMBER(1, 12, 0)
#  define OPENDDS_PACKET_TCP_H_MISSING_EXTERN_C
#endif

#ifdef OPENDDS_PACKET_TCP_H_MISSING_EXTERN_C
extern "C" {
#endif

#include <epan/dissectors/packet-tcp.h>

#ifdef OPENDDS_PACKET_TCP_H_MISSING_EXTERN_C
}
#endif

#endif
