#include "RelayStatisticsReporter.h"

#include <fstream>

namespace RtpsRelay {

using OpenDDS::DCPS::make_rch;

namespace {
  std::string get_topic_name(DDS::DataWriter_var writer)
  {
    DDS::Topic_var topic = writer->get_topic();
    CORBA::String_var name = topic->get_name();
    return name.in();
  }
}

RelayStatisticsReporter::RelayStatisticsReporter(const Config& config, RelayStatisticsDataWriter_var writer)
  : config_(config)
  , writer_(writer)
  , topic_name_(get_topic_name(DDS::DataWriter::_duplicate(writer)))
{
  log_relay_statistics_.relay_id(config.relay_id());
  publish_relay_statistics_.relay_id(config.relay_id());
}

void RelayStatisticsReporter::log_report(const OpenDDS::DCPS::MonotonicTimePoint& now, bool force)
{
  if (!log_helper_.prepare_report(log_relay_statistics_, now, force, config_.log_relay_statistics())) {
    return;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.c_str(), OpenDDS::DCPS::to_json(log_relay_statistics_).c_str()));

  log_helper_.reset(log_relay_statistics_, now);
  log_relay_statistics_.new_address_count(0);
  log_relay_statistics_.expired_address_count(0);
  log_relay_statistics_.admission_deferral_count(0);
  log_relay_statistics_.max_ips_per_client(0);
}

void RelayStatisticsReporter::log_json(const std::string& label, const std::string& json)
{
  // subtract estimate of characters for %P %t expansions, other fixed strings
  static constexpr auto space = size_t{ACE_MAXLOGMSGLEN - 65};

  for (size_t i = 0, len; i < json.size(); i += len) {
    std::string substr;
    if (json.size() - i <= space) {
      substr = json.substr(i);
    } else {
      substr = json.substr(i, space);
      // find the last comma so a name/value pair is not split across lines
      // special case, don't split the comma between "name":"x" and "value":y
      auto idx = substr.rfind(',');
      while (idx != std::string::npos && json.substr(i + idx, 9) == R"(,"value":)") {
        idx = substr.rfind(',', idx - 1);
      }
      if (idx != std::string::npos) {
        substr.erase(idx + 1);
      }
    }
    len = substr.size();
    ACE_DEBUG((LM_INFO, "(%P|%t) STAT: %C%C %C\n",
                        label.c_str(),
                        i ? " (cont.)" : len < json.size() ? " (split)" : "",
                        substr.c_str()));
  }
}

void RelayStatisticsReporter::publish_report(ACE_Guard<ACE_Thread_Mutex>& guard,
                                             const OpenDDS::DCPS::MonotonicTimePoint& now,
                                             bool force)
{
  if (!publish_helper_.prepare_report(publish_relay_statistics_, now, force, config_.publish_relay_statistics())) {
    return;
  }

  const auto writer_copy = writer_;
  auto stats_copy = publish_relay_statistics_;

  publish_helper_.reset(publish_relay_statistics_, now);
  publish_relay_statistics_.new_address_count(0);
  publish_relay_statistics_.expired_address_count(0);
  publish_relay_statistics_.admission_deferral_count(0);
  publish_relay_statistics_.max_ips_per_client(0);

  guard.release();

  const auto ret = writer_copy->write(stats_copy, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
               "RelayStatisticsReporter::publish_report failed to write relay statistics\n"));
  }
}

}
