/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "PublisherMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

namespace OpenDDS {
namespace DCPS {


PublisherMonitorImpl::PublisherMonitorImpl(PublisherImpl* pub,
              OpenDDS::DCPS::PublisherReportDataWriter_ptr pub_writer)
  : pub_(pub),
    pub_writer_(PublisherReportDataWriter::_duplicate(pub_writer))
{
}

PublisherMonitorImpl::~PublisherMonitorImpl()
{
}

void
PublisherMonitorImpl::report() {
  if (!CORBA::is_nil(this->pub_writer_.in())) {
    PublisherReport report;
    //report.pub_id   = pub_->get_id(); // There is no RepoId for the pub!
    //TransportImpl_rch ti = pub_->get_transport_impl();
    //report.transport_id = // No direct way to look up the transport ID
    //report.writers  = 
    this->pub_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

