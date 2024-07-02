/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ConfigStoreImpl.h"

#include "LogAddr.h"
#include "Qos_Helper.h"
#include "debug.h"

#include <ace/OS_NS_ctype.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OPENDDS_VECTOR(String)
split(const String& str,
      const String& delims,
      bool skip_leading,
      bool skip_consecutive)
{
  OPENDDS_VECTOR(String) retval;

  std::string::size_type pos = 0;
  if (skip_leading) {
    pos = str.find_first_not_of(delims);
  }

  while (pos != std::string::npos && pos < str.size()) {
    const std::string::size_type limit = str.find_first_of(delims, pos);
    if (limit == std::string::npos) {
      retval.push_back(str.substr(pos));
      break;
    } else {
      retval.push_back(str.substr(pos, limit - pos));
      if (limit == str.size() - 1) {
        // Delimeter at the end.
        retval.push_back(String());
      }
      pos = limit + 1;
      if (skip_consecutive && pos != std::string::npos && pos < str.size()) {
        pos = str.find_first_not_of(delims, pos);
      }
    }
  }

  return retval;
}

String ConfigPair::canonicalize(const String& key)
{
  String retval;
  size_t idx = 0;

  // Skip leading punctuation.
  while (idx < key.size() && !ACE_OS::ace_isalnum(key[idx])) {
    ++idx;
  }

  while (idx < key.size()) {
    const char x = key[idx];

    if (idx + 1 < key.size()) {
      // Deal with camelcase;
      const char y = key[idx + 1];

      if (ACE_OS::ace_isupper(x) && ACE_OS::ace_islower(y)) {
        if (!retval.empty() && retval[retval.size() - 1] != '_') {
          retval += '_';
        }
        retval += ACE_OS::ace_toupper(x);
        ++idx;
        continue;
      } else if (ACE_OS::ace_islower(x) && ACE_OS::ace_isupper(y)) {
        retval += ACE_OS::ace_toupper(x);
        if (!retval.empty() && retval[retval.size() - 1] != '_') {
          retval += '_';
        }
        ++idx;
        continue;
      }
    }

    // Deal with non-punctuation.
    if (ACE_OS::ace_isalnum(x)) {
      retval += ACE_OS::ace_toupper(x);
      ++idx;
      continue;
    }

    while (idx < key.size() && !ACE_OS::ace_isalnum(key[idx])) {
      ++idx;
    }

    if (idx < key.size() && !retval.empty() && retval[retval.size() - 1] != '_') {
      retval += '_';
    }
  }

  return retval;
}

ConfigStoreImpl::ConfigStoreImpl(ConfigTopic_rch config_topic)
  : config_topic_(config_topic)
  , config_writer_(make_rch<InternalDataWriter<ConfigPair> >(datawriter_qos()))
  , config_reader_(make_rch<InternalDataReader<ConfigPair> >(datareader_qos()))
{
  config_topic_->connect(config_writer_);
  config_topic_->connect(config_reader_);
}

ConfigStoreImpl::~ConfigStoreImpl()
{
  config_topic_->disconnect(config_reader_);
  config_topic_->disconnect(config_writer_);
}

DDS::Boolean
ConfigStoreImpl::has(const char* key)
{
  const ConfigPair cp(key, "");
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      return true;
    }
  }

  return false;
}

void
ConfigStoreImpl::set_boolean(const char* key,
                             DDS::Boolean value)
{
  set_string(key, value ? "true" : "false");
}

DDS::Boolean
ConfigStoreImpl::get_boolean(const char* key,
                             DDS::Boolean value)
{
  const ConfigPair cp(key, "");
  DDS::Boolean retval = value;
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      DDS::Boolean x = 0;
      if (sample.value() == "true") {
        retval = true;
      } else if (sample.value() == "false") {
        retval = false;
      } else if (DCPS::convertToInteger(sample.value(), x)) {
        retval = x;
      } else {
        retval = value;
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::parse_boolean: ")
                     ACE_TEXT("failed to parse boolean for %C=%C\n"),
                     sample.key().c_str(), sample.value().c_str()));
        }
      }
    }
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get_boolean: %C=%C\n",
               CONFIG_DEBUG_LOGGING,
               cp.key().c_str(),
               retval ? "true" : "false"));
  }

  return retval;
}

