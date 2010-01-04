/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SPMONITOR_IMPL_H
#define OPENDDS_DCPS_SPMONITOR_IMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

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
  Service_Participant* sp_;
  MonitorFactoryImpl* monitor_factory_;
  OpenDDS::DCPS::ServiceParticipantReportDataWriter_var sp_writer_;
  std::string hostname_;
  pid_t pid_;
};


} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_SPMONITOR_IMPL_H */
