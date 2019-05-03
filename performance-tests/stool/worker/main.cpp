#include "dds/DCPS/Service_Participant.h"

#include "ListenerFactory.h"

#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include "StoolC.h"
#include "StoolTypeSupportImpl.h"
#include "BuilderTypeSupportImpl.h"

#include "TopicListener.h"
#include "DataReaderListener.h"
#include "DataWriterListener.h"
#include "SubscriberListener.h"
#include "PublisherListener.h"
#include "ParticipantListener.h"
#include "Process.h"

#include "json_2_builder.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <iomanip>
#include <condition_variable>

// MyTopicListener

class MyTopicListener : public Builder::TopicListener {
public:
  void on_inconsistent_topic(DDS::Topic_ptr /*the_topic*/, const DDS::InconsistentTopicStatus& /*status*/) {
    std::unique_lock<std::mutex> lock(mutex_);
    ++inconsistent_count_;
  }

  void set_topic(Builder::Topic& topic) {
    topic_ = &topic;
  }

protected:
  std::mutex mutex_;
  Builder::Topic* topic_{0};
  size_t inconsistent_count_{0};
};

// MyDataReaderListener

class MyDataReaderListener : public Builder::DataReaderListener {
public:

  MyDataReaderListener() {}
  MyDataReaderListener(size_t expected) : expected_count_(expected) {}

  void on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/, const DDS::RequestedDeadlineMissedStatus& /*status*/) override {
  }

  void on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/, const DDS::RequestedIncompatibleQosStatus& /*status*/) override {
  }

  void on_sample_rejected(DDS::DataReader_ptr /*reader*/, const DDS::SampleRejectedStatus& /*status*/) override {
  }

  void on_liveliness_changed(DDS::DataReader_ptr /*reader*/, const DDS::LivelinessChangedStatus& /*status*/) override {
  }

  void on_data_available(DDS::DataReader_ptr reader) override {
    Stool::DataDataReader_var data_dr = Stool::DataDataReader::_narrow(reader);
    if (data_dr) {
      Stool::Data data;
      DDS::SampleInfo si;
      DDS::ReturnCode_t status = data_dr->take_next_sample(data, si);
      if (status == DDS::RETCODE_OK && si.valid_data) {
        const Builder::TimeStamp& now = Builder::get_time();
        double latency = Builder::to_seconds_double(now - data.created.time);

        std::unique_lock<std::mutex> lock(mutex_);
        if (datareader_) {
          auto prev_latency_mean = latency_mean_->value.double_prop();
          auto prev_latency_var_x_sample_count = latency_var_x_sample_count_->value.double_prop();

          sample_count_->value.ull_prop(sample_count_->value.ull_prop() + 1);

          if (latency < latency_min_->value.double_prop()) {
            latency_min_->value.double_prop(latency);
          }
          if (latency_max_->value.double_prop() < latency) {
            latency_max_->value.double_prop(latency);
          }
          // Incremental mean calculation (doesn't require storing all the data)
          latency_mean_->value.double_prop(prev_latency_mean + ((latency - prev_latency_mean) / static_cast<double>(sample_count_->value.double_prop())));
          // Incremental (variance * sample_count) calculation (doesn't require storing all the data, can be used to easily find variance / standard deviation)
          latency_var_x_sample_count_->value.double_prop(prev_latency_var_x_sample_count + ((latency - prev_latency_mean) * (latency - latency_mean_->value.double_prop())));
        }
      }
    }
  }

  void on_subscription_matched(DDS::DataReader_ptr /*reader*/, const DDS::SubscriptionMatchedStatus& status) override {
    std::unique_lock<std::mutex> lock(mutex_);
    if (expected_count_ != 0) {
      if (static_cast<size_t>(status.current_count) == expected_count_) {
        //std::cout << "MyDataReaderListener reached expected count!" << std::endl;
        if (datareader_) {
          last_discovery_time_->value.time_prop(Builder::get_time());
        }
      }
    } else {
      if (static_cast<size_t>(status.current_count) > matched_count_) {
        if (datareader_) {
          last_discovery_time_->value.time_prop(Builder::get_time());
        }
      }
    }
    matched_count_ = status.current_count;
  }

  void on_sample_lost(DDS::DataReader_ptr /*reader*/, const DDS::SampleLostStatus& /*status*/) override {
  }

  void set_datareader(Builder::DataReader& datareader) override {
    datareader_ = &datareader;
    last_discovery_time_ = get_or_create_property(datareader_->get_report().properties, "last_discovery_time", Builder::PVK_TIME);

    sample_count_ = get_or_create_property(datareader_->get_report().properties, "sample_count", Builder::PVK_ULL);
    sample_count_->value.ull_prop(0);
    latency_min_ = get_or_create_property(datareader_->get_report().properties, "latency_min", Builder::PVK_DOUBLE);
    latency_min_->value.double_prop(std::numeric_limits<double>::max());
    latency_max_ = get_or_create_property(datareader_->get_report().properties, "latency_max", Builder::PVK_DOUBLE);
    latency_max_->value.double_prop(0.0);
    latency_mean_ = get_or_create_property(datareader_->get_report().properties, "latency_mean", Builder::PVK_DOUBLE);
    latency_mean_->value.double_prop(0.0);
    latency_var_x_sample_count_ = get_or_create_property(datareader_->get_report().properties, "latency_var_x_sample_count", Builder::PVK_DOUBLE);
    latency_var_x_sample_count_->value.double_prop(0.0);
  }

