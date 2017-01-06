#ifndef OPENDDS_TOPIC_SETTINGS_H
#define OPENDDS_TOPIC_SETTINGS_H

#include "FACE/TS_common.hpp"
#include "dds/DCPS/PoolAllocator.h"
#include "FACE/OpenDDS_FACE_Export.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace FaceTSS { namespace config {

class OpenDDS_FACE_Export TopicSettings {
public:
  static const int TYPE_NAME_LEN = 128;

  int set(const char* name, const char* value);

  char type_name_[TYPE_NAME_LEN];
  FACE::MESSAGE_TYPE_GUID platform_view_guid_;
  FACE::MESSAGE_SIZE_TYPE max_message_size_;
};

typedef OPENDDS_MAP(OPENDDS_STRING, TopicSettings) TopicMap;

} } }

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
