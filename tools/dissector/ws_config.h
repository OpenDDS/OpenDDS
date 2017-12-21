#ifndef WS_CONFIG_H
#define WS_CONFIG_H

#ifdef _WIN64
// ssize_t is defined by both Wireshark and ACE, so rename it for Wireshark
#define ssize_t ws_sssize_t
#include <config.h>
#undef ssize_t
#else
#include <config.h>
#endif

#endif

