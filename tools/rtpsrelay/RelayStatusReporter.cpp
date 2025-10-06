#include "RelayStatusReporter.h"

#include "StatisticsWriterListener.h"

namespace RtpsRelay {

RelayStatusReporter::RelayStatusReporter(const Config& config,
                                         GuidAddrSet& guid_addr_set,
                                         DDS::Publisher_var relay_publisher,
                                         ACE_Reactor* reactor,
                                         RelayStatisticsReporter& relay_statistics_reporter)
  : ACE_Event_Handler(reactor)
  , InternalDataReaderListener{TheServiceParticipant->job_queue()}
  , config_(config)
  , guid_addr_set_(guid_addr_set)
  , relay_publisher_(relay_publisher)
  , relay_statistics_reporter_(relay_statistics_reporter)
  , publish_relay_status_(config.publish_relay_status())
  , publish_relay_status_liveliness_(config.publish_relay_status_liveliness())
{
  relay_status_.relay_id(config.relay_id());

  // Register itself to the Config object which will be called when one of the configuration options relevant
  // to this object changes.
  const std::vector<std::string> config_params{ RTPS_RELAY_PUBLISH_RELAY_STATUS, RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS };
  config_.observe_changes(*this, config_params);
}

bool RelayStatusReporter::setup_writer()
{
  ACE_Guard<ACE_Thread_Mutex> g{lock_};
  return setup_writer_i(true);
}

bool RelayStatusReporter::setup_writer_i(bool liveliness_changed)
{
  reactor()->cancel_timer(this);
  if (publish_relay_status_ == OpenDDS::DCPS::TimeDuration::zero_value) {
    writer_ = nullptr;
    return true;
  }

  // No need to create a new writer if we already have one and the liveliness QoS hasn't changed.
  if (!writer_ || liveliness_changed) {
    DDS::DomainParticipant_var participant = relay_publisher_->get_participant();
    DDS::TopicDescription_var tdesc = participant->lookup_topicdescription(RELAY_STATUS_TOPIC_NAME.c_str());
    DDS::Topic_var topic = DDS::Topic::_narrow(tdesc);
    if (!topic) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to get topic for Relay Status data writer\n"));
      return false;
    }

    DDS::DataWriterQos relay_status_writer_qos;
    relay_publisher_->get_default_datawriter_qos(relay_status_writer_qos);
    relay_status_writer_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    if (publish_relay_status_liveliness_ != OpenDDS::DCPS::TimeDuration::zero_value) {
      relay_status_writer_qos.liveliness.lease_duration = publish_relay_status_liveliness_.to_dds_duration();
      relay_status_writer_qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    }

    DDS::DataWriterListener_var relay_status_writer_listener =
      new StatisticsWriterListener(relay_statistics_reporter_, &RelayStatisticsReporter::relay_status_sub_count);
    DDS::DataWriter_var relay_status_writer = relay_publisher_->create_datawriter(topic, relay_status_writer_qos, relay_status_writer_listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!relay_status_writer) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Status data writer\n"));
      return false;
    }

    writer_ = RelayStatusDataWriter::_narrow(relay_status_writer);
    if (!writer_) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to narrow Relay Status data writer\n"));
      return false;
    }
  }

  reactor()->schedule_timer(this, 0, ACE_Time_Value(), publish_relay_status_.value());
  return true;
}

// void RelayStatusReporter::on_data_available(InternalDataReader_rch reader)
// {
//   ACE_Guard<ACE_Thread_Mutex> g{lock_};
//   OpenDDS::DCPS::ConfigReader::SampleSequence samples;
//   OpenDDS::DCPS::InternalSampleInfoSequence infos;
//   reader->read(samples, infos, DDS::LENGTH_UNLIMITED,
//                DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
//   for (size_t idx = 0; idx != samples.size(); ++idx) {
//     const auto& info = infos[idx];
//     const auto& pair = samples[idx];
//     if (info.valid_data) {
//       if (pair.key() == RTPS_RELAY_PUBLISH_RELAY_STATUS) {
//         OpenDDS::DCPS::TimeDuration td;
//         if (Config::to_time_duration(pair.value(), td)) {
//           if (td != publish_relay_status_) {
//             publish_relay_status_ = td;
//             setup_writer_i();
//           }
//         }
//       } else if (pair.key() == RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS) {
//         OpenDDS::DCPS::TimeDuration td;
//         if (Config::to_time_duration(pair.value(), td)) {
//           const bool liveliness_changed = td != publish_relay_status_liveliness_;
//           if (liveliness_changed) {
//             publish_relay_status_liveliness_ = td;
//             setup_writer_i(liveliness_changed);
//           }
//         }
//       }
//     }
//   }
// }

// void RelayStatusReporter::on_config_changed(const OpenDDS::DCPS::ConfigReader::SampleSequence& changes)
// {
//   ACE_Guard<ACE_Thread_Mutex> g{lock_};
//   for (const auto& pair : changes) {
//     if (pair.key() == RTPS_RELAY_PUBLISH_RELAY_STATUS) {
//       OpenDDS::DCPS::TimeDuration td;
//       if (Config::to_time_duration(pair.value(), td)) {
//         if (td != publish_relay_status_) {
//           publish_relay_status_ = td;
//           setup_writer_i();
//         }
//       }
//     } else if (pair.key() == RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS) {
//       OpenDDS::DCPS::TimeDuration td;
//       if (Config::to_time_duration(pair.value(), td)) {
//         const bool liveliness_changed = td != publish_relay_status_liveliness_;
//         if (liveliness_changed) {
//           publish_relay_status_liveliness_ = td;
//           setup_writer_i(liveliness_changed);
//         }
//       }
//     }
//   }
// }

void on_config_changed(const std::string& key, const std::string& value)
{
  ACE_Guard<ACE_Thread_Mutex> g{lock_};
  if (key == RTPS_RELAY_PUBLISH_RELAY_STATUS) {
    OpenDDS::DCPS::TimeDuration td;
    if (Config::to_time_duration(value, td)) {
      if (td != publish_relay_status_) {
        publish_relay_status_ = td;
        setup_writer_i();
      }
    }
  } else if (key == RTPS_RELAY_PUBLISH_RELAY_STATUS_LIVELINESS) {
    OpenDDS::DCPS::TimeDuration td;
    if (Config::to_time_duration(value, td)) {
      const bool liveliness_changed = td != publish_relay_status_liveliness_;
      if (liveliness_changed) {
        publish_relay_status_liveliness_ = td;
        setup_writer_i(liveliness_changed);
      }
    }
  }
}

int RelayStatusReporter::handle_timeout(const ACE_Time_Value&, const void*)
{
  if (config_.log_activity()) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RelayStatusReporter::handle_timeout\n"));
  }

  OpenDDS::DCPS::ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager(), TIMER_MASK);

  {
    GuidAddrSet::Proxy proxy(guid_addr_set_);
    proxy.populate_relay_status(relay_status_);
  }

  ACE_Guard<ACE_Thread_Mutex> g{lock_};

  if (!writer_) {
    return 0;
  }

  if (writer_->write(relay_status_, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayStatusReporter::handle_timeout failed to write Relay Status\n"));
  }

  return 0;
}

}
