/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DRMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

namespace OpenDDS {
namespace DCPS {


DRMonitorImpl::DRMonitorImpl(DataReaderImpl* dr,
              OpenDDS::DCPS::DataReaderReportDataWriter_ptr dr_writer)
  : dr_(dr),
    dr_writer_(DataReaderReportDataWriter::_duplicate(dr_writer))
{
}

DRMonitorImpl::~DRMonitorImpl()
{
}

void
DRMonitorImpl::report() {
  if (!CORBA::is_nil(this->dr_writer_.in())) {
    DataReaderReport report;
    report.dr_id   = dr_->get_subscription_id();
    //report.topic_id = dr_->
    //report.writers  = dr_->
    //report.instances  = dr_->
    //report.associations  = dr_->
    this->dr_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

