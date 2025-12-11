/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Config.h"

namespace RtpsRelay {

bool Config::from_arg(ACE_Arg_Shifter_T<char>& args)
{
  const char* arg = nullptr;
  if ((arg = args.get_the_parameter("-Lifespan"))) {
    lifespan(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-InactivePeriod"))) {
    inactive_period(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-ApplicationDomain"))) {
    application_domain(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-BufferSize"))) {
    buffer_size(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-AllowEmptyPartition"))) {
    allow_empty_partition(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-LogWarnings"))) {
    log_warnings(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-LogDiscovery"))) {
    log_discovery(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-LogActivity"))) {
    log_activity(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-LogHttp"))) {
    log_http(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-LogThreadStatus"))) {
    log_thread_status(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-ThreadStatusSafetyFactor"))) {
    thread_status_safety_factor(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-UtilizationLimit"))) {
    utilization_limit(ACE_OS::atof(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-LogUtilizationChanges"))) {
    log_utilization_changes(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-LogRelayStatistics"))) {
    log_relay_statistics(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-LogHandlerStatistics"))) {
    log_handler_statistics(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-PublishRelayStatistics"))) {
    publish_relay_statistics(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-PublishHandlerStatistics"))) {
    publish_handler_statistics(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-PublishRelayStatusLiveliness"))) {
    publish_relay_status_liveliness(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-PublishRelayStatus"))) {
    publish_relay_status(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-RestartDetection"))) {
    restart_detection(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-AdmissionControlQueueSize"))) {
    admission_control_queue_size(static_cast<size_t>(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-AdmissionControlQueueDuration"))) {
    admission_control_queue_duration(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-AdmissionMaxParticipantsRange"))) {
    if (!parse_admission_participants_range(arg)) {
      return false;
    }
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-RunTime"))) {
    run_time(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-HandlerThreads"))) {
    handler_threads(static_cast<size_t>(std::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-SynchronousOutput"))) {
    synchronous_output(ACE_OS::atoi(arg));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-MaxIpsPerClient"))) {
    max_ips_per_client(static_cast<size_t>(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-RejectedAddressDuration"))) {
    rejected_address_duration(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-Id"))) {
    relay_id(arg);
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-DrainInterval"))) {
    drain_interval(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else if ((arg = args.get_the_parameter("-DeniedPartitionsTimeout"))) {
    denied_partitions_timeout(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
    args.consume_arg();
  } else {
    return false;
  }
  return true;
}

bool Config::parse_admission_participants_range(const char* arg)
{
  auto conv = std::atoi(arg);
  if (conv <= 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Argument for -AdmissionMaxParticipantsRange option must be positive: %d\n", conv));
    return false;
  }
  const auto low = static_cast<size_t>(conv);
  admission_max_participants_low_water(low);
  const auto dash = std::strchr(arg, '-');
  if (!dash) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Argument for -AdmissionMaxParticipantsRange option must contain a '-': %C\n", arg));
    return false;
  }
  conv = std::atoi(dash + 1);
  if (conv <= 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Argument for -AdmissionMaxParticipantsRange option must be positive: %d\n", conv));
    return false;
  }
  if (static_cast<size_t>(conv) < low) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: High value for -AdmissionMaxParticipantsRange option must be greater than or equal to low value\n"));
    return false;
  }
  admission_max_participants_high_water(static_cast<size_t>(conv));
  return true;
}

void Config::set_defaults()
{
  using OpenDDS::DCPS::TimeDuration;
  cached_lifespan_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_LIFESPAN, default_val.to_dds_duration());
  });
  cached_inactive_period_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_INACTIVE_PERIOD, default_val.to_dds_duration());
  });
  cached_log_warnings_.default_if_empty([&](bool default_val) {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_WARNINGS, default_val);
  });
  cached_log_discovery_.default_if_empty([&](bool default_val) {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_DISCOVERY, default_val);
  });
  cached_log_activity_.default_if_empty([&](bool default_val) {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_ACTIVITY, default_val);
  });
  cached_log_http_.default_if_empty([&](bool default_val) {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_HTTP, default_val);
  });
  cached_log_thread_status_.default_if_empty([&](bool default_val) {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_THREAD_STATUS, default_val);
  });
  cached_thread_status_safety_factor_.default_if_empty([&](int default_val) {
    TheServiceParticipant->config_store()->set_int32(RTPS_RELAY_THREAD_STATUS_SAFETY_FACTOR, default_val);
  });
  cached_utilization_limit_.default_if_empty([&](double default_val) {
    TheServiceParticipant->config_store()->set_float64(RTPS_RELAY_UTILIZATION_LIMIT, default_val);
  });
  cached_log_utilization_changes_.default_if_empty([&](bool default_val) {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_LOG_UTILIZATION_CHANGES, default_val);
  });
  cached_log_relay_statistics_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_LOG_RELAY_STATISTICS, default_val.to_dds_duration());
  });
  cached_log_handler_statistics_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_LOG_HANDLER_STATISTICS, default_val.to_dds_duration());
  });
  cached_publish_relay_statistics_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_RELAY_STATISTICS, default_val.to_dds_duration());
  });
  cached_publish_handler_statistics_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_HANDLER_STATISTICS, default_val.to_dds_duration());
  });
  cached_publish_relay_status_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_RELAY_STATUS, default_val.to_dds_duration());
  });
  cached_publish_relay_status_liveliness_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS, default_val.to_dds_duration());
  });
  cached_restart_detection_.default_if_empty([&](bool default_val) {
    TheServiceParticipant->config_store()->set_boolean(RTPS_RELAY_RESTART_DETECTION, default_val);
  });
  cached_admission_control_queue_size_.default_if_empty([&](size_t default_val) {
    TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_ADMISSION_CONTROL_QUEUE_SIZE, default_val);
  });
  cached_admission_control_queue_duration_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_ADMISSION_CONTROL_QUEUE_DURATION, default_val.to_dds_duration());
  });
  cached_max_ips_per_client_.default_if_empty([&](size_t default_val) {
    TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_MAX_IPS_PER_CLIENT, default_val);
  });
  cached_rejected_address_duration_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_REJECTED_ADDRESS_DURATION, default_val.to_dds_duration());
  });
  cached_admission_max_participants_high_water_.default_if_empty([&](size_t default_val) {
    TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_MAX_PARTICIPANTS_HIGH_WATER, default_val);
  });
  cached_admission_max_participants_low_water_.default_if_empty([&](size_t default_val) {
    TheServiceParticipant->config_store()->set_uint64(RTPS_RELAY_MAX_PARTICIPANTS_LOW_WATER, default_val);
  });
  cached_denied_partitions_timeout_.default_if_empty([&](const TimeDuration& default_val) {
    TheServiceParticipant->config_store()->set_duration(RTPS_RELAY_DENIED_PARTITIONS_TIMEOUT, default_val.to_dds_duration());
  });
}

