#ifndef DDS_SECURITY_UTILS_H
#define DDS_SECURITY_UTILS_H

#include "dds/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

/**
* @class CommonUtilities
*
* @brief Provides several simple utility functions common to the Security plugins
*
*/
class CommonUtilities 
{
public:
  static void set_security_error(::DDS::Security::SecurityException& ex, 
                                 int code, 
                                 int minor_code, 
                                 const char* message);
};

} // Security
} // OpenDDS

#endif
