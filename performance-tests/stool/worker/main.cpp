#include "dds/DCPS/Service_Participant.h"

#include <ace/Proactor.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include "StoolC.h"
#include "StoolTypeSupportImpl.h"
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

  // Register some Stool-specific types
  Builder::TypeSupportRegistry::TypeSupportRegistration process_config_registration(new Builder::ProcessConfigTypeSupportImpl());
  Builder::TypeSupportRegistry::TypeSupportRegistration data_registration(new Stool::DataTypeSupportImpl());

  // Register some Stool-specific listener factories
  Builder::ListenerFactory<DDS::TopicListener>::Registration topic_registration("stool_tl", [](){ return DDS::TopicListener_var(new Stool::WorkerTopicListener()); });
  Builder::ListenerFactory<DDS::DataReaderListener>::Registration datareader_registration("stool_drl", [](){ return DDS::DataReaderListener_var(new Stool::WorkerDataReaderListener()); });
  Builder::ListenerFactory<DDS::SubscriberListener>::Registration subscriber_registration("stool_sl", [](){ return DDS::SubscriberListener_var(new Stool::WorkerSubscriberListener()); });
  Builder::ListenerFactory<DDS::DataWriterListener>::Registration datawriter_registration("stool_dwl", [](){ return DDS::DataWriterListener_var(new Stool::WorkerDataWriterListener()); });
  Builder::ListenerFactory<DDS::PublisherListener>::Registration publisher_registration("stool_pl", [](){ return DDS::PublisherListener_var(new Stool::WorkerPublisherListener()); });
  Builder::ListenerFactory<DDS::DomainParticipantListener>::Registration participant_registration("stool_partl", [](){ return DDS::DomainParticipantListener_var(new Stool::WorkerParticipantListener()); });

  // Disable some Proactor debug chatter to std out (eventually make this configurable?)
  ACE_Log_Category::ace_lib().priority_mask(0);

  ACE_Proactor proactor;

  // Register actions
  Stool::ActionManager::Registration write_action_registration("write", [&](){ return std::shared_ptr<Stool::Action>(new Stool::WriteAction(proactor)); });
  Stool::ActionManager::Registration forward_action_registration("forward", [&](){ return std::shared_ptr<Stool::Action>(new Stool::ForwardAction(proactor)); });

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

    Stool::ActionManager am(config.actions, config.action_reports, process.get_reader_map(), process.get_writer_map());

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
  worker_report.construction_time = process_construction_end_time - process_construction_begin_time;
  worker_report.enable_time = process_enable_end_time - process_enable_begin_time;
  worker_report.start_time = process_start_end_time - process_start_begin_time;
  worker_report.stop_time = process_stop_end_time - process_stop_begin_time;
  worker_report.destruction_time = process_destruction_end_time - process_destruction_begin_time;
  worker_report.undermatched_readers = 0;
  worker_report.undermatched_writers = 0;
  worker_report.max_discovery_time_delta = ZERO;
  worker_report.sample_count = 0;
  worker_report.latency_min = std::numeric_limits<double>::max();
  worker_report.latency_max = 0.0;
  worker_report.latency_mean = 0.0;
  worker_report.latency_var_x_sample_count = 0.0;
  worker_report.latency_stdev = 0.0;

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

  worker_report.latency_stdev = std::sqrt(worker_report.latency_var_x_sample_count / static_cast<double>(worker_report.sample_count));

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

  if (worker_report.sample_count > 0) {
    os << std::endl << "--- Latency Statistics ---" << std::endl << std::endl;

    os << "total sample count: " << worker_report.sample_count << std::endl;
    os << "minimum latency: " << std::fixed << std::setprecision(6) << worker_report.latency_min << " seconds" << std::endl;
    os << "maximum latency: " << std::fixed << std::setprecision(6) << worker_report.latency_max << " seconds" << std::endl;
    os << "mean latency: " << std::fixed << std::setprecision(6) << worker_report.latency_mean << " seconds" << std::endl;
    os << "latency standard deviation: " << std::fixed << std::setprecision(6) << worker_report.latency_stdev << " seconds" << std::endl;
    os << std::endl;
  }

  return 0;
}

