/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_PRIVATEKEY_H
#define OPENDDS_SECURITY_SSL_PRIVATEKEY_H

#include <openssl/err.h>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cerrno>


#define OPENDDS_SSL_LOG_ERR(MSG) do {\
  std::cerr << "Error '" << ERR_reason_error_string(ERR_get_error()) << "' " << (MSG) << " in file '" << __FILE__ << "' line '" << __LINE__ << "'\n";\
} while(0)


#endif
