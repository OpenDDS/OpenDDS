/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TopicMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/TopicImpl.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

namespace OpenDDS {
namespace DCPS {


TopicMonitorImpl::TopicMonitorImpl(TopicImpl* topic,
              OpenDDS::DCPS::TopicReportDataWriter_ptr topic_writer)
  : topic_(topic),
    topic_writer_(TopicReportDataWriter::_duplicate(topic_writer))
{
}

TopicMonitorImpl::~TopicMonitorImpl()
{
}

void
TopicMonitorImpl::report() {
  if (!CORBA::is_nil(this->topic_writer_.in())) {
    TopicReport report;
    DDS::DomainParticipant_var dp = this->topic_->get_participant();
    DomainParticipantImpl* dp_impl =
      dynamic_cast<DomainParticipantImpl*>(dp.in());
    report.dp_id      = dp_impl->get_id();
    report.topic_id   = this->topic_->get_id();
    report.topic_name = this->topic_->get_name();
    report.type_name  = this->topic_->get_type_name();
    this->topic_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