protected:
  std::mutex mutex_;
  size_t expected_count_{0};
  size_t matched_count_{0};
  Builder::DataReader* datareader_{0};
  Builder::PropertyIndex last_discovery_time_;
  Builder::PropertyIndex sample_count_;
  Builder::PropertyIndex latency_min_;
  Builder::PropertyIndex latency_max_;
  Builder::PropertyIndex latency_mean_;
  Builder::PropertyIndex latency_var_x_sample_count_;
};

// MySubscriberListener

class MySubscriberListener : public Builder::SubscriberListener {
public:

  // From DDS::DataReaderListener

  void on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/, const DDS::RequestedDeadlineMissedStatus& /*status*/) override {
  }

  void on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/, const DDS::RequestedIncompatibleQosStatus& /*status*/) override {
  }

  void on_sample_rejected(DDS::DataReader_ptr /*reader*/, const DDS::SampleRejectedStatus& /*status*/) override {
  }

  void on_liveliness_changed(DDS::DataReader_ptr /*reader*/, const DDS::LivelinessChangedStatus& /*status*/) override {
  }

  void on_data_available(DDS::DataReader_ptr /*reader*/) override {
  }

  void on_subscription_matched(DDS::DataReader_ptr /*reader*/, const DDS::SubscriptionMatchedStatus& /*status*/) override {
  }

  void on_sample_lost(DDS::DataReader_ptr /*reader*/, const DDS::SampleLostStatus& /*status*/) override {
  }

  // From DDS::SubscriberListener

  void on_data_on_readers(DDS::Subscriber_ptr /*subs*/) override {
  }

  // From Builder::SubscriberListener

  void set_subscriber(Builder::Subscriber& subscriber) override {
    subscriber_ = &subscriber;
  }

protected:
  std::mutex mutex_;
  Builder::Subscriber* subscriber_{0};
};

// MyDataWriterListener

class MyDataWriterListener : public Builder::DataWriterListener {
public:

  MyDataWriterListener() {}
  MyDataWriterListener(size_t expected) : expected_count_(expected) {}

  void on_offered_deadline_missed(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedDeadlineMissedStatus& /*status*/) override {
  }

  void on_offered_incompatible_qos(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedIncompatibleQosStatus& /*status*/) override {
  }

  void on_liveliness_lost(DDS::DataWriter_ptr /*writer*/, const DDS::LivelinessLostStatus& /*status*/) override {
  }

  void on_publication_matched(DDS::DataWriter_ptr /*writer*/, const DDS::PublicationMatchedStatus& status) override {
    std::unique_lock<std::mutex> lock(mutex_);
    if (expected_count_ != 0) {
      if (static_cast<size_t>(status.current_count) == expected_count_) {
        //std::cout << "MyDataWriterListener reached expected count!" << std::endl;
        if (datawriter_) {
          last_discovery_time_->value.time_prop(Builder::get_time());
        }
      }
    } else {
      if (static_cast<size_t>(status.current_count) > matched_count_) {
        if (datawriter_) {
          last_discovery_time_->value.time_prop(Builder::get_time());
        }
      }
    }
    matched_count_ = status.current_count;
  }