void
ConfigStoreImpl::set_int32(const char* key,
                           DDS::Int32 value)
{
  set(key, to_dds_string(value));
}

DDS::Int32
ConfigStoreImpl::get_int32(const char* key,
                           DDS::Int32 value)
{
  const ConfigPair cp(key, "");
  DDS::Int32 retval = value;
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      DDS::Int32 x = 0;
      if (sample.value() == "DURATION_INFINITE_SEC") {
        retval = DDS::DURATION_INFINITE_SEC;
      } else if (DCPS::convertToInteger(sample.value(), x)) {
        retval = x;
      } else {
        retval = value;
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get_int32: ")
                     ACE_TEXT("failed to parse int32 for %C=%C\n"),
                     sample.key().c_str(), sample.value().c_str()));
        }
      }
    }
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get_int32: %C=%d\n",
               CONFIG_DEBUG_LOGGING,
               cp.key().c_str(),
               retval));

  }

  return retval;
}

void
ConfigStoreImpl::set_uint32(const char* key,
                            DDS::UInt32 value)
{
  set(key, to_dds_string(value));
}

DDS::UInt32
ConfigStoreImpl::get_uint32(const char* key,
                            DDS::UInt32 value)
{
  const ConfigPair cp(key, "");
  DDS::UInt32 retval = value;
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      DDS::UInt32 x = 0;
      if (sample.value() == "DURATION_INFINITE_NANOSEC") {
        retval = DDS::DURATION_INFINITE_NSEC;
      } else if (DCPS::convertToInteger(sample.value(), x)) {
        retval = x;
      } else {
        retval = value;
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get_uint32: ")
                     ACE_TEXT("failed to parse uint32 for %C=%C\n"),
                     sample.key().c_str(), sample.value().c_str()));
        }
      }
    }
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get_int32: %C=%u\n",
               CONFIG_DEBUG_LOGGING,
               cp.key().c_str(),
               retval));

  }

  return retval;
}

void
ConfigStoreImpl::set_float64(const char* key,
                             DDS::Float64 value)
{
  set(key, to_dds_string(value));
}

DDS::Float64
ConfigStoreImpl::get_float64(const char* key,
                             DDS::Float64 value)
{
  const ConfigPair cp(key, "");
  DDS::Float64 retval = value;
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      DDS::Float64 x = 0;
      if (DCPS::convertToDouble(sample.value(), x)) {
        retval = x;
      } else {
        retval = value;
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get_float64: ")
                     ACE_TEXT("failed to parse float64 for %C=%C\n"),
                     sample.key().c_str(), sample.value().c_str()));
        }
      }
    }
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get_float64: %C=%g\n",
               CONFIG_DEBUG_LOGGING,
               cp.key().c_str(),
               retval));

  }

  return retval;
}

void
ConfigStoreImpl::set_string(const char* key,
                            const char* value)
{
  const ConfigPair cp(key, value);
  if (log_level >= LogLevel::Info || debug_logging) {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: ConfigStoreImpl::set_string: %C=%C\n",
               cp.key().c_str(),
               cp.value().c_str()));
  }
  config_writer_->write(cp);
}

char*
ConfigStoreImpl::get_string(const char* key,
                            const char* value)
{
  const ConfigPair cp(key, "");
  CORBA::String_var retval(value);

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      retval = sample.value().c_str();
    }
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get_string: %C=%C\n",
               CONFIG_DEBUG_LOGGING,
               cp.key().c_str(),
               retval.in()));

  }

  return retval._retn();
}

void
ConfigStoreImpl::set_duration(const char* key,
                              const DDS::Duration_t& value)
{
  set(key, to_dds_string(value));
}

