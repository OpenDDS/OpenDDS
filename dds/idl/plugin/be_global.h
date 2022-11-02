/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_BE_GLOBAL_H
#define OPENDDS_IDL_BE_GLOBAL_H

#include "opendds_idl_plugin_export.h"

class opendds_idl_plugin_Export BE_GlobalData {
public:
  // = TITLE
  //    BE_GlobalData
  //
  // = DESCRIPTION
  //    Implements the required methods.
  //    They call back into the backend interface manager.
  //
  BE_GlobalData();

  virtual ~BE_GlobalData();

  void destroy();
  // Cleanup function.

  void parse_args(long& i, char** av);
  // Parse args that affect the backend.
};

#endif /* OPENDDS_IDL_BE_GLOBAL_H */
