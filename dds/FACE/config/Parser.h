#ifndef FACE_CONFIG_PARSER_H
#define FACE_CONFIG_PARSER_H

#include "FACE/OpenDDS_FACE_Export.h"
#include "ConnectionSettings.h"
#include "QosSettings.h"
#include "TopicSettings.h"

class ACE_Configuration_Heap;
class ACE_Configuration_Section_Key;

namespace OpenDDS { namespace FaceTSS { namespace config {

class OpenDDS_FACE_Export Parser {
public:
  // Returns non-zero on failure
  int parse(const char* filename);
  int find_connection(const char* name, ConnectionSettings& target);
  int find_topic(const char* name, TopicSettings& target);
  int find_qos(const char* name, QosSettings& target);
  size_t pool_size() const;
  size_t pool_granularity() const;

private:
  size_t pool_size_;
  size_t pool_granularity_;
  static ConnectionMap connection_map_;
  static QosMap qos_map_;
  static TopicMap topic_map_;

  int parse_common(ACE_Configuration_Heap& config, bool required);

  int parse_topic(ACE_Configuration_Heap& config,
                  ACE_Configuration_Section_Key& key,
                  const char* topic_name);
  int parse_connection(ACE_Configuration_Heap& config,
                       ACE_Configuration_Section_Key& key,
                       const char* connection_name);
  int parse_qos(ACE_Configuration_Heap& config,
                ACE_Configuration_Section_Key& key,
                const char* qos_name,
                QosSettings::QosLevel level);
  int parse_sections(ACE_Configuration_Heap& config,
                     const char* section_type,
                     bool required);
};

} } }

#endif