DDS::Duration_t
ConfigStoreImpl::get_duration(const char* key,
                              const DDS::Duration_t& value)
{
  const ConfigPair cp(key, "");
  DDS::Duration_t retval = value;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      if (from_dds_string(sample.value(), retval)) {
        // Okay.
      } else {
        retval = value;
        if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get_duration: ")
                     ACE_TEXT("failed to parse DDS::Duration_t for %C=%C\n"),
                     sample.key().c_str(), sample.value().c_str()));
        }
      }
    }
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get_duration: %C=%C\n",
               CONFIG_DEBUG_LOGGING,
               cp.key().c_str(),
               to_dds_string(retval).c_str()));

  }

  return retval;
}

void
ConfigStoreImpl::unset(const char* key)
{
  const ConfigPair cp(key, "");
  config_writer_->unregister_instance(cp);
}

void
ConfigStoreImpl::set(const char* key,
                     const String& value)
{
  ConfigPair cp(key, value);

  if (log_level >= LogLevel::Info || debug_logging) {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: ConfigStoreImpl::set: %C=%C\n",
               cp.key().c_str(),
               cp.value().c_str()));
  }
  config_writer_->write(cp);
}

String
ConfigStoreImpl::get(const char* key,
                     const String& value,
                     bool allow_empty) const
{
  const ConfigPair cp(key, "");
  String retval = value;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data && (allow_empty || !sample.value().empty())) {
      retval = sample.value();
    }
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get: %C=%C\n",
               CONFIG_DEBUG_LOGGING,
               cp.key().c_str(),
               retval.c_str()));

  }

  return retval;
}

namespace {
  String time_duration_to_string(const TimeDuration& value,
                                 ConfigStoreImpl::TimeFormat format)
  {
    switch (format) {
    case ConfigStoreImpl::Format_IntegerMilliseconds:
      return to_dds_string(value.value().msec());
    case ConfigStoreImpl::Format_IntegerSeconds:
      return to_dds_string(value.value().sec());
    case ConfigStoreImpl::Format_FractionalSeconds:
      return to_dds_string(value.to_double());
    }
    return "";
  }
}

void
ConfigStoreImpl::set(const char* key,
                     const StringList& value)
{
  String s;
  for (StringList::const_iterator pos = value.begin(), limit = value.end(); pos != limit; ++pos) {
    if (!s.empty()) {
      s += ',';
    }
    s += *pos;
  }

  set(key, s);
}

ConfigStoreImpl::StringList
ConfigStoreImpl::get(const char* key,
                     const StringList& value) const
{
  // Join the default.
  String s;
  for (StringList::const_iterator pos = value.begin(), limit = value.end(); pos != limit; ++pos) {
    if (!s.empty()) {
      s += ',';
    }
    s += *pos;
  }

  const String t = get(key, s);

  StringList retval;

  const char* start = t.c_str();
  while (const char* next_comma = std::strchr(start, ',')) {
    const size_t size = next_comma - start;
    retval.push_back(String(start, size));
    start = next_comma + 1;
  }
  // Append everything after last comma
  retval.push_back(start);

  return retval;
}

void
ConfigStoreImpl::set(const char* key,
                     const TimeDuration& value,
                     TimeFormat format)
{
  switch (format) {
  case Format_IntegerMilliseconds:
    set_uint32(key, value.value().msec());
    break;
  case Format_IntegerSeconds:
    set_uint32(key, static_cast<DDS::UInt32>(value.value().sec()));
    break;
  case Format_FractionalSeconds:
    set_float64(key, value.to_double());
    break;
  }
}

