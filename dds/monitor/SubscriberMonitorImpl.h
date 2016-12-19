/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SUBSCRIBER_MONITOR_IMPL_H
#define OPENDDS_DCPS_SUBSCRIBER_MONITOR_IMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class SubscriberMonitorImpl : public Monitor {
public:
  SubscriberMonitorImpl(SubscriberImpl* sub,
                   OpenDDS::DCPS::SubscriberReportDataWriter_ptr sub_writer);
  virtual ~SubscriberMonitorImpl();
  virtual void report();

private:
  SubscriberImpl* sub_;
  OpenDDS::DCPS::SubscriberReportDataWriter_var sub_writer_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SUBSCRIBER_MONITOR_IMPL_H */
