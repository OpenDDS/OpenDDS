/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_MONITOR_IMPL_H
#define OPENDDS_DCPS_TRANSPORT_MONITOR_IMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"
#include "ace/Recursive_Thread_Mutex.h"
#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class TransportMonitorImpl : public Monitor {
public:
  TransportMonitorImpl(TransportImpl* transport,
                   OpenDDS::DCPS::TransportReportDataWriter_ptr transport_writer);
  virtual ~TransportMonitorImpl();
  virtual void report();

private:
  TransportImpl* transport_;
  OpenDDS::DCPS::TransportReportDataWriter_var transport_writer_;
  std::string hostname_;
  pid_t pid_;

  typedef std::vector<TransportReport> TransportReportVec;
  static ACE_Recursive_Thread_Mutex queue_lock_;
  static TransportReportVec queue_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_TRANSPORT_MONITOR_IMPL_H */
