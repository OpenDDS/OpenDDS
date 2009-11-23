/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DWMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/DataWriterImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

namespace OpenDDS {
namespace DCPS {


DWMonitorImpl::DWMonitorImpl(DataWriterImpl* dw,
              OpenDDS::DCPS::DataWriterReportDataWriter_ptr dw_writer)
  : dw_(dw),
    dw_writer_(DataWriterReportDataWriter::_duplicate(dw_writer))
{
}

DWMonitorImpl::~DWMonitorImpl()
{
}

void
DWMonitorImpl::report() {
  if (!CORBA::is_nil(this->dw_writer_.in())) {
    DataWriterReport report;
    report.dw_id   = dw_->get_publication_id();
    DDS::Topic_var topic = dw_->get_topic();
    report.topic_id = dynamic_cast<TopicImpl*>(topic.in())->get_id();
    //report.readers  = dw_->
    //report.instances  = dw_->
    //report.associations  = dw_->
    this->dw_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

