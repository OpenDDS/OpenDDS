/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_SSL_ERR_H
#define OPENDDS_DCPS_SECURITY_SSL_ERR_H

#include <ace/Log_Msg.h>
#include <openssl/err.h>

#define OPENDDS_SSL_LOG_ERR(MSG)                                    \
  for (unsigned long e = ERR_get_error(); e != 0; e = ERR_get_error()) { \
    char buf[256];                                                      \
    ERR_error_string(e, buf);                                           \
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: %C: %C\n"),        \
               (MSG), buf));                                            \
  }

#endif
