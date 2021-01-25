/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_SSL_ERR_H
#define OPENDDS_DCPS_SECURITY_SSL_ERR_H

#include <ace/Log_Msg.h>
#include <openssl/err.h>

#define OPENDDS_SSL_LOG_ERR(MSG)                                  \
  ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: '%C' %C\n"), \
             ERR_reason_error_string(ERR_get_error()), (MSG)))

#endif
