#include "RelayThreadMonitor.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/JsonValueWriter.h>

#include <ace/Thread.h>

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

  int count = 0;

  while (running_) {
    const MonotonicTimePoint expire = MonotonicTimePoint::now() + thread_status_interval;
    condition_.wait_until(expire, thread_status_manager);
    if (running_) {
      OpenDDS::DCPS::InternalThreadBuiltinTopicDataSeq datas;
      DDS::SampleInfoSeq infos;
      DDS::ReturnCode_t ret = thread_status_reader_->read(datas,
                                                          infos,
                                                          DDS::LENGTH_UNLIMITED,
                                                          DDS::ANY_SAMPLE_STATE,
                                                          DDS::ANY_VIEW_STATE,
                                                          DDS::ANY_INSTANCE_STATE);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayThreadMonitor::svc failed to read %C\n"), OpenDDS::DCPS::retcode_to_string(ret)));
        continue;
      }

      const DDS::Time_t timestamp = SystemTimePoint::now().to_dds_time() - time_value_to_duration((thread_status_interval * safety_factor).value());

      for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
        if (infos[idx].valid_data) {
          utilization_[datas[idx].thread_id.in()] = datas[idx].utilization;
        } else if (infos[idx].instance_state != DDS::ALIVE_INSTANCE_STATE) {
          utilization_.erase(datas[idx].thread_id.in());
        }

        if (count == 0 && config_.log_thread_status()) {
          ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: Thread Status %C %C\n"),
                     to_json(datas[idx]).c_str(),
                     to_json(infos[idx]).c_str()));
        }

        if (infos[idx].instance_state == DDS::ALIVE_INSTANCE_STATE && infos[idx].source_timestamp < timestamp) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: RelayThreadMonitor::svc thread %C failed to update status.  Aborting...\n"),
                     datas[idx].thread_id.in()));
          abort();
        }
      }

      count = (count + 1) % safety_factor;
    }
  }

  return 0;
}

void RelayThreadMonitor::on_data_available(DDS::DataReader_ptr /*reader*/)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  OpenDDS::DCPS::InternalThreadBuiltinTopicDataSeq datas;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = thread_status_reader_->read(datas,
                                                      infos,
                                                      DDS::LENGTH_UNLIMITED,
                                                      DDS::NOT_READ_SAMPLE_STATE,
                                                      DDS::ANY_VIEW_STATE,
                                                      DDS::ANY_INSTANCE_STATE);

  if (ret == DDS::RETCODE_NO_DATA) {
    return;
  }

  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayThreadMonitor::on_data_available failed to read %C\n"), OpenDDS::DCPS::retcode_to_string(ret)));
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
