/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MONITOR_DPMONITORIMPL_H
#define OPENDDS_MONITOR_DPMONITORIMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

class DPMonitorImpl : public DCPS::Monitor {
public:
  DPMonitorImpl(DCPS::DomainParticipantImpl* dp,
                DomainParticipantReportDataWriter_ptr dp_writer);
  virtual ~DPMonitorImpl();
  virtual void report();

private:
  DCPS::DomainParticipantImpl* dp_;
  DomainParticipantReportDataWriter_var dp_writer_;
  std::string hostname_;
  pid_t pid_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DPMONITOR_IMPL_H */
