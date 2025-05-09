/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MONITOR_DRMONITORIMPL_H
#define OPENDDS_MONITOR_DRMONITORIMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

class DRMonitorImpl : public DCPS::Monitor {
public:
  DRMonitorImpl(DCPS::DataReaderImpl* dr,
                DataReaderReportDataWriter_ptr dr_writer);
  virtual ~DRMonitorImpl();
  virtual void report();

private:
  DCPS::DataReaderImpl* dr_;
  DataReaderReportDataWriter_var dr_writer_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DR_MONITOR_IMPL_H */