  void set_datawriter(Builder::DataWriter& datawriter) override {
    datawriter_ = &datawriter;
    last_discovery_time_ = get_or_create_property(datawriter_->get_report().properties, "last_discovery_time", Builder::PVK_TIME);
  }

protected:
  std::mutex mutex_;
  size_t expected_count_{0};
  size_t matched_count_{0};
  Builder::DataWriter* datawriter_{0};
  Builder::PropertyIndex last_discovery_time_;
};

// MyPublisherListener

class MyPublisherListener : public Builder::PublisherListener {
public:

  // From DDS::DataWriterListener

  void on_offered_deadline_missed(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedDeadlineMissedStatus& /*status*/) override {
  }

  void on_offered_incompatible_qos(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedIncompatibleQosStatus& /*status*/) override {
  }

  void on_liveliness_lost(DDS::DataWriter_ptr /*writer*/, const DDS::LivelinessLostStatus& /*status*/) override {
  }

  void on_publication_matched(DDS::DataWriter_ptr /*writer*/, const DDS::PublicationMatchedStatus& /*status*/) override {
  }

  // From DDS::PublisherListener

  // From Stool::PublisherListener

  void set_publisher(Builder::Publisher& publisher) override {
    publisher_ = &publisher;
  }

protected:
  std::mutex mutex_;
  Builder::Publisher* publisher_{0};
};

// MyParticipantListener

class MyParticipantListener : public Builder::ParticipantListener {
public:

  // From DDS::DataWriterListener

  void on_offered_deadline_missed(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedDeadlineMissedStatus& /*status*/) {
  }

  void on_offered_incompatible_qos(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedIncompatibleQosStatus& /*status*/) {
  }

  void on_liveliness_lost(DDS::DataWriter_ptr /*writer*/, const DDS::LivelinessLostStatus& /*status*/) {
  }

  void on_publication_matched(DDS::DataWriter_ptr /*writer*/, const DDS::PublicationMatchedStatus& /*status*/) {
  }

  void on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/, const DDS::RequestedDeadlineMissedStatus& /*status*/) {
  }

  void on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/, const DDS::RequestedIncompatibleQosStatus& /*status*/) {
  }

  // From DDS::SubscriberListener

  void on_data_on_readers(DDS::Subscriber_ptr /*subscriber*/) {
  }

  // From DDS::DataReaderListener

  void on_sample_rejected(DDS::DataReader_ptr /*reader*/, const DDS::SampleRejectedStatus& /*status*/) {
  }

  void on_liveliness_changed(DDS::DataReader_ptr /*reader*/, const DDS::LivelinessChangedStatus& /*status*/) {
  }

  void on_data_available(DDS::DataReader_ptr /*reader*/) {
  }

  void on_subscription_matched(DDS::DataReader_ptr /*reader*/, const DDS::SubscriptionMatchedStatus& /*status*/) {
  }

  void on_sample_lost(DDS::DataReader_ptr /*reader*/, const DDS::SampleLostStatus& /*status*/) {
  }

  void on_inconsistent_topic(DDS::Topic_ptr /*the_topic*/, const DDS::InconsistentTopicStatus& /*status*/) {
  }

  // From Builder::ParticipantListener

  void set_participant(Builder::Participant& participant) override {
    participant_ = &participant;
  }

protected:
  std::mutex mutex_;
  Builder::Participant* participant_{0};
};

using Builder::ReaderMap;
using Builder::WriterMap;

// Action

class Action {
public:
  virtual ~Action() {}
  virtual bool init(const Stool::ActionConfig& config, Stool::ActionReport& report, ReaderMap& readers, WriterMap& writers);