TimeDuration
ConfigStoreImpl::get(const char* key,
                     const TimeDuration& value,
                     TimeFormat format) const
{
  const ConfigPair cp(key, "");
  TimeDuration retval = value;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      switch (format) {
      case Format_IntegerMilliseconds:
        {
          DDS::UInt32 x = 0;
          if (DCPS::convertToInteger(sample.value(), x)) {
            retval = TimeDuration::from_msec(x);
          } else {
            retval = value;
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                         ACE_TEXT("failed to parse TimeDuration (integer milliseconds) for %C=%C\n"),
                         sample.key().c_str(), sample.value().c_str()));
            }
          }
        }
        break;
      case Format_IntegerSeconds:
        {
          DDS::UInt32 x = 0;
          if (DCPS::convertToInteger(sample.value(), x)) {
            retval = TimeDuration(x);
          } else {
            retval = value;
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                         ACE_TEXT("failed to parse TimeDuration (integer seconds) for %C=%C\n"),
                         sample.key().c_str(), sample.value().c_str()));
            }
          }
        }
        break;
      case Format_FractionalSeconds:
        {
          double x = 0.0;
          if (DCPS::convertToDouble(sample.value(), x)) {
            retval = TimeDuration::from_double(x);
          } else {
            retval = value;
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                         ACE_TEXT("failed to parse TimeDuration (fractional seconds) for %C=%C\n"),
                         sample.key().c_str(), sample.value().c_str()));
            }
          }
        }
        break;
      }
    }
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get: %C=%C\n",
               CONFIG_DEBUG_LOGGING,
               cp.key().c_str(),
               time_duration_to_string(retval, format).c_str()));
  }

  return retval;
}

namespace {

  bool expected_kind(const NetworkAddress& value,
                     ConfigStoreImpl::NetworkAddressKind kind)
  {
    switch (kind) {
    case ConfigStoreImpl::Kind_ANY:
      return value.get_type() == AF_INET
#ifdef ACE_HAS_IPV6
        || value.get_type() == AF_INET6
#endif
        ;
    case ConfigStoreImpl::Kind_IPV4:
      return value.get_type() == AF_INET;
#ifdef ACE_HAS_IPV6
    case ConfigStoreImpl::Kind_IPV6:
      return value.get_type() == AF_INET6;
#endif
    }

    return false;
  }

}

void
ConfigStoreImpl::set(const char* key,
                     const NetworkAddress& value,
                     NetworkAddressFormat format,
                     NetworkAddressKind kind)
{
  String addr_str;

  switch (format) {
  case Format_No_Port:
    addr_str = LogAddr(value, LogAddr::Ip).str();
    break;
  case Format_Required_Port:
    addr_str = LogAddr(value, LogAddr::IpPort).str();
    break;
  case Format_Optional_Port:
    if (value.get_port_number() == 0) {
      addr_str = LogAddr(value, LogAddr::Ip).str();
    } else {
      addr_str = LogAddr(value, LogAddr::IpPort).str();
    }
    break;
  }

  if (!expected_kind(value, kind)) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::set: ")
                 ACE_TEXT("NetworkAddress kind mismatch for %C=%C\n"),
                 key, addr_str.c_str()));
    }
    return;
  }

  set(key, addr_str);
}

namespace {
  bool parse_no_port(const ConfigPair& sample, NetworkAddress& retval, const NetworkAddress& value)
  {
    ACE_INET_Addr addr;
    if (addr.set(u_short(0), sample.value().c_str()) == 0) {
      retval = NetworkAddress(addr);
      return true;
    } else {
      retval = value;
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                   ACE_TEXT("failed to parse NetworkAddress for %C=%C\n"),
                   sample.key().c_str(), sample.value().c_str()));
      }
    }
    return false;
  }

  bool parse_port(const ConfigPair& sample, NetworkAddress& retval, const NetworkAddress& value)
  {
    ACE_INET_Addr addr;
    if (addr.set(sample.value().c_str()) == 0) {
      retval = NetworkAddress(addr);
      return true;
    } else {
      retval = value;
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                   ACE_TEXT("failed to parse NetworkAddress for %C=%C\n"),
                   sample.key().c_str(), sample.value().c_str()));
      }
    }
    return false;
  }

  bool parse_optional_port(const ConfigPair& sample, NetworkAddress& retval, const NetworkAddress& value)
  {
    ACE_INET_Addr addr;
    if (addr.set(sample.value().c_str()) == 0) {
      retval = NetworkAddress(addr);
      return true;
    } else if (addr.set(u_short(0), sample.value().c_str()) == 0) {
      retval = NetworkAddress(addr);
      return true;
    } else {
      retval = value;
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                   ACE_TEXT("failed to parse NetworkAddress for %C=%C\n"),
                   sample.key().c_str(), sample.value().c_str()));
      }
    }
    return false;
  }
}

