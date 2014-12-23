/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */


#ifndef OPENDDS_DDS_DCPS_FILE_LOGGER_H
#define OPENDDS_DDS_DCPS_FILE_LOGGER_H


#include "ace/Log_Msg_Backend.h"


# ifdef OPENDDS_SAFETY_PROFILE
namespace OpenDDS { namespace DCPS {

class FileLogger : public ACE_Log_Msg_Backend {
public:
  FileLogger(const char* filename);
  ~FileLogger();
protected:
  virtual int open(const ACE_TCHAR *logger_key);
  virtual int reset(void);
  virtual int close(void);
  virtual ssize_t log(ACE_Log_Record &log_record);
private:
  FILE* log_;
};

} }
#endif

#endif
