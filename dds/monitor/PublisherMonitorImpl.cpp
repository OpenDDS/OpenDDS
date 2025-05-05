/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "PublisherMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/DomainParticipantImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {


PublisherMonitorImpl::PublisherMonitorImpl(DCPS::PublisherImpl* pub,
                                           PublisherReportDataWriter_ptr pub_writer)
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
    report.handle = pub_->get_instance_handle();
    DDS::DomainParticipant_var dp = pub_->get_participant();
    DCPS::DomainParticipantImpl* dpi = dynamic_cast<DCPS::DomainParticipantImpl*>(dp.in());
    if (!dpi) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PublisherMonitorImpl::report():")
        ACE_TEXT(" failed to obtain DomainParticipantImpl.\n")));
      return;
    }
    report.dp_id   = dpi->get_id();
    DCPS::TransportImpl_rch ti; //TODO: transport    = pub_->get_transport_impl();
    // TODO: remove/replace
    report.transport_id = 0;
    DCPS::PublisherImpl::PublicationIdVec writers;
    pub_->get_publication_ids(writers);
    CORBA::ULong length = 0;
    report.writers.length(static_cast<CORBA::ULong>(writers.size()));
    for (DCPS::PublisherImpl::PublicationIdVec::iterator iter = writers.begin();
         iter != writers.end();
         ++iter) {
      report.writers[length++] = *iter;
    }
    this->pub_writer_->write(report, DDS::HANDLE_NIL);
  }
}


}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
