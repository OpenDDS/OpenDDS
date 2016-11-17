/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TOPICMONITOR_IMPL_H
#define OPENDDS_DCPS_TOPICMONITOR_IMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TopicMonitorImpl : public Monitor {
public:
  TopicMonitorImpl(TopicImpl* topic,
                   OpenDDS::DCPS::TopicReportDataWriter_ptr topic_writer);
  virtual ~TopicMonitorImpl();
  virtual void report();

private:
  TopicImpl* topic_;
  OpenDDS::DCPS::TopicReportDataWriter_var topic_writer_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TOPICMONITOR_IMPL_H */
