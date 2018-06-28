/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_ERR_H
#define OPENDDS_SECURITY_SSL_ERR_H

#include <openssl/err.h>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <cstring>

#define OPENDDS_SSL_LOG_ERR(MSG)                                         \
  do {                                                                   \
    std::cerr << "Warning '" << ERR_reason_error_string(ERR_get_error()) \
              << "' " << (MSG) << " in file '" << __FILE__ << "' line '" \
              << __LINE__ << "'\n";                                      \
  } while (0)

#endif
