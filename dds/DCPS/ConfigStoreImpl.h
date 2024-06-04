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
#include "debug.h"

#include "dds/DdsDcpsInfrastructureC.h"

#include <ace/Configuration.h>

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const char CONFIG_DEBUG_LOGGING[] = "CONFIG_DEBUG_LOGGING";
const bool CONFIG_DEBUG_LOGGING_default = false;

OpenDDS_Dcps_Export
OPENDDS_VECTOR(String) split(const String& str,
                             const String& delims,
                             bool skip_leading,
                             bool skip_consecutive);

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

template<typename T>
struct EnumList {
  T value;
  const char* name;
};

class OpenDDS_Dcps_Export ConfigStoreImpl : public ConfigStore {
public:
  enum TimeFormat {
    Format_IntegerSeconds,
    Format_IntegerMilliseconds,
    Format_FractionalSeconds
  };

  enum NetworkAddressFormat {
    Format_No_Port,
    Format_Required_Port,
    Format_Optional_Port
  };

  enum NetworkAddressKind {
    Kind_ANY,
    Kind_IPV4
#ifdef ACE_HAS_IPV6
    , Kind_IPV6
#endif
  };

  ConfigStoreImpl(ConfigTopic_rch config_topic);
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
             const String& value,
             bool allow_empty = true) const;

  typedef OPENDDS_VECTOR(String) StringList;
  void set(const char* key,
           const StringList& value);
  StringList get(const char* key,
                 const StringList& value) const;

  template<typename T, size_t count>
  T get(const char* key,
        T value,
        const EnumList<T> (&decoder)[count])
  {
    bool found = false;
    String value_as_string;
    for (size_t idx = 0; idx < count; ++idx) {
      if (decoder[idx].value == value) {
        value_as_string = decoder[idx].name;
        found = true;
        break;
      }
    }

    if (!found && log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING,
                 "(%P|%t) WARNING: ConfigStoreImpl::get: "
                 "failed to convert default value to string\n"));
    }

    const String actual = get(key, value_as_string);
    for (size_t idx = 0; idx < count; ++idx) {
      if (decoder[idx].name == actual) {
        return decoder[idx].value;
      }
    }

    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING,
                 "(%P|%t) WARNING: ConfigStoreImpl::get: "
                 "for %C, failed to encode (%C) to enumerated value, defaulting to (%C)\n",
                 key, actual.c_str(), value_as_string.c_str()));
    }

    return value;
  }

  template<typename T, size_t count>
  void set(const char* key,
           T value,
           const EnumList<T> (&decoder)[count])
  {
    bool found = false;
    String value_as_string;
    for (size_t idx = 0; idx < count; ++idx) {
      if (decoder[idx].value == value) {
        value_as_string = decoder[idx].name;
        found = true;
        break;
      }
    }

    if (!found) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) WARNING: ConfigStoreImpl::set: "
                   "for %C, failed to convert enum value to string\n",
                   key));
      }
      return;
    }

    set(key, value_as_string);
  }

  template<typename T, size_t count>
  void set(const char* key,
           const String& value,
           const EnumList<T> (&decoder)[count])
  {
    bool found = false;
    // Sanity check.
    String value_as_string;
    for (size_t idx = 0; idx < count; ++idx) {
      if (value == decoder[idx].name) {
        set(key, value);
        found = true;
        break;
      }
    }

    if (!found && log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING,
                 "(%P|%t) WARNING: ConfigStoreImpl::set: "
                 "for %C, %C is not a valid enum value\n",
                 key, value.c_str()));
    }
  }

  void set(const char* key,
           const TimeDuration& value,
           TimeFormat format);
  TimeDuration get(const char* key,
                   const TimeDuration& value,
                   TimeFormat format) const;

  void set(const char* key,
           const NetworkAddress& value,
           NetworkAddressFormat format,
           NetworkAddressKind kind);
  NetworkAddress get(const char* key,
                     const NetworkAddress& value,
                     NetworkAddressFormat format,
                     NetworkAddressKind kind) const;

  void set(const char* key,
           const NetworkAddressSet& value,
           NetworkAddressFormat format,
           NetworkAddressKind kind);
  NetworkAddressSet get(const char* key,
                        const NetworkAddressSet& value,
                        NetworkAddressFormat format,
                        NetworkAddressKind kind) const;

  // Section names are identified as values starting with '@' and
  // having the original text of the last part of the section name.
  // This is used to create objects of different types.
  StringList get_section_names(const String& prefix) const;

  typedef OPENDDS_MAP(String, String) StringMap;
  StringMap get_section_values(const String& prefix) const;

  /// Remove the section key and all section values.
  void unset_section(const String& prefix) const;

  static DDS::DataWriterQos datawriter_qos();
  static DDS::DataReaderQos datareader_qos();

  static bool debug_logging;

private:
  ConfigTopic_rch config_topic_;
  ConfigWriter_rch config_writer_;
  ConfigReader_rch config_reader_;
};

// Takes all samples from reader and returns true if any have the key prefix.
OpenDDS_Dcps_Export bool
take_has_prefix(ConfigReader_rch reader,
                const String& prefix);

OpenDDS_Dcps_Export void
process_section(ConfigStoreImpl& config_store,
                ConfigReader_rch reader,
                ConfigReaderListener_rch listener,
                const String& key_prefix,
                ACE_Configuration_Heap& config,
                const ACE_Configuration_Section_Key& base,
                bool allow_overwrite);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_CONFIG_STORE_IMPL_H */
