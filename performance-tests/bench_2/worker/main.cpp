#include "dds/DCPS/Service_Participant.h"

#include <ace/Proactor.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/tcp/Tcp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#endif

#include "BenchC.h"
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
#include "BenchTypeSupportImpl.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include "BuilderTypeSupportImpl.h"

#include "ListenerFactory.h"

#include "TopicListener.h"
#include "DataReaderListener.h"
#include "DataWriterListener.h"
#include "SubscriberListener.h"
#include "PublisherListener.h"
#include "ParticipantListener.h"
#include "BuilderProcess.h"

#include "Utils.h"

#include "ActionManager.h"
#include "ForwardAction.h"
#include "PropertyStatBlock.h"
#include "WorkerDataReaderListener.h"
#include "WorkerDataWriterListener.h"
#include "WorkerTopicListener.h"
#include "WorkerSubscriberListener.h"
#include "WorkerPublisherListener.h"
#include "WorkerParticipantListener.h"
#include "WriteAction.h"
#include "DataReader.h"
#include "DataWriter.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <iomanip>

#include <util.h>
#include <json_conversion.h>

using Builder::Log;
using Builder::ZERO;
using Bench::get_option_argument;

double weighted_median(std::vector<double> medians, std::vector<size_t> weights, double default_value) {
  typedef std::multiset<std::pair<double, size_t> > WMMS;
  WMMS wmms;
  size_t total_weight = 0;
  assert(medians.size() == weights.size());
  for (size_t i = 0; i < medians.size(); ++i) {
    wmms.insert(WMMS::value_type(medians[i], weights[i]));
    total_weight += weights[i];
  }
  size_t mid_weight = total_weight / 2;
  for (auto it = wmms.begin(); it != wmms.end(); ++it) {
    if (mid_weight > it->second) {
      mid_weight -= it->second;
    } else {
      return it->first;
    }
  }
  return default_value;
}

