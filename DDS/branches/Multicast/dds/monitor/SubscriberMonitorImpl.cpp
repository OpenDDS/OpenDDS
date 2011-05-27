/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SubscriberMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

namespace OpenDDS {
namespace DCPS {


SubscriberMonitorImpl::SubscriberMonitorImpl(SubscriberImpl* sub,
              OpenDDS::DCPS::SubscriberReportDataWriter_ptr sub_writer)
  : sub_(sub),
    sub_writer_(SubscriberReportDataWriter::_duplicate(sub_writer))
{
}

SubscriberMonitorImpl::~SubscriberMonitorImpl()
{
}

void
SubscriberMonitorImpl::report() {
  if (!CORBA::is_nil(this->sub_writer_.in())) {
    SubscriberReport report;
    //report.sub_id   = sub_->get_id();  // There is no RepoId for the sub!
    //report.transport_id = sub_->
    //report.readers  = sub_->
    this->sub_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

