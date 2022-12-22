#ifndef OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_TVBUFF_H
#define OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_TVBUFF_H

#include "config.h"

#ifdef __TVBUFF_H__
#  error "tvbuff.h already was included without this wrapper!"
#endif

#if WIRESHARK_VERSION < WIRESHARK_VERSION_NUMBER(1, 8, 0)
#  define OPENDDS_TVBUFF_H_MISSING_EXTERN_C
#endif

#ifdef OPENDDS_TVBUFF_H_MISSING_EXTERN_C
extern "C" {
#endif

#include <epan/tvbuff.h>

#ifdef OPENDDS_TVBUFF_H_MISSING_EXTERN_C
}
#endif

#endif
