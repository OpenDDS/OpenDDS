/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RelayEventLoop.h"

#include <ace/Select_Reactor.h>
#include <ace/TP_Reactor.h>
#include <ace/Task.h>

#include <cstdlib>

namespace RtpsRelay {

struct ThreadPool : ACE_Task_Base {

  ThreadPool(const Config& config, ACE_Reactor& reactor, RelayThreadMonitor& monitor)
    : config_(config)
    , reactor_(reactor)
    , monitor_(monitor)
  {}

  int svc() override;
  int run();

  const Config& config_;
  ACE_Reactor& reactor_;
  RelayThreadMonitor& monitor_;
  OpenDDS::DCPS::ThreadStatusManager& thread_status_manager_ = TheServiceParticipant->get_thread_status_manager();
};

struct RunThreadMonitor {

  RunThreadMonitor(OpenDDS::DCPS::ThreadStatusManager& thread_status_manager, RelayThreadMonitor& monitor)
    : should_run_(thread_status_manager.update_thread_status())
    , monitor_(monitor)
    , status_(should_run_ ? monitor.start() : EXIT_SUCCESS)
  {}

  ~RunThreadMonitor()
  {
    if (should_run_ && status_ == EXIT_SUCCESS) {
      monitor_.stop();
    }
  }

  const bool should_run_;
  RelayThreadMonitor& monitor_;
  const int status_;
};

int ThreadPool::run()
{
  RunThreadMonitor rtm{thread_status_manager_, monitor_};
  if (rtm.status_ != EXIT_SUCCESS) {
    ACE_ERROR((LM_ERROR, "(%P:%t) ERROR: RtpsRelay::ThreadPool::run: failed to start Relay Thread Monitor\n"));
    return EXIT_FAILURE;
  }

  const auto threads = config_.handler_threads();
  if (threads == 1) {
    return svc();
  }

  const auto status = activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED, static_cast<int>(threads));
  if (status != EXIT_SUCCESS) {
    ACE_ERROR((LM_ERROR, "(%P:%t) ERROR: RtpsRelay::ThreadPool::run: failed to start thread pool: %m\n"));
    return status;
  }

  return wait();
}

int ThreadPool::svc()
{
  const auto has_run_time = !config_.run_time().is_zero();
  const auto end_time = OpenDDS::DCPS::MonotonicTimePoint::now() + config_.run_time();

  if (thread_status_manager_.update_thread_status()) {
    OpenDDS::DCPS::ThreadStatusManager::Start thread_status_monitoring_active(thread_status_manager_, "RtpsRelay Event Loop");

    while (!has_run_time || OpenDDS::DCPS::MonotonicTimePoint::now() < end_time) {
      auto t = thread_status_manager_.thread_status_interval().value();
      OpenDDS::DCPS::ThreadStatusManager::Sleeper s(thread_status_manager_);
      if (reactor_.run_reactor_event_loop(t, 0) != 0) {
        break;
      }
    }

  } else if (has_run_time) {
    while (OpenDDS::DCPS::MonotonicTimePoint::now() < end_time) {
      auto t = (end_time - OpenDDS::DCPS::MonotonicTimePoint::now()).value();
      if (reactor_.run_reactor_event_loop(t, 0) != 0) {
        break;
      }
    }

  } else {
    reactor_.run_reactor_event_loop();
  }
  return EXIT_SUCCESS;
}

ACE_Reactor_Impl* RelayEventLoop::make_reactor_impl(const Config& config)
{
  return config.handler_threads() == 1 ? new ACE_Select_Reactor : new ACE_TP_Reactor;
}

int RelayEventLoop::run(const Config& config, ACE_Reactor& reactor, RelayThreadMonitor& monitor)
{
  return ThreadPool{config, reactor, monitor}.run();
}

}
