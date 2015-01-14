#ifndef OPENDDS_CONNECTION_SETTINGS_H
#define OPENDDS_CONNECTION_SETTINGS_H

#include <map>
#include "FACE/TS_common.hpp"
#include "dds/DCPS/PoolAllocator.h"
#include "FACE/OpenDDS_FACE_Export.h"

namespace OpenDDS { namespace FaceTSS { namespace config {

class OpenDDS_FACE_Export ConnectionSettings {
public:
  ConnectionSettings();

  int set(const char* name, const char* value);

  char topic_name_[64];
  char qos_name_[64];
  FACE::CONNECTION_ID_TYPE connection_id_;
  FACE::CONNECTION_DIRECTION_TYPE direction_;
  int domain_id_;
};

typedef OPENDDS_MAP(OPENDDS_STRING, ConnectionSettings) ConnectionMap;

} } }

#endif
