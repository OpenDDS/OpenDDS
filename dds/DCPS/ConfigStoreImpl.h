/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CONFIG_STORE_IMPL_H
#define OPENDDS_DCPS_CONFIG_STORE_IMPL_H

#include "InternalTopic.h"
#include "NetworkAddress.h"
#include "SafetyProfileStreams.h"
#include "TimeDuration.h"
#include "dcps_export.h"

#include "dds/DdsDcpsInfrastructureC.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class ConfigPair
 *
 * @brief Key-value pair used for configuration.
 */
class OpenDDS_Dcps_Export ConfigPair {
public:
  ConfigPair()
  {}

  ConfigPair(const String& key,
             const String& value)
    : key_(canonicalize(key))
    , value_(value)
  {}

  bool operator<(const ConfigPair& other) const
  {
    return key_ < other.key_;
  }

  bool operator==(const ConfigPair& other) const
  {
    return key_ == other.key_ && value_ == other.value_;
  }

  const String& key() const { return key_; }
  const String& value() const { return value_; }

  bool key_has_prefix(const String& prefix) const
  {
    const String p = canonicalize(prefix);
    if (p.size() <= key_.size()) {
      return std::strncmp(p.data(), key_.data(), p.size()) == 0;
    }

    return false;
  }

  static String canonicalize(const String& key);

private:
  String key_;
  String value_;
};

typedef InternalTopic<ConfigPair> ConfigTopic;
typedef RcHandle<ConfigTopic> ConfigTopic_rch;
typedef InternalDataWriter<ConfigPair> ConfigWriter;
typedef RcHandle<ConfigWriter> ConfigWriter_rch;
typedef InternalDataReaderListener<ConfigPair> ConfigListener;
typedef RcHandle<ConfigListener> ConfigReaderListener_rch;
typedef InternalDataReader<ConfigPair> ConfigReader;
typedef RcHandle<ConfigReader> ConfigReader_rch;

class OpenDDS_Dcps_Export ConfigStoreImpl : public ConfigStore {
public:
  enum IntegerTimeFormat {
    Format_IntegerSeconds,
    Format_IntegerMilliseconds
  };

  ConfigStoreImpl();
  ~ConfigStoreImpl();

  DDS::Boolean has(const char* key);

  void set_boolean(const char* key,
                   DDS::Boolean value);
  DDS::Boolean get_boolean(const char* key,
                           DDS::Boolean value);

  void set_int32(const char* key,
                 DDS::Int32 value);
  DDS::Int32 get_int32(const char* key,
                        DDS::Int32 value);

  void set_uint32(const char* key,
                  DDS::UInt32 value);
  DDS::UInt32 get_uint32(const char* key,
                          DDS::UInt32 value);

  void set_float64(const char* key,
                   DDS::Float64 value);
  DDS::Float64 get_float64(const char* key,
                            DDS::Float64 value);

  void set_string(const char* key,
                  const char* value);
  char* get_string(const char* key,
                   const char* value);

  void set_duration(const char* key,
                    const DDS::Duration_t& value);
  DDS::Duration_t get_duration(const char* key,
                               const DDS::Duration_t& value);

  void unset(const char* key);


  void set(const char* key,
           const String& value);
  String get(const char* key,
             const String& value) const;

  void set(const char* key,
           const TimeDuration& value,
           IntegerTimeFormat format);
  TimeDuration get(const char* key,
                   const TimeDuration& value,
                   IntegerTimeFormat format) const;

  // Currently, this only supports IPv4 Addresses without port numbers.
  NetworkAddress get(const char* key,
                     const NetworkAddress& value) const;

  static DDS::DataWriterQos datawriter_qos();
  static DDS::DataReaderQos datareader_qos();

  void connect(ConfigReader_rch config_reader)
  {
    config_topic_->connect(config_reader);
  }

  void disconnect(ConfigReader_rch config_reader)
  {
    config_topic_->disconnect(config_reader);
  }

  // Takes all samples from reader and returns true if any have the given prefix.
  static bool contains_prefix(ConfigReader_rch reader,
                              const String& prefix);

private:
  ConfigTopic_rch config_topic_;
  ConfigWriter_rch config_writer_;
  ConfigReader_rch config_reader_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_CONFIG_STORE_IMPL_H */
