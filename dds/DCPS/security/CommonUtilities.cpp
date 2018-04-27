#include "dds/DCPS/security/CommonUtilities.h"

#include <string>
#include <cstdio>

namespace OpenDDS {
namespace Security {
namespace CommonUtilities {

void set_security_error(DDS::Security::SecurityException& ex,
                        int code,
                        int minor_code,
                        const char* message)
{
  ex.code = code;
  ex.minor_code = minor_code;
  ex.message = message;
}

void set_security_error(DDS::Security::SecurityException& ex,
                        int code,
                        int minor_code,
                        const char* message,
                        const unsigned char (&a1)[4],
                        const unsigned char (&a2)[4])
{
  std::string full(message);
  const size_t i = full.size();
  full.resize(i + 25);
  std::sprintf(&full[i], " %.2x %.2x %.2x %.2x, %.2x %.2x %.2x %.2x",
               a1[0], a1[1], a1[2], a1[3], a2[0], a2[1], a2[2], a2[3]);
  set_security_error(ex, code, minor_code, full.c_str());
}

}
}
}