NetworkAddress
ConfigStoreImpl::get(const char* key,
                     const NetworkAddress& value,
                     NetworkAddressFormat format,
                     NetworkAddressKind kind) const
{
  OPENDDS_ASSERT(expected_kind(value, kind));

  const ConfigPair cp(key, "");
  NetworkAddress retval = value;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, cp,
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      if (!sample.value().empty()) {
        switch (format) {
        case Format_No_Port:
          parse_no_port(sample, retval, value);
          break;
        case Format_Required_Port:
          parse_port(sample, retval, value);
          break;
        case Format_Optional_Port:
          parse_optional_port(sample, retval, value);
          break;
        }
      }
    }
  }

  if (!expected_kind(retval, kind)) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                 ACE_TEXT("NetworkAddress kind mismatch for %C\n"),
                 cp.key().c_str()));
    }
    retval = value;
  }

  if (retval.get_port_number() == 0) {
    retval.set_port_number(value.get_port_number());
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get: %C=%C\n",
               CONFIG_DEBUG_LOGGING,
               cp.key().c_str(),
               LogAddr(retval.to_addr()).c_str()));
  }

  return retval;
}

void
ConfigStoreImpl::set(const char* key,
                     const NetworkAddressSet& value,
                     NetworkAddressFormat format,
                     NetworkAddressKind kind)
{
  String addr_str;

  for (NetworkAddressSet::const_iterator pos = value.begin(), limit = value.end(); pos != limit; ++pos) {
    String temp;
    switch (format) {
    case Format_No_Port:
      temp = LogAddr(*pos, LogAddr::Ip).str();
      break;
    case Format_Required_Port:
      temp = LogAddr(*pos, LogAddr::IpPort).str();
      break;
    case Format_Optional_Port:
      if (pos->get_port_number() == 0) {
        temp = LogAddr(*pos, LogAddr::Ip).str();
      } else {
        temp = LogAddr(*pos, LogAddr::IpPort).str();
      }
      break;
    }

    if (!expected_kind(*pos, kind)) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::set: ")
                   ACE_TEXT("NetworkAddress kind mismatch for %C member %C\n"),
                   key, temp.c_str()));
      }
      return;
    }

    if (!addr_str.empty()) {
      addr_str += ",";
    }
    addr_str += temp;
  }

  set(key, addr_str);
}

namespace {
  String network_address_set_to_string(const NetworkAddressSet& value,
                                       ConfigStoreImpl::NetworkAddressFormat format)
  {
    String retval;

    for (NetworkAddressSet::const_iterator pos = value.begin(), limit = value.end(); pos != limit; ++pos) {
      if (!retval.empty()) {
        retval += ",";
      }

      switch (format) {
      case ConfigStoreImpl::Format_No_Port:
        retval += LogAddr(*pos, LogAddr::Ip).str();
        break;
      case ConfigStoreImpl::Format_Required_Port:
        retval += LogAddr(*pos, LogAddr::IpPort).str();
        break;
      case ConfigStoreImpl::Format_Optional_Port:
        if (pos->get_port_number()) {
          retval += LogAddr(*pos, LogAddr::IpPort).str();
        } else {
          retval += LogAddr(*pos, LogAddr::Ip).str();
        }
        break;
      }
    }

    return retval;
  }
}