  virtual void start() = 0;
  virtual void stop() = 0;

protected:
  const Stool::ActionConfig* config_{0};
  Stool::ActionReport* report_{0};
  ReaderMap readers_by_name_;
  WriterMap writers_by_name_;
  std::vector<std::shared_ptr<Builder::DataReader> > readers_by_index_;
  std::vector<std::shared_ptr<Builder::DataWriter> > writers_by_index_;
};

bool Action::init(const Stool::ActionConfig& config, Stool::ActionReport& report, ReaderMap& reader_map, WriterMap& writer_map) {
  config_ = &config;
  report_ = &report;
  for (CORBA::ULong j = 0; j < config.readers.length(); ++j) {
    auto it = reader_map.find(config.readers[j].in());
    if (it != reader_map.end()) {
      readers_by_name_.insert(*it);
      readers_by_index_.push_back(it->second);
    } else {
      return false;
    }
  }
  for (CORBA::ULong j = 0; j < config.writers.length(); ++j) {
    auto it = writer_map.find(config.writers[j].in());
    if (it != writer_map.end()) {
      writers_by_name_.insert(*it);
      writers_by_index_.push_back(it->second);
    } else {
      return false;
    }
  }
  return true;
};

// Action Manager

class ActionManager {
public:
  explicit ActionManager(const Stool::ActionConfigSeq& configs, Stool::ActionReportSeq& reports, ReaderMap& reader_map, WriterMap& writer_map);

  void start();
  void stop();

  using action_factory = std::function<std::shared_ptr<Action>()>;
  using action_factory_map = std::map<std::string, action_factory>;

  static bool register_action_factory(const std::string& name, const action_factory& factory) {
    std::unique_lock<std::mutex> lock(s_mutex);
    bool result = false;

    auto it = s_factory_map.find(name);
    if (it == s_factory_map.end()) {
      s_factory_map[name] = factory;
      result = true;
    }
    return result;
  }

  static std::shared_ptr<Action> create_action(const std::string& name) {
    std::unique_lock<std::mutex> lock(s_mutex);
    std::shared_ptr<Action> result;

    auto it = s_factory_map.find(name);
    if (it != s_factory_map.end()) {
      result = (it->second)();
    }
    return result;
  }

  class Registration {
  public:
    Registration(const std::string& name, const action_factory& factory) {
      std::cout << "Action registration created for name '" << name << "'" << std::endl;
      if (!register_action_factory(name, factory)) {
        std::stringstream ss;
        ss << "unable to register action factory with name '" << name << "'" << std::flush;
        throw std::runtime_error(ss.str());
      }
    }
  };

protected:
  static std::mutex s_mutex;
  static action_factory_map s_factory_map;

  std::vector<std::shared_ptr<Action>> actions_;
};

std::mutex ActionManager::s_mutex;
ActionManager::action_factory_map ActionManager::s_factory_map;

ActionManager::ActionManager(const Stool::ActionConfigSeq& configs, Stool::ActionReportSeq& reports, ReaderMap& reader_map, WriterMap& writer_map) {
  reports.length(configs.length());
  for (CORBA::ULong i = 0; i < configs.length(); ++i) {
    auto action = create_action(configs[i].type.in());
    if (action) {
      action->init(configs[i], reports[i], reader_map, writer_map);
    }
    actions_.push_back(action);
  }
}

void ActionManager::start() {
  for (auto it = actions_.begin(); it != actions_.end(); ++it) {
    (*it)->start();
  }
}

void ActionManager::stop() {
  for (auto it = actions_.begin(); it != actions_.end(); ++it) {
    (*it)->stop();
  }
}

// WriteAction
class WriteAction : public Action {
public:

};

// Main

