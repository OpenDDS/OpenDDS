#ifndef OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_PACKET_GIOP_H
#define OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_PACKET_GIOP_H

// packet-giop.h doesn't include these
#include <glib.h>
#include <epan/packet.h>

extern "C" {
#include <epan/dissectors/packet-giop.h>
}

#endif
