/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MONITOR_PUBLISHERMONITORIMPL_H
#define OPENDDS_MONITOR_PUBLISHERMONITORIMPL_H

#include "monitor_export.h"
#include "dds/DCPS/MonitorFactory.h"
#include "monitorTypeSupportImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

class PublisherMonitorImpl : public DCPS::Monitor {
public:
  PublisherMonitorImpl(DCPS::PublisherImpl* pub,
                       PublisherReportDataWriter_ptr pub_writer);
  virtual ~PublisherMonitorImpl();
  virtual void report();

private:
  DCPS::PublisherImpl* pub_;
  PublisherReportDataWriter_var pub_writer_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_PUBLISHER_MONITOR_IMPL_H */
