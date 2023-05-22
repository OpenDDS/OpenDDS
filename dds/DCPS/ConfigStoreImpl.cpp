/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ConfigStoreImpl.h"

#include "Qos_Helper.h"
#include "debug.h"

#include <ace/OS_NS_ctype.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

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

      if (ACE_OS::ace_isupper(x) && ACE_OS::ace_islower(y) && !retval.empty() && retval[retval.size() - 1] != '_') {
        retval += '_';
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

ConfigStoreImpl::ConfigStoreImpl()
  : config_topic_(make_rch<InternalTopic<ConfigPair> >())
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
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
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
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      DDS::Boolean x = 0;
      if (sample.value() == "true") {
        value = true;
      } else if (sample.value() == "false") {
        value = false;
      } else if (DCPS::convertToInteger(sample.value(), x)) {
        value = x;
      } else if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get_boolean: ")
                   ACE_TEXT("failed to parse boolean for %C=%C\n"),
                   sample.key().c_str(), sample.value().c_str()));
      }
    }
  }

  return value;
}

void
ConfigStoreImpl::set_int32(const char* key,
                           DDS::Int32 value)
{
  set_string(key, to_dds_string(value).c_str());
}

DDS::Int32
ConfigStoreImpl::get_int32(const char* key,
                           DDS::Int32 value)
{
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      DDS::Int32 x = 0;
      if (DCPS::convertToInteger(sample.value(), x)) {
        value = x;
      } else if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get_int32: ")
                   ACE_TEXT("failed to parse int32 for %C=%C\n"),
                   sample.key().c_str(), sample.value().c_str()));
      }
    }
  }

  return value;
}

void
ConfigStoreImpl::set_uint32(const char* key,
                            DDS::UInt32 value)
{
  set_string(key, to_dds_string(value).c_str());
}

DDS::UInt32
ConfigStoreImpl::get_uint32(const char* key,
                            DDS::UInt32 value)
{
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      DDS::UInt32 x = 0;
      if (DCPS::convertToInteger(sample.value(), x)) {
        value = x;
      } else if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get_uint32: ")
                   ACE_TEXT("failed to parse uint32 for %C=%C\n"),
                   sample.key().c_str(), sample.value().c_str()));
      }
    }
  }

  return value;
}

void
ConfigStoreImpl::set_float64(const char* key,
                             DDS::Float64 value)
{
  set_string(key, to_dds_string(value).c_str());
}

DDS::Float64
ConfigStoreImpl::get_float64(const char* key,
                             DDS::Float64 value)
{
  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      DDS::Float64 x = 0;
      if (DCPS::convertToDouble(sample.value(), x)) {
        value = x;
      } else if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get_float64: ")
                   ACE_TEXT("failed to parse float64 for %C=%C\n"),
                   sample.key().c_str(), sample.value().c_str()));
      }
    }
  }

  return value;
}

void
ConfigStoreImpl::set_string(const char* key,
                            const char* value)
{
  const ConfigPair cp(key, value);
  if (log_level >= LogLevel::Info) {
    ACE_DEBUG((LM_INFO,
               "(%P|%t) INFO: ConfigStoreImpl::set_string: %C=%C\n",
               cp.key().c_str(), cp.value().c_str()));
  }
  config_writer_->write(cp);
}

char*
ConfigStoreImpl::get_string(const char* key,
                            const char* value)
{
  CORBA::String_var retval(value);

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      retval = sample.value().c_str();
    }
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
  DDS::Duration_t retval = value;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      if (from_dds_string(sample.value(), retval)) {
        // Okay.
      } else if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get_duration: ")
                   ACE_TEXT("failed to parse DDS::Duration_t for %C=%C\n"),
                   sample.key().c_str(), sample.value().c_str()));
      }
    }
  }

  return retval;
}

void
ConfigStoreImpl::unset(const char* key)
{
  config_writer_->unregister_instance(ConfigPair(key, ""));
}

void
ConfigStoreImpl::set(const char* key,
                     const String& value)
{
  ConfigPair cp(key, value);

  if (log_level >= LogLevel::Info) {
    ACE_DEBUG((LM_INFO,
               "(%P|%t) INFO: ConfigStoreImpl::set: %C=%C\n",
               cp.key().c_str(), cp.value().c_str()));
  }
  config_writer_->write(cp);
}

String
ConfigStoreImpl::get(const char* key,
                     const String& value) const
{
  String retval = value;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      retval = sample.value();
    }
  }

  return retval;
}

void
ConfigStoreImpl::set(const char* key,
                     const TimeDuration& value,
                     IntegerTimeFormat format)
{
  switch (format) {
  case Format_IntegerMilliseconds:
    set_int32(key, value.value().msec());
    break;
  case Format_IntegerSeconds:
    set_int32(key, static_cast<DDS::Int32>(value.value().sec()));
    break;
  }
}

TimeDuration
ConfigStoreImpl::get(const char* key,
                     const TimeDuration& value,
                     IntegerTimeFormat format) const
{
  TimeDuration retval = value;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      int x = 0;
      if (DCPS::convertToInteger(sample.value(), x)) {
        switch (format) {
        case Format_IntegerMilliseconds:
          retval = TimeDuration::from_msec(x);
          break;
        case Format_IntegerSeconds:
          retval = TimeDuration(x);
          break;
        }
      } else if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                   ACE_TEXT("failed to parse TimeDuration for %C=%C\n"),
                   sample.key().c_str(), sample.value().c_str()));
      }
    }
  }

  return retval;
}

NetworkAddress
ConfigStoreImpl::get(const char* key,
                     const NetworkAddress& value) const
{
  NetworkAddress retval = value;

  DCPS::InternalDataReader<ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->read_instance(samples, infos, DDS::LENGTH_UNLIMITED, ConfigPair(key, ""),
                                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const ConfigPair& sample = samples[idx];
    const DDS::SampleInfo& info = infos[idx];
    if (info.valid_data) {
      if (!sample.value().empty()) {
        ACE_INET_Addr addr;
        if (addr.set(u_short(0), sample.value().c_str()) == 0) {
          retval = NetworkAddress(addr);
        } else if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ConfigStoreImpl::get: ")
                     ACE_TEXT("failed to parse NetworkAddress for %C=%C\n"),
                     sample.key().c_str(), sample.value().c_str()));
        }
      }
    }
  }

  return retval;
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
ConfigStoreImpl::contains_prefix(ConfigReader_rch reader,
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


}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
