/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MONITOR_DWMONITORIMPL_H
#define OPENDDS_MONITOR_DWMONITORIMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

class DWMonitorImpl : public DCPS::Monitor {
public:
  DWMonitorImpl(DCPS::DataWriterImpl* dw,
                DataWriterReportDataWriter_ptr dw_writer);
  virtual ~DWMonitorImpl();
  virtual void report();

private:
  DCPS::DataWriterImpl* dw_;
  DataWriterReportDataWriter_var dw_writer_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DW_MONITOR_IMPL_H */
