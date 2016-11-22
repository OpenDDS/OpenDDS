/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TransportMonitorImpl::TransportReportVec TransportMonitorImpl::queue_;
ACE_Recursive_Thread_Mutex TransportMonitorImpl::queue_lock_;

TransportMonitorImpl::TransportMonitorImpl(TransportImpl* /*transport*/,
              OpenDDS::DCPS::TransportReportDataWriter_ptr transport_writer)
  : transport_writer_(TransportReportDataWriter::_duplicate(transport_writer))
{
  char host[256];
  ACE_OS::hostname(host, 256);
  hostname_ = host;
  pid_  = ACE_OS::getpid();
}

TransportMonitorImpl::~TransportMonitorImpl()
{
}

void
TransportMonitorImpl::report() {
  // ACE_DEBUG((LM_DEBUG, "TransportMonitorImpl::report()\n"));
  TransportReport report;
  report.host = this->hostname_.c_str();
  report.pid  = this->pid_;
  // TODO: remove/replace
  report.transport_id  = 0;
  report.transport_type = "";
  // ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, queue_lock_);
  if (!CORBA::is_nil(this->transport_writer_.in())) {
    if (this->queue_.size()) {
      // ACE_DEBUG((LM_DEBUG, "TransportMonitorImpl::report(): popping\n"));
      for (unsigned int i = 0; i < this->queue_.size(); i++) {
        // ACE_DEBUG((LM_DEBUG, "TransportMonitorImpl::report(): writing, id = %d\n",
        //                      this->queue_[i].transport_id));
        this->transport_writer_->write(this->queue_[i], DDS::HANDLE_NIL);
      }
      this->queue_.clear();
    }
    // ACE_DEBUG((LM_DEBUG, "TransportMonitorImpl::report(): writing, id = %d\n",
    //                      report.transport_id));
    this->transport_writer_->write(report, DDS::HANDLE_NIL);
  } else {
    // ACE_DEBUG((LM_DEBUG, "TransportMonitorImpl::report(): queueing, id = %d\n",
    //                      report.transport_id));
    queue_.push_back(report);
  }
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
