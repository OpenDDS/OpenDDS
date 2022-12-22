#ifndef OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_CONFIG_H
#define OPENDDS_WIRESHARK_DISSECTOR_WRAPPER_CONFIG_H

// ssize_t is defined by both Wireshark and ACE, so rename it for Wireshark
#ifdef _WIN64
#  define ssize_t opendds_workaround_ssize_t
#endif

#include <config.h>

#ifdef _WIN64
#  undef ssize_t
#endif

// Make a simple integer value for version numbers
#define WIRESHARK_VERSION_NUMBER(MAJOR, MINOR, MICRO) \
  MAJOR * 1000000 + MINOR * 1000 + MICRO
#define WIRESHARK_VERSION \
  WIRESHARK_VERSION_NUMBER(VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO)

#endif
