#include "BenchC.h"
#ifdef __GNUC__
#pragma GCC diagnostic push
#  if defined(__has_warning)
#    if __has_warning("-Wclass-memaccess")
#      pragma GCC diagnostic ignored "-Wclass-memaccess"
#    endif
#  endif
#endif
#include "BenchTypeSupportImpl.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include "BuilderTypeSupportImpl.h"

#include "ListenerFactory.h"

#include "BuilderProcess.h"
#include "DataReader.h"
#include "DataReaderListener.h"
#include "DataWriter.h"
#include "DataWriterListener.h"
#include "ParticipantListener.h"
#include "PublisherListener.h"
#include "SubscriberListener.h"
#include "TopicListener.h"

#include "Utils.h"
#include "PropertyStatBlock.h"

#include "ActionManager.h"
#include "ForwardAction.h"
#include "ReadAction.h"
#include "SetCftParametersAction.h"
#include "WorkerDataReaderListener.h"
#include "WorkerDataWriterListener.h"
#include "WorkerTopicListener.h"
#include "WorkerSubscriberListener.h"
#include "WorkerPublisherListener.h"
#include "WorkerParticipantListener.h"
#include "WriteAction.h"

#include <util.h>
#include <json_conversion.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/transport/udp/Udp.h>
#  include <dds/DCPS/transport/tcp/Tcp.h>
#  include <dds/DCPS/transport/multicast/Multicast.h>
#  ifdef OPENDDS_SECURITY
#    include <dds/DCPS/security/BuiltInPlugins.h>
#  endif
#endif

#include <ace/Proactor.h>
#ifdef ACE_HAS_AIO_CALLS
#  include <ace/POSIX_CB_Proactor.h>
#endif

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

using Builder::Log;
using Builder::ZERO;
using Bench::get_option_argument;

