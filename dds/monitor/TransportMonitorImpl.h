/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MONITOR_TRANSPORTMONITORIMPL_H
#define OPENDDS_MONITOR_TRANSPORTMONITORIMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"
#include "ace/Recursive_Thread_Mutex.h"
#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

class TransportMonitorImpl : public DCPS::Monitor {
public:
  TransportMonitorImpl(DCPS::TransportImpl* transport,
                       TransportReportDataWriter_ptr transport_writer);
  virtual ~TransportMonitorImpl();
  virtual void report();

private:
  TransportReportDataWriter_var transport_writer_;
  std::string hostname_;
  pid_t pid_;

  typedef std::vector<TransportReport> TransportReportVec;
  static ACE_Recursive_Thread_Mutex queue_lock_;
  static TransportReportVec queue_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_MONITOR_IMPL_H */
