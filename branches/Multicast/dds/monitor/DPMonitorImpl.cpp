/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DPMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

namespace OpenDDS {
namespace DCPS {


DPMonitorImpl::DPMonitorImpl(DomainParticipantImpl* dp,
              OpenDDS::DCPS::DomainParticipantReportDataWriter_ptr dp_writer)
  : dp_(dp),
    dp_writer_(DomainParticipantReportDataWriter::_duplicate(dp_writer))
{
}

DPMonitorImpl::~DPMonitorImpl()
{
}

void
DPMonitorImpl::report() {
  if (!CORBA::is_nil(this->dp_writer_.in())) {
    DomainParticipantReport report;
    report.dp_id      = dp_->get_id();
    report.domain_id  = dp_->get_domain_id();
    this->dp_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

