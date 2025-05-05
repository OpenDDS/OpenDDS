/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MONITOR_TOPICMONITORIMPL_H
#define OPENDDS_MONITOR_TOPICMONITORIMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

class TopicMonitorImpl : public DCPS::Monitor {
public:
  TopicMonitorImpl(DCPS::TopicImpl* topic,
                   TopicReportDataWriter_ptr topic_writer);
  virtual ~TopicMonitorImpl();
  virtual void report();

private:
  DCPS::TopicImpl* topic_;
  TopicReportDataWriter_var topic_writer_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TOPICMONITOR_IMPL_H */
