
#include "FileLogger.h"

# ifdef OPENDDS_SAFETY_PROFILE
namespace OpenDDS { namespace DCPS {

int
FileLogger::open(const ACE_TCHAR *logger_key)
{
  return 0;
}

int
FileLogger::reset(void)
{
  return 0;
}

int
FileLogger::close(void)
{
  return 0;
}

ssize_t
FileLogger::log(ACE_Log_Record &log_record)
{
  return 0;
}

} }
#endif
