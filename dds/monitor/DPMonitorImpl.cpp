/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DPMonitorImpl.h"
#include "monitorC.h"
#include "monitorTypeSupportImpl.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

DPMonitorImpl::DPMonitorImpl(DCPS::DomainParticipantImpl* dp,
                             DomainParticipantReportDataWriter_ptr dp_writer)
  : dp_(dp)
  , dp_writer_(DomainParticipantReportDataWriter::_duplicate(dp_writer))
{
  char host[256];
  ACE_OS::hostname(host, 256);
  hostname_ = host;
  pid_ = ACE_OS::getpid();
}

DPMonitorImpl::~DPMonitorImpl()
{
}

void
DPMonitorImpl::report() {
  if (!CORBA::is_nil(dp_writer_.in())) {
    DomainParticipantReport report;
    report.host = hostname_.c_str();
    report.pid = pid_;
    report.dp_id = dp_->get_id();
    report.domain_id = dp_->get_domain_id();
    DCPS::DomainParticipantImpl::TopicIdVec topics;
    dp_->get_topic_ids(topics);
    CORBA::ULong length = 0;
    report.topics.length(static_cast<CORBA::ULong>(topics.size()));
    for (DCPS::DomainParticipantImpl::TopicIdVec::iterator iter = topics.begin();
         iter != topics.end();
         ++iter) {
      report.topics[length++] = *iter;
    }
    dp_writer_->write(report, DDS::HANDLE_NIL);
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