int ACE_TMAIN(int argc, ACE_TCHAR* argv[]) {

  if (argc < 2) {
    std::cerr << "Configuration file expected as second argument." << std::endl;
    return 1;
  }

  std::ifstream ifs(argv[1]);
  if (!ifs.good()) {
    std::cerr << "Unable to open configuration file: '" << argv[1] << "'" << std::endl;
    return 2;
  }

  using Builder::ZERO;

  Stool::WorkerConfig config;

  config.enable_time = ZERO;
  config.start_time = ZERO;
  config.stop_time = ZERO;

  if (!json_2_builder(ifs, config)) {
    std::cerr << "Unable to parse configuration file: '" << argv[1] << "'" << std::endl;
    return 3;
  }

  /* Manually building the ProcessConfig object

  config.config_sections.length(1);
  config.config_sections[0].name = "common";
  config.config_sections[0].properties.length(2);
  config.config_sections[0].properties[0].name = "DCPSSecurity";
  config.config_sections[0].properties[0].value = "0";
  config.config_sections[0].properties[1].name = "DCPSDebugLevel";
  config.config_sections[0].properties[1].value = "0";

  config.discoveries.length(1);
  config.discoveries[0].type = "rtps";
  config.discoveries[0].name = "stool_test_rtps";
  config.discoveries[0].domain = 7;

  const size_t ips_per_proc_max = 10;
  const size_t subs_per_ip_max = 5;
  const size_t pubs_per_ip_max = 5;
  const size_t drs_per_sub_max = 10;
  const size_t dws_per_pub_max = 10;

  const size_t expected_datareader_matches = ips_per_proc_max * pubs_per_ip_max * dws_per_pub_max;
  const size_t expected_datawriter_matches = ips_per_proc_max * subs_per_ip_max * drs_per_sub_max;

  config.instances.length(ips_per_proc_max);
  config.participants.length(ips_per_proc_max);

  for (size_t ip = 0; ip < ips_per_proc_max; ++ip) {
    std::stringstream instance_name_ss;
    instance_name_ss << "transport_" << ip << std::flush;
    config.instances[ip].type = "rtps_udp";
    config.instances[ip].name = instance_name_ss.str().c_str();
    config.instances[ip].domain = 7;
    std::stringstream participant_name_ss;
    participant_name_ss << "participant_" << ip << std::flush;
    config.participants[ip].name = participant_name_ss.str().c_str();
    config.participants[ip].domain = 7;
    config.participants[ip].listener_type_name = "stool_partl";
    config.participants[ip].listener_status_mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK;
    //config.participants[ip].type_names.length(1);
    //config.participants[ip].type_names[0] = "Stool::Data";
    config.participants[ip].transport_config_name = instance_name_ss.str().c_str();

    config.participants[ip].qos.entity_factory.autoenable_created_entities = false;
    config.participants[ip].qos_mask.entity_factory.has_autoenable_created_entities = false;

    config.participants[ip].topics.length(1);
    std::stringstream topic_name_ss;
    topic_name_ss << "topic" << std::flush;
    config.participants[ip].topics[0].name = topic_name_ss.str().c_str();
    //config.participants[ip].topics[0].type_name = "Stool::Data";
    config.participants[ip].topics[0].listener_type_name = "stool_tl";
    config.participants[ip].topics[0].listener_status_mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

    config.participants[ip].subscribers.length(subs_per_ip_max);
    for (size_t sub = 0; sub < subs_per_ip_max; ++sub) {
      std::stringstream subscriber_name_ss;
      subscriber_name_ss << "subscriber_" << ip << "_" << sub << std::flush;
      config.participants[ip].subscribers[sub].name = subscriber_name_ss.str().c_str();
      config.participants[ip].subscribers[sub].listener_type_name = "stool_sl";
      config.participants[ip].subscribers[sub].listener_status_mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

      config.participants[ip].subscribers[sub].datareaders.length(drs_per_sub_max);
      for (size_t dr = 0; dr < drs_per_sub_max; ++dr) {
        std::stringstream datareader_name_ss;
        datareader_name_ss << "datareader_" << ip << "_" << sub << "_" << dr << std::flush;
        config.participants[ip].subscribers[sub].datareaders[dr].name = datareader_name_ss.str().c_str();
        config.participants[ip].subscribers[sub].datareaders[dr].topic_name = topic_name_ss.str().c_str();
        config.participants[ip].subscribers[sub].datareaders[dr].listener_type_name = "stool_drl";
        config.participants[ip].subscribers[sub].datareaders[dr].listener_status_mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

        config.participants[ip].subscribers[sub].datareaders[dr].qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
        config.participants[ip].subscribers[sub].datareaders[dr].qos_mask.reliability.has_kind = true;
      }
    }
    config.participants[ip].publishers.length(pubs_per_ip_max);
    for (size_t pub = 0; pub < pubs_per_ip_max; ++pub) {
      std::stringstream publisher_name_ss;
      publisher_name_ss << "publisher_" << ip << "_" << pub << std::flush;
      config.participants[ip].publishers[pub].name = publisher_name_ss.str().c_str();
      config.participants[ip].publishers[pub].listener_type_name = "stool_pl";
      config.participants[ip].publishers[pub].listener_status_mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

      config.participants[ip].publishers[pub].datawriters.length(dws_per_pub_max);
      for (size_t dw = 0; dw < dws_per_pub_max; ++dw) {
        std::stringstream datawriter_name_ss;
        datawriter_name_ss << "datawriter_" << ip << "_" << pub << "_" << dw << std::flush;
        config.participants[ip].publishers[pub].datawriters[dw].name = datawriter_name_ss.str().c_str();
        config.participants[ip].publishers[pub].datawriters[dw].topic_name = topic_name_ss.str().c_str();
        config.participants[ip].publishers[pub].datawriters[dw].listener_type_name = "stool_dwl";
        config.participants[ip].publishers[pub].datawriters[dw].listener_status_mask = OpenDDS::DCPS::DEFAULT_STATUS_MASK;
      }
    }
  }
  */

  // Register some Stool-specific types
  Builder::TypeSupportRegistry::TypeSupportRegistration process_config_registration(new Builder::ProcessConfigTypeSupportImpl());
  Builder::TypeSupportRegistry::TypeSupportRegistration data_registration(new Stool::DataTypeSupportImpl());

  // Register some Stool-specific listener factories
  Builder::ListenerFactory<DDS::TopicListener>::Registration topic_registration("stool_tl", [](){ return DDS::TopicListener_var(new MyTopicListener()); });
  Builder::ListenerFactory<DDS::DataReaderListener>::Registration datareader_registration("stool_drl", [&](){ return DDS::DataReaderListener_var(new MyDataReaderListener()); });
  Builder::ListenerFactory<DDS::SubscriberListener>::Registration subscriber_registration("stool_sl", [](){ return DDS::SubscriberListener_var(new MySubscriberListener()); });
  Builder::ListenerFactory<DDS::DataWriterListener>::Registration datawriter_registration("stool_dwl", [&](){ return DDS::DataWriterListener_var(new MyDataWriterListener()); });
  Builder::ListenerFactory<DDS::PublisherListener>::Registration publisher_registration("stool_pl", [](){ return DDS::PublisherListener_var(new MyPublisherListener()); });
  Builder::ListenerFactory<DDS::DomainParticipantListener>::Registration participant_registration("stool_partl", [](){ return DDS::DomainParticipantListener_var(new MyParticipantListener()); });

  // Timestamps used to measure method call durations
  Builder::TimeStamp process_construction_begin_time = ZERO, process_construction_end_time = ZERO;
  Builder::TimeStamp process_enable_begin_time = ZERO, process_enable_end_time = ZERO;
  Builder::TimeStamp process_start_begin_time = ZERO, process_start_end_time = ZERO;
  Builder::TimeStamp process_stop_begin_time = ZERO, process_stop_end_time = ZERO;
  Builder::TimeStamp process_destruction_begin_time = ZERO, process_destruction_end_time = ZERO;

  Builder::ProcessReport process_report;

  try {
    std::string line;
    std::condition_variable cv;
    std::mutex cv_mutex;

    std::cout << "Beginning process construction / entity creation." << std::endl;

    process_construction_begin_time = Builder::get_time();
    Builder::Process process(config.process);
    process_construction_end_time = Builder::get_time();

    std::cout << std::endl << "Process construction / entity creation complete." << std::endl << std::endl;

    if (config.enable_time == ZERO) {
      std::cout << "No test enable time specified. Press any key to enable process entities." << std::endl;
      std::getline(std::cin, line);
    } else {
      if (config.enable_time < ZERO) {
        auto dur = -get_duration(config.enable_time);
        std::unique_lock<std::mutex> lock(cv_mutex);
        while (cv.wait_for(lock, dur) != std::cv_status::timeout) {}
      } else {
        auto timeout_time = std::chrono::system_clock::time_point(get_duration(config.enable_time));
        std::unique_lock<std::mutex> lock(cv_mutex);
        while (cv.wait_until(lock, timeout_time) != std::cv_status::timeout) {}
      }
    }

    std::cout << "Enabling DDS entities (if not already enabled)." << std::endl;

    process_enable_begin_time = Builder::get_time();
    process.enable_dds_entities();
    process_enable_end_time = Builder::get_time();

    std::cout << "DDS entities enabled." << std::endl << std::endl;

    if (config.start_time == ZERO) {
      std::cout << "No test start time specified. Press any key to start process testing." << std::endl;
      std::getline(std::cin, line);
    } else {
      if (config.start_time < ZERO) {
        auto dur = -get_duration(config.start_time);
        std::unique_lock<std::mutex> lock(cv_mutex);
        while (cv.wait_for(lock, dur) != std::cv_status::timeout) {}
      } else {
        auto timeout_time = std::chrono::system_clock::time_point(get_duration(config.start_time));
        std::unique_lock<std::mutex> lock(cv_mutex);
        while (cv.wait_until(lock, timeout_time) != std::cv_status::timeout) {}
      }
    }

    std::cout << "Starting process tests." << std::endl;

    process_start_begin_time = Builder::get_time();
    //process.start();
    process_start_end_time = Builder::get_time();

    std::cout << "Process tests started." << std::endl << std::endl;

    if (config.stop_time == ZERO) {
      std::cout << "No stop time specified. Press any key to stop process testing." << std::endl;
      std::getline(std::cin, line);
    } else {
      if (config.stop_time < ZERO) {
        auto dur = -get_duration(config.stop_time);
        std::unique_lock<std::mutex> lock(cv_mutex);
        while (cv.wait_for(lock, dur) != std::cv_status::timeout) {}
      } else {
        auto timeout_time = std::chrono::system_clock::time_point(get_duration(config.stop_time));
        std::unique_lock<std::mutex> lock(cv_mutex);
        while (cv.wait_until(lock, timeout_time) != std::cv_status::timeout) {}
      }
    }

    std::cout << "Stopping process tests." << std::endl;

    process_stop_begin_time = Builder::get_time();
    //process.stop();
    process_stop_end_time = Builder::get_time();

    std::cout << "Process tests stopped." << std::endl << std::endl;

    process_report = process.get_report();

    std::cout << "Beginning process destruction / entity deletion." << std::endl;

    process_destruction_begin_time = Builder::get_time();
  } catch (const std::exception& e) {
    std::cerr << "Exception caught trying to build process object: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception caught trying to build process object" << std::endl;
    return 1;
  }
  process_destruction_end_time = Builder::get_time();

  std::cout << "Process destruction / entity deletion complete." << std::endl << std::endl;

  // Some preliminary measurements and reporting (eventually will shift to another process?)
  Stool::WorkerReport worker_report;
  worker_report.undermatched_readers = 0;
  worker_report.undermatched_writers = 0;
  worker_report.max_discovery_time_delta = ZERO;
  worker_report.sample_count = 0;
  worker_report.latency_min = std::numeric_limits<double>::max();
  worker_report.latency_max = 0.0;
  worker_report.latency_mean = 0.0;
  worker_report.latency_var_x_sample_count = 0.0;

  for (CORBA::ULong i = 0; i < process_report.participants.length(); ++i) {
    for (CORBA::ULong j = 0; j < process_report.participants[i].subscribers.length(); ++j) {
      for (CORBA::ULong k = 0; k < process_report.participants[i].subscribers[j].datareaders.length(); ++k) {
        Builder::DataReaderReport& dr_report = process_report.participants[i].subscribers[j].datareaders[k];
        const Builder::TimeStamp dr_enable_time = get_or_create_property(dr_report.properties, "enable_time", Builder::PVK_TIME)->value.time_prop();
        const Builder::TimeStamp dr_last_discovery_time = get_or_create_property(dr_report.properties, "last_discovery_time", Builder::PVK_TIME)->value.time_prop();
        const CORBA::ULongLong dr_sample_count = get_or_create_property(dr_report.properties, "sample_count", Builder::PVK_ULL)->value.ull_prop();
        const double dr_latency_min = get_or_create_property(dr_report.properties, "latency_min", Builder::PVK_DOUBLE)->value.double_prop();
        const double dr_latency_max = get_or_create_property(dr_report.properties, "latency_max", Builder::PVK_DOUBLE)->value.double_prop();
        const double dr_latency_mean = get_or_create_property(dr_report.properties, "latency_mean", Builder::PVK_DOUBLE)->value.double_prop();
        const double dr_latency_var_x_sample_count = get_or_create_property(dr_report.properties, "latency_var_x_sample_count", Builder::PVK_DOUBLE)->value.double_prop();
        if (ZERO < dr_enable_time && ZERO < dr_last_discovery_time) {
          auto delta = dr_last_discovery_time - dr_enable_time;
          if (worker_report.max_discovery_time_delta < delta) {
            worker_report.max_discovery_time_delta = delta;
          }
        } else {
          ++worker_report.undermatched_readers;
        }
        if (dr_latency_min < worker_report.latency_min) {
          worker_report.latency_min = dr_latency_min;
        }
        if (worker_report.latency_max < dr_latency_max) {
          worker_report.latency_max = dr_latency_max;
        }
        if ((worker_report.sample_count + dr_sample_count) > 0) {
          worker_report.latency_mean = (worker_report.latency_mean * static_cast<double>(worker_report.sample_count) + dr_latency_mean * static_cast<double>(dr_sample_count)) / (worker_report.sample_count + dr_sample_count);
        }
        worker_report.latency_var_x_sample_count += dr_latency_var_x_sample_count;
        worker_report.sample_count += dr_sample_count;
      }
    }
    for (CORBA::ULong j = 0; j < process_report.participants[i].publishers.length(); ++j) {
      for (CORBA::ULong k = 0; k < process_report.participants[i].publishers[j].datawriters.length(); ++k) {
        Builder::DataWriterReport& dw_report = process_report.participants[i].publishers[j].datawriters[k];
        const Builder::TimeStamp dw_enable_time = get_or_create_property(dw_report.properties, "enable_time", Builder::PVK_TIME)->value.time_prop();
        const Builder::TimeStamp dw_last_discovery_time = get_or_create_property(dw_report.properties, "last_discovery_time", Builder::PVK_TIME)->value.time_prop();
        if (ZERO < dw_enable_time && ZERO < dw_last_discovery_time) {
          auto delta = dw_last_discovery_time - dw_enable_time;
          if (worker_report.max_discovery_time_delta < delta) {
            worker_report.max_discovery_time_delta = delta;
          }
        } else {
          ++worker_report.undermatched_writers;
        }
      }
    }
  }

  std::cout << "undermatched readers: " << worker_report.undermatched_readers << ", undermatched writers: " << worker_report.undermatched_writers << std::endl << std::endl;

  std::cout << "construction_time: " << process_construction_end_time - process_construction_begin_time << std::endl;
  std::cout << "enable_time: " << process_enable_end_time - process_enable_begin_time << std::endl;
  //std::cout << "start_time: " << process_start_end_time - process_start_begin_time << std::endl;
  //std::cout << "stop_time: " << process_stop_end_time - process_stop_begin_time << std::endl;
  std::cout << "destruction_time: " << process_destruction_end_time - process_destruction_begin_time << std::endl;
  std::cout << "max_discovery_time_delta: " << worker_report.max_discovery_time_delta << std::endl;

  if (worker_report.sample_count > 0) {
    std::cout << "--- Latency Statistics ---" << std::endl;
    std::cout << "total sample count: " << worker_report.sample_count << std::endl;
    std::cout << "minimum latency: " << worker_report.latency_min << " seconds" << std::endl;
    std::cout << "maximum latency: " << worker_report.latency_max << " seconds" << std::endl;
    std::cout << "mean latency: " << worker_report.latency_mean << " seconds" << std::endl;
    std::cout << "latency standard deviation: " << std::sqrt(worker_report.latency_var_x_sample_count / static_cast<double>(worker_report.sample_count)) << " seconds" << std::endl;
  }

  return 0;
}

