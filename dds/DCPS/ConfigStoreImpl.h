/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CONFIG_STORE_IMPL_H
#define OPENDDS_DCPS_CONFIG_STORE_IMPL_H

#include "dcps_export.h"

#include "SafetyProfileStreams.h"
#include "InternalTopic.h"
#include "NetworkAddress.h"
#include "TimeDuration.h"

#include "dds/DdsDcpsInfrastructureC.h"

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
    : key_(key)
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

  CORBA::Boolean has(const char* key);

  void set_boolean(const char* key,
                   CORBA::Boolean value);
  CORBA::Boolean get_boolean(const char* key,
                             CORBA::Boolean value);

  void set_int32(const char* key,
                 CORBA::Long value);
  CORBA::Long get_int32(const char* key,
                        CORBA::Long value);

  void set_uint32(const char* key,
                  CORBA::ULong value);
  CORBA::ULong get_uint32(const char* key,
                          CORBA::ULong value);

  void set_float64(const char* key,
                   CORBA::Double value);
  CORBA::Double get_float64(const char* key,
                            CORBA::Double value);

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

  void connect(ConfigReader_rch config_reader)
  {
    config_topic_->connect(config_reader);
  }

  void disconnect(ConfigReader_rch config_reader)
  {
    config_topic_->disconnect(config_reader);
  }

private:
  ConfigTopic_rch config_topic_;
  ConfigWriter_rch config_writer_;
  ConfigReader_rch config_reader_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_CONFIG_STORE_IMPL_H */
