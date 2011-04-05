#ifndef UTILITIES_H
#define UTILITIES_H

#include <ace/config-macros.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include "model_export.h"

namespace OpenDDS { namespace Model {

  /// Load a string into an Octet sequence.
  void OpenDDS_Model_Export stringToByteSeq(
                              DDS::OctetSeq& target,
                              const char* source
                            );

} } // End of namespace OpenDDS::Model

#endif /* UTILITIES_H */


