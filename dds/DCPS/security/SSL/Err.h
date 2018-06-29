/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_ERR_H
#define OPENDDS_SECURITY_SSL_ERR_H

#include <ace/Log_Msg.h>
#include <openssl/err.h>

#define OPENDDS_SSL_LOG_ERR(MSG)                                            \
  ACE_ERROR((LM_ERROR,                                                      \
             ACE_TEXT("(%P|%t) ERROR: '%C' ") ACE_TEXT(MSG) ACE_TEXT("\n"), \
             ERR_reason_error_string(ERR_get_error())))

#endif
