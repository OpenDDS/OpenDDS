/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

namespace OpenDDS {
namespace DCPS {


TransportMonitorImpl::TransportMonitorImpl(TransportImpl* transport,
              OpenDDS::DCPS::TransportReportDataWriter_ptr transport_writer)
  : transport_(transport),
    transport_writer_(TransportReportDataWriter::_duplicate(transport_writer))
{
  char host[256];
  ACE_OS::hostname(host, 256);
  hostname_ = host;
}

TransportMonitorImpl::~TransportMonitorImpl()
{
}

void
TransportMonitorImpl::report() {
  if (!CORBA::is_nil(this->transport_writer_.in())) {
    TransportReport report;
    report.host = hostname_.c_str();
    report.pid  = ACE_OS::getpid();
    report.transport_id  = transport_->get_transport_id();
    report.transport_type = ACE_TEXT_ALWAYS_CHAR(transport_->get_factory_id().c_str());
    this->transport_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

