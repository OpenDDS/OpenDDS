#include "dds/DCPS/Service_Participant.h"

#include <ace/Proactor.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include "BenchC.h"
#include "BenchTypeSupportImpl.h"
#include "BuilderTypeSupportImpl.h"

#include "ListenerFactory.h"

#include "TopicListener.h"
#include "DataReaderListener.h"
#include "DataWriterListener.h"
#include "SubscriberListener.h"
#include "PublisherListener.h"
#include "ParticipantListener.h"
#include "Process.h"

#include "json_2_builder.h"

#include "ActionManager.h"
#include "ForwardAction.h"
#include "WorkerDataReaderListener.h"
#include "WorkerDataWriterListener.h"
#include "WorkerTopicListener.h"
#include "WorkerSubscriberListener.h"
#include "WorkerPublisherListener.h"
#include "WorkerParticipantListener.h"
#include "WriteAction.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <iomanip>
#include <condition_variable>

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

  Bench::WorkerConfig config;

  config.enable_time = ZERO;
  config.start_time = ZERO;
  config.stop_time = ZERO;

  if (!json_2_builder(ifs, config)) {
    std::cerr << "Unable to parse configuration file: '" << argv[1] << "'" << std::endl;
    return 3;
  }

  // Register some Bench-specific types
  Builder::TypeSupportRegistry::TypeSupportRegistration
    process_config_registration(new Builder::ProcessConfigTypeSupportImpl());
  Builder::TypeSupportRegistry::TypeSupportRegistration
    data_registration(new Bench::DataTypeSupportImpl());

  // Register some Bench-specific listener factories
  Builder::ListenerFactory<DDS::TopicListener>::Registration
    topic_registration("bench_tl", [](){
      return DDS::TopicListener_var(new Bench::WorkerTopicListener());
    });
  Builder::ListenerFactory<DDS::DataReaderListener>::Registration
    datareader_registration("bench_drl", [](){
      return DDS::DataReaderListener_var(new Bench::WorkerDataReaderListener());
    });
  Builder::ListenerFactory<DDS::SubscriberListener>::Registration
    subscriber_registration("bench_sl", [](){
      return DDS::SubscriberListener_var(new Bench::WorkerSubscriberListener());
    });
  Builder::ListenerFactory<DDS::DataWriterListener>::Registration
    datawriter_registration("bench_dwl", [](){
      return DDS::DataWriterListener_var(new Bench::WorkerDataWriterListener());
    });
  Builder::ListenerFactory<DDS::PublisherListener>::Registration
    publisher_registration("bench_pl", [](){
      return DDS::PublisherListener_var(new Bench::WorkerPublisherListener());
    });
  Builder::ListenerFactory<DDS::DomainParticipantListener>::Registration
    participant_registration("bench_partl", [](){
      return DDS::DomainParticipantListener_var(new Bench::WorkerParticipantListener());
    });

  // Disable some Proactor debug chatter to std out (eventually make this configurable?)
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

  Builder::ProcessReport process_report;

  const size_t THREAD_POOL_SIZE = 4;
  std::vector<std::shared_ptr<std::thread> > thread_pool;
  for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
    thread_pool.emplace_back(std::make_shared<std::thread>([&](){ proactor.proactor_run_event_loop(); }));
  }

  try {
    std::string line;
    std::condition_variable cv;
    std::mutex cv_mutex;

    std::cout << "Beginning process construction / entity creation." << std::endl;

    process_construction_begin_time = Builder::get_time();
    Builder::Process process(config.process);
    process_construction_end_time = Builder::get_time();

    std::cout << std::endl << "Process construction / entity creation complete." << std::endl << std::endl;

    std::cout << "Beginning action construction / initialization." << std::endl;

    Bench::ActionManager am(config.actions, config.action_reports, process.get_reader_map(), process.get_writer_map());

    std::cout << "Action construction / initialization complete." << std::endl << std::endl;

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
    am.start();
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
    am.stop();
    process_stop_end_time = Builder::get_time();

    std::cout << "Process tests stopped." << std::endl << std::endl;

    proactor.proactor_end_event_loop();
    for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
      thread_pool[i]->join();
    }
    thread_pool.clear();

    process.detach_listeners();

    process_report = process.get_report();

    std::cout << "Beginning process destruction / entity deletion." << std::endl;

    process_destruction_begin_time = Builder::get_time();
  } catch (const std::exception& e) {
    std::cerr << "Exception caught trying to build process object: " << e.what() << std::endl;
    TheServiceParticipant->shutdown();
    proactor.proactor_end_event_loop();
    for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
      thread_pool[i]->join();
    }
    thread_pool.clear();
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception caught trying to build process object" << std::endl;
    TheServiceParticipant->shutdown();
    proactor.proactor_end_event_loop();
    for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
      thread_pool[i]->join();
    }
    thread_pool.clear();
    return 1;
  }
  process_destruction_end_time = Builder::get_time();

  std::cout << "Process destruction / entity deletion complete." << std::endl << std::endl;

  // Some preliminary measurements and reporting (eventually will shift to another process?)
  Bench::WorkerReport worker_report;
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

  for (CORBA::ULong i = 0; i < process_report.participants.length(); ++i) {
    for (CORBA::ULong j = 0; j < process_report.participants[i].subscribers.length(); ++j) {
      for (CORBA::ULong k = 0; k < process_report.participants[i].subscribers[j].datareaders.length(); ++k) {
        Builder::DataReaderReport& dr_report = process_report.participants[i].subscribers[j].datareaders[k];
        const Builder::TimeStamp dr_enable_time =
          get_or_create_property(dr_report.properties, "enable_time", Builder::PVK_TIME)->value.time_prop();
        const Builder::TimeStamp dr_last_discovery_time =
          get_or_create_property(dr_report.properties, "last_discovery_time", Builder::PVK_TIME)->value.time_prop();

        // Normal Latency
        const CORBA::ULongLong dr_latency_sample_count =
          get_or_create_property(dr_report.properties, "latency_sample_count", Builder::PVK_ULL)->value.ull_prop();
        const double dr_latency_min =
          get_or_create_property(dr_report.properties, "latency_min", Builder::PVK_DOUBLE)->value.double_prop();
        const double dr_latency_max =
          get_or_create_property(dr_report.properties, "latency_max", Builder::PVK_DOUBLE)->value.double_prop();
        const double dr_latency_mean =
          get_or_create_property(dr_report.properties, "latency_mean", Builder::PVK_DOUBLE)->value.double_prop();
        const double dr_latency_var_x_sample_count =
          get_or_create_property(dr_report.properties, "latency_var_x_sample_count", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_latency_median =
          get_or_create_property(dr_report.properties, "latency_median", Builder::PVK_DOUBLE)->value.double_prop();
        const CORBA::ULongLong dr_latency_median_sample_count =
          get_or_create_property(dr_report.properties, "latency_median_sample_count", Builder::PVK_ULL)->
            value.ull_prop();

        // Normal Jitter
        const CORBA::ULongLong dr_jitter_sample_count =
          get_or_create_property(dr_report.properties, "jitter_sample_count", Builder::PVK_ULL)->
            value.ull_prop();
        const double dr_jitter_min =
          get_or_create_property(dr_report.properties, "jitter_min", Builder::PVK_DOUBLE)->
          value.double_prop();
        const double dr_jitter_max =
          get_or_create_property(dr_report.properties, "jitter_max", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_jitter_mean =
          get_or_create_property(dr_report.properties, "jitter_mean", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_jitter_var_x_sample_count =
          get_or_create_property(dr_report.properties, "jitter_var_x_sample_count", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_jitter_median =
          get_or_create_property(dr_report.properties, "jitter_median", Builder::PVK_DOUBLE)->
            value.double_prop();
        const CORBA::ULongLong dr_jitter_median_sample_count =
          get_or_create_property(dr_report.properties, "jitter_median_sample_count", Builder::PVK_ULL)->
          value.ull_prop();

        // Round-Trip Latency
        const CORBA::ULongLong dr_round_trip_latency_sample_count =
          get_or_create_property(dr_report.properties, "round_trip_latency_sample_count", Builder::PVK_ULL)->
            value.ull_prop();
        const double dr_round_trip_latency_min =
          get_or_create_property(dr_report.properties, "round_trip_latency_min", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_round_trip_latency_max =
          get_or_create_property(dr_report.properties, "round_trip_latency_max", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_round_trip_latency_mean =
          get_or_create_property(dr_report.properties, "round_trip_latency_mean", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_round_trip_latency_var_x_sample_count =
          get_or_create_property(dr_report.properties, "round_trip_latency_var_x_sample_count", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_round_trip_latency_median =
          get_or_create_property(dr_report.properties, "round_trip_latency_median", Builder::PVK_DOUBLE)->
            value.double_prop();
        const CORBA::ULongLong dr_round_trip_latency_median_sample_count =
          get_or_create_property(dr_report.properties, "round_trip_latency_median_sample_count", Builder::PVK_ULL)->
            value.ull_prop();

        // Round-Trip Jitter
        const CORBA::ULongLong dr_round_trip_jitter_sample_count =
          get_or_create_property(dr_report.properties, "round_trip_jitter_sample_count", Builder::PVK_ULL)->
            value.ull_prop();
        const double dr_round_trip_jitter_min =
          get_or_create_property(dr_report.properties, "round_trip_jitter_min", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_round_trip_jitter_max =
          get_or_create_property(dr_report.properties, "round_trip_jitter_max", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_round_trip_jitter_mean =
          get_or_create_property(dr_report.properties, "round_trip_jitter_mean", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_round_trip_jitter_var_x_sample_count =
          get_or_create_property(dr_report.properties, "round_trip_jitter_var_x_sample_count", Builder::PVK_DOUBLE)->
            value.double_prop();
        const double dr_round_trip_jitter_median =
          get_or_create_property(dr_report.properties, "round_trip_jitter_median", Builder::PVK_DOUBLE)->
            value.double_prop();
        const CORBA::ULongLong dr_round_trip_jitter_median_sample_count =
          get_or_create_property(dr_report.properties, "round_trip_jitter_median_sample_count", Builder::PVK_ULL)->
            value.ull_prop();

        if (ZERO < dr_enable_time && ZERO < dr_last_discovery_time) {
          auto delta = dr_last_discovery_time - dr_enable_time;
          if (worker_report.max_discovery_time_delta < delta) {
            worker_report.max_discovery_time_delta = delta;
          }
        } else {
          ++worker_report.undermatched_readers;
        }

        // Normal Latency
        if (dr_latency_min < worker_report.latency_min) {
          worker_report.latency_min = dr_latency_min;
        }
        if (worker_report.latency_max < dr_latency_max) {
          worker_report.latency_max = dr_latency_max;
        }
        if ((worker_report.latency_sample_count + dr_latency_sample_count) > 0) {
          worker_report.latency_mean =
            (worker_report.latency_mean * static_cast<double>(worker_report.latency_sample_count) +
              dr_latency_mean * static_cast<double>(dr_latency_sample_count)) /
            (worker_report.latency_sample_count + dr_latency_sample_count);
        }
        worker_report.latency_var_x_sample_count += dr_latency_var_x_sample_count;
        worker_report.latency_sample_count += dr_latency_sample_count;

        latency_medians.push_back(dr_latency_median);
        latency_median_counts.push_back(dr_latency_median_sample_count);
        if (dr_latency_median_sample_count < dr_latency_sample_count) {
          worker_report.latency_weighted_median_overflow += dr_latency_sample_count - dr_latency_median_sample_count;
        }

        // Normal Jitter
        if (dr_jitter_min < worker_report.jitter_min) {
          worker_report.jitter_min = dr_jitter_min;
        }
        if (worker_report.jitter_max < dr_jitter_max) {
          worker_report.jitter_max = dr_jitter_max;
        }
        if ((worker_report.jitter_sample_count + dr_jitter_sample_count) > 0) {
          worker_report.jitter_mean =
            (worker_report.jitter_mean * static_cast<double>(worker_report.jitter_sample_count) +
              dr_jitter_mean * static_cast<double>(dr_jitter_sample_count)) /
            (worker_report.jitter_sample_count + dr_jitter_sample_count);
        }
        worker_report.jitter_var_x_sample_count += dr_jitter_var_x_sample_count;
        worker_report.jitter_sample_count += dr_jitter_sample_count;

        jitter_medians.push_back(dr_jitter_median);
        jitter_median_counts.push_back(dr_jitter_median_sample_count);
        if (dr_jitter_median_sample_count < dr_jitter_sample_count) {
          worker_report.jitter_weighted_median_overflow += dr_jitter_sample_count - dr_jitter_median_sample_count;
        }

        // Round-Trip Latency
        if (dr_round_trip_latency_min < worker_report.round_trip_latency_min) {
          worker_report.round_trip_latency_min = dr_round_trip_latency_min;
        }
        if (worker_report.round_trip_latency_max < dr_round_trip_latency_max) {
          worker_report.round_trip_latency_max = dr_round_trip_latency_max;
        }
        if ((worker_report.round_trip_latency_sample_count + dr_round_trip_latency_sample_count) > 0) {
          worker_report.round_trip_latency_mean =
            (worker_report.round_trip_latency_mean * static_cast<double>(worker_report.round_trip_latency_sample_count) +
              dr_round_trip_latency_mean * static_cast<double>(dr_round_trip_latency_sample_count)) /
            (worker_report.round_trip_latency_sample_count + dr_round_trip_latency_sample_count);
        }
        worker_report.round_trip_latency_var_x_sample_count += dr_round_trip_latency_var_x_sample_count;
        worker_report.round_trip_latency_sample_count += dr_round_trip_latency_sample_count;

        round_trip_latency_medians.push_back(dr_round_trip_latency_median);
        round_trip_latency_median_counts.push_back(dr_round_trip_latency_median_sample_count);
        if (dr_round_trip_latency_median_sample_count < dr_round_trip_latency_sample_count) {
          worker_report.round_trip_latency_weighted_median_overflow +=
            dr_round_trip_latency_sample_count - dr_round_trip_latency_median_sample_count;
        }

        // Round-Trip Jitter
        if (dr_round_trip_jitter_min < worker_report.round_trip_jitter_min) {
          worker_report.round_trip_jitter_min = dr_round_trip_jitter_min;
        }
        if (worker_report.round_trip_jitter_max < dr_round_trip_jitter_max) {
          worker_report.round_trip_jitter_max = dr_round_trip_jitter_max;
        }
        if ((worker_report.round_trip_jitter_sample_count + dr_round_trip_jitter_sample_count) > 0) {
          worker_report.round_trip_jitter_mean =
            (worker_report.round_trip_jitter_mean * static_cast<double>(worker_report.round_trip_jitter_sample_count) +
              dr_round_trip_jitter_mean * static_cast<double>(dr_round_trip_jitter_sample_count)) /
            (worker_report.round_trip_jitter_sample_count + dr_round_trip_jitter_sample_count);
        }
        worker_report.round_trip_jitter_var_x_sample_count += dr_round_trip_jitter_var_x_sample_count;
        worker_report.round_trip_jitter_sample_count += dr_round_trip_jitter_sample_count;

        round_trip_jitter_medians.push_back(dr_round_trip_jitter_median);
        round_trip_jitter_median_counts.push_back(dr_round_trip_jitter_median_sample_count);
        if (dr_round_trip_jitter_median_sample_count < dr_round_trip_jitter_sample_count) {
          worker_report.round_trip_jitter_weighted_median_overflow +=
            dr_round_trip_jitter_sample_count - dr_round_trip_jitter_median_sample_count;
        }
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

  worker_report.latency_stdev =
    std::sqrt(worker_report.latency_var_x_sample_count / static_cast<double>(worker_report.latency_sample_count));
  worker_report.latency_weighted_median = weighted_median(latency_medians, latency_median_counts, 0.0);

  worker_report.jitter_stdev =
    std::sqrt(worker_report.jitter_var_x_sample_count / static_cast<double>(worker_report.jitter_sample_count));
  worker_report.jitter_weighted_median = weighted_median(jitter_medians, jitter_median_counts, 0.0);

  worker_report.round_trip_latency_stdev =
    std::sqrt(worker_report.round_trip_latency_var_x_sample_count /
      static_cast<double>(worker_report.round_trip_latency_sample_count));
  worker_report.round_trip_latency_weighted_median =
    weighted_median(round_trip_latency_medians, round_trip_latency_median_counts, 0.0);

  worker_report.round_trip_jitter_stdev =
    std::sqrt(worker_report.round_trip_jitter_var_x_sample_count /
      static_cast<double>(worker_report.round_trip_jitter_sample_count));
  worker_report.round_trip_jitter_weighted_median =
    weighted_median(round_trip_jitter_medians, round_trip_jitter_median_counts, 0.0);

  std::string output_file_name;
  std::unique_ptr<std::ofstream> ofs;
  if (argc > 2 && std::string(argv[2]) == "true") {
    std::stringstream ss;
    ss << "worker_" << getpid() << "_output.txt" << std::flush;
    output_file_name = ss.str();
    ofs.reset(new std::ofstream(output_file_name.c_str()));
  }

  std::ostream& os = output_file_name.empty() ? std::cout : *ofs;

  os << std::endl << "--- Process Statistics ---" << std::endl << std::endl;

  os << "construction time: " << process_construction_end_time - process_construction_begin_time << std::endl;
  os << "enable time: " << process_enable_end_time - process_enable_begin_time << std::endl;
  os << "start time: " << process_start_end_time - process_start_begin_time << std::endl;
  os << "stop time: " << process_stop_end_time - process_stop_begin_time << std::endl;
  os << "destruction time: " << process_destruction_end_time - process_destruction_begin_time << std::endl;

  os << std::endl << "--- Discovery Statistics ---" << std::endl << std::endl;

  os << "undermatched readers: " << worker_report.undermatched_readers << std::endl;
  os << "undermatched writers: " << worker_report.undermatched_writers << std::endl << std::endl;
  os << "max discovery time delta: " << worker_report.max_discovery_time_delta << std::endl;

  if (worker_report.latency_sample_count > 0) {
    os << std::endl << "--- Latency Statistics ---" << std::endl << std::endl;

    os << "total (latency) sample count: " << worker_report.latency_sample_count << std::endl;
    os << "minimum latency: " << std::fixed << std::setprecision(6) << worker_report.latency_min << " seconds" << std::endl;
    os << "maximum latency: " << std::fixed << std::setprecision(6) << worker_report.latency_max << " seconds" << std::endl;
    os << "mean latency: " << std::fixed << std::setprecision(6) << worker_report.latency_mean << " seconds" << std::endl;
    os << "latency standard deviation: " << std::fixed << std::setprecision(6) << worker_report.latency_stdev << " seconds" << std::endl;
    os << "latency weighted median: " << std::fixed << std::setprecision(6) << worker_report.latency_weighted_median << " seconds" << std::endl;
    os << "latency weighted median overflow: " << worker_report.latency_weighted_median_overflow << std::endl;
  }

  if (worker_report.jitter_sample_count > 0) {
    os << std::endl << "--- Jitter Statistics ---" << std::endl << std::endl;

    os << "total (jitter) sample count: " << worker_report.jitter_sample_count << std::endl;
    os << "minimum jitter: " << std::fixed << std::setprecision(6) << worker_report.jitter_min << " seconds" << std::endl;
    os << "maximum jitter: " << std::fixed << std::setprecision(6) << worker_report.jitter_max << " seconds" << std::endl;
    os << "mean jitter: " << std::fixed << std::setprecision(6) << worker_report.jitter_mean << " seconds" << std::endl;
    os << "jitter standard deviation: " << std::fixed << std::setprecision(6) << worker_report.jitter_stdev << " seconds" << std::endl;
    os << "jitter weighted median: " << std::fixed << std::setprecision(6) << worker_report.jitter_weighted_median << " seconds" << std::endl;
    os << "jitter weighted median overflow: " << worker_report.jitter_weighted_median_overflow << std::endl;
    os << std::endl;
  }

  if (worker_report.round_trip_latency_sample_count > 0) {
    os << std::endl << "--- Round-Trip Latency Statistics ---" << std::endl << std::endl;

    os << "total (round_trip_latency) sample count: " << worker_report.round_trip_latency_sample_count << std::endl;
    os << "minimum round_trip_latency: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_min << " seconds" << std::endl;
    os << "maximum round_trip_latency: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_max << " seconds" << std::endl;
    os << "mean round_trip_latency: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_mean << " seconds" << std::endl;
    os << "round_trip_latency standard deviation: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_stdev << " seconds" << std::endl;
    os << "round_trip_latency weighted median: " << std::fixed << std::setprecision(6) << worker_report.round_trip_latency_weighted_median << " seconds" << std::endl;
    os << "round_trip_latency weighted median overflow: " << worker_report.round_trip_latency_weighted_median_overflow << std::endl;
  }

  if (worker_report.round_trip_jitter_sample_count > 0) {
    os << std::endl << "--- Round-Trip Jitter Statistics ---" << std::endl << std::endl;

    os << "total (round_trip_jitter) sample count: " << worker_report.round_trip_jitter_sample_count << std::endl;
    os << "minimum round_trip_jitter: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_min << " seconds" << std::endl;
    os << "maximum round_trip_jitter: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_max << " seconds" << std::endl;
    os << "mean round_trip_jitter: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_mean << " seconds" << std::endl;
    os << "round_trip_jitter standard deviation: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_stdev << " seconds" << std::endl;
    os << "round_trip_jitter weighted median: " << std::fixed << std::setprecision(6) << worker_report.round_trip_jitter_weighted_median << " seconds" << std::endl;
    os << "round_trip_jitter weighted median overflow: " << worker_report.round_trip_jitter_weighted_median_overflow << std::endl;
    os << std::endl;
  }

  return 0;
}

