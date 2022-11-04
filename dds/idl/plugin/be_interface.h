/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_BE_INTERFACE_H
#define OPENDDS_IDL_BE_INTERFACE_H

#include <ace/SString.h>

#include "language_mapping.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class BE_Interface {
public:
  virtual ~BE_Interface() {}

  /// Called by BE_init() to provide the backend an opportunity to initialize
  /// and process command line arguments.
  virtual int init(int&, ACE_TCHAR*[]) = 0;

  /// Called by BE_post_init() to provide the backend the ability to perform
  /// operations directly after initialization.
  virtual void post_init(char*[], long) = 0;

  /// Called by BE_version().  The backend can print version information.
  virtual void version() const = 0;

  /// Called by BE_produce() to generate code by the backend.
  virtual void produce() = 0;

  /// Called by BE_cleanup() to cleanup resources allocated by the backend.
  virtual void cleanup() = 0;

  /// Called during the destruction of the BE_GlobalData.
  virtual void destroy() = 0;

  /// Called to parse arguments that are unknown to the driver.  Allows the
  /// backend to process general command line arguments.
  virtual void parse_args(long& i, char** av) = 0;

  /// Called to process arguments specifically destined for the backend.
  /// (i.e., -Wb,...)
  virtual void prep_be_arg(char* arg) = 0;

  /// Called when DRV_parse_args() is finished processing command line
  /// arguments.
  virtual void arg_post_proc() = 0;

  /// Allows the backend to provide description for arguments processed
  /// specifically by the backend.
  virtual void usage() = 0;
};

#endif /* OPENDDS_IDL_BE_INTERFACE_H */