NetworkAddressSet
ConfigStoreImpl::get(const char* key,
                     const NetworkAddressSet& value,
                     NetworkAddressFormat format,
                     NetworkAddressKind kind) const
{
  if (DCPS_debug_level) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: ConfigStoreImpl::get: %C\n", key));
  }

  NetworkAddress default_na;
  switch (kind) {
  case Kind_ANY:
  case Kind_IPV4:
    default_na = NetworkAddress::default_IPV4;
    break;
#ifdef ACE_HAS_IPV6
  case Kind_IPV6:
    default_na = NetworkAddress::default_IPV6;
    break;
#endif
  }

  NetworkAddressSet retval = value;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      retval.clear();
      if (!sample.value().empty()) {
        typedef OPENDDS_VECTOR(String) Vector;
        const Vector vec = split(sample.value(), " ,", true, true);

        // Expand any addresses specified with optional :quantity:interval
        Vector expandedVec;
        for (Vector::const_iterator i = vec.begin(); i != vec.end(); ++i){
          const Vector parts = split(*i, ":", true, true);
          String baseAddress = parts[0];
          if (parts.size() > 1){
            baseAddress.append(":");
            baseAddress.append(parts[1]);
          }
          expandedVec.push_back(baseAddress);

          if (parts.size() > 2){
            // quantity has been specified
            int basePort = atoi(parts[1].c_str());
            int numPorts = atoi(parts[2].c_str());
            // default of 2 for specifying multiple SpdpSendAddrs endpoints.
            int interval = 2;
            if (parts.size() == 4)
              interval = atoi(parts[3].c_str());

            if (numPorts){
              for (int j=1; j < numPorts; ++j){
                String additionalAddr = parts[0];
                additionalAddr.append(":");
                char portString[10];
                ACE_OS::itoa(basePort + (j*interval), portString, 10);
                additionalAddr.append(portString);
                expandedVec.push_back(additionalAddr);
              }
            }
          }
        }
        bool err = false;
        for (Vector::const_iterator pos = expandedVec.begin(), limit = expandedVec.end(); !err && pos != limit; ++pos) {
          NetworkAddress addr;
          switch (format) {
          case Format_No_Port:
            err = !parse_no_port(ConfigPair(sample.key(), *pos), addr, default_na);
            break;
          case Format_Required_Port:
            err = !parse_port(ConfigPair(sample.key(), *pos), addr, default_na);
            break;
          case Format_Optional_Port:
            err = !parse_optional_port(ConfigPair(sample.key(), *pos), addr, default_na);
            break;
          }

          if (err) {
            retval = value;
            break;
          }

          if (!expected_kind(addr, kind)) {
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                         ACE_TEXT("NetworkAddress kind mismatch for %C\n"),
                         key));
            }
            retval = value;
            break;
          }

          retval.insert(addr);
        }
      }
    }
  }

  if (debug_logging) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) %C: ConfigStoreImpl::get: %C=%C\n",
               CONFIG_DEBUG_LOGGING,
               key,
               network_address_set_to_string(retval, format).c_str()));
  }

  return retval;
}

