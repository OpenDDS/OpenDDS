/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DR_MONITOR_IMPL_H
#define OPENDDS_DCPS_DR_MONITOR_IMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DRMonitorImpl : public Monitor {
public:
  DRMonitorImpl(DataReaderImpl* dr,
                   OpenDDS::DCPS::DataReaderReportDataWriter_ptr dr_writer);
  virtual ~DRMonitorImpl();
  virtual void report();

private:
  DataReaderImpl* dr_;
  OpenDDS::DCPS::DataReaderReportDataWriter_var dr_writer_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DR_MONITOR_IMPL_H */