void do_wait(const Builder::TimeStamp& ts, const std::string& ts_name, bool zero_equals_key_press = true) {
  if (zero_equals_key_press && ts == ZERO) {
    std::stringstream ss;
    ss << "No " << ts_name << " time specified. Press enter to continue." << std::endl;
    std::cerr << ss.str() << std::flush;
    std::string line;
    std::getline(std::cin, line);
  } else {
    if (ts < ZERO) {
      auto duration = -1 * get_duration(ts);
      if (duration > std::chrono::milliseconds(100)) {
        std::this_thread::sleep_until(std::chrono::steady_clock::now() + duration);
      }
    } else {
      auto now = std::chrono::system_clock::now();
      auto duration = std::chrono::system_clock::time_point(get_duration(ts)) - now;
      if (duration > std::chrono::milliseconds(100)) {
        std::this_thread::sleep_until(std::chrono::steady_clock::now() + duration);
      }
    }
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[]) {
  Builder::NullStream null_stream_i;
  std::ostream null_stream(&null_stream_i);

  std::string log_file_path;
  std::string report_file_path;
  std::string config_file_path;

  try {
    for (int i = 1; i < argc; i++) {
      const ACE_TCHAR* argument = argv[i];
      if (!ACE_OS::strcmp(argv[i], ACE_TEXT("--log"))) {
        log_file_path = get_option_argument(i, argc, argv);
      } else if (!ACE_OS::strcmp(argv[i], ACE_TEXT("--report"))) {
        report_file_path = get_option_argument(i, argc, argv);
      } else if (config_file_path.empty()) {
        config_file_path = ACE_TEXT_ALWAYS_CHAR(argument);
      } else {
        std::cerr << "Invalid option: " << argument << std::endl;
        return 1;
      }
    }

    if (config_file_path.empty()) {
      std::cerr << "Must pass a configuration file" << std::endl;
      throw 1;
    }
  } catch(int value) {
    std::cerr << "See DDS_ROOT/performance-tests/bench_2/README.md for usage" << std::endl;
    return value;
  }

  std::ifstream config_file(config_file_path);
  if (!config_file.is_open()) {
    std::cerr << "Unable to open configuration file: '" << config_file_path << "'" << std::endl;
    return 2;
  }

  std::ofstream log_file;
  if (!log_file_path.empty()) {
    log_file.open(log_file_path);
    if (!log_file.good()) {
      std::cerr << "Unable to open log file: '" << log_file_path << "'" << std::endl;
      return 2;
    }
    Log::stream = &log_file;
  } else {
    Log::stream = &std::cout;
  }

  std::ofstream report_file;
  if (!report_file_path.empty()) {
    report_file.open(report_file_path);
    if (!report_file.good()) {
      std::cerr << "Unable to open report file: '" << report_file_path << "'" << std::endl;
      return 2;
    }
  }

  using Builder::ZERO;

  Bench::WorkerConfig config{};

  config.create_time = ZERO;
  config.enable_time = ZERO;
  config.start_time = ZERO;
  config.stop_time = ZERO;
  config.wait_for_discovery = false;
  config.wait_for_discovery_seconds = 0;

  if (!json_2_idl(config_file, config)) {
    std::cerr << "Unable to parse configuration" << std::endl;
    return 3;
  }

  // Register some Bench-specific types
  Builder::TypeSupportRegistry::TypeSupportRegistration
    process_config_registration(new Builder::ProcessConfigTypeSupportImpl());
  Builder::TypeSupportRegistry::TypeSupportRegistration
    data_registration(new Bench::DataTypeSupportImpl());

  // Register some Bench-specific listener factories
  Builder::register_topic_listener("bench_tl", [](const Builder::PropertySeq& properties){
      return DDS::TopicListener_var(new Bench::WorkerTopicListener(properties));
    });
  Builder::register_data_reader_listener("bench_drl", [](const Builder::PropertySeq& properties){
      return DDS::DataReaderListener_var(new Bench::WorkerDataReaderListener(properties));
    });
  Builder::register_subscriber_listener("bench_sl", [](const Builder::PropertySeq& properties){
      return DDS::SubscriberListener_var(new Bench::WorkerSubscriberListener(properties));
    });
  Builder::register_data_writer_listener("bench_dwl", [](const Builder::PropertySeq& properties){
      return DDS::DataWriterListener_var(new Bench::WorkerDataWriterListener(properties));
    });
  Builder::register_publisher_listener("bench_pl", [](const Builder::PropertySeq& properties){
      return DDS::PublisherListener_var(new Bench::WorkerPublisherListener(properties));
    });
  Builder::register_domain_participant_listener("bench_partl", [](const Builder::PropertySeq& properties){
      return DDS::DomainParticipantListener_var(new Bench::WorkerParticipantListener(properties));
    });

  // Disable some Proactor debug chatter to stdout (eventually make this configurable?)
  ACE_Log_Category::ace_lib().priority_mask(0);

  ACE_Proactor proactor;

  // Register actions
  Bench::ActionManager::Registration
    write_action_registration("write", [&](){
      return std::shared_ptr<Bench::Action>(new Bench::WriteAction(proactor));
    });
  Bench::ActionManager::Registration
    forward_action_registration("forward", [&](){
      return std::shared_ptr<Bench::Action>(new Bench::ForwardAction(proactor));
    });

  // Timestamps used to measure method call durations
  Builder::TimeStamp process_construction_begin_time = ZERO, process_construction_end_time = ZERO;
  Builder::TimeStamp process_enable_begin_time = ZERO, process_enable_end_time = ZERO;
  Builder::TimeStamp process_start_begin_time = ZERO, process_start_end_time = ZERO;
  Builder::TimeStamp process_stop_begin_time = ZERO, process_stop_end_time = ZERO;
  Builder::TimeStamp process_destruction_begin_time = ZERO, process_destruction_end_time = ZERO;
  Builder::TimeStamp process_start_discovery_time = ZERO, process_stop_discovery_time = ZERO;

  set_global_properties(config.properties);

  Bench::WorkerReport worker_report{};
  Builder::ProcessReport& process_report = worker_report.process_report;

  const size_t THREAD_POOL_SIZE = 4;
  std::vector<std::shared_ptr<std::thread> > thread_pool;
  for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
    thread_pool.emplace_back(std::make_shared<std::thread>([&](){ proactor.proactor_run_event_loop(); }));
  }

  try {
    std::string line;

    do_wait(config.create_time, "create", false);

    Log::log() << "Beginning process construction / entity creation." << std::endl;

    process_construction_begin_time = Builder::get_hr_time();
    Builder::BuilderProcess process(config.process);
    process_construction_end_time = Builder::get_hr_time();

    Log::log() << std::endl << "Process construction / entity creation complete." << std::endl << std::endl;

    Log::log() << "Beginning action construction / initialization." << std::endl;

    Bench::ActionManager am(config.actions, config.action_reports, process.get_reader_map(), process.get_writer_map());

    Log::log() << "Action construction / initialization complete." << std::endl << std::endl;

    do_wait(config.enable_time, "enable");

    Log::log() << "Enabling DDS entities (if not already enabled)." << std::endl;

    process_enable_begin_time = Builder::get_hr_time();
    process.enable_dds_entities();
    process_enable_end_time = Builder::get_hr_time();

    Log::log() << "DDS entities enabled." << std::endl << std::endl;

    if (config.wait_for_discovery) {

      Log::log() << "Starting Discovery Check." << std::endl;

      process_start_discovery_time = Builder::get_sys_time();

      if (config.wait_for_discovery_seconds > 0) {

        const std::chrono::seconds timeoutPeriod(config.wait_for_discovery_seconds);
        const std::chrono::system_clock::time_point timeout_time = std::chrono::system_clock::now() + timeoutPeriod;

        auto readMap = process.get_reader_map();
        if (readMap.size() > 0) {
          typedef std::map<std::string, std::shared_ptr<Builder::DataReader>>::iterator ReadMapIt;
          std::shared_ptr<Builder::DataReader> dtRdrPtr(nullptr);

          for (ReadMapIt it = readMap.begin(); it != readMap.end(); ++it) {
            dtRdrPtr = it->second;
            Bench::WorkerDataReaderListener* wdrl = dynamic_cast<Bench::WorkerDataReaderListener*>(dtRdrPtr->get_dds_datareaderlistener().in());

            if (wdrl->wait_for_expected_match(timeout_time)) {
              Log::log() << it->first << "Found expected writers." << std::endl << std::endl;
            }
            else {
              Log::log() << "Error: " << it->first << " Expected writers not found." << std::endl << std::endl;
            }
          }
        }

        auto writeMap = process.get_writer_map();
        if (writeMap.size() > 0) {
          typedef std::map<std::string, std::shared_ptr<Builder::DataWriter>>::iterator WriteMapIt;
          std::shared_ptr<Builder::DataWriter> dtWtrPtr(nullptr);

          for (WriteMapIt it = writeMap.begin(); it != writeMap.end(); ++it) {
            dtWtrPtr = it->second;
            Bench::WorkerDataWriterListener* wdwl = dynamic_cast<Bench::WorkerDataWriterListener*>(dtWtrPtr->get_dds_datawriterlistener().in());

            if (wdwl->wait_for_expected_match(timeout_time)) {
              Log::log() << it->first << "Found expected writers." << std::endl << std::endl;
            }
            else {
              Log::log() << "Error: " << it->first << " Expected writers not found." << std::endl << std::endl;
            }
          }
        }
      }

      process_stop_discovery_time = Builder::get_sys_time();

      Log::log() << "Discovery of expected entities took " << process_stop_discovery_time - process_start_discovery_time << " seconds." << std::endl << std::endl;
    }

    do_wait(config.start_time, "start");

    Log::log() << "Starting process tests." << std::endl;

    process_start_begin_time = Builder::get_hr_time();
    am.start();
    process_start_end_time = Builder::get_hr_time();

    Log::log() << "Process tests started." << std::endl << std::endl;

    do_wait(config.stop_time, "stop");

    Log::log() << "Stopping process tests." << std::endl;

    process_stop_begin_time = Builder::get_hr_time();
    am.stop();
    process_stop_end_time = Builder::get_hr_time();

    Log::log() << "Process tests stopped." << std::endl << std::endl;

    proactor.proactor_end_event_loop();
    for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
      thread_pool[i]->join();
    }
    thread_pool.clear();

    do_wait(config.destruction_time, "destruction");

    process.detach_listeners();

    process_report = process.get_report();

    Log::log() << "Beginning process destruction / entity deletion." << std::endl;

    process_destruction_begin_time = Builder::get_hr_time();
  } catch (const std::exception& e) {
    std::cerr << "Exception caught trying to build process object: " << e.what() << std::endl;
    proactor.proactor_end_event_loop();
    for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
      thread_pool[i]->join();
    }
    thread_pool.clear();
    TheServiceParticipant->shutdown();
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception caught trying to build process object" << std::endl;
    proactor.proactor_end_event_loop();
    for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
      thread_pool[i]->join();
    }
    thread_pool.clear();
    TheServiceParticipant->shutdown();
    return 1;
  }
  process_destruction_end_time = Builder::get_hr_time();

  Log::log() << "Process destruction / entity deletion complete." << std::endl << std::endl;

  // Some preliminary measurements and reporting (eventually will shift to another process?)
  worker_report.construction_time = process_construction_end_time - process_construction_begin_time;
  worker_report.enable_time = process_enable_end_time - process_enable_begin_time;
  worker_report.start_time = process_start_end_time - process_start_begin_time;
  worker_report.stop_time = process_stop_end_time - process_stop_begin_time;
  worker_report.destruction_time = process_destruction_end_time - process_destruction_begin_time;
  worker_report.undermatched_readers = 0;
  worker_report.undermatched_writers = 0;
  worker_report.max_discovery_time_delta = ZERO;

  worker_report.latency_sample_count = 0;
  worker_report.latency_min = std::numeric_limits<double>::max();
  worker_report.latency_max = std::numeric_limits<double>::min();
  worker_report.latency_mean = 0.0;
  worker_report.latency_var_x_sample_count = 0.0;
  worker_report.latency_stdev = 0.0;
  worker_report.latency_weighted_median = 0.0;
  worker_report.latency_weighted_median_overflow = 0;

  worker_report.jitter_sample_count = 0;
  worker_report.jitter_min = std::numeric_limits<double>::max();
  worker_report.jitter_max = std::numeric_limits<double>::min();
  worker_report.jitter_mean = 0.0;
  worker_report.jitter_var_x_sample_count = 0.0;
  worker_report.jitter_stdev = 0.0;
  worker_report.jitter_weighted_median = 0.0;
  worker_report.jitter_weighted_median_overflow = 0;

  worker_report.round_trip_latency_sample_count = 0;
  worker_report.round_trip_latency_min = std::numeric_limits<double>::max();
  worker_report.round_trip_latency_max = std::numeric_limits<double>::min();
  worker_report.round_trip_latency_mean = 0.0;
  worker_report.round_trip_latency_var_x_sample_count = 0.0;
  worker_report.round_trip_latency_stdev = 0.0;
  worker_report.round_trip_latency_weighted_median = 0.0;
  worker_report.round_trip_latency_weighted_median_overflow = 0;

  worker_report.round_trip_jitter_sample_count = 0;
  worker_report.round_trip_jitter_min = std::numeric_limits<double>::max();
  worker_report.round_trip_jitter_max = std::numeric_limits<double>::min();
  worker_report.round_trip_jitter_mean = 0.0;
  worker_report.round_trip_jitter_var_x_sample_count = 0.0;
  worker_report.round_trip_jitter_stdev = 0.0;
  worker_report.round_trip_jitter_weighted_median = 0.0;
  worker_report.round_trip_jitter_weighted_median_overflow = 0;

  std::vector<double> latency_medians;
  std::vector<size_t> latency_median_counts;

  std::vector<double> jitter_medians;
  std::vector<size_t> jitter_median_counts;

  std::vector<double> round_trip_latency_medians;
  std::vector<size_t> round_trip_latency_median_counts;

  std::vector<double> round_trip_jitter_medians;
  std::vector<size_t> round_trip_jitter_median_counts;

  Bench::SimpleStatBlock consolidated_latency_stats;
  Bench::SimpleStatBlock consolidated_jitter_stats;
  Bench::SimpleStatBlock consolidated_round_trip_latency_stats;
  Bench::SimpleStatBlock consolidated_round_trip_jitter_stats;

  for (CORBA::ULong i = 0; i < process_report.participants.length(); ++i) {
    for (CORBA::ULong j = 0; j < process_report.participants[i].subscribers.length(); ++j) {
      for (CORBA::ULong k = 0; k < process_report.participants[i].subscribers[j].datareaders.length(); ++k) {
        Builder::DataReaderReport& dr_report = process_report.participants[i].subscribers[j].datareaders[k];

        const Builder::TimeStamp dr_enable_time =
          get_or_create_property(dr_report.properties, "enable_time", Builder::PVK_TIME)->value.time_prop();
        const Builder::TimeStamp dr_last_discovery_time =
          get_or_create_property(dr_report.properties, "last_discovery_time", Builder::PVK_TIME)->value.time_prop();

        if (ZERO < dr_enable_time && ZERO < dr_last_discovery_time) {
          auto delta = dr_last_discovery_time - dr_enable_time;
          if (worker_report.max_discovery_time_delta < delta) {
            worker_report.max_discovery_time_delta = delta;
          }
        } else {
          ++worker_report.undermatched_readers;
        }

        Bench::ConstPropertyStatBlock dr_latency(dr_report.properties, "latency");
        Bench::ConstPropertyStatBlock dr_jitter(dr_report.properties, "jitter");
        Bench::ConstPropertyStatBlock dr_round_trip_latency(dr_report.properties, "round_trip_latency");
        Bench::ConstPropertyStatBlock dr_round_trip_jitter(dr_report.properties, "round_trip_jitter");

        consolidated_latency_stats = consolidate(consolidated_latency_stats, dr_latency.to_simple_stat_block());
        consolidated_jitter_stats = consolidate(consolidated_jitter_stats, dr_jitter.to_simple_stat_block());
        consolidated_round_trip_latency_stats = consolidate(consolidated_round_trip_latency_stats, dr_round_trip_latency.to_simple_stat_block());
        consolidated_round_trip_jitter_stats = consolidate(consolidated_round_trip_jitter_stats, dr_round_trip_jitter.to_simple_stat_block());
      }
    }

    for (CORBA::ULong j = 0; j < process_report.participants[i].publishers.length(); ++j) {
      for (CORBA::ULong k = 0; k < process_report.participants[i].publishers[j].datawriters.length(); ++k) {
        Builder::DataWriterReport& dw_report = process_report.participants[i].publishers[j].datawriters[k];

        const Builder::TimeStamp dw_enable_time =
          get_or_create_property(dw_report.properties, "enable_time", Builder::PVK_TIME)->value.time_prop();
        const Builder::TimeStamp dw_last_discovery_time =
          get_or_create_property(dw_report.properties, "last_discovery_time", Builder::PVK_TIME)->value.time_prop();

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

  worker_report.latency_sample_count = consolidated_latency_stats.sample_count_;
  worker_report.latency_min = consolidated_latency_stats.min_;
  worker_report.latency_max = consolidated_latency_stats.max_;
  worker_report.latency_mean = consolidated_latency_stats.mean_;
  worker_report.latency_var_x_sample_count = consolidated_latency_stats.var_x_sample_count_;
  worker_report.latency_stdev = 0.0;
  worker_report.latency_weighted_median = consolidated_latency_stats.median_;
  worker_report.latency_weighted_median_overflow = consolidated_latency_stats.median_sample_overflow_;

  if (worker_report.latency_sample_count) {
    worker_report.latency_stdev =
      std::sqrt(worker_report.latency_var_x_sample_count / static_cast<double>(worker_report.latency_sample_count));
  }
  worker_report.latency_weighted_median = weighted_median(latency_medians, latency_median_counts, 0.0);

  worker_report.jitter_sample_count = consolidated_jitter_stats.sample_count_;
  worker_report.jitter_min = consolidated_jitter_stats.min_;
  worker_report.jitter_max = consolidated_jitter_stats.max_;
  worker_report.jitter_mean = consolidated_jitter_stats.mean_;
  worker_report.jitter_var_x_sample_count = consolidated_jitter_stats.var_x_sample_count_;
  worker_report.jitter_stdev = 0.0;
  worker_report.jitter_weighted_median = consolidated_jitter_stats.median_;
  worker_report.jitter_weighted_median_overflow = consolidated_jitter_stats.median_sample_overflow_;

  if (worker_report.jitter_sample_count) {
    worker_report.jitter_stdev =
      std::sqrt(worker_report.jitter_var_x_sample_count / static_cast<double>(worker_report.jitter_sample_count));
  }
  worker_report.jitter_weighted_median = weighted_median(jitter_medians, jitter_median_counts, 0.0);

  worker_report.round_trip_latency_sample_count = consolidated_round_trip_latency_stats.sample_count_;
  worker_report.round_trip_latency_min = consolidated_round_trip_latency_stats.min_;
  worker_report.round_trip_latency_max = consolidated_round_trip_latency_stats.max_;
  worker_report.round_trip_latency_mean = consolidated_round_trip_latency_stats.mean_;
  worker_report.round_trip_latency_var_x_sample_count = consolidated_round_trip_latency_stats.var_x_sample_count_;
  worker_report.round_trip_latency_stdev = 0.0;
  worker_report.round_trip_latency_weighted_median = consolidated_round_trip_latency_stats.median_;
  worker_report.round_trip_latency_weighted_median_overflow = consolidated_round_trip_latency_stats.median_sample_overflow_;

  if (worker_report.round_trip_latency_sample_count) {
    worker_report.round_trip_latency_stdev =
      std::sqrt(worker_report.round_trip_latency_var_x_sample_count /
        static_cast<double>(worker_report.round_trip_latency_sample_count));
  }
  worker_report.round_trip_latency_weighted_median =
    weighted_median(round_trip_latency_medians, round_trip_latency_median_counts, 0.0);

  worker_report.round_trip_jitter_sample_count = consolidated_round_trip_jitter_stats.sample_count_;
  worker_report.round_trip_jitter_min = consolidated_round_trip_jitter_stats.min_;
  worker_report.round_trip_jitter_max = consolidated_round_trip_jitter_stats.max_;
  worker_report.round_trip_jitter_mean = consolidated_round_trip_jitter_stats.mean_;
  worker_report.round_trip_jitter_var_x_sample_count = consolidated_round_trip_jitter_stats.var_x_sample_count_;
  worker_report.round_trip_jitter_stdev = 0.0;
  worker_report.round_trip_jitter_weighted_median = consolidated_round_trip_jitter_stats.median_;
  worker_report.round_trip_jitter_weighted_median_overflow = consolidated_round_trip_jitter_stats.median_sample_overflow_;

  if (worker_report.round_trip_jitter_sample_count) {
    worker_report.round_trip_jitter_stdev =
      std::sqrt(worker_report.round_trip_jitter_var_x_sample_count /
        static_cast<double>(worker_report.round_trip_jitter_sample_count));
  }
  worker_report.round_trip_jitter_weighted_median =
    weighted_median(round_trip_jitter_medians, round_trip_jitter_median_counts, 0.0);

  // If requested, write out worker report to file

  if (!report_file_path.empty()) {
    idl_2_json(worker_report, report_file);
  }

  // Log / print a few of the stats

  Log::log() << std::endl << "--- Process Statistics ---" << std::endl << std::endl;

  Log::log() << "construction time: " << worker_report.construction_time << std::endl;
  Log::log() << "enable time: " << worker_report.enable_time << std::endl;
  Log::log() << "start time: " << worker_report.start_time << std::endl;
  Log::log() << "stop time: " << worker_report.stop_time << std::endl;
  Log::log() << "destruction time: " << worker_report.destruction_time << std::endl;

  Log::log() << std::endl << "--- Discovery Statistics ---" << std::endl << std::endl;

  Log::log() << "undermatched readers: " << worker_report.undermatched_readers << std::endl;
  Log::log() << "undermatched writers: " << worker_report.undermatched_writers << std::endl << std::endl;
  Log::log() << "max discovery time delta: " << worker_report.max_discovery_time_delta << std::endl;

  if (worker_report.latency_sample_count > 0) {
    Log::log() << std::endl << "--- Latency Statistics ---" << std::endl << std::endl;

    Log::log() << "total (latency) sample count: " << worker_report.latency_sample_count << std::endl;
    Log::log() << "minimum latency: " << std::fixed << std::setprecision(6) << worker_report.latency_min << " seconds" << std::endl;
    Log::log() << "maximum latency: " << std::fixed << std::setprecision(6) << worker_report.latency_max << " seconds" << std::endl;
    Log::log() << "mean latency: " << std::fixed << std::setprecision(6) << worker_report.latency_mean << " seconds" << std::endl;
    Log::log() << "latency standard deviation: " << std::fixed << std::setprecision(6) << worker_report.latency_stdev << " seconds" << std::endl;
    Log::log() << "latency weighted median: " << std::fixed << std::setprecision(6) << worker_report.latency_weighted_median << " seconds" << std::endl;
    Log::log() << "latency weighted median overflow: " << worker_report.latency_weighted_median_overflow << std::endl;
  }

  if (worker_report.jitter_sample_count > 0) {
    Log::log() << std::endl << "--- Jitter Statistics ---" << std::endl << std::endl;

    Log::log() << "total (jitter) sample count: " << worker_report.jitter_sample_count << std::endl;
    Log::log() << "minimum jitter: " << std::fixed << std::setprecision(6) << worker_report.jitter_min << " seconds" << std::endl;
    Log::log() << "maximum jitter: " << std::fixed << std::setprecision(6) << worker_report.jitter_max << " seconds" << std::endl;
    Log::log() << "mean jitter: " << std::fixed << std::setprecision(6) << worker_report.jitter_mean << " seconds" << std::endl;
    Log::log() << "jitter standard deviation: " << std::fixed << std::setprecision(6) << worker_report.jitter_stdev << " seconds" << std::endl;
    Log::log() << "jitter weighted median: " << std::fixed << std::setprecision(6) << worker_report.jitter_weighted_median << " seconds" << std::endl;
    Log::log() << "jitter weighted median overflow: " << worker_report.jitter_weighted_median_overflow << std::endl;
    Log::log() << std::endl;
  }

  if (worker_report.round_trip_latency_sample_count > 0) {
    Log::log() << std::endl << "--- Round-Trip Latency Statistics ---" << std::endl << std::endl;

    Log::log() << "total (round_trip_latency) sample count: " << worker_report.round_trip_latency_sample_count << std::endl;
    Log::log() << "minimum round_trip_latency: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_min << " seconds" << std::endl;
    Log::log() << "maximum round_trip_latency: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_max << " seconds" << std::endl;
    Log::log() << "mean round_trip_latency: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_mean << " seconds" << std::endl;
    Log::log() << "round_trip_latency standard deviation: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_stdev << " seconds" << std::endl;
    Log::log() << "round_trip_latency weighted median: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_weighted_median << " seconds" << std::endl;
    Log::log() << "round_trip_latency weighted median overflow: " << worker_report.round_trip_latency_weighted_median_overflow << std::endl;
  }

  if (worker_report.round_trip_jitter_sample_count > 0) {
    Log::log() << std::endl << "--- Round-Trip Jitter Statistics ---" << std::endl << std::endl;

    Log::log() << "total (round_trip_jitter) sample count: " << worker_report.round_trip_jitter_sample_count << std::endl;
    Log::log() << "minimum round_trip_jitter: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_min << " seconds" << std::endl;
    Log::log() << "maximum round_trip_jitter: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_max << " seconds" << std::endl;
    Log::log() << "mean round_trip_jitter: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_mean << " seconds" << std::endl;
    Log::log() << "round_trip_jitter standard deviation: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_stdev << " seconds" << std::endl;
    Log::log() << "round_trip_jitter weighted median: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_weighted_median << " seconds" << std::endl;
    Log::log() << "round_trip_jitter weighted median overflow: " << worker_report.round_trip_jitter_weighted_median_overflow << std::endl;
    Log::log() << std::endl;
  }

  return 0;
}

