/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MONITOR_SPMONITORIMPL_H
#define OPENDDS_MONITOR_SPMONITORIMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MonitorFactoryImpl;

class SPMonitorImpl : public Monitor {
public:
  SPMonitorImpl(MonitorFactoryImpl* monitor_factory,
                Service_Participant* sp);
  virtual ~SPMonitorImpl();
  virtual void report();

private:
  MonitorFactoryImpl* monitor_factory_;
  OpenDDS::DCPS::ServiceParticipantReportDataWriter_var sp_writer_;
  std::string hostname_;
  pid_t pid_;
};


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SPMONITOR_IMPL_H */
