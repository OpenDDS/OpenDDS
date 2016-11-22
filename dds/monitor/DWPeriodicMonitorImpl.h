/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DW_PERIODIC_MONITOR_IMPL_H
#define OPENDDS_DCPS_DW_PERIODIC_MONITOR_IMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DWPeriodicMonitorImpl : public Monitor {
public:
  DWPeriodicMonitorImpl(DataWriterImpl* dw,
                   OpenDDS::DCPS::DataWriterPeriodicReportDataWriter_ptr dw_per_writer);
  virtual ~DWPeriodicMonitorImpl();
  virtual void report();

private:
  DataWriterImpl* dw_;
  OpenDDS::DCPS::DataWriterPeriodicReportDataWriter_var dw_per_writer_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DW_PERIODIC_MONITOR_IMPL_H */
