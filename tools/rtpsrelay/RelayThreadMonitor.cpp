#include "RelayThreadMonitor.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/TimeTypes.h>

#include <ace/Thread.h>

#include <cstdlib>
#include <vector>

using namespace OpenDDS::DCPS;
namespace RtpsRelay {

int RelayThreadMonitor::start()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, -1);

  if (running_) {
    return 0;
  }
  running_ = true;

  if (activate(THR_NEW_LWP | THR_JOINABLE, 1) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayThreadMonitor::start Failed to activate\n"));
    return -1;
  }

  return 0;
}

void RelayThreadMonitor::stop()
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    running_ = false;
  }
  condition_.notify_all();

  OpenDDS::DCPS::ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  ThreadStatusManager::Sleeper s(thread_status_manager);
  wait();
}

int RelayThreadMonitor::svc()
{
  OpenDDS::DCPS::ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  ThreadStatusManager::Start s(thread_status_manager, "RtpsRelay RelayThreadMonitor");
  const TimeDuration thread_status_interval = thread_status_manager.thread_status_interval();
  const int safety_factor = config_.thread_status_safety_factor();

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, -1);

  for (auto count = 0; running_; count = (count + 1) % safety_factor) {
    condition_.wait_until(MonotonicTimePoint::now() + thread_status_interval, thread_status_manager);
    if (!running_) {
      break;
    }
    OpenDDS::DCPS::InternalThreadBuiltinTopicDataSeq datas;
    DDS::SampleInfoSeq infos;
    const DDS::ReturnCode_t ret = thread_status_reader_->read(datas,
                                                              infos,
                                                              DDS::LENGTH_UNLIMITED,
                                                              DDS::ANY_SAMPLE_STATE,
                                                              DDS::ANY_VIEW_STATE,
                                                              DDS::ANY_INSTANCE_STATE);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayThreadMonitor::svc failed to read %C\n", OpenDDS::DCPS::retcode_to_string(ret)));
      continue;
    }

    const SystemTimePoint expire = SystemTimePoint::now() - thread_status_interval * safety_factor;
    const auto log_all_threads = count == 0 && config_.log_thread_status();
    std::vector<DDS::UInt32> late_thread_indexes;

    for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
      if (infos[idx].valid_data) {
        const auto& bit_sample = datas[idx];
        const auto thread_id = bit_sample.thread_id.in();
        auto& utilization = utilization_[thread_id];
        const auto old_utilization = utilization;
        utilization = bit_sample.utilization;
        if (config_.log_utilization_changes() && std::abs(utilization - old_utilization) > 0.2) { // 20% change
          ACE_DEBUG((LM_INFO, "(%P|%t) INFO: Thread %C utilization changed significantly: %.2f%% -> %.2f%%\n",
            thread_id, old_utilization * 100, utilization * 100));
        }

      } else if (infos[idx].instance_state != DDS::ALIVE_INSTANCE_STATE) {
        utilization_.erase(datas[idx].thread_id.in());
      }

      if (log_all_threads) {
        ACE_DEBUG((LM_INFO, "(%P|%t) INFO: Thread Status %C %C\n",
          infos[idx].valid_data ? to_json(datas[idx]).c_str() : datas[idx].thread_id.in(),
          to_json(infos[idx]).c_str()));
      }

      if (infos[idx].instance_state == DDS::ALIVE_INSTANCE_STATE && SystemTimePoint(infos[idx].source_timestamp) < expire) {
        late_thread_indexes.push_back(idx);
      }
    }

    for (const auto idx : late_thread_indexes) {
      const SystemTimePoint timestamp(infos[idx].source_timestamp);
      const auto& thread = datas[idx];
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayThreadMonitor::svc thread %C (%d, %d) last update %#T.  Aborting...\n",
                 thread.thread_id.in(), thread.detail1, thread.detail2, &timestamp.value()));
    }

    if (!late_thread_indexes.empty()) {
      std::abort();
    }
  }

  return 0;
}

void RelayThreadMonitor::on_data_available(DDS::DataReader_ptr /*reader*/)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  OpenDDS::DCPS::InternalThreadBuiltinTopicDataSeq datas;
  DDS::SampleInfoSeq infos;
  const DDS::ReturnCode_t ret = thread_status_reader_->read(datas,
                                                            infos,
                                                            DDS::LENGTH_UNLIMITED,
                                                            DDS::NOT_READ_SAMPLE_STATE,
                                                            DDS::ANY_VIEW_STATE,
                                                            DDS::ANY_INSTANCE_STATE);

  if (ret == DDS::RETCODE_NO_DATA) {
    return;
  }

  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayThreadMonitor::on_data_available failed to read %C\n", OpenDDS::DCPS::retcode_to_string(ret)));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    if (infos[idx].valid_data) {
      utilization_[datas[idx].thread_id.in()] = datas[idx].utilization;
    } else if (infos[idx].instance_state != DDS::ALIVE_INSTANCE_STATE) {
      utilization_.erase(datas[idx].thread_id.in());
    }
  }
}

}
