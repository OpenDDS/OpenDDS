/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#include "ThreadMonitor.h"
#include "ThreadStatusManager.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ThreadMonitor::UpdateMode IMPLICIT_IDLE = {true, true, true};
ThreadMonitor::UpdateMode EXPLICIT_IDLE = {false, true, true};
ThreadMonitor::UpdateMode IMPLICIT_BUSY = {true, false, true};
ThreadMonitor::UpdateMode EXPLICIT_BUSY = {false, false, true};
ThreadMonitor::UpdateMode INITIAL_MODE = {true, false, false};
ThreadMonitor::UpdateMode FINAL_MODE = {false, false, false};
ThreadMonitor *ThreadMonitor::installed_monitor_ = 0;

ThreadMonitor::~ThreadMonitor()
{
}

void ThreadMonitor::set_levels(double, double)
{
}

void ThreadMonitor::preset(ThreadStatusManager*, const char*)
{
}

size_t ThreadMonitor::thread_count()
{
  return 0;
}

void ThreadMonitor::summarize()
{
}

void ThreadMonitor::report()
{
}

void ThreadMonitor::update(UpdateMode, const char*)
{
}

double ThreadMonitor::get_utilization(const char*) const
{
  return 0.0;
}

ThreadMonitor::GreenLight::GreenLight(const char* alias, bool initial)
: is_initial_(initial)
{
  if (installed_monitor_) {
    installed_monitor_->update(is_initial_ ? INITIAL_MODE : EXPLICIT_BUSY, alias);
  }
}

ThreadMonitor::GreenLight::~GreenLight()
{
  if (installed_monitor_ && !is_initial_) {
    installed_monitor_->update(IMPLICIT_IDLE);
  }
}


ThreadMonitor::RedLight::RedLight(const char* alias, bool final)
: is_final_(final)
{
  if (installed_monitor_) {
    installed_monitor_->update(is_final_ ? FINAL_MODE : EXPLICIT_IDLE, alias);
  }
}

ThreadMonitor::RedLight::~RedLight()
{
  if (installed_monitor_ && !is_final_) {
    installed_monitor_->update(IMPLICIT_BUSY);
  }
}
#if 0
int ThreadMonitor::BitListener::init()
{
  // Get the Built-In Subscriber for Built-In Topics
  DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();

  DDS::DataReader_var thread_reader =
  bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC);
  if (!thread_reader) {
    std::cerr << "Could not get " << OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC
              << " DataReader." << std::endl;
    return EXIT_FAILURE;
  }

}

ThreadMonitor::BITListenerImpl::BITListenerImpl()
{
  DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<InMemoryDistributedConditionSet>();
  InternalThreadStatusListenerImpl* listener = new InternalThreadStatusListenerImpl("Publisher", dcs);
  DDS::DataReaderListener_var listener_var(listener);

  const DDS::ReturnCode_t retcode =
  thread_reader->set_listener(listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (retcode != DDS::RETCODE_OK) {
    std::cerr << "set_listener for " << OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC << " failed." << std::endl;
    return EXIT_FAILURE;
  }
}

void ThreadMonitor::BITListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader_var builtin_dr =
  OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader::_narrow(reader);
  if (!builtin_dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ThreadMonitor::BITListenerImpl::"
               "on_data_available: _narrow failed.\n"));
  }

  OpenDDS::DCPS::InternalThreadBuiltinTopicData thread_info;
  DDS::SampleInfo si;

  for (DDS::ReturnCode_t status = builtin_dr->read_next_sample(thread_info, si);
       status == DDS::RETCODE_OK;
       status = builtin_dr->read_next_sample(thread_info, si)) {

  }
}
#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
