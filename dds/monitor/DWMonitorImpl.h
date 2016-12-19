/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DW_MONITOR_IMPL_H
#define OPENDDS_DCPS_DW_MONITOR_IMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DWMonitorImpl : public Monitor {
public:
  DWMonitorImpl(DataWriterImpl* dw,
                   OpenDDS::DCPS::DataWriterReportDataWriter_ptr dw_writer);
  virtual ~DWMonitorImpl();
  virtual void report();

private:
  DataWriterImpl* dw_;
  OpenDDS::DCPS::DataWriterReportDataWriter_var dw_writer_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DW_MONITOR_IMPL_H */