void Config::on_data_available(InternalDataReader_rch reader)
{
  OpenDDS::DCPS::ConfigReader::SampleSequence samples;
  OpenDDS::DCPS::InternalSampleInfoSequence infos;
  reader->read(samples, infos, DDS::LENGTH_UNLIMITED,
               DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const auto& info = infos[idx];
    const auto& pair = samples[idx];
    if (info.valid_data) {
      if (pair.key() == RTPS_RELAY_LIFESPAN) {
        cached_lifespan_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_INACTIVE_PERIOD) {
        cached_inactive_period_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_LOG_WARNINGS) {
        cached_log_warnings_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_LOG_DISCOVERY) {
        cached_log_discovery_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_LOG_ACTIVITY) {
        cached_log_activity_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_LOG_HTTP) {
        cached_log_http_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_LOG_THREAD_STATUS) {
        cached_log_thread_status_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_THREAD_STATUS_SAFETY_FACTOR) {
        cached_thread_status_safety_factor_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_UTILIZATION_LIMIT) {
        cached_utilization_limit_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_LOG_UTILIZATION_CHANGES) {
        cached_log_utilization_changes_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_LOG_RELAY_STATISTICS) {
        cached_log_relay_statistics_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_LOG_HANDLER_STATISTICS) {
        cached_log_handler_statistics_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_PUBLISH_RELAY_STATISTICS) {
        cached_publish_relay_statistics_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_PUBLISH_HANDLER_STATISTICS) {
        cached_publish_handler_statistics_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_PUBLISH_RELAY_STATUS) {
        cached_publish_relay_status_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS) {
        cached_publish_relay_status_liveliness_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_RESTART_DETECTION) {
        cached_restart_detection_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_ADMISSION_CONTROL_QUEUE_SIZE) {
        cached_admission_control_queue_size_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_ADMISSION_CONTROL_QUEUE_DURATION) {
        cached_admission_control_queue_duration_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_MAX_IPS_PER_CLIENT) {
        cached_max_ips_per_client_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_REJECTED_ADDRESS_DURATION) {
        cached_rejected_address_duration_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_MAX_PARTICIPANTS_HIGH_WATER) {
        cached_admission_max_participants_high_water_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_MAX_PARTICIPANTS_LOW_WATER) {
        cached_admission_max_participants_low_water_.set(pair.value());
      } else if (pair.key() == RTPS_RELAY_DENIED_PARTITIONS_TIMEOUT) {
        cached_denied_partitions_timeout_.set(pair.value());
      }
    }
  }
}

bool Config::to_time_duration(const std::string& value, OpenDDS::DCPS::TimeDuration& out)
{
  DDS::Duration_t d;
  if (OpenDDS::DCPS::from_dds_string(value, d)) {
    out = OpenDDS::DCPS::TimeDuration{d};
    return true;
  }
  return false;
}

}
