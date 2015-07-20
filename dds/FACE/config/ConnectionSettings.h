#ifndef OPENDDS_CONNECTION_SETTINGS_H
#define OPENDDS_CONNECTION_SETTINGS_H

#include "FACE/TS_common.hpp"
#include "dds/DCPS/PoolAllocator.h"
#include "FACE/OpenDDS_FACE_Export.h"

namespace OpenDDS { namespace FaceTSS { namespace config {

class OpenDDS_FACE_Export ConnectionSettings {
public:
  ConnectionSettings();

  int set(const char* name, const char* value);

  const char* datawriter_qos_name() const;
  const char* datareader_qos_name() const;
  const char* publisher_qos_name() const;
  const char* subscriber_qos_name() const;
  const char* transport_name() const;

  bool datawriter_qos_set() const;
  bool datareader_qos_set() const;
  bool publisher_qos_set() const;
  bool subscriber_qos_set() const;
  bool transport_set() const;

  char topic_name_[64];
  char datawriter_qos_name_[64]; // QOS name of datawriter
  char datareader_qos_name_[64]; // QOS name of datareader
  char publisher_qos_name_[64];  // QOS name of publisher
  char subscriber_qos_name_[64]; // QOS name of subscriber
  FACE::CONNECTION_ID_TYPE connection_id_;
  FACE::CONNECTION_DIRECTION_TYPE direction_;
  int domain_id_;
  int participant_id_;
  char transport_name_[64];
};

typedef OPENDDS_MAP(OPENDDS_STRING, ConnectionSettings) ConnectionMap;

} } }

#endif
