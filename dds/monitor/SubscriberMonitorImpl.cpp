/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SubscriberMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/DomainParticipantImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
    report.handle = sub_->get_instance_handle();
    DDS::DomainParticipant_var dp = sub_->get_participant();
    report.dp_id   = dynamic_cast<DomainParticipantImpl*>(dp.in())->get_id();
    TransportImpl_rch ti; //TODO: transport    = sub_->get_transport_impl();
    // TODO: remove transport_id and replace with name
    report.transport_id = 0;
    SubscriberImpl::SubscriptionIdVec readers;
    sub_->get_subscription_ids(readers);
    CORBA::ULong length = 0;
    report.readers.length(static_cast<CORBA::ULong>(readers.size()));
    for (SubscriberImpl::SubscriptionIdVec::iterator iter = readers.begin();
         iter != readers.end();
         ++iter) {
      report.readers[length++] = *iter;
    }
    this->sub_writer_->write(report, DDS::HANDLE_NIL);
  }
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
