#include "dds/DCPS/security/CommonUtilities.h"

namespace OpenDDS {
namespace Security {

void CommonUtilities::set_security_error(::DDS::Security::SecurityException& ex, 
                                         int code, 
                                         int minor_code, 
                                         const char* message)
{
  ex.code = code;
  ex.minor_code = minor_code;
  ex.message = message;
}




} // Security
} // OpenDDS