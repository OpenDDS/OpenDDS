#include "RelayStatisticsReporter.h"

namespace RtpsRelay {

using OpenDDS::DCPS::DataReaderQosBuilder;
using OpenDDS::DCPS::Statistics;
using OpenDDS::DCPS::StatisticsDataReader;
using OpenDDS::DCPS::make_rch;

RelayStatisticsReporter::RelayStatisticsReporter(const Config& config, RelayStatisticsDataWriter_var writer)
  : config_(config)
  , writer_(writer)
  , topic_name_(DDS::Topic_var(writer_->get_topic())->get_name())
  , internal_reader_(make_rch<StatisticsDataReader>(DataReaderQosBuilder().reliability_reliable()))
{
  log_relay_statistics_.relay_id(config.relay_id());
  publish_relay_statistics_.relay_id(config.relay_id());

  if (config.log_relay_statistics() || config.publish_relay_statistics()) {
    TheServiceParticipant->statistics_topic()->connect(internal_reader_);
  }
}

RelayStatisticsReporter::~RelayStatisticsReporter()
{
  TheServiceParticipant->statistics_topic()->disconnect(internal_reader_);
}

void RelayStatisticsReporter::get_opendds_stats(RelayStatistics& out)
{
  StatisticsDataReader::SampleSequence samples;
  OpenDDS::DCPS::InternalSampleInfoSequence infos;
  internal_reader_->read(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  out.opendds_modules().resize(samples.size());
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    Statistics& is = samples[idx];
    OpenDDSModuleStatistics& mod = out.opendds_modules()[idx];
    mod.id() = is.id;
    mod.stats().resize(is.stats.length());
    for (DDS::UInt32 idxStats = 0; idxStats < is.stats.length(); ++idxStats) {
      mod.stats()[idxStats] = Statistic{is.stats[idxStats].name.in(), is.stats[idxStats].value};
    }
  }
}

void RelayStatisticsReporter::log_report(const OpenDDS::DCPS::MonotonicTimePoint& now, bool force)
{
  if (!log_helper_.prepare_report(log_relay_statistics_, now, force, config_.log_relay_statistics())) {
    return;
  }

  get_opendds_stats(log_relay_statistics_);

  ACE_DEBUG((LM_INFO, "(%P|%t) STAT: %C %C\n", topic_name_.in(), OpenDDS::DCPS::to_json(log_relay_statistics_).c_str()));

  log_helper_.reset(log_relay_statistics_, now);
  log_relay_statistics_.new_address_count(0);
  log_relay_statistics_.expired_address_count(0);
  log_relay_statistics_.admission_deferral_count(0);
  log_relay_statistics_.max_ips_per_client(0);
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

  get_opendds_stats(stats_copy);

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
