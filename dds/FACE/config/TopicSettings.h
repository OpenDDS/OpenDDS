#ifndef OPENDDS_TOPIC_SETTINGS_H
#define OPENDDS_TOPIC_SETTINGS_H

#include "FACE/TS_common.hpp"
#include "dds/DCPS/PoolAllocator.h"
#include "FACE/OpenDDS_FACE_Export.h"

namespace OpenDDS { namespace FaceTSS { namespace config {

class OpenDDS_FACE_Export TopicSettings {
public:
  int set(const char* name, const char* value);

  char type_name_[128];
  FACE::MESSAGE_TYPE_GUID message_definition_guid_;
  FACE::MESSAGE_SIZE_TYPE max_message_size_;
};

typedef OPENDDS_MAP(OPENDDS_STRING, TopicSettings) TopicMap;

} } }

#endif