const size_t DEFAULT_MAX_DECIMAL_PLACES = 9u;
const size_t DEFAULT_THREAD_POOL_SIZE = 4u;

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
  } catch (int value) {
    std::cerr << "See DDS_ROOT/performance-tests/bench/README.md for usage" << std::endl;
    return value;
  }

  std::ifstream config_file(config_file_path);
  if (!config_file.is_open()) {
    std::cerr << "Unable to open configuration file: '" << config_file_path << "'" << std::endl;
    return 2;
  }

  std::ofstream log_file;
  if (!log_file_path.empty()) {
    log_file.open(log_file_path, ios::app);
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

  // Bad-actor test & debugging options for node & test controllers

  Builder::ConstPropertyIndex force_worker_segfault_prop =
    get_property(config.properties, "force_worker_segfault", Builder::PVK_ULL);
  if (force_worker_segfault_prop) {
    if (force_worker_segfault_prop->value.ull_prop()) {
      DDS::DataReaderListener* drl_ptr = 0;
      drl_ptr->on_data_available(0);
    }
  }

  Builder::ConstPropertyIndex force_worker_assert_prop =
    get_property(config.properties, "force_worker_assert", Builder::PVK_ULL);
  if (force_worker_assert_prop) {
    if (force_worker_assert_prop->value.ull_prop()) {
      OPENDDS_ASSERT(false);
    }
  }

  Builder::ConstPropertyIndex force_worker_deadlock_prop =
    get_property(config.properties, "force_worker_deadlock", Builder::PVK_ULL);
  if (force_worker_deadlock_prop) {
    if (force_worker_deadlock_prop->value.ull_prop()) {
      std::mutex m;
      std::unique_lock<std::mutex> l(m);
      std::thread t([&](){
        std::unique_lock<std::mutex> tl(m);
      });
      t.join();
    }
  }

  // Register some Bench-specific types
  Builder::TypeSupportRegistry::TypeSupportRegistration
    process_config_registration(new Builder::ProcessConfigTypeSupportImpl());
  Builder::TypeSupportRegistry::TypeSupportRegistration
    data_registration(new Bench::DataTypeSupportImpl());

  // Register some Bench-specific listener factories
  try {
    Builder::register_topic_listener("bench_tl", [](const Builder::PropertySeq& properties){
        return DDS::TopicListener_var(new Bench::WorkerTopicListener(properties));
      });
    Builder::register_datareader_listener("bench_drl", [](const Builder::PropertySeq& properties){
        return DDS::DataReaderListener_var(new Bench::WorkerDataReaderListener(properties));
      });
    Builder::register_subscriber_listener("bench_sl", [](const Builder::PropertySeq& properties){
        return DDS::SubscriberListener_var(new Bench::WorkerSubscriberListener(properties));
      });
    Builder::register_datawriter_listener("bench_dwl", [](const Builder::PropertySeq& properties){
        return DDS::DataWriterListener_var(new Bench::WorkerDataWriterListener(properties));
      });
    Builder::register_publisher_listener("bench_pl", [](const Builder::PropertySeq& properties){
        return DDS::PublisherListener_var(new Bench::WorkerPublisherListener(properties));
      });
    Builder::register_domain_participant_listener("bench_partl", [](const Builder::PropertySeq& properties){
        return DDS::DomainParticipantListener_var(new Bench::WorkerParticipantListener(properties));
      });
  } catch (const std::exception& e) {
    std::cerr << "Exception caught trying to register listener factories: " << e.what() << std::endl;
    return 4;
  }

  // Disable some Proactor debug chatter to stdout (eventually make this configurable?)
  ACE_Log_Category::ace_lib().priority_mask(0);

#ifdef ACE_HAS_AIO_CALLS
  Builder::ConstPropertyIndex use_aio_proactor_prop =
    get_property(config.properties, "use_aio_proactor", Builder::PVK_ULL);
#endif

  std::shared_ptr<ACE_Proactor> proactor;
#ifdef ACE_HAS_AIO_CALLS
  if (use_aio_proactor_prop && use_aio_proactor_prop->value.ull_prop()) {
    proactor.reset(new ACE_Proactor(new ACE_POSIX_AIOCB_Proactor()));
  } else {
#endif
    proactor.reset(new ACE_Proactor());
#ifdef ACE_HAS_AIO_CALLS
  }
#endif

  int max_decimal_places = DEFAULT_MAX_DECIMAL_PLACES;
  Builder::ConstPropertyIndex max_decimal_places_prop =
    get_property(config.properties, "max_decimal_places", Builder::PVK_ULL);
  if (max_decimal_places_prop) {
    max_decimal_places = static_cast<int>(max_decimal_places_prop->value.ull_prop());
  }

  size_t action_thread_pool_size = std::min(static_cast<size_t>(config.actions.length()), DEFAULT_THREAD_POOL_SIZE);
  Builder::ConstPropertyIndex action_thread_pool_size_prop =
    get_property(config.properties, "action_thread_pool_size", Builder::PVK_ULL);
  if (action_thread_pool_size_prop) {
    action_thread_pool_size = static_cast<size_t>(action_thread_pool_size_prop->value.ull_prop());
  }

  size_t redirect_ace_log = 1;
  Builder::ConstPropertyIndex redirect_ace_log_prop =
    get_property(config.properties, "redirect_ace_log", Builder::PVK_ULL);
  if (redirect_ace_log_prop) {
    redirect_ace_log = static_cast<size_t>(redirect_ace_log_prop->value.ull_prop());
  }

  if (redirect_ace_log && !log_file_path.empty()) {
    std::ofstream* output_stream = new std::ofstream(log_file_path.c_str(), ios::app);
    if (output_stream->bad()) {
      delete output_stream;
    } else {
      ACE_LOG_MSG->msg_ostream(output_stream, true);
    }
    ACE_LOG_MSG->clr_flags(ACE_Log_Msg::STDERR | ACE_Log_Msg::LOGGER);
    ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
  }

  // Register actions
  Bench::ActionManager::Registration
    write_action_registration("write", [&](){
      return std::shared_ptr<Bench::Action>(new Bench::WriteAction(*proactor));
    });
  Bench::ActionManager::Registration
    read_action_registration("read", [&](){
      return std::shared_ptr<Bench::Action>(new Bench::ReadAction(*proactor));
    });
  Bench::ActionManager::Registration
    forward_action_registration("forward", [&](){
      return std::shared_ptr<Bench::Action>(new Bench::ForwardAction(*proactor));
    });
  Bench::ActionManager::Registration
    set_cft_parameters_action_registration("set_cft_parameters", [&]() {
      return std::shared_ptr<Bench::Action>(new Bench::SetCftParametersAction(*proactor));
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

  const size_t thread_pool_size = static_cast<size_t>(action_thread_pool_size);
  std::vector<std::shared_ptr<std::thread> > thread_pool;
  for (size_t i = 0; i < thread_pool_size; ++i) {
    thread_pool.emplace_back(std::make_shared<std::thread>([&](){ proactor->proactor_run_event_loop(); }));
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

    Bench::ActionManager am(config.actions, config.action_reports, process.get_reader_map(), process.get_writer_map(), process.get_cft_map());

    Log::log() << "Action construction / initialization complete." << std::endl << std::endl;

    do_wait(config.enable_time, "enable");

    Log::log() << "Enabling DDS entities (if not already enabled)." << std::endl;

    process_enable_begin_time = Builder::get_hr_time();
    process.enable_dds_entities(true);
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

            if (!wdrl->wait_for_expected_match(timeout_time)) {
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

            if (!wdwl->wait_for_expected_match(timeout_time)) {
              Log::log() << "Error: " << it->first << " Expected readers not found." << std::endl << std::endl;
            }
          }
        }
      }

      process_stop_discovery_time = Builder::get_sys_time();

      Log::log() << "Discovery of expected entities took " << process_stop_discovery_time - process_start_discovery_time << " seconds." << std::endl << std::endl;
    }

    Log::log() << "Initializing process actions." << std::endl;

    am.action_start();

    do_wait(config.start_time, "start");

    Log::log() << "Starting process actions." << std::endl;

    process_start_begin_time = Builder::get_hr_time();
    am.test_start();
    process_start_end_time = Builder::get_hr_time();

    Log::log() << "Process tests started." << std::endl << std::endl;

    do_wait(config.stop_time, "stop");

    Log::log() << "Stopping process tests." << std::endl;

    process_stop_begin_time = Builder::get_hr_time();
    am.test_stop();
    process_stop_end_time = Builder::get_hr_time();

    Log::log() << "Process tests stopped." << std::endl << std::endl;

    do_wait(config.destruction_time, "destruction");

    Log::log() << "Stopping process actions." << std::endl;

    am.action_stop();

    proactor->proactor_end_event_loop();
    for (size_t i = 0; i < thread_pool_size; ++i) {
      thread_pool[i]->join();
    }
    thread_pool.clear();

    Log::log() << "Detaching Listeners." << std::endl;

    process.detach_listeners();

    process_report = process.get_report();

    Log::log() << "Beginning process destruction / entity deletion." << std::endl;

    process_destruction_begin_time = Builder::get_hr_time();
  } catch (const std::exception& e) {
    std::cerr << "Exception caught trying to execute test sequence: " << e.what() << std::endl;
    proactor->proactor_end_event_loop();
    for (size_t i = 0; i < thread_pool_size; ++i) {
      thread_pool[i]->join();
    }
    thread_pool.clear();
    TheServiceParticipant->shutdown();
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception caught trying to execute test sequence" << std::endl;
    proactor->proactor_end_event_loop();
    for (size_t i = 0; i < thread_pool_size; ++i) {
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

  try {
    for (CORBA::ULong i = 0; i < process_report.participants.length(); ++i) {
      for (CORBA::ULong j = 0; j < process_report.participants[i].subscribers.length(); ++j) {
        for (CORBA::ULong k = 0; k < process_report.participants[i].subscribers[j].datareaders.length(); ++k) {
          Builder::DataReaderReport& dr_report = process_report.participants[i].subscribers[j].datareaders[k];

          const Builder::TimeStamp dr_enable_time =
            get_or_create_property(dr_report.properties, "enable_time", Builder::PVK_TIME)->value.time_prop();
          const Builder::TimeStamp dr_last_discovery_time =
            get_or_create_property(dr_report.properties, "last_discovery_time", Builder::PVK_TIME)->value.time_prop();

          if (!(ZERO < dr_enable_time && ZERO < dr_last_discovery_time)) {
            ++worker_report.undermatched_readers;
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

        if (!(ZERO < dw_enable_time && ZERO < dw_last_discovery_time)) {
            ++worker_report.undermatched_writers;
          }
        }
      }
    }
  } catch (...) {
    std::cerr << "Unknown exception caught trying to consolidate statistics" << std::endl;
    return 5;
  }

  // If requested, write out worker report to file

  if (!report_file_path.empty()) {
    idl_2_json(worker_report, report_file, max_decimal_places);
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

  return 0;
}
