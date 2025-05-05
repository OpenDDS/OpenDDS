/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MONITOR_SUBSCRIBERMONITORIMPL_H
#define OPENDDS_MONITOR_SUBSCRIBERMONITORIMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

class SubscriberMonitorImpl : public DCPS::Monitor {
public:
  SubscriberMonitorImpl(DCPS::SubscriberImpl* sub,
                        SubscriberReportDataWriter_ptr sub_writer);
  virtual ~SubscriberMonitorImpl();
  virtual void report();

private:
  DCPS::SubscriberImpl* sub_;
  SubscriberReportDataWriter_var sub_writer_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SUBSCRIBER_MONITOR_IMPL_H */