ConfigStoreImpl::StringList
ConfigStoreImpl::get_section_names(const String& prefix) const
{
  const String cprefix = ConfigPair::canonicalize(prefix);

  ConfigReader::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read(samples, infos, DDS::LENGTH_UNLIMITED,
                       DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  StringList retval;
  for (ConfigReader::SampleSequence::const_iterator pos = samples.begin(), limit = samples.end(); pos != limit; ++pos) {
    if (pos->key_has_prefix(cprefix) &&
        pos->key() != cprefix &&
        !pos->value().empty() &&
        pos->value().substr(0, 1) == "@") {
      const String name = pos->value().substr(1);
      if (ConfigPair::canonicalize(prefix + "_" + name) == pos->key()) {
        retval.push_back(name);
      }
    }
  }

  return retval;
}

ConfigStoreImpl::StringMap
ConfigStoreImpl::get_section_values(const String& prefix) const
{
  const String cprefix = ConfigPair::canonicalize(prefix);

  ConfigReader::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read(samples, infos, DDS::LENGTH_UNLIMITED,
                       DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  StringMap retval;
  for (ConfigReader::SampleSequence::const_iterator pos = samples.begin(), limit = samples.end(); pos != limit; ++pos) {
    if (pos->key_has_prefix(cprefix) &&
        pos->key() != cprefix) {
      retval[pos->key().substr(cprefix.size() + 1)] = pos->value();
    }
  }

  return retval;
}

void
ConfigStoreImpl::unset_section(const String& prefix) const
{
  const String cprefix = ConfigPair::canonicalize(prefix);

  ConfigReader::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read(samples, infos, DDS::LENGTH_UNLIMITED,
                       DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (ConfigReader::SampleSequence::const_iterator pos = samples.begin(), limit = samples.end(); pos != limit; ++pos) {
    if (pos->key_has_prefix(cprefix)) {
      config_writer_->unregister_instance(*pos);
    }
  }
}

DDS::DataWriterQos ConfigStoreImpl::datawriter_qos()
{
  return DataWriterQosBuilder().durability_transient_local();
}

DDS::DataReaderQos ConfigStoreImpl::datareader_qos()
{
  return DataReaderQosBuilder()
    .reliability_reliable()
    .durability_transient_local()
    .reader_data_lifecycle_autopurge_nowriter_samples_delay(make_duration_t(0, 0))
    .reader_data_lifecycle_autopurge_disposed_samples_delay(make_duration_t(0, 0));
}

bool
take_has_prefix(ConfigReader_rch reader,
                const String& prefix)
{
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  reader->take(samples, infos, DDS::LENGTH_UNLIMITED,
               DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    if (samples[idx].key_has_prefix(prefix)) {
      return true;
    }
  }

  return false;
}

bool ConfigStoreImpl::debug_logging = CONFIG_DEBUG_LOGGING_default;

void
process_section(ConfigStoreImpl& config_store,
                ConfigReader_rch reader,
                ConfigReaderListener_rch listener,
                const String& key_prefix,
                ACE_Configuration_Heap& config,
                const ACE_Configuration_Section_Key& base,
                bool allow_overwrite)
{
  // Process the values.
  int status = 0;
  for (int idx = 0; status == 0; ++idx) {
    ACE_TString key;
    ACE_Configuration_Heap::VALUETYPE value_type;
    status = config.enumerate_values(base, idx, key, value_type);
    if (status == 0) {
      switch (value_type) {
      case ACE_Configuration_Heap::STRING:
        {
          ACE_TString value;
          if (config.get_string_value(base, key.c_str(), value) == 0) {
            const String key_name = key_prefix + "_" + ACE_TEXT_ALWAYS_CHAR(key.c_str());
            String value_str = ACE_TEXT_ALWAYS_CHAR(value.c_str());
            if (allow_overwrite || !config_store.has(key_name.c_str())) {
              config_store.set(key_name.c_str(), value_str);
              if (listener && reader) {
                listener->on_data_available(reader);
              }
            } else if (log_level >= LogLevel::Notice) {
              ACE_DEBUG((LM_NOTICE,
                         "(%P|%t) NOTICE: process_section: "
                         "value from commandline or user for %s overrides value in config file\n",
                         key.c_str()));
            }
          } else {
            if (log_level >= LogLevel::Error) {
              ACE_ERROR((LM_ERROR,
                         "(%P|%t) ERROR: process_section: "
                         "get_string_value() failed for key \"%s\"\n",
                         key.c_str()));
            }
          }
        }
        break;
      case ACE_Configuration_Heap::INTEGER:
      case ACE_Configuration_Heap::BINARY:
      case ACE_Configuration_Heap::INVALID:
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: process_section: "
                     "unsupported value type for key \"%s\"\n",
                     key.c_str()));
        }
        break;
      }
    }
  }

  // Recur on the subsections.
  status = 0;
  for (int idx = 0; status == 0; ++idx) {
    ACE_TString section_name;
    status = config.enumerate_sections(base, idx, section_name);
    const String next_key_prefix = key_prefix + "_" + ACE_TEXT_ALWAYS_CHAR(section_name.c_str());

    if (status == 0) {
      ACE_Configuration_Section_Key key;
      if (config.open_section(base, section_name.c_str(), 0, key) == 0) {
        process_section(config_store, reader, listener, next_key_prefix, config, key, allow_overwrite);
      } else {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: process_section: "
                     "open_section() failed for name \"%s\"\n",
                     section_name.c_str()));
        }
      }

      // Indicate the section last.
      // This allows the listener to see complete sections.
      if (allow_overwrite || !config_store.has(next_key_prefix.c_str())) {
        config_store.set(next_key_prefix.c_str(), String("@") + ACE_TEXT_ALWAYS_CHAR(section_name.c_str()));
        if (listener && reader) {
          listener->on_data_available(reader);
        }
      }
    }
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
